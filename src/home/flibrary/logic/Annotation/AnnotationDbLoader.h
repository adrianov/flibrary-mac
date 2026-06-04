#pragma once

#include "AnnotationCacheCodec.h"

#include "data/DataItem.h"
#include "database/interface/IDatabase.h"
#include "interface/logic/ICollectionProvider.h"
#include "settings/ISettings.h"

namespace HomeCompa::Flibrary
{

class AnnotationDbLoader
{
public:
	[[nodiscard]] static IDataItem::Ptr CreateBook(DB::IDatabase& db, long long id);

	[[nodiscard]] static AnnotationDbCache Build(
		DB::IDatabase&                   db,
		const IDataItem::Ptr&            book,
		bool                             filterEnabled,
		bool                             showReviews,
		const ICollectionProvider&       collectionProvider,
		const ISettings&                 settings
	);
};

} // namespace HomeCompa::Flibrary
