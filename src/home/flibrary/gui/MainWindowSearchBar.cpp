#include "MainWindowSearchBar.h"

#include "ui_MainWindow.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QToolBar>

#include "interface/localization.h"

#include "MainWindow.h"

using namespace HomeCompa;
using namespace Flibrary;

constexpr auto CONTEXT = "MainWindow";

TR_DEF

namespace
{

constexpr auto SEARCH_BOOKS_PLACEHOLDER            = QT_TRANSLATE_NOOP("MainWindow", "To search for books by %1, enter the name or title here and press Enter");
constexpr auto SEARCH_BOOKS_PLACEHOLDER_AUTHOR     = QT_TRANSLATE_NOOP("MainWindow", "author");
constexpr auto SEARCH_BOOKS_PLACEHOLDER_SERIES     = QT_TRANSLATE_NOOP("MainWindow", "series");
constexpr auto SEARCH_BOOKS_PLACEHOLDER_TITLE      = QT_TRANSLATE_NOOP("MainWindow", "title");
constexpr auto SEARCH_BOOKS_PLACEHOLDER_OR         = QT_TRANSLATE_NOOP("MainWindow", " or %1");
constexpr auto SEARCH_BOOKS_PLACEHOLDER_ANNOTATION = QT_TRANSLATE_NOOP("MainWindow", "annotation");

#define SEARCH_BOOKS_PLACEHOLDER_ITEMS_X_MACRO \
	SEARCH_BOOKS_PLACEHOLDER_ITEM(AUTHOR)      \
	SEARCH_BOOKS_PLACEHOLDER_ITEM(SERIES)      \
	SEARCH_BOOKS_PLACEHOLDER_ITEM(TITLE)       \
	SEARCH_BOOKS_PLACEHOLDER_ITEM(ANNOTATION)

class LineEditPlaceholderTextController final : public QObject
{
public:
	LineEditPlaceholderTextController(
		MainWindow& mainWindow,
		QLineEdit& lineEdit
#define SEARCH_BOOKS_PLACEHOLDER_ITEM(NAME) , QAction& action##NAME
			SEARCH_BOOKS_PLACEHOLDER_ITEMS_X_MACRO
#undef SEARCH_BOOKS_PLACEHOLDER_ITEM
	)
		: QObject(&lineEdit)
		, m_mainWindow { mainWindow }
		, m_lineEdit { lineEdit }
#define SEARCH_BOOKS_PLACEHOLDER_ITEM(NAME) , m_action##NAME { action##NAME }
		SEARCH_BOOKS_PLACEHOLDER_ITEMS_X_MACRO
#undef SEARCH_BOOKS_PLACEHOLDER_ITEM
	{
		m_lineEdit.setPlaceholderText(GetPlaceholderText());
#define SEARCH_BOOKS_PLACEHOLDER_ITEM(NAME) QObject::connect(&m_action##NAME, &QAction::toggled, [this]{m_lineEdit.setPlaceholderText(GetPlaceholderText());});
		SEARCH_BOOKS_PLACEHOLDER_ITEMS_X_MACRO
#undef SEARCH_BOOKS_PLACEHOLDER_ITEM
		if (const auto* stackedWidget = m_mainWindow.findChild<QStackedWidget*>("stackedWidget"))
		{
			QObject::connect(stackedWidget, &QStackedWidget::currentChanged, [this] {
				m_lineEdit.setPlaceholderText(GetPlaceholderText());
			});
		}
	}

private: // QObject
	bool eventFilter(QObject* watched, QEvent* event) override
	{
		if (event->type() == QEvent::Enter)
			m_lineEdit.setPlaceholderText(GetPlaceholderText());
		else if (event->type() == QEvent::Leave)
			m_lineEdit.setPlaceholderText({});
		else if (event->type() == QEvent::Show)
		{
			if (!IsMainPageVisible())
				m_lineEdit.setVisible(false);
			emit m_mainWindow.BookTitleToSearchVisibleChanged();
		}
		return QObject::eventFilter(watched, event);
	}

	QString GetPlaceholderText() const
	{
		QStringList list;
#define SEARCH_BOOKS_PLACEHOLDER_ITEM(NAME) if (m_action##NAME.isVisible() && m_action##NAME.isChecked()) list << Tr(SEARCH_BOOKS_PLACEHOLDER_##NAME);
		SEARCH_BOOKS_PLACEHOLDER_ITEMS_X_MACRO
#undef SEARCH_BOOKS_PLACEHOLDER_ITEM

		m_lineEdit.setVisible(!list.isEmpty() && IsMainPageVisible());
		if (!m_lineEdit.isVisible())
			return {};

		auto last = list.size() > 1 ? std::move(list.back()) : QString {};
		if (!last.isEmpty())
			list.pop_back();

		return Tr(SEARCH_BOOKS_PLACEHOLDER).arg(QString("%1%2").arg(list.join(", "), last.isEmpty() ? "" : Tr(SEARCH_BOOKS_PLACEHOLDER_OR).arg(last)));
	}

	[[nodiscard]] bool IsMainPageVisible() const
	{
		if (const auto* stackedWidget = m_mainWindow.findChild<QStackedWidget*>("stackedWidget"))
			return stackedWidget->currentIndex() == 0;
		return true;
	}

private:
	MainWindow& m_mainWindow;
	QLineEdit&  m_lineEdit;
#define SEARCH_BOOKS_PLACEHOLDER_ITEM(NAME) QAction& m_action##NAME;
	SEARCH_BOOKS_PLACEHOLDER_ITEMS_X_MACRO
#undef SEARCH_BOOKS_PLACEHOLDER_ITEM
};

} // namespace

namespace HomeCompa::Flibrary
{

MainWindowSearchBarLayout SetupMainWindowSearchBar(QMainWindow& window, Ui::MainWindow& ui, MainWindow& mainWindow)
{
	ui.lineEditBookTitleToSearch->installEventFilter(new LineEditPlaceholderTextController(
		mainWindow,
		*ui.lineEditBookTitleToSearch
#define SEARCH_BOOKS_PLACEHOLDER_ITEM(NAME) , *ui.actionSearchBy##NAME
			SEARCH_BOOKS_PLACEHOLDER_ITEMS_X_MACRO
#undef SEARCH_BOOKS_PLACEHOLDER_ITEM
	));

	MainWindowSearchBarLayout result;
#ifdef Q_OS_MACOS
	ui.lineEditBookTitleToSearch->setMinimumWidth(280);
	ui.lineEditBookTitleToSearch->setMaximumWidth(420);

	auto* searchBar = new QToolBar(&window);
	searchBar->setObjectName("searchBar");
	searchBar->setMovable(false);
	searchBar->setFloatable(false);
	searchBar->addWidget(ui.lineEditBookTitleToSearch);
	window.addToolBar(Qt::TopToolBarArea, searchBar);
#else
	auto* menuBar = new QWidget(&window);
	result.layout = new QHBoxLayout(menuBar);
	window.menuBar()->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	result.layout->addWidget(window.menuBar());
	result.layout->addItem((result.left = new QSpacerItem(72, 20, QSizePolicy::Fixed)));
	result.layout->addWidget(ui.lineEditBookTitleToSearch);
	result.layout->addItem(new QSpacerItem(72, 20, QSizePolicy::Expanding));
	result.layout->setContentsMargins(0, 0, 0, 0);
	window.setMenuWidget(menuBar);
#endif
	return result;
}

} // namespace HomeCompa::Flibrary
