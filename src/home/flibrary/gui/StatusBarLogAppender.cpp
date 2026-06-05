#include "StatusBarLogAppender.h"

#include <QPointer>
#include <QStatusBar>
#include <QTimer>

#include "MainWindow.h"

using namespace HomeCompa::Flibrary;

namespace
{

template <typename T>
QString ToString(const T* source) = delete;

template <>
QString ToString<char>(const char* source)
{
	return QString::fromStdString(source);
}

template <>
QString ToString<wchar_t>(const wchar_t* source)
{
	return QString::fromStdWString(source);
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
	const auto     message = ToString(record.getMessage());
	QTimer::singleShot(0, &m_window, [statusBar, message] {
		if (statusBar && statusBar->isVisible())
			statusBar->showMessage(message, 2000);
	});
}
