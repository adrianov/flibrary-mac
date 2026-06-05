#include "NativeUi.h"

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QScrollBar>
#include <QTimer>
#include <QToolBar>
#include <QWindow>

#import <AppKit/AppKit.h>

namespace HomeCompa::Platform
{

namespace
{

void ApplyMacWindowChrome(QMainWindow& window)
{
	QTimer::singleShot(0, &window, [&window] {
		auto* handle = window.windowHandle();
		if (!handle)
			return;

		const auto view = reinterpret_cast<NSView*>(handle->winId());
		if (!view)
			return;

		NSWindow* const nsWindow = view.window;
		if (!nsWindow)
			return;

		nsWindow.titlebarAppearsTransparent = YES;
		nsWindow.toolbarStyle               = NSWindowToolbarStyleUnifiedCompact;
	});
}

} // namespace

void ApplyNativeScrollArea(QAbstractScrollArea* area)
{
	if (!area)
		return;

	area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	if (auto* bar = area->verticalScrollBar())
		bar->setStyleSheet({});
	if (auto* bar = area->horizontalScrollBar())
		bar->setStyleSheet({});
}

void ApplyNativeItemView(QAbstractItemView* view)
{
	(void)view;
}

void ApplyNativeApplication(QApplication& app)
{
	QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
	QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, false);
	app.setDesktopSettingsAware(true);
}

void ApplyNativeMainWindow(QMainWindow& window)
{
	window.setUnifiedTitleAndToolBarOnMac(true);

	if (auto* menuBar = window.menuBar())
		menuBar->setNativeMenuBar(true);

	for (auto* toolBar : window.findChildren<QToolBar*>())
	{
		toolBar->setMovable(false);
		toolBar->setFloatable(false);
	}

	for (auto* area : window.findChildren<QAbstractScrollArea*>())
		ApplyNativeScrollArea(area);

	ApplyMacWindowChrome(window);
}

} // namespace HomeCompa::Platform
