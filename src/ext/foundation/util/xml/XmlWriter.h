#pragma once

#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"

#include "export/util.h"

class QString;
class QIODevice;

namespace HomeCompa::Util
{

class XmlAttributes;

namespace details
{

enum class XmlWriterType
{
	Xml,
	Html,
	Headless,
};

struct XmlWriterOptions
{
	XmlWriterType type { XmlWriterType::Xml };
	bool          indented { true };
	const char*   encoding { "utf-8" };
};

}

class UTIL_EXPORT XmlWriter
{
	NON_COPY_MOVABLE(XmlWriter)

public:
	using Type    = details::XmlWriterType;
	using Options = details::XmlWriterOptions;

public:
	class XmlNodeGuard
	{
		NON_COPYABLE(XmlNodeGuard)

	public:
		XmlNodeGuard(XmlNodeGuard&& rhs) noexcept
			: m_writer { rhs.m_writer }
		{
			rhs.m_writer = nullptr;
		}

		XmlNodeGuard& operator=(XmlNodeGuard&& rhs) noexcept
		{
			if (this != &rhs)
			{
				m_writer     = rhs.m_writer;
				rhs.m_writer = nullptr;
			}
			return *this;
		}

		XmlNodeGuard(XmlWriter& writer, const QString& name)
			: m_writer(&writer)
		{
			assert(m_writer);
			(void)m_writer->WriteStartElement(name);
		}

		~XmlNodeGuard()
		{
			if (m_writer)
				(void)m_writer->WriteEndElement();
		}

		XmlWriter* operator->() const noexcept
		{
			assert(m_writer);
			return m_writer;
		}

	private:
		XmlWriter* m_writer;
	};

public:
	explicit XmlWriter(QIODevice& stream, const Options& options = Options {});
	~XmlWriter();

	XmlWriter& WriteProcessingInstruction(const QString& target, const QString& data);
	XmlWriter& WriteStartElement(const QString& name);
	XmlWriter& WriteStartElement(const QString& name, const XmlAttributes& attributes);
	XmlWriter& WriteEndElement();
	XmlWriter& WriteAttribute(const QString& name, const QString& value);
	XmlWriter& WriteCharacters(const QString& data);
	XmlWriter& CloseTag();

	XmlNodeGuard Guard(const QString& name);

private:
	class Impl;
	PropagateConstPtr<Impl> m_impl;
};

} // namespace HomeCompa::Util
