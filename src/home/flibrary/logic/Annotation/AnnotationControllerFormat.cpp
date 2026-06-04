#include "AnnotationControllerFormat.h"

#include <cmath>
#include <ranges>

#include "interface/constants/Enums.h"
#include "interface/constants/ExportStat.h"
#include "interface/constants/ProductConstant.h"
#include "interface/localization.h"

#include "data/DataItem.h"
#include "util/language.h"

#include "log.h"

using namespace HomeCompa;
using namespace HomeCompa::Flibrary;
using namespace HomeCompa::Flibrary::Constant;

namespace
{

constexpr auto CONTEXT           = "Annotation";

constexpr auto KEYWORDS_FB2      = QT_TRANSLATE_NOOP("Annotation", "Keywords: %1");
constexpr auto FILENAME          = QT_TRANSLATE_NOOP("Annotation", "File:");
constexpr auto SOURCE_LIBRARY    = QT_TRANSLATE_NOOP("Annotation", "Source:");
constexpr auto BOOK_SIZE         = QT_TRANSLATE_NOOP("Annotation", "Size:");
constexpr auto IMAGES            = QT_TRANSLATE_NOOP("Annotation", "Images:");
constexpr auto TRANSLATORS       = QT_TRANSLATE_NOOP("Annotation", "Translators:");
constexpr auto TEXT_SIZE         = QT_TRANSLATE_NOOP("Annotation", "%L1 letters (%2%3 pages, %2%L4 words)");
constexpr auto EXPORT_STATISTICS = QT_TRANSLATE_NOOP("Annotation", "Export statistics:");
constexpr auto OR_SUFFIX         = QT_TRANSLATE_NOOP("Annotation", " or %1");
constexpr auto TRANSLATION_FROM  = QT_TRANSLATE_NOOP("Annotation", ", translated from %1");
constexpr auto REVIEWS           = QT_TRANSLATE_NOOP("Annotation", "Readers' Reviews");

TR_DEF

constexpr auto ERROR_PATTERN    = R"(<p style="font-style:italic;">%1</p>)";
constexpr auto TITLE_PATTERN    = "<p align=center><b>%1</b></p>";
constexpr auto EPIGRAPH_PATTERN = R"(<p align=right style="font-style:italic;">%1</p>)";

template <typename T>
T Round(const T value, const int digits)
{
	if (value == 0)
		return 0;

	const double factor = pow(10.0, digits + ceil(log10(value)));
	if (factor < 10.0)
		return value;

	return static_cast<T>(static_cast<int64_t>(value / factor + 0.5) * factor + 0.5);
}

QString GetTitle(const IDataItem& item)
{
	return item.GetData(DataItem::Column::Title);
}

QString GetTitleAuthor(const IDataItem& item)
{
	auto result = item.GetData(AuthorItem::Column::LastName);
	AppendTitle(result, item.GetData(AuthorItem::Column::FirstName));
	AppendTitle(result, item.GetData(AuthorItem::Column::MiddleName));
	return result;
}

using TitleGetter = QString (*)(const IDataItem& item);

QString Join(const std::vector<QString>& strings, const QString& delimiter = ", ")
{
	if (strings.empty())
		return {};

	auto result = strings.front();
	std::ranges::for_each(strings | std::views::drop(1), [&](const QString& item) {
		result.append(delimiter).append(item);
	});

	return result;
}

QString Urls(const IAnnotationController::IStrategy& strategy, const char* type, const IDataItem& parent, const TitleGetter titleGetter = &GetTitle)
{
	std::vector<QString> urls;
	for (size_t i = 0, sz = parent.GetChildCount(); i < sz; ++i)
	{
		const auto item = parent.GetChild(i);
		urls.emplace_back(strategy.GenerateUrl(type, item->GetId(), titleGetter(*item)));
	}

	return Join(urls);
}

QString GetPublishInfo(const IAnnotationController::IDataProvider& dataProvider)
{
	QString result = dataProvider.GetPublisher();
	AppendTitle(result, dataProvider.GetPublishCity(), ", ");
	AppendTitle(result, dataProvider.GetPublishYear(), ", ");
	const auto isbn = dataProvider.GetPublishIsbn().isEmpty() ? QString {} : QString("ISBN %1").arg(dataProvider.GetPublishIsbn());
	AppendTitle(result, isbn, ". ");
	return result;
}

struct Table
{
	explicit Table(const IAnnotationController::IStrategy& strategy)
		: m_strategy { strategy }
	{
	}

	Table& Add(const char* name, const QString& value)
	{
		if (!value.isEmpty())
			m_data << m_strategy.AddTableRow(name, value);

		return *this;
	}

	Table& Add(const QStringList& values)
	{
		m_data << m_strategy.AddTableRow(values);
		return *this;
	}

	[[nodiscard]] QString ToString() const
	{
		return m_strategy.TableRowsToString(m_data);
	}

private:
	QStringList                             m_data;
	const IAnnotationController::IStrategy& m_strategy;
};

QString TranslateLang(const QString& code)
{
	const auto  it       = std::ranges::find(LANGUAGES, code, [](const auto& item) {
		return item.key;
	});
	const auto* language = it != std::end(LANGUAGES) ? it->title : UNDEFINED;
	return Loc::Tr(LANGUAGES_CONTEXT, language);
}

Table CreateUrlTable(const IAnnotationController::IDataProvider& dataProvider, const IAnnotationController::IStrategy& strategy)
{
	const auto& book         = dataProvider.GetBook();
	const auto& keywords     = dataProvider.GetKeywords();
	const auto& lang         = book.GetRawData(BookItem::Column::Lang);
	const auto& fbLang       = dataProvider.GetLanguage();
	const auto& fbSourceLang = dataProvider.GetSourceLanguage();

	const auto langFlags = dataProvider.GetFlags(NavigationMode::Languages, { lang, fbLang, fbSourceLang });

	const auto langTr  = TranslateLang(lang);
	auto       langStr = strategy.GenerateUrl(Loc::LANGUAGE, lang, langTr, !!(langFlags[0] & IDataItem::Flags::Filtered));

	if (!fbLang.isEmpty())
		if (const auto fbLangTr = TranslateLang(fbLang); fbLangTr != langTr && fbLangTr != Loc::Tr(LANGUAGES_CONTEXT, UNDEFINED))
			langStr.append(Tr(OR_SUFFIX).arg(strategy.GenerateUrl(Loc::LANGUAGE, fbLang, fbLangTr, !!(langFlags[1] & IDataItem::Flags::Filtered))));

	if (!fbSourceLang.isEmpty() && fbSourceLang != lang && fbSourceLang != fbLang)
		if (const auto fbSourceLangTr = TranslateLang(fbSourceLang); fbSourceLangTr != langTr)
			langStr.append(Tr(TRANSLATION_FROM).arg(strategy.GenerateUrl(Loc::LANGUAGE, fbSourceLang, fbSourceLangTr, !!(langFlags[2] & IDataItem::Flags::Filtered))));

	Table table(strategy);
	table.Add(Loc::AUTHORS, Urls(strategy, Loc::AUTHORS, dataProvider.GetAuthors(), &GetTitleAuthor))
		.Add(Loc::SERIES, Urls(strategy, Loc::SERIES, dataProvider.GetSeries()))
		.Add(Loc::GENRES, Urls(strategy, Loc::GENRES, dataProvider.GetGenres()))
		.Add(Loc::PUBLISH_YEAR, strategy.GenerateUrl(Loc::PUBLISH_YEAR, book.GetRawData(BookItem::Column::Year), book.GetRawData(BookItem::Column::Year)))
		.Add(Loc::ARCHIVE, Urls(strategy, Loc::ARCHIVE, dataProvider.GetFolder()))
		.Add(Loc::GROUPS, Urls(strategy, Loc::GROUPS, dataProvider.GetGroups()))
		.Add(Loc::KEYWORDS, Urls(strategy, Loc::KEYWORDS, keywords))
		.Add(Loc::LANGUAGE, langStr)
		.Add(Loc::UPDATES, Urls(strategy, Loc::UPDATES, dataProvider.GetUpdate()));

	return table;
}

} // namespace

namespace HomeCompa::Flibrary
{

QString CreateBookAnnotation(const IAnnotationController::IDataProvider& dataProvider, const IAnnotationController::IStrategy& strategy)
{
	const auto& book = dataProvider.GetBook();
	if (book.GetId().isEmpty())
		return {};

	QString annotation;
	strategy.Add(IAnnotationController::IStrategy::Section::Title, annotation, strategy.GenerateUrl(Constant::BOOK, book.GetId(), book.GetRawData(BookItem::Column::Title)), TITLE_PATTERN);
	strategy.Add(IAnnotationController::IStrategy::Section::Epigraph, annotation, dataProvider.GetEpigraph(), EPIGRAPH_PATTERN);
	strategy.Add(IAnnotationController::IStrategy::Section::EpigraphAuthor, annotation, dataProvider.GetEpigraphAuthor(), EPIGRAPH_PATTERN);
	strategy.Add(IAnnotationController::IStrategy::Section::Annotation, annotation, dataProvider.GetAnnotation());

	auto& keywords = dataProvider.GetKeywords();
	if (keywords.GetChildCount() == 0)
		strategy.Add(IAnnotationController::IStrategy::Section::Keywords, annotation, Join(dataProvider.GetFb2Keywords()), KEYWORDS_FB2);

	strategy.Add(IAnnotationController::IStrategy::Section::UrlTable, annotation, CreateUrlTable(dataProvider, strategy).ToString());

	if (const auto translators = dataProvider.GetTranslators(); translators && translators->GetChildCount() > 0)
	{
		QStringList translatorList;
		translatorList.reserve(static_cast<int>(translators->GetChildCount()));
		for (size_t i = 0, sz = translators->GetChildCount(); i < sz; ++i)
			translatorList << GetAuthorFull(*translators->GetChild(i));
		Table table(strategy);
		table.Add(TRANSLATORS, translatorList.join(", "));
		strategy.Add(IAnnotationController::IStrategy::Section::Translators, annotation, table.ToString());
	}

	{
		auto info = Table(strategy).Add(FILENAME, book.GetRawData(BookItem::Column::FileName)).Add(SOURCE_LIBRARY, dataProvider.GetSourceLibrary());
		if (dataProvider.GetTextSize() > 0)
			info.Add(
				BOOK_SIZE,
				Tr(TEXT_SIZE).arg(dataProvider.GetTextSize()).arg(QChar(0x2248)).arg(std::max(1ULL, Round(dataProvider.GetTextSize() / 2000ULL, -2))).arg(Round(dataProvider.GetWordCount(), -3))
			);
		info.Add(Loc::RATE, strategy.GenerateLibRateStars(book.GetRawData(BookItem::Column::LibRate).toInt()));
		info.Add(Loc::USER_RATE, strategy.GenerateUserRateStars(book.GetRawData(BookItem::Column::UserRate).toInt()));

		if (const auto& covers = dataProvider.GetCovers(); !covers.empty())
		{
			const auto total = std::accumulate(covers.cbegin(), covers.cend(), qsizetype { 0 }, [](const auto init, const auto& cover) {
				return init + cover.bytes.size();
			});
			info.Add(IMAGES, QString("%1 (%L2)").arg(covers.size()).arg(total));
		}

		strategy.Add(IAnnotationController::IStrategy::Section::BookInfo, annotation, info.ToString());
	}

	strategy.Add(IAnnotationController::IStrategy::Section::PublishInfo, annotation, GetPublishInfo(dataProvider));

	strategy.Add(IAnnotationController::IStrategy::Section::ParseError, annotation, dataProvider.GetError(), ERROR_PATTERN);

	if (!dataProvider.GetExportStatistics().empty())
	{
		const auto toDateList = [](const std::vector<QDateTime>& dates) {
			QStringList result;
			result.reserve(static_cast<int>(dates.size()));
			std::ranges::transform(dates, std::back_inserter(result), [](const QDateTime& date) {
				return date.toString("yy.MM.dd hh:mm");
			});
			return result.join(", ");
		};
		auto exportStatistics = Table(strategy).Add(EXPORT_STATISTICS, " ");
		for (const auto& [type, dates] : dataProvider.GetExportStatistics())
			exportStatistics.Add(GetName(type), toDateList(dates));
		strategy.Add(IAnnotationController::IStrategy::Section::ExportStatistics, annotation, exportStatistics.ToString());
	}

	if (!dataProvider.GetReviews().empty())
	{
		annotation.append(strategy.GetReviewsDelimiter()).append("<hr/>").append(QString(TITLE_PATTERN).arg(Tr(REVIEWS)));
		Table table(strategy);
		for (const auto& review : dataProvider.GetReviews())
			table.Add(QStringList() << review.name << review.time.toString("yyyy.MM.dd<br>hh:mm:ss") << review.text);
		strategy.Add(IAnnotationController::IStrategy::Section::Reviews, annotation, table.ToString());
	}

#ifndef NDEBUG
	PLOGV << annotation;
#endif
	return annotation;
}

} // namespace HomeCompa::Flibrary
