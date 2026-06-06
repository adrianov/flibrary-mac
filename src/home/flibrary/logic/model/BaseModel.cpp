#include "BaseModel.h"

#include "fnd/ScopedCall.h"

#include "interface/constants/ModelRole.h"
#include "interface/localization.h"
#include "interface/logic/IModelProvider.h"

#include "data/DataItem.h"

using namespace HomeCompa;
using namespace Flibrary;

BaseModel::BaseModel(const std::shared_ptr<IModelProvider>& modelProvider, QObject* parent)
	: QAbstractItemModel(parent)
	, m_data { modelProvider->GetData() }
	, m_libRateProvider { modelProvider->GetLibRateProvider() }
{
}

BaseModel::~BaseModel() = default;

QVariant BaseModel::headerData(const int section, const Qt::Orientation orientation, const int role) const
{
	if (orientation != Qt::Horizontal)
		return {};

	switch (role)
	{
		case Role::HeaderName:
			return m_data->GetData(section);

		case Qt::DisplayRole:
		case Role::HeaderTitle:
			return Loc::Tr(Loc::Ctx::BOOK, m_data->GetData(section).toUtf8().data());

		default:
			break;
	}

	return {};
}

bool BaseModel::removeRows(const int row, const int count, const QModelIndex& parent)
{
	auto* parentItem = parent.isValid() ? GetInternalPointer(parent) : m_data.get();
	assert(parentItem);
	const auto uRow   = static_cast<size_t>(row);
	const auto uCount = static_cast<size_t>(count);
	if (uRow + uCount > parentItem->GetChildCount())
		return false;

	const ScopedCall removeGuard(
		[&] {
			beginRemoveRows(parent, row, row + count - 1);
		},
		[this]() {
			endRemoveRows();
		}
	);
	parentItem->RemoveChild(uRow, uCount);
	return true;
}

Qt::ItemFlags BaseModel::flags(const QModelIndex& index) const
{
	auto flags = QAbstractItemModel::flags(index);

	if (m_checkableColumn && index.column() == *m_checkableColumn)
		flags |= Qt::ItemIsUserCheckable;

	return flags;
}

IDataItem* BaseModel::GetInternalPointer(const QModelIndex& index) const
{
	return static_cast<IDataItem*>(index.internalPointer());
}
