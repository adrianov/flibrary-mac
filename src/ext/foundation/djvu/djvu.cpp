#include "djvu.h"

#include <QBuffer>
#include <QByteArray>
#include <QFileInfo>
#include <QImage>

#include <libdjvu/ddjvuapi.h>

#include "fnd/ScopedCall.h"
#include "fnd/algorithm.h"

#include "platform/DyLib.h"

#include "log.h"

using namespace HomeCompa::DjVu;
using namespace HomeCompa;

namespace
{

unsigned int MASKS[4] = { 0xff0000, 0x00ff00, 0x0000ff, 0xff000000 };

using DjvuContextCreate      = ddjvu_context_t* (*)(const char* programName);
using DjvuContextRelease     = void (*)(ddjvu_context_t* context);
using DjvuDocumentCreate     = DDJVUAPI ddjvu_document_t* (*)(ddjvu_context_t * context, const char* url, int cache);
using DjvuDocumentJob        = ddjvu_job_t* (*)(ddjvu_document_t * document);
using DjvuJobStatus          = ddjvu_status_t (*)(ddjvu_job_t* job);
using DjvuJobRelease         = void (*)(ddjvu_job_t* job);
using DjvuMessageWait        = ddjvu_message_t* (*)(ddjvu_context_t * context);
using DjvuMessagePeek        = ddjvu_message_t* (*)(ddjvu_context_t * context);
using DjvuMessagePop         = void (*)(ddjvu_context_t* context);
using DjvuPageCreateByPageNo = ddjvu_page_t* (*)(ddjvu_document_t * document, int pageNo);
using DjvuPageJob            = ddjvu_job_t* (*)(ddjvu_page_t * page);
using DjvuPageRender =
	int (*)(ddjvu_page_t* page, ddjvu_render_mode_t mode, const ddjvu_rect_t* pageRect, const ddjvu_rect_t* renderRect, const ddjvu_format_t* pixelFormat, unsigned long rowSize, char* imageBuffer);
using DjvuPageGetWidth      = int (*)(ddjvu_page_t* page);
using DjvuPageGetHeight     = int (*)(ddjvu_page_t* page);
using DjvuFormatCreate      = ddjvu_format_t* (*)(ddjvu_format_style_t style, int argsCount, unsigned int* args);
using DjvuFormatRelease     = void (*)(ddjvu_format_t* format);
using DjvuFormatSetRowOrder = void (*)(ddjvu_format_t* format, int top_to_bottom);
using DjvuStreamWrite       = void (*)(ddjvu_document_t* document, int streamId, const char* data, unsigned long size);
using DjvuStreamClose       = void (*)(ddjvu_document_t* document, int streamId, int stop);

struct DocumentReleaser
{
	DjvuDocumentJob documentJob { nullptr };
	DjvuJobRelease  jobRelease { nullptr };

	void operator()(ddjvu_document_t* p) const
	{
		if (p)
			jobRelease(documentJob(p));
	}
};

struct PageReleaser
{
	DjvuPageJob    pageJob { nullptr };
	DjvuJobRelease jobRelease { nullptr };

	void operator()(ddjvu_page_t* p) const
	{
		if (p)
			jobRelease(pageJob(p));
	}
};

using DocumentPtr = std::unique_ptr<ddjvu_document_t, DocumentReleaser>;
using PagePtr     = std::unique_ptr<ddjvu_page_t, PageReleaser>;

std::unique_ptr<Platform::DyLib> CreateLibrary()
{
	auto lib = std::make_unique<Platform::DyLib>("djvulibre");
	if (!lib->IsOpen())
		throw std::runtime_error(lib->GetErrorDescription());
	return lib;
}

template <typename T>
T GetEntryPoint(Platform::DyLib& lib, const std::string& procName)
{
	if (void* const procPtr = lib.GetProc(procName))
		return reinterpret_cast<T>(procPtr);
	throw std::runtime_error(lib.GetErrorDescription());
}

class DjVuParser
{
public:
	DjVuParser()
	{
		if (!m_context)
			throw std::runtime_error("cannot create DjVu context");

		if (!m_format)
			throw std::runtime_error("cannot create DjVu format");
	}

	QByteArray getCover(QIODevice& stream) const
	{
		const auto document = OpenDocument(stream);
		if (!document)
			return {};

		PagePtr page(m_djvu_page_create_by_page_no(document.get(), 0), { .pageJob = m_djvu_page_job, .jobRelease = m_djvu_job_release });
		if (!page)
			return {};

		while (m_djvu_job_status(m_djvu_page_job(page.get())) < DDJVU_JOB_OK)
			WaitForDjvuMessage(DDJVU_PAGEINFO);

		ddjvu_rect_t pageRect, rendRect;
		if ((pageRect.w = m_djvu_page_get_width(page.get())) < 16)
			return {};

		if ((pageRect.h = m_djvu_page_get_height(page.get())) < 16)
			return {};

		pageRect.x = 0;
		pageRect.y = 0;

		static constexpr unsigned int maxImageSize = 1440;
		Util::FixSize(pageRect.w, pageRect.h, maxImageSize);
		if (pageRect.w > maxImageSize || pageRect.h > maxImageSize)
		{
			if (pageRect.w > pageRect.h)
			{
				pageRect.h = pageRect.h * maxImageSize / pageRect.w;
				pageRect.w = maxImageSize;
			}
			else
			{
				pageRect.w = pageRect.w * maxImageSize / pageRect.h;
				pageRect.h = maxImageSize;
			}
		}

		rendRect = pageRect;

		m_djvu_format_set_row_order(m_format.get(), 1);
		QImage image(static_cast<int>(pageRect.w), static_cast<int>(pageRect.h), QImage::Format_RGB32);
		if (!m_djvu_page_render(page.get(), DDJVU_RENDER_COLOR, &pageRect, &rendRect, m_format.get(), static_cast<unsigned long>(image.bytesPerLine()), reinterpret_cast<char*>(image.bits())))
			return {};

		QByteArray result;
		{
			QBuffer buffer(&result);
			buffer.open(QIODevice::WriteOnly);
			if (!image.save(&buffer, "png"))
				return {};
		}

		return result;
	}

private:
	DocumentPtr OpenDocument(QIODevice& stream) const
	{
		DocumentPtr document(m_djvu_document_create(m_context.get(), "protocol://machine/path", 0), { .documentJob = m_djvu_document_job, .jobRelease = m_djvu_job_release });
		if (!document) [[unlikely]]
		{
			PLOGW << "cannot create decoder";
			return {};
		}

		WaitForDjvuMessage(DDJVU_NEWSTREAM, [&](const ddjvu_message_s& msg) {
			const auto buffer = stream.readAll();
			m_djvu_stream_write(document.get(), msg.m_newstream.streamid, buffer.constData(), static_cast<unsigned long>(buffer.size()));
			m_djvu_stream_close(document.get(), msg.m_newstream.streamid, false);
		});

		while (m_djvu_job_status(m_djvu_document_job(document.get())) < DDJVU_JOB_OK)
			WaitForDjvuMessage(DDJVU_DOCINFO);

		return document;
	}

	void WaitForDjvuMessage(
		const ddjvu_message_tag_t                          tag,
		const std::function<void(const ddjvu_message_s&)>& callback = [](const auto&) {
		}
	) const
	{
		m_djvu_message_wait(m_context.get());
		for (const ddjvu_message_t* msg = m_djvu_message_peek(m_context.get()); msg; msg = m_djvu_message_peek(m_context.get()))
		{
			const ScopedCall messageGuard([this] {
				m_djvu_message_pop(m_context.get());
			});

			if (msg->m_any.tag == tag)
				return callback(*msg);

			if (msg->m_any.tag == DDJVU_ERROR) [[unlikely]]
			{
				if (msg->m_error.filename)
				{
					PLOGW << msg->m_error.message << ": " << std::quoted(msg->m_error.filename) << ":" << msg->m_error.lineno;
				}
				else
				{
					PLOGW << msg->m_error.message;
				}
				return;
			}
		}
	}

	std::unique_ptr<Platform::DyLib> m_lib { CreateLibrary() };

	DjvuContextCreate      m_djvu_context_create { GetEntryPoint<DjvuContextCreate>(*m_lib, "ddjvu_context_create") };
	DjvuContextRelease     m_djvu_context_release { GetEntryPoint<DjvuContextRelease>(*m_lib, "ddjvu_context_release") };
	DjvuDocumentCreate     m_djvu_document_create { GetEntryPoint<DjvuDocumentCreate>(*m_lib, "ddjvu_document_create") };
	DjvuStreamWrite        m_djvu_stream_write { GetEntryPoint<DjvuStreamWrite>(*m_lib, "ddjvu_stream_write") };
	DjvuStreamClose        m_djvu_stream_close { GetEntryPoint<DjvuStreamClose>(*m_lib, "ddjvu_stream_close") };
	DjvuDocumentJob        m_djvu_document_job { GetEntryPoint<DjvuDocumentJob>(*m_lib, "ddjvu_document_job") };
	DjvuJobStatus          m_djvu_job_status { GetEntryPoint<DjvuJobStatus>(*m_lib, "ddjvu_job_status") };
	DjvuJobRelease         m_djvu_job_release { GetEntryPoint<DjvuJobRelease>(*m_lib, "ddjvu_job_release") };
	DjvuMessageWait        m_djvu_message_wait { GetEntryPoint<DjvuMessageWait>(*m_lib, "ddjvu_message_wait") };
	DjvuMessagePeek        m_djvu_message_peek { GetEntryPoint<DjvuMessagePeek>(*m_lib, "ddjvu_message_peek") };
	DjvuMessagePop         m_djvu_message_pop { GetEntryPoint<DjvuMessagePop>(*m_lib, "ddjvu_message_pop") };
	DjvuPageCreateByPageNo m_djvu_page_create_by_page_no { GetEntryPoint<DjvuPageCreateByPageNo>(*m_lib, "ddjvu_page_create_by_pageno") };
	DjvuPageJob            m_djvu_page_job { GetEntryPoint<DjvuPageJob>(*m_lib, "ddjvu_page_job") };
	DjvuPageRender         m_djvu_page_render { GetEntryPoint<DjvuPageRender>(*m_lib, "ddjvu_page_render") };
	DjvuPageGetWidth       m_djvu_page_get_width { GetEntryPoint<DjvuPageGetWidth>(*m_lib, "ddjvu_page_get_width") };
	DjvuPageGetHeight      m_djvu_page_get_height { GetEntryPoint<DjvuPageGetHeight>(*m_lib, "ddjvu_page_get_height") };
	DjvuFormatCreate       m_djvu_format_create { GetEntryPoint<DjvuFormatCreate>(*m_lib, "ddjvu_format_create") };
	DjvuFormatRelease      m_djvu_format_release { GetEntryPoint<DjvuFormatRelease>(*m_lib, "ddjvu_format_release") };
	DjvuFormatSetRowOrder  m_djvu_format_set_row_order { GetEntryPoint<DjvuFormatSetRowOrder>(*m_lib, "ddjvu_format_set_row_order") };

	std::unique_ptr<ddjvu_context_t, DjvuContextRelease> m_context { m_djvu_context_create("FLibrary"), m_djvu_context_release };
	std::unique_ptr<ddjvu_format_t, DjvuFormatRelease>   m_format { m_djvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, MASKS), m_djvu_format_release };
};

using CoverGetter = QByteArray (*)(QIODevice& /*stream*/);

std::unique_ptr<DjVuParser> DJVU_PARSER;

QByteArray GetCoverImpl(QIODevice& stream)
{
	assert(DJVU_PARSER);
	return DJVU_PARSER->getCover(stream);
}

QByteArray GetCoverStub(QIODevice& /*stream*/)
{
	return {};
}

CoverGetter COVER_GETTER = &GetCoverStub;

} // namespace

Initializer::Initializer()
{
	try
	{
		DJVU_PARSER  = std::make_unique<DjVuParser>();
		COVER_GETTER = &GetCoverImpl;
	}
	catch (const std::exception& ex)
	{
		PLOGW << ex.what();
	}
}

Initializer::~Initializer()
{
	COVER_GETTER = &GetCoverStub;
	DJVU_PARSER.reset();
}

namespace HomeCompa::DjVu
{

QByteArray GetCover(QIODevice& stream)
{
	return COVER_GETTER(stream);
}

} // namespace HomeCompa::DjVu
