#include "Fb2InpxParser.h"

#include <QFileInfo>

#include "fnd/FindPair.h"

#include "util/xml/SaxParser.h"
#include "util/xml/XmlAttributes.h"

#include "GenresLocalization.h"
#include "QtTypes.h"
#include "StrUtil.h"
#include "log.h"
#include "zip.h"

using namespace HomeCompa;
using namespace Util;
using namespace Fb2InpxParser;

namespace
{

constexpr auto NAME                   = "name";
constexpr auto NUMBER                 = "number";
constexpr auto GENRE                  = "FictionBook/description/title-info/genre";
constexpr auto AUTHOR                 = "FictionBook/description/title-info/author";
constexpr auto AUTHOR_FIRST_NAME      = "FictionBook/description/title-info/author/first-name";
constexpr auto AUTHOR_LAST_NAME       = "FictionBook/description/title-info/author/last-name";
constexpr auto AUTHOR_MIDDLE_NAME     = "FictionBook/description/title-info/author/middle-name";
constexpr auto ANNOTATION             = "FictionBook/description/title-info/annotation";
constexpr auto AUTHOR_DOC             = "FictionBook/description/document-info/author";
constexpr auto AUTHOR_FIRST_NAME_DOC  = "FictionBook/description/document-info/author/first-name";
constexpr auto AUTHOR_LAST_NAME_DOC   = "FictionBook/description/document-info/author/last-name";
constexpr auto AUTHOR_MIDDLE_NAME_DOC = "FictionBook/description/document-info/author/middle-name";
constexpr auto BOOK_TITLE             = "FictionBook/description/title-info/book-title";
constexpr auto LANG                   = "FictionBook/description/title-info/lang";
constexpr auto SEQUENCE               = "FictionBook/description/title-info/sequence";
constexpr auto KEYWORDS               = "FictionBook/description/title-info/keywords";
constexpr auto PUBLISH_INFO_YEAR      = "FictionBook/description/publish-info/year";

struct Data
{
	struct Author
	{
		QString first;
		QString last;
		QString middle;
	};

	using Authors = std::vector<Author>;

	Authors     authors;
	QStringList genres;
	QString     title;
	QString     lang;
	QString     series;
	QString     keywords;
	QString     seqNumber;
	QString     year;
	size_t      size { 0 };

	QStringList annotation;

	QString error;
};

QString AuthorsToString(const Data::Authors& authors)
{
	QStringList values;
	values.reserve(static_cast<int>(authors.size()));
	std::ranges::transform(authors, std::back_inserter(values), [](const auto& author) {
		return (QStringList() << author.last << author.first << author.middle).join(NAMES_SEPARATOR);
	});
	return values.join(LIST_SEPARATOR) + LIST_SEPARATOR;
}

QString GenresToString(const QStringList& genres)
{
	return genres.empty() ? QString {} : genres.join(LIST_SEPARATOR) + LIST_SEPARATOR;
}

class Fb2InpxParserImpl final : public SaxParser
{
public:
	Fb2InpxParserImpl(QIODevice& stream, const QString& fileName)
		: SaxParser(stream, 512)
		, m_fileName(fileName)
	{
	}

	Data GetData()
	{
		Parse();

		auto data = std::move(m_data);
		m_data    = {};
		return data;
	}

private: // SaxParser
	bool OnStartElement(const QString& /*name*/, const QString& path, const XmlAttributes& attributes) override
	{
		using ParseElementFunction = bool (Fb2InpxParserImpl::*)(const XmlAttributes&);
		using ParseElementItem     = std::pair<const char*, ParseElementFunction>;
		static constexpr ParseElementItem PARSERS[] {
			{     AUTHOR,     &Fb2InpxParserImpl::OnStartElementAuthor },
			{ AUTHOR_DOC,  &Fb2InpxParserImpl::OnStartElementAuthorDoc },
			{   SEQUENCE,   &Fb2InpxParserImpl::OnStartElementSequence },
			{ ANNOTATION, &Fb2InpxParserImpl::OnStartElementAnnotation },
		};

		return Parse(*this, PARSERS, path, attributes);
	}

	bool OnEndElement(const QString& /*name*/, const QString& path) override
	{
		using ParseElementFunction = bool (Fb2InpxParserImpl::*)();
		using ParseElementItem     = std::pair<const char*, ParseElementFunction>;
		static constexpr ParseElementItem PARSERS[] {
			{     AUTHOR,     &Fb2InpxParserImpl::OnEndElementAuthor },
			{ AUTHOR_DOC,     &Fb2InpxParserImpl::OnEndElementAuthor },
			{ ANNOTATION, &Fb2InpxParserImpl::OnEndElementAnnotation },
		};

		return Parse(*this, PARSERS, path);
	}

	bool OnCharacters(const QString& path, const QString& value) override
	{
		using ParseCharacterFunction = bool (Fb2InpxParserImpl::*)(const QString&);
		using ParseCharacterItem     = std::pair<const char*, ParseCharacterFunction>;
		static constexpr ParseCharacterItem PARSERS[] {
			{				  GENRE,            &Fb2InpxParserImpl::ParseGenre },
			{      AUTHOR_FIRST_NAME,  &Fb2InpxParserImpl::ParseAuthorFirstName },
			{       AUTHOR_LAST_NAME,   &Fb2InpxParserImpl::ParseAuthorLastName },
			{     AUTHOR_MIDDLE_NAME, &Fb2InpxParserImpl::ParseAuthorMiddleName },
			{  AUTHOR_FIRST_NAME_DOC,  &Fb2InpxParserImpl::ParseAuthorFirstName },
			{   AUTHOR_LAST_NAME_DOC,   &Fb2InpxParserImpl::ParseAuthorLastName },
			{ AUTHOR_MIDDLE_NAME_DOC, &Fb2InpxParserImpl::ParseAuthorMiddleName },
			{             BOOK_TITLE,        &Fb2InpxParserImpl::ParseBookTitle },
			{				   LANG,             &Fb2InpxParserImpl::ParseLang },
			{			   KEYWORDS,         &Fb2InpxParserImpl::ParseKeywords },
			{      PUBLISH_INFO_YEAR,      &Fb2InpxParserImpl::ParsePublishYear },
		};

		if (m_annotationMode)
			m_data.annotation << value.trimmed();

		{
			auto valueCopy = value;
			PrepareTitle(valueCopy);
			RemoveIf(valueCopy, [](const QChar ch) {
				const auto category = ch.category();
				return category < QChar::Letter_Lowercase || category > QChar::Letter_Other;
			});
			m_data.size += valueCopy.length();
		}

		return Parse(*this, PARSERS, path, value.trimmed());
	}

	bool OnWarning(const size_t line, const size_t column, const QString& text) override
	{
		PLOGW << m_fileName << " " << line << ":" << column << " " << text;
		return true;
	}

	bool OnError(const size_t line, const size_t column, const QString& text) override
	{
		m_data.error = text;
		PLOGE << m_fileName << " " << line << ":" << column << " " << text;
		return false;
	}

	bool OnFatalError(const size_t line, const size_t column, const QString& text) override
	{
		return OnError(line, column, text);
	}

private:
	bool OnStartElementAuthor(const XmlAttributes&)
	{
		m_insertAuthorMode = true;
		m_data.authors.emplace_back();
		return true;
	}

	bool OnStartElementAuthorDoc(const XmlAttributes& attributes)
	{
		return m_data.authors.empty() ? OnStartElementAuthor(attributes) : true;
	}

	bool OnStartElementSequence(const XmlAttributes& attributes)
	{
		m_data.series    = attributes.GetAttribute(NAME).trimmed();
		m_data.seqNumber = GetSeqNumber(attributes.GetAttribute(NUMBER));
		return true;
	}

	bool OnStartElementAnnotation(const XmlAttributes&)
	{
		m_annotationMode = true;
		return true;
	}

	bool OnEndElementAuthor()
	{
		if (m_insertAuthorMode)
		{
			assert(!m_data.authors.empty());
			const auto& author = m_data.authors.back();
			if (author.first.isEmpty() && author.last.isEmpty() && author.middle.isEmpty())
				m_data.authors.pop_back();
		}

		m_insertAuthorMode = false;
		return true;
	}

	bool OnEndElementAnnotation()
	{
		m_annotationMode = false;
		return true;
	}

	bool ParseGenre(const QString& value)
	{
		ParseGenresString(m_data.genres, value);
		return true;
	}

	bool ParseAuthorFirstName(const QString& value)
	{
		if (m_insertAuthorMode)
			m_data.authors.back().first = value;
		return true;
	}

	bool ParseAuthorLastName(const QString& value)
	{
		if (m_insertAuthorMode)
			m_data.authors.back().last = value;
		return true;
	}

	bool ParseAuthorMiddleName(const QString& value)
	{
		if (m_insertAuthorMode)
			m_data.authors.back().middle = value;
		return true;
	}

	bool ParseBookTitle(const QString& value)
	{
		m_data.title = value;
		return true;
	}

	bool ParseLang(const QString& value)
	{
		m_data.lang = value;
		return true;
	}

	bool ParseKeywords(const QString& value)
	{
		m_data.keywords = value;
		return true;
	}

	bool ParsePublishYear(const QString& value)
	{
		m_data.year = value;
		return true;
	}

private:
	const QString& m_fileName;
	Data           m_data {};
	bool           m_insertAuthorMode { false };
	bool           m_annotationMode { false };
};

} // namespace

namespace HomeCompa::Util::Fb2InpxParser
{

ParseResult Parse(const QString& folder, const Zip& zip, const QString& fileName, const QDateTime& zipDateTime, const bool isDeleted)
{
	try
	{
		QFileInfo         fileInfo(fileName);
		const auto        stream = zip.Read(fileName);
		Fb2InpxParserImpl parser(stream->GetStream(), fileName);
		auto              parserData = parser.GetData();

		if (!parserData.error.isEmpty())
		{
			PLOGE << QString("%1/%2: %3").arg(folder, fileName, parserData.error);
			return {};
		}

		if (parserData.authors.empty())
		{
			PLOGW << QString("%1/%2: author empty").arg(folder, fileName);
		}

		if (parserData.title.isEmpty())
		{
			PLOGW << QString("%1/%2: title empty").arg(folder, fileName);
		}

		if (zip.GetFileSize(fileName) == 0)
		{
			PLOGW << QString("%1/%2: file empty").arg(folder, fileName);
		}

		const auto& fileDateTime = zip.GetFileTime(fileName);
		auto        dateTime     = (fileDateTime.isValid() ? fileDateTime : zipDateTime).toString("yyyy-MM-dd");

		//"AUTHOR;GENRE;TITLE;SERIES;SERNO;FILE;SIZE;LIBID;DEL;EXT;DATE;LANG;LIBRATE;KEYWORDS;YEAR;"
		const auto values = QStringList() << AuthorsToString(parserData.authors) << GenresToString(parserData.genres) << parserData.title << parserData.series << parserData.seqNumber
		                                  << fileInfo.completeBaseName() << QString::number(parserData.size) << fileInfo.completeBaseName() << (isDeleted ? "1" : "0") << fileInfo.suffix()
		                                  << std::move(dateTime) << parserData.lang << "0" << parserData.keywords << parserData.year;

		return { .line = values.join(FIELDS_SEPARATOR), .annotation = std::move(parserData.annotation) };
	}
	catch (const std::exception& ex)
	{
		PLOGE << ex.what();
	}
	catch (...)
	{
		PLOGE << "unknown error";
	}

	return {};
}

QString GetSeqNumber(QString seqNumber)
{
	bool ok = false;
	if (const auto value = seqNumber.toInt(&ok); ok && value > 0)
		return seqNumber;
	return QString {};
}

} // namespace HomeCompa::Util::Fb2InpxParser
