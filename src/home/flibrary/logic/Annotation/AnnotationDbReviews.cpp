#include "AnnotationDbReviews.h"

#include <format>
#include <ranges>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "fnd/FindPair.h"

#include "database/interface/IQuery.h"
#include "interface/localization.h"

#include "Constant.h"
#include "log.h"
#include "zip.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace
{

constexpr auto REVIEWS_QUERY = "select f.FolderTitle||'#'||b.FileName||b.Ext, r.Folder from Reviews r join Books b on b.BookID = r.BookID join Folders f on f.FolderID = b.FolderID where r.BookID = :id";

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

} // namespace

namespace HomeCompa::Flibrary::AnnotationDbReviews
{

IAnnotationController::IDataProvider::Reviews Collect(
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

}
