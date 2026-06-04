#pragma once

#include "interface/logic/IAnnotationController.h"

namespace HomeCompa::Flibrary
{

[[nodiscard]] QString CreateBookAnnotation(const IAnnotationController::IDataProvider& dataProvider, const IAnnotationController::IStrategy& strategy);

} // namespace HomeCompa::Flibrary
