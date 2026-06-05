#include <ranges>

#include <QBuffer>
#include <QCryptographicHash>

#include "EpubParser.h"
#include "hashbook.h"
#include "log.h"
#include "parser.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

class EpubParserImpl final : public BookHash::IParser
{
public:
	explicit EpubParserImpl(QIODevice& stream)
		: m_result { Parse(stream, EpubParser::Mode::All) }
	{
	}

private: // BookHash::IParser
	HashParseResult GetResult() override
	{
		static constexpr const char* textExt[] { ".htm", ".html", ".xhtml", ".xml" };

		QCryptographicHash                  md5 { QCryptographicHash::Md5 };
		QStringList                         sections;
		std::unordered_map<QString, size_t> hist;

		for (auto [id, body] : m_result.texts | std::views::filter([](const auto& item) {
								   return std::ranges::any_of(textExt, [&](const char* ext) {
									   return item.id.endsWith(ext, Qt::CaseInsensitive);
								   });
							   }))
		{
#ifdef ADDITIONAL_LOG_ENABLED
			PLOGV << "process " << id;
#endif

			auto sectionHist = CollectHistogram(body, md5);

			for (const auto& [word, count] : sectionHist)
				hist[word] += count;

			auto [hashValues, hash, count, size] = CalculateHash(sectionHist);
			sections << QString("1\t%1\t%2\t%3").arg(hash).arg(count).arg(size);
		}

		auto [hashValues, hash, count, size] = CalculateHash(hist);
		sections.push_front(QString("0\t%1\t%2\t%3").arg(hash).arg(count).arg(size));

		HashParseResult result {
			.id           = QString::fromUtf8(md5.result().toHex()),
			.title        = std::move(m_result.title),
			.hashText     = std::move(hash),
			.hashSections = std::move(sections),
			.annotation   = QStringList { std::move(m_result.annotation) },
			.hashValues   = std::move(hashValues),
		};
		return result;
	}

	ImageHashItem GetCover() override
	{
		if (!m_result.coverExists)
			return {};

		assert(!m_result.images.empty());
		auto& cover = m_result.images.front();
		return { .file = std::move(cover.id), .body = std::move(cover.body) };
	}

	ImageHashItems GetImages() override
	{
		return m_result.images | std::views::as_rvalue | std::views::drop(m_result.coverExists ? 1 : 0) | std::views::transform([](auto&& item) {
				   return ImageHashItem { .file = std::move(item.id), .body = std::move(item.body) };
			   })
		     | std::ranges::to<std::vector>();
	}

private:
	EpubParser::ParseResult m_result;
};

} // namespace

namespace HomeCompa::Util::BookHash
{

std::unique_ptr<IParser> create_epub_parser(QIODevice& stream)
{
	return std::make_unique<EpubParserImpl>(stream);
}

} // namespace HomeCompa::Util::BookHash
