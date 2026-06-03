#pragma once

class QLayout;
class QMainWindow;
class QSpacerItem;

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
};

MainWindowSearchBarLayout SetupMainWindowSearchBar(QMainWindow& window, Ui::MainWindow& ui, MainWindow& mainWindow);

} // namespace HomeCompa::Flibrary
