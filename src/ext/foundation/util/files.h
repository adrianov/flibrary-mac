#pragma once

#include <vector>

#include <QString>

#include "export/util.h"

namespace HomeCompa::Util
{

UTIL_EXPORT std::vector<QString> ResolveWildcard(const QString& wildcard);
UTIL_EXPORT QString              ToRelativePath(const QString& path);
UTIL_EXPORT QString              ToAbsolutePath(const QString& path);

}
