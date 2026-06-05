#pragma once

#include <QObject>

#include "export/utilgui.h"

namespace HomeCompa::Util
{

class UTILGUI_EXPORT ItemViewToolTipper final : public QObject
{
public:
	explicit ItemViewToolTipper(QObject* parent = nullptr);

private: // QObject
	bool eventFilter(QObject* obj, QEvent* event) override;
};

}
