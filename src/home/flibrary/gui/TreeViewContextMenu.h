#pragma once

#include <functional>
#include <vector>

#include <QObject>
#include <QTimer>

#include "interface/logic/IDataItem.h"

class QMenu;
class QModelIndex;
class QAbstractItemModel;

namespace HomeCompa::Flibrary::TreeViewContextMenu
{

class MenuEventFilter final : public QObject
{
public:
	explicit MenuEventFilter(QObject* parent = nullptr);

private:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	QTimer  m_timer;
	QString m_text;
};

std::vector<QString> CollectNextBookIds(const QAbstractItemModel& model, const QModelIndex& current, int maxCount);

void Generate(
	QMenu& menu,
	const IDataItem& item,
	const char* trContext,
	MenuEventFilter& menuEventFilter,
	std::function<void(IDataItem::Ptr)> onTriggered
);

}
