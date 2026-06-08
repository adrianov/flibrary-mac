#include "StatusBarLogAppender.h"

#include <QPointer>
#include <QStatusBar>
#include <QTimer>

#include <plog/Util.h>

#include "MainWindow.h"

using namespace HomeCompa::Flibrary;

namespace
{

QString LogMessage(const plog::util::nchar* source)
{
#if PLOG_CHAR_IS_UTF8
	return QString::fromUtf8(source);
#else
	return QString::fromStdWString(source);
#endif
}

} // namespace

StatusBarLogAppender::StatusBarLogAppender(MainWindow& window, StatusBarProvider statusBar)
	: m_window { window }
	, m_statusBar { std::move(statusBar) }
{
}

void StatusBarLogAppender::write(const plog::Record& record)
{
	if (record.getSeverity() >= plog::Severity::verbose)
		return;

	const QPointer statusBar { m_statusBar() };
	const auto     message = LogMessage(record.getMessage());
	QTimer::singleShot(0, &m_window, [statusBar, message] {
		if (statusBar && statusBar->isVisible())
			statusBar->showMessage(message, 2000);
	});
}
