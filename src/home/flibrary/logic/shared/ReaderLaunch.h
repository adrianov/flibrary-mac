#pragma once

#include <QString>

#include "interface/logic/ILogicFactory.h"
#include "interface/ui/IUiFactory.h"
#include "settings/ISettings.h"

namespace HomeCompa::Flibrary
{

bool OpenFb2InBooks(const QString& fb2Path);

void LaunchConfiguredReader(
	ISettings&                                    settings,
	const IUiFactory&                             uiFactory,
	std::shared_ptr<ILogicFactory::ITemporaryDir> temporaryDir,
	QString                                       fileName,
	const QString&                                error,
	long long                                     bookId,
	const QString&                                archiveFolder    = {},
	const QString&                                bookFileInArchive = {}
);

} // namespace HomeCompa::Flibrary
