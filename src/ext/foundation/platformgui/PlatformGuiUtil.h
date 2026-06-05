#pragma once

#include "export/platformgui.h"

class QRect;
class QWidget;

namespace HomeCompa::Platform
{

PLATFORMGUI_EXPORT void SetGeometry(QWidget& widget, const QRect& rect);

}
