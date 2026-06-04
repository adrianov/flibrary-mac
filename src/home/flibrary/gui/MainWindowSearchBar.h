#pragma once

class QLayout;
class QMainWindow;
class QSpacerItem;
class QWidget;

namespace Ui
{
class MainWindow;
}

namespace HomeCompa::Flibrary
{

class MainWindow;

struct MainWindowSearchBarLayout
{
	QSpacerItem* left { nullptr };
	QLayout*     layout { nullptr };
	QWidget*     container { nullptr };
};

MainWindowSearchBarLayout SetupMainWindowSearchBar(QMainWindow& window, Ui::MainWindow& ui, MainWindow& mainWindow);

} // namespace HomeCompa::Flibrary
