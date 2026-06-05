#include "PlatformGuiUtil.h"

#include <QWidget>

namespace HomeCompa::Platform
{

void SetGeometry(QWidget& widget, const QRect& rect)
{
	widget.setGeometry(rect);
}

} // namespace HomeCompa::Platform
