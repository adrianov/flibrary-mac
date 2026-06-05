#include "hashparser.h"

#include "xml/SaxParser.h"
#include "xml/XmlAttributes.h"

#include "Constant.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

class HashParserImpl final : public SaxParser
{
	static constexpr auto BOOKS      = "books";
	static constexpr auto BOOK       = "books/book";
	static constexpr auto COVER      = "books/book/cover";
	static constexpr auto IMAGE      = "books/book/image";
	static constexpr auto ORIGIN     = "books/book/origin";
	static constexpr auto HISTOGRAM  = "books/book/histogram/item";
	static constexpr auto SECTION    = "section";
	static constexpr auto ANNOTATION = "books/book/annotation/p";

public:
	HashParserImpl(QIODevice& input, HashParser::IObserver& observer)
		: SaxParser(input, 512)
		, m_observer { observer }
	{
		Parse();
	}

private: // Util::SaxParser
	bool OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes) override
	{
		if (path == BOOKS)
		{
			m_observer.OnParseStarted(attributes.GetAttribute("source"));
		}
		else if (path == BOOK)
		{
#define HASH_PARSER_CALLBACK_ITEM(NAME) m_##NAME = attributes.GetAttribute(#NAME);
			HASH_PARSER_CALLBACK_ITEMS_X_MACRO
#undef HASH_PARSER_CALLBACK_ITEM
			m_section        = std::make_unique<HashParser::Section>();
			m_currentSection = m_section.get();
		}
		else if (path == ORIGIN)
		{
			m_originFolder = attributes.GetAttribute(Inpx::FOLDER);
			m_originFile   = attributes.GetAttribute(Inpx::FILE);
		}
		else if (name == SECTION)
		{
			auto& section    = m_currentSection->children.try_emplace(attributes.GetAttribute("id"), std::make_unique<HashParser::Section>()).first->second;
			section->count   = attributes.GetAttribute("count").toULongLong();
			section->size    = attributes.GetAttribute("size").toULongLong();
			section->parent  = m_currentSection;
			m_currentSection = section.get();
		}
		else if (path == COVER)
		{
			m_cover.pHash = attributes.GetAttribute("pHash");
		}
		else if (path == IMAGE)
		{
			m_images.emplace_back(attributes.GetAttribute("id"), QString(), attributes.GetAttribute("pHash"));
		}
		else if (path == HISTOGRAM)
		{
			m_textHistogram.emplace_back(attributes.GetAttribute("count").toULongLong(), attributes.GetAttribute("word"));
		}

		return true;
	}

	bool OnEndElement(const QString& name, const QString& path) override
	{
		if (path == BOOK)
		{
			assert(!m_id.isEmpty());
			if (!m_observer.OnBookParsed(
#define HASH_PARSER_CALLBACK_ITEM(NAME) std::move(m_##NAME),
					HASH_PARSER_CALLBACK_ITEMS_X_MACRO
#undef HASH_PARSER_CALLBACK_ITEM
						std::move(m_cover),
					std::move(m_images),
					std::move(m_section),
					std::move(m_textHistogram),
					std::move(m_annotation)
				))
				return false;

#define HASH_PARSER_CALLBACK_ITEM(NAME) m_##NAME = QString {};
			HASH_PARSER_CALLBACK_ITEMS_X_MACRO
#undef HASH_PARSER_CALLBACK_ITEM

			m_cover          = {};
			m_images         = {};
			m_section        = {};
			m_textHistogram  = {};
			m_annotation     = QStringList {};
			m_currentSection = nullptr;
		}
		else if (name == SECTION)
		{
			m_currentSection = m_currentSection->parent;
		}

		return true;
	}

	bool OnCharacters(const QString& path, const QString& value) override
	{
		if (path == COVER)
			m_cover.hash = value;
		else if (path == IMAGE)
			m_images.back().hash = value;
		else if (path == ANNOTATION)
			m_annotation << value;
		return true;
	}

private:
	HashParser::IObserver& m_observer;
#define HASH_PARSER_CALLBACK_ITEM(NAME) QString m_##NAME;
	HASH_PARSER_CALLBACK_ITEMS_X_MACRO
#undef HASH_PARSER_CALLBACK_ITEM

	HashParser::HashImageItem              m_cover;
	std::vector<HashParser::HashImageItem> m_images;
	HashParser::Section::Ptr               m_section;
	HashParser::Section*                   m_currentSection { nullptr };
	TextHistogram                          m_textHistogram;
	QStringList                            m_annotation;
};

} // namespace

void HashParser::Parse(QIODevice& input, IObserver& observer)
{
	[[maybe_unused]] const HashParserImpl parser(input, observer);
}
