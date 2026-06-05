#pragma once

#include <functional>
#include <vector>

#include <QString>

class QActionGroup;

namespace Ui
{
class MainWindow;
}

namespace HomeCompa
{
class ISettings;

namespace Flibrary
{

class MainWindow;
class IStyleApplierFactory;
class IUiFactory;

struct MainWindowThemeMenuDeps
{
	MainWindow&                 self;
	Ui::MainWindow&             ui;
	ISettings&                  settings;
	const IStyleApplierFactory& styleApplierFactory;
	const IUiFactory&           uiFactory;
	QActionGroup&               stylesActionGroup;
	QString&                    lastStyleFileHovered;
	std::function<void()>       rebootDialog;
};

void CreateMainWindowThemeMenu(const MainWindowThemeMenuDeps& deps);
void ConnectMainWindowThemeMenuActions(const MainWindowThemeMenuDeps& deps);
void OpenExternalThemeStyles(const MainWindowThemeMenuDeps& deps, const QStringList& fileNames);
void DeleteCustomThemes(const MainWindowThemeMenuDeps& deps);
void RemoveCustomThemeFile(const MainWindowThemeMenuDeps& deps);

} // namespace Flibrary
} // namespace HomeCompa
