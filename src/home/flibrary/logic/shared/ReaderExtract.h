#pragma once

#include <QString>

#include "interface/logic/ILogicFactory.h"
#include "settings/ISettings.h"

namespace HomeCompa::Flibrary
{

void ExtractBookForReading(
	const ISettings&                            settings,
	const ILogicFactory::ITemporaryDir&         temporaryDir,
	const QString&                                archive,
	QString&                                    fileName,
	QString&                                    error,
	const std::shared_ptr<const ILogicFactory>& logicFactory
);

} // namespace HomeCompa::Flibrary
