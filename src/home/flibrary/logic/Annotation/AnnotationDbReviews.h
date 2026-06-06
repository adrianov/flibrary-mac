#pragma once

#include "interface/logic/IAnnotationController.h"

#include "database/interface/IDatabase.h"
#include "interface/logic/ICollectionProvider.h"
#include "settings/ISettings.h"

namespace HomeCompa::Flibrary::AnnotationDbReviews
{

IAnnotationController::IDataProvider::Reviews Collect(
	DB::IDatabase&       db,
	long long            bookId,
	bool                 showReviews,
	const ICollectionProvider& collectionProvider,
	const ISettings&     settings
);

}
