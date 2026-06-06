#include "TreeViewContextMenu.h"

#include <chrono>
#include <functional>
#include <ranges>
#include <stack>

#include <QFontMetrics>
#include <QKeyEvent>
#include <QMenu>
#include <QTimer>

#include "interface/constants/Enums.h"
#include "interface/constants/ModelRole.h"
#include "interface/localization.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace HomeCompa::Flibrary::TreeViewContextMenu
{

MenuEventFilter::MenuEventFilter(QObject* parent)
	: QObject(parent)
{
	m_timer.setSingleShot(true);
	m_timer.setInterval(std::chrono::seconds(2));
	connect(&m_timer, &QTimer::timeout, [this] {
		m_text.clear();
	});
}

bool MenuEventFilter::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() != QEvent::KeyPress)
		return QObject::eventFilter(watched, event);

	const auto text = static_cast<const QKeyEvent*>(event)->text(); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
	if (text.isEmpty())
		return false;

	auto* menu = qobject_cast<QMenu*>(watched);
	if (!menu)
		return false;

	m_text.append(text);
	m_timer.start();

	const auto actions = menu->actions();
	if (const auto it = std::ranges::find_if(
			actions,
			[this](const QAction* action) {
				return action->text().startsWith(m_text, Qt::CaseInsensitive);
			}
		);
	    it != actions.end())
		menu->setActiveAction(*it);

	return false;
}

std::vector<QString> CollectNextBookIds(const QAbstractItemModel& model, const QModelIndex& current, const int maxCount)
{
	std::vector<QString> result;
	if (!current.isValid() || maxCount <= 0 || current.data(Role::Type).value<ItemType>() != ItemType::Books)
		return result;

	result.reserve(static_cast<size_t>(maxCount));

	bool afterCurrent = false;
	std::function<void(const QModelIndex&)> visit = [&](const QModelIndex& parent) {
		for (int row = 0, count = model.rowCount(parent); row < count && static_cast<int>(result.size()) < maxCount; ++row)
		{
			const auto index = model.index(row, 0, parent);
			if (index == current)
			{
				afterCurrent = true;
				continue;
			}

			if (afterCurrent && index.data(Role::Type).value<ItemType>() == ItemType::Books)
				result.emplace_back(index.data(Role::Id).toString());

			if (model.rowCount(index) > 0)
				visit(index);
		}
	};

	visit({});
	return result;
}

void Generate(
	QMenu& menu,
	const IDataItem& item,
	const char* trContext,
	MenuEventFilter& menuEventFilter,
	std::function<void(IDataItem::Ptr)> onTriggered
)
{
	const auto                                      font = menu.font();
	const QFontMetrics                              metrics(font);
	std::stack<std::pair<const IDataItem*, QMenu*>> stack { { { &item, &menu } } };

	const auto getBool = [](const IDataItem& menuItem, const int column, const bool defaultValue) {
		const auto& str = menuItem.GetData(column);
		return str.isEmpty() ? defaultValue : QVariant(str).toBool();
	};

	while (!stack.empty())
	{
		auto [parent, subMenu] = stack.top();
		stack.pop();

		auto maxWidth = subMenu->minimumWidth();

		for (size_t i = 0, sz = parent->GetChildCount(); i < sz; ++i)
		{
			auto       child     = parent->GetChild(i);
			const auto enabled   = getBool(*child, MenuItem::Column::Enabled, true);
			const auto title     = child->GetData().toStdString();
			const auto titleText = Loc::Tr(trContext, title.data());
			auto       statusTip = titleText;
			statusTip.replace("&", "");
			maxWidth = std::max(maxWidth, metrics.boundingRect(statusTip).width() + 3 * (metrics.ascent() + metrics.descent()));

			if (child->GetChildCount() != 0)
			{
				auto* subSubMenu = stack.emplace(child.get(), subMenu->addMenu(titleText)).second;
				subSubMenu->setFont(font);
				subSubMenu->setEnabled(enabled);
				subSubMenu->setStatusTip(statusTip);
				continue;
			}

			if (const auto childId = child->GetData(MenuItem::Column::Id).toInt(); childId < 0)
			{
				subMenu->addSeparator();
				continue;
			}

			const auto checkable = getBool(*child, MenuItem::Column::Checkable, false);
			const auto checked   = getBool(*child, MenuItem::Column::Checked, false);

			auto* action = subMenu->addAction(titleText, [onTriggered, child = std::move(child)]() mutable {
				onTriggered(std::move(child));
			});
			action->setStatusTip(statusTip);
			action->setEnabled(enabled);
			action->setCheckable(checkable);
			if (checkable)
				action->setChecked(checked);
		}

		subMenu->setMinimumWidth(maxWidth);
		if (parent->GetChildCount() > 16)
			subMenu->installEventFilter(&menuEventFilter);
	}
}

}
