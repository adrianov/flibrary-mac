#pragma once

#include <functional>
#include <memory>

#include "export/settings.h"

class QTimer;

namespace HomeCompa::Util
{

SETTINGS_EXPORT std::unique_ptr<QTimer> CreateUiTimer(std::function<void()> f);

}
