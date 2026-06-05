#pragma once

#include <functional>

#include <plog/Appenders/IAppender.h>

class QStatusBar;

namespace HomeCompa::Flibrary
{

class MainWindow;

class StatusBarLogAppender final : public plog::IAppender
{
public:
	using StatusBarProvider = std::function<QStatusBar*()>;

	StatusBarLogAppender(MainWindow& window, StatusBarProvider statusBar);

private:
	void write(const plog::Record& record) override;

	MainWindow&       m_window;
	StatusBarProvider m_statusBar;
};

} // namespace HomeCompa::Flibrary
