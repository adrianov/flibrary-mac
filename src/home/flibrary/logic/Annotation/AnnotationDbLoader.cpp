#include "AnnotationDbLoader.h"

#include <format>
#include <unordered_map>

#include "fnd/FindPair.h"

#include "database/interface/IQuery.h"
#include "interface/constants/ExportStat.h"
#include "interface/logic/IAnnotationController.h"

#include "data/DataItem.h"
#include "database/DatabaseUtil.h"
#include "AnnotationDbReviews.h"
#include "Constant.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace
{

using Extractor = IDataItem::Ptr (*)(const DB::IQuery& query);

constexpr auto SERIES_QUERY = "select s.SeriesID, s.SeriesTitle, sl.SeqNumber from Series s join Series_List sl on sl.SeriesID = s.SeriesID and sl.BookID = :id where s.Flags & {} = 0 order by sl.OrdNum";
constexpr auto AUTHORS_QUERY =
	"select a.AuthorID, a.LastName, a.FirstName, a.MiddleName from Authors a join Author_List al on al.AuthorID = a.AuthorID and al.BookID = :id where a.Flags & {} = 0 order by al.OrdNum";
constexpr auto GENRES_QUERY =
	"select g.GenreCode, g.GenreAlias, g.FB2Code, g.IsDeleted, g.Flags from Genres g join Genre_List gl on gl.GenreCode = g.GenreCode and gl.BookID = :id where g.Flags & {} = 0 order by gl.OrdNum";
constexpr auto GROUPS_QUERY   = "select g.GroupID, g.Title from Groups_User g join Groups_List_User_View gl on gl.GroupID = g.GroupID and gl.BookID = :id";
constexpr auto KEYWORDS_QUERY = "select k.KeywordID, k.KeywordTitle from Keywords k join Keyword_List kl on kl.KeywordID = k.KeywordID and kl.BookID = :id where k.Flags & {} = 0 order by kl.OrdNum";
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
		CreateDictionary(db, std::format(GENRES_QUERY, filterFlag), bookId, &DatabaseUtil::CreateGenreItem),
		CreateDictionary(db, GROUPS_QUERY, bookId, &DatabaseUtil::CreateSimpleListItem),
		CreateDictionary(db, std::format(KEYWORDS_QUERY, filterFlag), bookId, &DatabaseUtil::CreateSimpleListItem),
		CreateDictionary(db, FOLDER_QUERY, bookId, &DatabaseUtil::CreateSimpleListItem),
		CreateDictionary(db, UPDATE_QUERY, bookId, &DatabaseUtil::CreateSimpleListItem),
		std::move(exportStatistics),
		AnnotationDbReviews::Collect(db, bookId, showReviews, collectionProvider, settings),
		std::move(sourceLib),
	};
}
