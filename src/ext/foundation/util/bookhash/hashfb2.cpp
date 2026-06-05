#include <set>

#include <QCryptographicHash>

#include "fnd/IsOneOf.h"

#include "xml/SaxParser.h"
#include "xml/XmlAttributes.h"

#include "QtTypes.h"
#include "StrUtil.h"
#include "hashbook.h"
#include "log.h"
#include "parser.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

class Fb2Parser final
	: public SaxParser
	, public BookHash::IParser
{
	static constexpr auto BODY            = "FictionBook/body";
	static constexpr auto BINARY          = "FictionBook/binary";
	static constexpr auto BODY_BINARY     = "FictionBook/body/binary";
	static constexpr auto TITLE           = "FictionBook/description/title-info/book-title";
	static constexpr auto COVERPAGE_IMAGE = "FictionBook/description/title-info/coverpage/image";
	static constexpr auto ANNOTATION      = "FictionBook/description/title-info/annotation";

	static constexpr auto ID      = "id";
	static constexpr auto SECTION = "section";

	struct Section
	{
		Section* parent { nullptr };
		int      depth { 0 };
		size_t   count { 0 };
		size_t   size { 0 };

		Hist                                  hist;
		QString                               hash;
		std::vector<std::unique_ptr<Section>> children;

		HashValues GetHashValues()
		{
			auto [hashValues, h, c, s] = CalculateHash(hist);
			hash                       = std::move(h);
			count                      = c;
			size                       = s;
			return hashValues;
		}
	};

public:
	explicit Fb2Parser(QIODevice& input)
		: SaxParser(input, 512)
	{
		Parse();
		//		assert(m_tags.empty());
	}

private: // BookHash::IParser
	HashParseResult GetResult() override
	{
		QStringList sections;
		const auto  enumerate = [&](const Section& parent, const auto& r) -> void {
			sections << QString("%1\t%2\t%3\t%4").arg(parent.depth).arg(parent.hash).arg(parent.count).arg(parent.size);

			for (const auto& child : parent.children)
				r(*child, r);
		};

		auto hashValues = m_section.GetHashValues();
		enumerate(m_section, enumerate);

		return { .id           = QString::fromUtf8(m_md5.result().toHex()),
			     .title        = std::move(m_title),
			     .hashText     = std::move(m_section.hash),
			     .hashSections = std::move(sections),
			     .annotation   = std::move(m_annotation),
			     .hashValues   = std::move(hashValues) };
	}

	ImageHashItem GetCover() override
	{
		auto result = std::move(m_cover);
		return result;
	}

	ImageHashItems GetImages() override
	{
		auto result = std::move(m_images);
		return result;
	}

private: // Util::SaxParser
	bool OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes) override
	{
		if (name == SECTION)
		{
			m_currentSection = m_currentSection->children.emplace_back(std::make_unique<Section>(m_currentSection, m_currentSection->depth + 1)).get();
			return true;
		}

		if (IsOneOf(path, BINARY, BODY_BINARY))
		{
			m_isBinary = true;
			m_picId    = attributes.GetAttribute(ID).trimmed();
			if (const auto it = std::ranges::find_if(
					m_picId,
					[](const auto ch) {
						return ch != '#';
					}
				);
			    it != m_picId.end())
				m_picId = Last(m_picId, std::distance(it, m_picId.end())).trimmed();
			return true;
		}

		if (path == ANNOTATION)
		{
			m_isAnnotation = true;
			return true;
		}

		if (path == COVERPAGE_IMAGE)
		{
			for (size_t i = 0, sz = attributes.GetCount(); i < sz; ++i)
			{
				auto attributeName  = attributes.GetName(i);
				auto attributeValue = attributes.GetValue(i);
				if (attributeName.endsWith(":href"))
				{
					if (const auto it = std::ranges::find_if(
							attributeValue,
							[](const auto ch) {
								return ch != '#';
							}
						);
					    it != attributeValue.end())
						m_coverPage = Last(attributeValue, std::distance(it, attributeValue.end())).trimmed();
					break;
				}
			}
			return true;
		}
		return true;
	}

	bool OnEndElement(const QString& name, const QString& path) override
	{
		if (IsOneOf(path, BINARY, BODY_BINARY))
		{
			m_isBinary = false;
		}
		else if (name == SECTION)
		{
			m_currentSection->GetHashValues();
			m_currentSection = m_currentSection->parent;
			assert(m_currentSection);
		}
		else if (path == ANNOTATION)
		{
			m_isAnnotation = false;
		}

		return true;
	}

	bool OnCharacters(const QString& path, const QString& value) override
	{
		if (!m_picId.isEmpty())
		{
			if (!m_isBinary || !IsOneOf(path, BINARY, BODY_BINARY))
			{
				PLOGW << "bad binary";
				m_picId = QString {};
				return true;
			}

			const auto isCover       = m_picId == m_coverPage;
			auto&      imageItemHash = isCover ? m_cover : m_images.emplace_back();
			imageItemHash            = { .file = std::move(m_picId), .body = QByteArray::fromBase64(value.toUtf8()) };
			m_picId                  = QString {};

			return true;
		}

		if (m_isAnnotation)
			m_annotation << value;

		auto valueCopy = value;

		PrepareTitle(valueCopy);

		if (path == TITLE)
			return (m_title = SimplifyTitle(valueCopy)), true;

		if (path.startsWith(BODY, Qt::CaseInsensitive))
		{
			for (auto&& word : valueCopy.split(' ', Qt::SkipEmptyParts))
			{
				UpdateHash(word);

				RemoveIf(word, [](const QChar ch) {
					const auto category = ch.category();
					return category < QChar::Letter_Lowercase || category > QChar::Letter_Other;
				});
				if (word.isEmpty())
					continue;

				for (auto* section = m_currentSection; section; section = section->parent)
					++section->hist[word];
			}
		}

		return true;
	}

	bool OnFatalError(const size_t line, const size_t column, const QString& text) override
	{
		return OnError(line, column, text);
	}

private:
	void UpdateHash(QString value)
	{
		RemoveIf(value, [](const QChar ch) {
			return ch.category() != QChar::Letter_Lowercase;
		});
		m_md5.addData(value.toUtf8());
	}

private:
	QString            m_title;
	Section            m_section;
	Section*           m_currentSection { &m_section };
	QCryptographicHash m_md5 { QCryptographicHash::Md5 };

	ImageHashItem  m_cover;
	ImageHashItems m_images;
	QStringList    m_annotation;

	bool    m_isAnnotation { false };
	bool    m_isBinary { false };
	QString m_coverPage;
	QString m_picId;
};

} // namespace

namespace HomeCompa::Util::BookHash
{

std::unique_ptr<IParser> create_fb2_parser(QIODevice& stream)
{
	return std::make_unique<Fb2Parser>(stream);
}

} // namespace HomeCompa::Util::BookHash
