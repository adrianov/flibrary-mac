#pragma once

#include "export/settings.h"

namespace HomeCompa
{

class ISettings;

}

class QFont;

namespace HomeCompa::Util
{

SETTINGS_EXPORT void Serialize(const QFont& font, ISettings& settings);
SETTINGS_EXPORT void Deserialize(QFont& font, const ISettings& settings);

}
