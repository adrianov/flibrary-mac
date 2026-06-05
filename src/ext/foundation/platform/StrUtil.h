#pragma once

#include <filesystem>

#include "export/platform.h"

class QString;

namespace HomeCompa::Platform
{

PLATFORM_EXPORT QString PathToString(const std::filesystem::path& path);
PLATFORM_EXPORT std::filesystem::path StringToPath(const QString& string);

}
