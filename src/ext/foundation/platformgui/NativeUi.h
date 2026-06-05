#pragma once

#include "export/platformgui.h"

class QApplication;
class QAbstractItemView;
class QAbstractScrollArea;
class QMainWindow;

namespace HomeCompa::Platform
{

PLATFORMGUI_EXPORT void ApplyNativeApplication(QApplication& app);
PLATFORMGUI_EXPORT void ApplyNativeMainWindow(QMainWindow& window);
PLATFORMGUI_EXPORT void ApplyNativeScrollArea(QAbstractScrollArea* area);
PLATFORMGUI_EXPORT void ApplyNativeItemView(QAbstractItemView* view);

} // namespace HomeCompa::Platform
