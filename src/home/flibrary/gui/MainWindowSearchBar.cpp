#include "MainWindowSearchBar.h"

#include "ui_MainWindow.h"

#include <QBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QMainWindow>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>

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
		QLineEdit& lineEdit,
		QWidget* visibilityWidget
#define SEARCH_BOOKS_PLACEHOLDER_ITEM(NAME) , QAction& action##NAME
			SEARCH_BOOKS_PLACEHOLDER_ITEMS_X_MACRO
#undef SEARCH_BOOKS_PLACEHOLDER_ITEM
	)
		: QObject(&lineEdit)
		, m_mainWindow { mainWindow }
		, m_lineEdit { lineEdit }
		, m_visibilityWidget { visibilityWidget }
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
		else if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut)
			SetFocused(event->type() == QEvent::FocusIn);
		else if (event->type() == QEvent::Show)
		{
			if (!IsMainPageVisible())
				SetVisible(false);
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

		const auto visible = !list.isEmpty() && IsMainPageVisible();
		SetVisible(visible);
		if (!visible)
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

	void SetVisible(const bool visible) const
	{
		m_lineEdit.setVisible(visible);
		if (m_visibilityWidget)
			m_visibilityWidget->setVisible(visible);
	}

	void SetFocused(const bool focused) const
	{
		if (!m_visibilityWidget)
			return;

		m_visibilityWidget->setProperty("focused", focused);
		m_visibilityWidget->style()->unpolish(m_visibilityWidget);
		m_visibilityWidget->style()->polish(m_visibilityWidget);
	}

private:
	MainWindow& m_mainWindow;
	QLineEdit&  m_lineEdit;
	QWidget*    m_visibilityWidget;
#define SEARCH_BOOKS_PLACEHOLDER_ITEM(NAME) QAction& m_action##NAME;
	SEARCH_BOOKS_PLACEHOLDER_ITEMS_X_MACRO
#undef SEARCH_BOOKS_PLACEHOLDER_ITEM
};

} // namespace

namespace HomeCompa::Flibrary
{

MainWindowSearchBarLayout SetupMainWindowSearchBar(QMainWindow& window, Ui::MainWindow& ui, MainWindow& mainWindow)
{
	MainWindowSearchBarLayout result;
	QWidget*                  visibilityWidget { nullptr };

#ifdef Q_OS_MACOS
	ui.actionSearchBookByTitle->setIcon(QIcon(":/icons/SearchFieldFind.svg"));
	ui.lineEditBookTitleToSearch->setClearButtonEnabled(false);
	ui.lineEditBookTitleToSearch->setFrame(false);

	auto* container = new QWidget(&window);
	result.container = container;
	container->setObjectName("bookTitleSearchContainer");
	container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	visibilityWidget = container;

	auto* layout = new QHBoxLayout(container);
	layout->setContentsMargins(12, 0, 8, 0);
	layout->setSpacing(6);

	auto* searchButton = new QToolButton(container);
	searchButton->setObjectName("bookTitleSearchButton");
	searchButton->setDefaultAction(ui.actionSearchBookByTitle);
	searchButton->setAutoRaise(true);
	searchButton->setFocusPolicy(Qt::NoFocus);
	layout->addWidget(searchButton);
	layout->addWidget(ui.lineEditBookTitleToSearch, 1);

	auto* clearButton = new QToolButton(container);
	clearButton->setObjectName("bookTitleSearchClearButton");
	clearButton->setIcon(QIcon(":/icons/SearchFieldClear.svg"));
	clearButton->setAutoRaise(true);
	clearButton->setFocusPolicy(Qt::NoFocus);
	clearButton->setVisible(!ui.lineEditBookTitleToSearch->text().isEmpty());
	QObject::connect(clearButton, &QToolButton::clicked, ui.lineEditBookTitleToSearch, &QLineEdit::clear);
	QObject::connect(ui.lineEditBookTitleToSearch, &QLineEdit::textChanged, clearButton, [clearButton](const QString& text) {
		clearButton->setVisible(!text.isEmpty());
	});
	layout->addWidget(clearButton);

	if (auto* rightWidget = window.findChild<QWidget*>("rightWidget"))
		if (auto* rightLayout = qobject_cast<QBoxLayout*>(rightWidget->layout()))
			rightLayout->insertWidget(0, container);
#else
	ui.lineEditBookTitleToSearch->addAction(ui.actionSearchBookByTitle, QLineEdit::LeadingPosition);
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

	ui.lineEditBookTitleToSearch->installEventFilter(new LineEditPlaceholderTextController(
		mainWindow,
		*ui.lineEditBookTitleToSearch,
		visibilityWidget
#define SEARCH_BOOKS_PLACEHOLDER_ITEM(NAME) , *ui.actionSearchBy##NAME
			SEARCH_BOOKS_PLACEHOLDER_ITEMS_X_MACRO
#undef SEARCH_BOOKS_PLACEHOLDER_ITEM
	));
	return result;
}

} // namespace HomeCompa::Flibrary
