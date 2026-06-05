#pragma once

#include <qlogging.h>

#include "fnd/NonCopyMovable.h"

class QMessageLogContext;
class QString;

class QtLogHandler
{
	NON_COPY_MOVABLE(QtLogHandler)

public:
	QtLogHandler();
	~QtLogHandler();

private:
	static void HandleStatic(QtMsgType type, const QMessageLogContext& ctx, const QString& message);

private:
	QtMessageHandler     m_qtLogHandlerPrev;
	static QtLogHandler* s_qtLogHandler;
};
