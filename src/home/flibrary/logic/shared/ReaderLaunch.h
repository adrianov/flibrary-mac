#pragma once

#include <QString>

#include "interface/logic/ILogicFactory.h"
#include "interface/ui/IUiFactory.h"
#include "settings/ISettings.h"

namespace HomeCompa::Flibrary
{

bool OpenDefaultReader(const QString& fileName, const QString& ext);

void LaunchConfiguredReader(
	ISettings&                                  settings,
	const IUiFactory&                           uiFactory,
	std::shared_ptr<ILogicFactory::ITemporaryDir> temporaryDir,
	QString                                     fileName,
	const QString&                              error,
	long long                                   bookId
);

} // namespace HomeCompa::Flibrary
