#include "AnnotationDbLoader.h"

#include <format>
#include <ranges>
#include <unordered_map>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "fnd/FindPair.h"

#include "database/interface/IQuery.h"
#include "interface/constants/ExportStat.h"
#include "interface/localization.h"

#include "data/DataItem.h"
#include "log.h"
#include "database/DatabaseUtil.h"
#include "Constant.h"
#include "zip.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace
{

using Extractor = IDataItem::Ptr (*)(const DB::IQuery& query);

constexpr auto SERIES_QUERY = "select s.SeriesID, s.SeriesTitle, sl.SeqNumber from Series s join Series_List sl on sl.SeriesID = s.SeriesID and sl.BookID = :id where s.Flags & {} = 0 order by sl.OrdNum";
constexpr auto AUTHORS_QUERY =
	"select a.AuthorID, a.LastName, a.FirstName, a.MiddleName from Authors a join Author_List al on al.AuthorID = a.AuthorID and al.BookID = :id where a.Flags & {} = 0 order by al.OrdNum";
constexpr auto GENRES_QUERY   = "select g.GenreCode, g.GenreAlias from Genres g join Genre_List gl on gl.GenreCode = g.GenreCode and gl.BookID = :id  where g.Flags & {} = 0 order by gl.OrdNum";
constexpr auto GROUPS_QUERY   = "select g.GroupID, g.Title from Groups_User g join Groups_List_User_View gl on gl.GroupID = g.GroupID and gl.BookID = :id";
constexpr auto KEYWORDS_QUERY = "select k.KeywordID, k.KeywordTitle from Keywords k join Keyword_List kl on kl.KeywordID = k.KeywordID and kl.BookID = :id where k.Flags & {} = 0 order by kl.OrdNum";
constexpr auto REVIEWS_QUERY  = "select f.FolderTitle||'#'||b.FileName||b.Ext, r.Folder from Reviews r join Books b on b.BookID = r.BookID join Folders f on f.FolderID = b.FolderID where r.BookID = :id";
constexpr auto FOLDER_QUERY   = "select f.FolderID, f.FolderTitle from Folders f join Books b on b.FolderID = f.FolderID and b.BookID = :id";
constexpr auto UPDATE_QUERY   = "select u.UpdateID, b.UpdateDate from Updates u join Books b on b.UpdateID = u.UpdateID and b.BookID = :id";

IDataItem::Ptr CreateDictionary(DB::IDatabase& db, const std::string_view queryText, const long long id, const Extractor extractor)
{
	auto       root  = NavigationItem::Create();
	const auto query = db.CreateQuery(queryText);
	query->Bind(":id", id);
	for (query->Execute(); !query->Eof(); query->Next())
	{
		auto child = extractor(*query);
		child->Reduce();
		root->AppendChild(std::move(child));
	}

	return root;
}

template <typename T>
struct TimeProj
{
	constexpr QDateTime operator()(const T& obj) noexcept
	{
		return obj.time;
	}
};

template <typename T>
struct NameProj
{
	constexpr QString operator()(const T& obj) noexcept
	{
		return obj.name;
	}
};

template <typename T>
struct TextProj
{
	constexpr QString operator()(const T& obj) noexcept
	{
		return obj.text;
	}
};

template <typename Comp, typename Proj>
void SortReviews(IAnnotationController::IDataProvider::Reviews& reviews)
{
	std::ranges::sort(reviews, Comp {}, Proj {});
}

constexpr std::pair<const char*, void (*)(IAnnotationController::IDataProvider::Reviews&)> REVIEW_SORTERS[] {
	{		 "Time",    &SortReviews<std::less<QDateTime>, TimeProj<IAnnotationController::IDataProvider::Review>> },
	{     "TimeDesc", &SortReviews<std::greater<QDateTime>, TimeProj<IAnnotationController::IDataProvider::Review>> },
	{     "Reviewer",      &SortReviews<std::less<QString>, NameProj<IAnnotationController::IDataProvider::Review>> },
	{ "ReviewerDesc",   &SortReviews<std::greater<QString>, NameProj<IAnnotationController::IDataProvider::Review>> },
	{		 "Text",      &SortReviews<std::less<QString>, TextProj<IAnnotationController::IDataProvider::Review>> },
	{     "TextDesc",   &SortReviews<std::greater<QString>, TextProj<IAnnotationController::IDataProvider::Review>> },
};
constexpr auto REVIEW_SORTER_DEFAULT = REVIEW_SORTERS[0].second;

IAnnotationController::IDataProvider::Reviews CollectReviews(
	DB::IDatabase&             db,
	const long long            bookId,
	const bool                 showReviews,
	const ICollectionProvider& collectionProvider,
	const ISettings&           settings
)
{
	if (!showReviews)
		return {};

	std::vector<std::pair<QString, QString>> reviewFolders;
	{
		const auto query = db.CreateQuery(REVIEWS_QUERY);
		query->Bind(0, bookId);
		for (query->Execute(); !query->Eof(); query->Next())
			reviewFolders.emplace_back(query->Get<const char*>(0), query->Get<const char*>(1));
	}
	const auto archivesFolder = collectionProvider.GetActiveCollection().GetAdditionalFolder() + "/" + Inpx::REVIEWS_FOLDER;

	IAnnotationController::IDataProvider::Reviews reviews;
	for (const auto& [uid, reviewFolder] : reviewFolders)
	{
		if (!QFile::exists(archivesFolder + "/" + reviewFolder))
			continue;

		Zip zip(archivesFolder + "/" + reviewFolder);
		if (!zip.GetFileNameList().contains(uid))
			continue;

		const auto      stream = zip.Read(uid);
		QJsonParseError jsonParseError;
		const auto      doc = QJsonDocument::fromJson(stream->GetStream().readAll(), &jsonParseError);
		if (jsonParseError.error != QJsonParseError::NoError)
		{
			PLOGW << jsonParseError.errorString();
			continue;
		}
		if (!doc.isArray())
			continue;

		for (const auto jsonValue : doc.array())
		{
			assert(jsonValue.isObject());
			const auto obj    = jsonValue.toObject();
			auto&      review = reviews.emplace_back(QDateTime::fromString(obj[Inpx::TIME].toString(), "yyyy-MM-dd hh:mm:ss"), obj[Inpx::NAME].toString(), obj[Inpx::TEXT].toString());
			if (review.name.isEmpty())
				review.name = Loc::Tr(Loc::Ctx::COMMON, Loc::ANONYMOUS);
		}
	}

	const auto reviewsSortMode = settings.Get("Preferences/AnnotationReviewSortMode", QString { "Time" }).toStdString();
	const auto invoker         = FindSecond(REVIEW_SORTERS, reviewsSortMode.data(), REVIEW_SORTER_DEFAULT, PszComparer {});
	std::invoke(invoker, std::ref(reviews));

	return reviews;
}

} // namespace

IDataItem::Ptr AnnotationDbLoader::CreateBook(DB::IDatabase& db, const long long id)
{
	const auto query = db.CreateQuery(std::format(DatabaseUtil::BOOKS_QUERY, "", "", "from Books_View b", "where b.BookID = :id"));
	query->Bind(":id", id);
	query->Execute();
	return query->Eof() ? NavigationItem::Create() : DatabaseUtil::CreateBookItem(*query);
}

AnnotationDbCache AnnotationDbLoader::Build(
	DB::IDatabase&             db,
	const IDataItem::Ptr&      book,
	const bool                 filterEnabled,
	const bool                 showReviews,
	const ICollectionProvider& collectionProvider,
	const ISettings&           settings
)
{
	const auto bookId     = book->GetId().toLongLong();
	const auto filterFlag = filterEnabled ? 1 : 0;

	IAnnotationController::IDataProvider::ExportStatistics exportStatistics;
	{
		std::unordered_map<ExportStat::Type, std::vector<QDateTime>> exportStatisticsBuffer;

		const auto query = db.CreateQuery("select ExportType, CreatedAt from Export_List_User where BookID = ?");
		for (query->Bind(0, bookId), query->Execute(); !query->Eof(); query->Next())
			exportStatisticsBuffer[static_cast<ExportStat::Type>(query->Get<int>(0))].emplace_back(QDateTime::fromString(query->Get<const char*>(1), Qt::ISODate));
		std::ranges::move(exportStatisticsBuffer, std::back_inserter(exportStatistics));
	}

	const auto sourceLib = [&] {
		const auto query = db.CreateQuery("select b.SourceLib from Books b where b.BookID = ?");
		query->Bind(0, bookId);
		query->Execute();
		return query->Eof() ? QString {} : QString { query->Get<const char*>(0) };
	}();

	return {
		book,
		CreateDictionary(db, std::format(SERIES_QUERY, filterFlag), bookId, &DatabaseUtil::CreateSeriesItem),
		CreateDictionary(db, std::format(AUTHORS_QUERY, filterFlag), bookId, &DatabaseUtil::CreateFullAuthorItem),
		CreateDictionary(db, std::format(GENRES_QUERY, filterFlag), bookId, &DatabaseUtil::CreateSimpleListItem),
		CreateDictionary(db, GROUPS_QUERY, bookId, &DatabaseUtil::CreateSimpleListItem),
		CreateDictionary(db, std::format(KEYWORDS_QUERY, filterFlag), bookId, &DatabaseUtil::CreateSimpleListItem),
		CreateDictionary(db, FOLDER_QUERY, bookId, &DatabaseUtil::CreateSimpleListItem),
		CreateDictionary(db, UPDATE_QUERY, bookId, &DatabaseUtil::CreateSimpleListItem),
		std::move(exportStatistics),
		CollectReviews(db, bookId, showReviews, collectionProvider, settings),
		std::move(sourceLib),
	};
}
