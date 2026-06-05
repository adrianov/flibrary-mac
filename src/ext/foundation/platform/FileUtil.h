#pragma once

#include <QString>

#include "export/platform.h"

namespace HomeCompa::Platform
{

PLATFORM_EXPORT QString RemoveIllegalPathCharacters(QString str);

}
