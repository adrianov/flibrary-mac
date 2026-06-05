#include "MainWindowThemeMenu.h"

#include "ui_MainWindow.h"

#include <ranges>
#include <set>

#include <QAction>
#include <QActionGroup>
#include <QDirIterator>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QStyleFactory>

#include "interface/localization.h"
#include "interface/ui/IStyleApplier.h"
#include "interface/ui/IStyleApplierFactory.h"
#include "interface/ui/IUiFactory.h"

#include "platformgui/NativeUi.h"
#include "settings/ISettings.h"

#include "MainWindow.h"
#include "log.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace
{

constexpr auto CONTEXT                  = "MainWindow";
constexpr auto CONFIRM_REMOVE_ALL_THEMES = QT_TRANSLATE_NOOP("MainWindow", "Are you sure you want to delete all themes?");
constexpr auto SELECT_QSS_FILE          = QT_TRANSLATE_NOOP("MainWindow", "Select stylesheet files");
constexpr auto QSS_FILE_FILTER          = QT_TRANSLATE_NOOP("MainWindow", "Qt stylesheet files (*.%1 *.dll);;All files (*.*)");
constexpr auto QSS                      = "qss";

TR_DEF

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QString GetStyleName(const QString& key)
{
	if (const auto* style = QStyleFactory::create(key))
		return style->name();
	return {};
}
#else
QString GetStyleName(const QString& key)
{
	return QStyleFactory::create(key) ? key : QString {};
}
#endif

std::set<QString> GetQssList()
{
	std::set<QString> list;
	QDirIterator      it(":/", QStringList() << "*.qss", QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext())
		list.emplace(it.next());
	return list;
}

void ApplyStyleAction(const MainWindowThemeMenuDeps& deps, QAction& action, const QActionGroup& group)
{
	PLOGV << "ApplyStyleAction";
	for (auto* groupAction : group.actions())
		groupAction->setEnabled(true);

	action.setEnabled(false);

	auto applier = deps.styleApplierFactory.CreateStyleApplier(static_cast<IStyleApplier::Type>(action.property(IStyleApplier::ACTION_PROPERTY_THEME_TYPE).toInt()));
	applier->Apply(action.property(IStyleApplier::ACTION_PROPERTY_THEME_NAME).toString(), action.property(IStyleApplier::ACTION_PROPERTY_THEME_FILE).toString());
	deps.rebootDialog();
}

void AddStyleActionsToGroup(const MainWindowThemeMenuDeps& deps, const std::vector<QAction*>& actions, QActionGroup* group)
{
	for (auto* action : actions)
	{
		group->addAction(action);
		QObject::connect(action, &QAction::triggered, &deps.self, [&deps, action, group] {
			ApplyStyleAction(deps, *action, *group);
		});
	}
}

QAction* CreateStyleAction(const MainWindowThemeMenuDeps& deps, QMenu& menu, const IStyleApplier::Type type, const QString& actionName, const QString& name, const QString& file = {})
{
	auto* action = menu.addAction(QFileInfo(actionName).completeBaseName());
	action->setObjectName(QString::fromUtf8(actionName.toUtf8().toBase64().toHex()));

	action->setProperty(IStyleApplier::ACTION_PROPERTY_THEME_NAME, name);
	action->setProperty(IStyleApplier::ACTION_PROPERTY_THEME_TYPE, static_cast<int>(type));
	action->setProperty(IStyleApplier::ACTION_PROPERTY_THEME_FILE, file);
	action->setCheckable(true);

	QObject::connect(action, &QAction::hovered, &deps.self, [&deps, file] {
		deps.lastStyleFileHovered = file;
	});

	return action;
}

std::vector<QAction*> AddExternalStyleDll(const MainWindowThemeMenuDeps& deps, const QFileInfo& fileInfo)
{
	const auto addLibList = [&](const std::set<QString>& libList) -> std::vector<QAction*> {
		if (libList.empty())
			return {};

		auto* menu = deps.ui.menuTheme->addMenu(fileInfo.completeBaseName());
		menu->setFont(deps.self.font());

		auto* action = menu->menuAction();
		action->setProperty(IStyleApplier::ACTION_PROPERTY_THEME_FILE, fileInfo.filePath());
		QObject::connect(action, &QAction::hovered, &deps.self, [&deps, file = fileInfo.filePath()] {
			deps.lastStyleFileHovered = file;
		});

		std::vector<QAction*> result;
		result.reserve(libList.size());
		std::ranges::transform(libList, std::back_inserter(result), [&](const auto& qss) {
			return CreateStyleAction(deps, *menu, IStyleApplier::Type::DllStyle, qss, qss, fileInfo.filePath());
		});

		return result;
	};

	auto currentList = GetQssList();

	if (auto applier = deps.styleApplierFactory.CreateThemeApplier(); applier->GetType() == IStyleApplier::Type::DllStyle && applier->GetChecked().second == fileInfo.filePath())
	{
		currentList.erase(IStyleApplier::STYLE_FILE_NAME);
		return addLibList(currentList);
	}

	Platform::DyLib lib(fileInfo.filePath().toStdString());
	if (!lib.IsOpen())
	{
		PLOGE << lib.GetErrorDescription();
		return {};
	}

	auto libList = GetQssList();
	erase_if(libList, [&](const auto& item) {
		return currentList.contains(item);
	});
	return addLibList(libList);
}

std::vector<QAction*> AddExternalStyle(const MainWindowThemeMenuDeps& deps, const QString& fileName)
{
	assert(!fileName.isEmpty());

	const QFileInfo fileInfo(fileName);
	if (!fileInfo.exists())
		return {};

	const auto ext = fileInfo.suffix().toLower();
	if (ext == "qss")
		return { CreateStyleAction(deps, *deps.ui.menuTheme, IStyleApplier::Type::QssStyle, fileName, fileInfo.completeBaseName(), fileName) };

	if (ext == "dll")
		return AddExternalStyleDll(deps, fileInfo);

	return {};
}

} // namespace

namespace HomeCompa::Flibrary
{

void CreateMainWindowThemeMenu(const MainWindowThemeMenuDeps& deps)
{
	PLOGV << "CreateStylesMenu";
	const auto addActionGroup = [&deps](const std::vector<QAction*>& actions, QActionGroup* group) {
		AddStyleActionsToGroup(deps, actions, group);
		deps.styleApplierFactory.CheckAction(actions);
	};
	addActionGroup({ deps.ui.actionColorSchemeSystem, deps.ui.actionColorSchemeLight, deps.ui.actionColorSchemeDark }, new QActionGroup(&deps.self));

	std::vector<QAction*> styles;
	for (const auto& name : QStyleFactory::keys())
	{
#ifdef Q_OS_MACOS
		if (name.compare(IStyleApplier::THEME_NAME_DEFAULT, Qt::CaseInsensitive) != 0)
			continue;
#endif
		if (const auto actionName = GetStyleName(name); !actionName.isEmpty())
			styles.emplace_back(CreateStyleAction(deps, *deps.ui.menuTheme, IStyleApplier::Type::PluginStyle, actionName, name));
	}

	deps.ui.menuTheme->addSeparator();

#ifndef Q_OS_MACOS
	if (const auto externalThemesVar = deps.settings.Get(IStyleApplier::THEME_FILES_KEY); externalThemesVar.isValid())
	{
		auto externalThemes = externalThemesVar.toStringList() | std::ranges::to<std::vector>();
		std::ranges::sort(externalThemes);
		for (const auto& fileName : externalThemes)
			std::ranges::copy(AddExternalStyle(deps, fileName), std::back_inserter(styles));
	}
#endif
	addActionGroup(styles, &deps.stylesActionGroup);

#ifndef Q_OS_MACOS
	deps.ui.menuTheme->addSeparator();
	deps.ui.menuTheme->addAction(deps.ui.actionAddThemes);
	deps.ui.menuTheme->addAction(deps.ui.actionDeleteAllThemes);
#endif
}

void OpenExternalThemeStyles(const MainWindowThemeMenuDeps& deps, const QStringList& fileNames)
{
	auto list = deps.settings.Get(IStyleApplier::THEME_FILES_KEY).toStringList();

	for (const auto& fileName : fileNames)
	{
		if (list.contains(fileName, Qt::CaseInsensitive))
			continue;

		auto actions = AddExternalStyle(deps, fileName);
		if (actions.empty())
			continue;

		AddStyleActionsToGroup(deps, actions, &deps.stylesActionGroup);
		list << fileName;
	}

	deps.settings.Set(IStyleApplier::THEME_FILES_KEY, list);
}

void DeleteCustomThemes(const MainWindowThemeMenuDeps& deps)
{
	if (deps.uiFactory.ShowQuestion(Tr(CONFIRM_REMOVE_ALL_THEMES), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
		return;

	for (auto* action : deps.ui.menuTheme->actions())
	{
		const auto file = action->property(IStyleApplier::ACTION_PROPERTY_THEME_FILE).toString();
		if (!file.isEmpty())
			deps.ui.menuTheme->removeAction(action);
	}

	deps.settings.Remove(IStyleApplier::THEME_FILES_KEY);

	if (deps.styleApplierFactory.CreateThemeApplier()->GetType() == IStyleApplier::Type::PluginStyle)
		return;

	deps.styleApplierFactory.CreateStyleApplier(IStyleApplier::Type::PluginStyle)->Apply(IStyleApplier::THEME_NAME_DEFAULT, {});
	deps.rebootDialog();
}

void RemoveCustomThemeFile(const MainWindowThemeMenuDeps& deps)
{
	if (deps.lastStyleFileHovered.isEmpty())
		return;

	auto list = deps.settings.Get(IStyleApplier::THEME_FILES_KEY).toStringList();
	if (!list.removeAll(deps.lastStyleFileHovered))
		return;

	deps.settings.Set(IStyleApplier::THEME_FILES_KEY, list);

	auto actions = deps.ui.menuTheme->actions();
	if (const auto it = std::ranges::find(
			actions,
			deps.lastStyleFileHovered,
			[](const QAction* action) {
				return action->property(IStyleApplier::ACTION_PROPERTY_THEME_FILE).toString();
			}
		);
	    it != actions.end())
	{
		deps.ui.menuTheme->removeAction(*it);
		if (auto* menu = (*it)->menu())
			menu->close();
	}

	if (deps.styleApplierFactory.CreateThemeApplier()->GetChecked().second != deps.lastStyleFileHovered)
		return;

	deps.styleApplierFactory.CreateStyleApplier(IStyleApplier::Type::PluginStyle)->Apply(IStyleApplier::THEME_NAME_DEFAULT, {});
	deps.rebootDialog();
}

void ConnectMainWindowThemeMenuActions(const MainWindowThemeMenuDeps& deps)
{
	QObject::connect(deps.ui.actionAddThemes, &QAction::triggered, &deps.self, [&deps] {
		OpenExternalThemeStyles(deps, deps.uiFactory.GetOpenFileNames(QSS, Tr(SELECT_QSS_FILE), Tr(QSS_FILE_FILTER).arg(QSS)));
	});
	QObject::connect(deps.ui.actionDeleteAllThemes, &QAction::triggered, &deps.self, [&deps] {
		DeleteCustomThemes(deps);
	});
}

} // namespace HomeCompa::Flibrary
