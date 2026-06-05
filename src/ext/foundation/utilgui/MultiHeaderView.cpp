#include "MultiHeaderView.h"

#include <QAbstractTableModel>
#include <QMouseEvent>
#include <QPainter>

using namespace HomeCompa::Util;

namespace
{

class TableHeaderItem
{
public:
	explicit TableHeaderItem(TableHeaderItem* parent = nullptr)
		: TableHeaderItem(0, 0, parent)
	{
	}

	TableHeaderItem(const int row, const int column, TableHeaderItem* parent = nullptr)
		: m_parentItem(parent)
		, _row(row)
		, _column(column)
	{
	}

	TableHeaderItem* insertChild(int row, int col)
	{
		TableHeaderItem* child = new TableHeaderItem(row, col, this);
		m_childItems.insert(QPair(row, col), child);
		return child;
	}

	TableHeaderItem* child(int row, int col)
	{
		if (const auto it = m_childItems.find(QPair(row, col)); it != m_childItems.end())
			return it.value();

		return nullptr;
	}

	TableHeaderItem* parent() const
	{
		return m_parentItem;
	}

	int row() const
	{
		return _row;
	}

	int column() const
	{
		return _column;
	}

	void setData(const QVariant& data, const int role)
	{
		m_itemData.insert(role, data);
	}

	QVariant data(const int role) const
	{
		if (const auto it = m_itemData.find(role); it != m_itemData.end())
			return it.value();

		return {};
	}

	void clear()
	{
		for (TableHeaderItem* item : m_childItems)
		{
			item->clear();
			delete item;
		}
		m_childItems.clear();
	}

private:
	TableHeaderItem* m_parentItem;
	int              _row;
	int              _column;

	QHash<QPair<int, int>, TableHeaderItem*> m_childItems;
	QHash<int, QVariant>                     m_itemData;
};

class GridTableHeaderModel final : public QAbstractTableModel
{
	NON_COPY_MOVABLE(GridTableHeaderModel)

public:
	enum HeaderRole
	{
		ColumnSpanRole = Qt::UserRole + 1,
		RowSpanRole,
	};

	GridTableHeaderModel(const int row, const int column, QObject* parent = nullptr)
		: QAbstractTableModel(parent)
		, _row { row }
		, _column { column }
		, _rootItem { new TableHeaderItem }
	{
	}

	~GridTableHeaderModel() override
	{
		_rootItem->clear();
		delete _rootItem;
	}

	QModelIndex index(const int row, const int column, const QModelIndex& parent = QModelIndex()) const override
	{
		if (!hasIndex(row, column, parent))
			return {};

		TableHeaderItem* parentItem;
		if (!parent.isValid())
			parentItem = _rootItem;
		else
			parentItem = static_cast<TableHeaderItem*>(parent.internalPointer());

		TableHeaderItem* childItem = parentItem->child(row, column);
		if (!childItem)
			childItem = parentItem->insertChild(row, column);
		return createIndex(row, column, childItem);
	}

	int rowCount(const QModelIndex& parent = QModelIndex()) const override
	{
		Q_UNUSED(parent)
		return _row;
	}

	int columnCount(const QModelIndex& parent = QModelIndex()) const override
	{
		Q_UNUSED(parent)
		return _column;
	}

	Qt::ItemFlags flags(const QModelIndex& index) const override
	{
		if (!index.isValid())
			return Qt::NoItemFlags;
		return QAbstractTableModel::flags(index);
	}

	QVariant data(const QModelIndex& index, const int role) const override
	{
		if (!index.isValid())
			return {};

		if (index.row() >= _row || index.row() < 0 || index.column() >= _column || index.column() < 0)
			return {};

		auto* item = static_cast<TableHeaderItem*>(index.internalPointer());
		return item->data(role);
	}

	bool setData(const QModelIndex& index, const QVariant& value, const int role = Qt::EditRole) override
	{
		if (index.isValid())
		{
			TableHeaderItem* item = static_cast<TableHeaderItem*>(index.internalPointer());
			if (role == ColumnSpanRole)
			{
				int col  = index.column();
				int span = value.toInt();
				if (span > 0)
				{
					if (col + span - 1 >= _column)
						span = _column - col;
					item->setData(span, ColumnSpanRole);
				}
			}
			else if (role == RowSpanRole)
			{
				int row  = index.row();
				int span = value.toInt();
				if (span > 0)
				{
					if (row + span - 1 > _row)
						span = _column - row;
					item->setData(span, RowSpanRole);
				}
			}
			else
				item->setData(value, role);
			return true;
		}
		return false;
	}

private:
	int              _row;
	int              _column;
	TableHeaderItem* _rootItem;
};

} // namespace

struct MultiHeaderView::Impl
{
	GridTableHeaderModel* model;

	Impl(const int rows, const int columns)
		: model { new GridTableHeaderModel(rows, columns) }
	{
	}
};

MultiHeaderView::MultiHeaderView(const Qt::Orientation orientation, const int rows, const int columns, QWidget* parent)
	: QHeaderView(orientation, parent)
	, m_impl(rows, columns)
{
	QSize baseSectionSize;
	if (orientation == Qt::Horizontal)
	{
		baseSectionSize.setWidth(defaultSectionSize());
		baseSectionSize.setHeight(20);
	}
	else
	{
		baseSectionSize.setWidth(50);
		baseSectionSize.setHeight(defaultSectionSize());
	}

	auto& model = *m_impl->model;
	for (int row = 0; row < rows; ++row)
		for (int col = 0; col < columns; ++col)
			model.setData(model.index(row, col), baseSectionSize, Qt::SizeHintRole);

	QHeaderView::setModel(&model);
	connect(this, &QHeaderView::sectionResized, this, &MultiHeaderView::onSectionResized);
}

MultiHeaderView::~MultiHeaderView() = default;

void MultiHeaderView::setCellLabel(const int row, const int column, const QString& label)
{
	auto& model = *m_impl->model;
	model.setData(model.index(row, column), label, Qt::DisplayRole);
}

void MultiHeaderView::setCellBackgroundColor(const int row, const int column, const QColor& color)
{
	auto& model = *m_impl->model;
	model.setData(model.index(row, column), color, Qt::BackgroundRole);
}

void MultiHeaderView::setCellForegroundColor(const int row, const int column, const QColor& color)
{
	auto& model = *m_impl->model;
	model.setData(model.index(row, column), color, Qt::ForegroundRole);
}

QModelIndex MultiHeaderView::indexAt(const QPoint& pos)
{
	auto&     tblModel   = *m_impl->model;
	const int rows       = tblModel.rowCount();
	const int cols       = tblModel.columnCount();
	int       logicalIdx = logicalIndexAt(pos);
	int       delta      = 0;
	if (orientation() == Qt::Horizontal)
	{
		for (int row = 0; row < rows; ++row)
		{
			QModelIndex cellIndex  = tblModel.index(row, logicalIdx);
			delta                 += cellIndex.data(Qt::SizeHintRole).toSize().height();
			if (pos.y() <= delta)
				return cellIndex;
		}
	}
	else
	{
		for (int col = 0; col < cols; ++col)
		{
			QModelIndex cellIndex  = tblModel.index(logicalIdx, col);
			delta                 += cellIndex.data(Qt::SizeHintRole).toSize().width();
			if (pos.x() <= delta)
				return cellIndex;
		}
	}

	return {};
}

void MultiHeaderView::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
	auto&     tblModel = *m_impl->model;
	const int level    = (orientation() == Qt::Horizontal) ? tblModel.rowCount() : tblModel.columnCount();
	for (int i = 0; i < level; ++i)
	{
		QModelIndex cellIndex = (orientation() == Qt::Horizontal) ? tblModel.index(i, logicalIndex) : tblModel.index(logicalIndex, i);
		QSize       cellSize  = cellIndex.data(Qt::SizeHintRole).toSize();
		QRect       sectionRect(rect);

		// set position of the cell
		if (orientation() == Qt::Horizontal)
			sectionRect.setTop(rowSpanSize(logicalIndex, 0, i)); // distance from 0 to i-1 rows
		else
			sectionRect.setLeft(columnSpanSize(logicalIndex, 0, i));

		sectionRect.setSize(cellSize);

		// check up span column or row
		QModelIndex colSpanIdx = columnSpanIndex(cellIndex);
		QModelIndex rowSpanIdx = rowSpanIndex(cellIndex);
		if (colSpanIdx.isValid())
		{
			int colSpanFrom = colSpanIdx.column();
			int colSpanCnt  = colSpanIdx.data(GridTableHeaderModel::ColumnSpanRole).toInt();
			int colSpanTo   = colSpanFrom + colSpanCnt - 1;
			int colSpan     = columnSpanSize(cellIndex.row(), colSpanFrom, colSpanCnt);
			if (orientation() == Qt::Horizontal)
				sectionRect.setLeft(sectionViewportPosition(colSpanFrom));
			else
			{
				sectionRect.setLeft(columnSpanSize(logicalIndex, 0, colSpanFrom));
				i = colSpanTo;
			}

			sectionRect.setWidth(colSpan);

			// check up  if the column span index has row span
			QVariant subRowSpanData = colSpanIdx.data(GridTableHeaderModel::RowSpanRole);
			if (subRowSpanData.isValid())
			{
				int subRowSpanFrom = colSpanIdx.row();
				int subRowSpanCnt  = subRowSpanData.toInt();
				int subRowSpanTo   = subRowSpanFrom + subRowSpanCnt - 1;
				int subRowSpan     = rowSpanSize(colSpanFrom, subRowSpanFrom, subRowSpanCnt);
				if (orientation() == Qt::Vertical)
					sectionRect.setTop(sectionViewportPosition(subRowSpanFrom));
				else
				{
					sectionRect.setTop(rowSpanSize(colSpanFrom, 0, subRowSpanFrom));
					i = subRowSpanTo;
				}
				sectionRect.setHeight(subRowSpan);
			}
			cellIndex = colSpanIdx;
		}
		if (rowSpanIdx.isValid())
		{
			int rowSpanFrom = rowSpanIdx.row();
			int rowSpanCnt  = rowSpanIdx.data(GridTableHeaderModel::RowSpanRole).toInt();
			int rowSpanTo   = rowSpanFrom + rowSpanCnt - 1;
			int rowSpan     = rowSpanSize(cellIndex.column(), rowSpanFrom, rowSpanCnt);
			if (orientation() == Qt::Vertical)
				sectionRect.setTop(sectionViewportPosition(rowSpanFrom));
			else
			{
				sectionRect.setTop(rowSpanSize(logicalIndex, 0, rowSpanFrom));
				i = rowSpanTo;
			}
			sectionRect.setHeight(rowSpan);

			// check up if the row span index has column span
			QVariant subColSpanData = rowSpanIdx.data(GridTableHeaderModel::ColumnSpanRole);
			if (subColSpanData.isValid())
			{
				int subColSpanFrom = rowSpanIdx.column();
				int subColSpanCnt  = subColSpanData.toInt();
				int subColSpanTo   = subColSpanFrom + subColSpanCnt - 1;
				int subColSpan     = columnSpanSize(rowSpanFrom, subColSpanFrom, subColSpanCnt);
				if (orientation() == Qt::Horizontal)
					sectionRect.setLeft(sectionViewportPosition(subColSpanFrom));
				else
				{
					sectionRect.setLeft(columnSpanSize(rowSpanFrom, 0, subColSpanFrom));
					i = subColSpanTo;
				}
				sectionRect.setWidth(subColSpan);
			}
			cellIndex = rowSpanIdx;
		}

		// draw section with style
		QStyleOptionHeader opt;
		initStyleOption(&opt);
		opt.textAlignment = Qt::AlignCenter;
		opt.iconAlignment = Qt::AlignVCenter;
		opt.section       = logicalIndex;
		opt.text          = cellIndex.data(Qt::DisplayRole).toString();
		opt.rect          = sectionRect;

		QVariant bg = cellIndex.data(Qt::BackgroundRole);
		QVariant fg = cellIndex.data(Qt::ForegroundRole);
		if (bg.canConvert<QBrush>())
		{
			opt.palette.setBrush(QPalette::Button, bg.value<QBrush>());
			opt.palette.setBrush(QPalette::Window, bg.value<QBrush>());
		}
		if (fg.canConvert<QBrush>())
		{
			opt.palette.setBrush(QPalette::ButtonText, fg.value<QBrush>());
		}

		painter->save();
		style()->drawControl(QStyle::CE_Header, &opt, painter, this);
		painter->restore();
	}
}

QSize MultiHeaderView::sectionSizeFromContents(const int logicalIndex) const
{
	auto&     tblModel = *m_impl->model;
	const int level    = (orientation() == Qt::Horizontal) ? tblModel.rowCount() : tblModel.columnCount();

	QSize size = QHeaderView::sectionSizeFromContents(logicalIndex);
	for (int i = 0; i < level; ++i)
	{
		QModelIndex cellIndex  = (orientation() == Qt::Horizontal) ? tblModel.index(i, logicalIndex) : tblModel.index(logicalIndex, i);
		QModelIndex colSpanIdx = columnSpanIndex(cellIndex);
		QModelIndex rowSpanIdx = rowSpanIndex(cellIndex);
		size                   = cellIndex.data(Qt::SizeHintRole).toSize();

		if (colSpanIdx.isValid())
		{
			int colSpanFrom = colSpanIdx.column();
			int colSpanCnt  = colSpanIdx.data(GridTableHeaderModel::ColumnSpanRole).toInt();
			int colSpanTo   = colSpanFrom + colSpanCnt - 1;
			size.setWidth(columnSpanSize(colSpanIdx.row(), colSpanFrom, colSpanCnt));
			if (orientation() == Qt::Vertical)
				i = colSpanTo;
		}
		if (rowSpanIdx.isValid())
		{
			int rowSpanFrom = rowSpanIdx.row();
			int rowSpanCnt  = rowSpanIdx.data(GridTableHeaderModel::RowSpanRole).toInt();
			int rowSpanTo   = rowSpanFrom + rowSpanCnt - 1;
			size.setHeight(rowSpanSize(rowSpanIdx.column(), rowSpanFrom, rowSpanCnt));
			if (orientation() == Qt::Horizontal)
				i = rowSpanTo;
		}
	}
	return size;
}

void MultiHeaderView::setRowHeight(const int row, const int height)
{
	auto*     m    = model();
	const int cols = m->columnCount();
	for (int col = 0; col < cols; ++col)
	{
		QSize sz = m->index(row, col).data(Qt::SizeHintRole).toSize();
		sz.setHeight(height);
		m->setData(m->index(row, col), sz, Qt::SizeHintRole);
	}
	if (orientation() == Qt::Vertical)
		resizeSection(row, height);
}

void MultiHeaderView::setColumnWidth(const int col, const int width)
{
	auto*     m    = model();
	const int rows = m->rowCount();
	for (int row = 0; row < rows; ++row)
	{
		QSize sz = m->index(row, col).data(Qt::SizeHintRole).toSize();
		sz.setWidth(width);
		m->setData(m->index(row, col), sz, Qt::SizeHintRole);
	}
	if (orientation() == Qt::Horizontal)
		resizeSection(col, width);
}

void MultiHeaderView::setSpan(const int row, const int column, const int rowSpanCount, const int columnSpanCount)
{
	auto*       m   = model();
	QModelIndex idx = m->index(row, column);
	if (rowSpanCount > 0)
		m->setData(idx, rowSpanCount, GridTableHeaderModel::RowSpanRole);
	if (columnSpanCount > 0)
		m->setData(idx, columnSpanCount, GridTableHeaderModel::ColumnSpanRole);
}

void MultiHeaderView::setSpan(const int row, const int column)
{
	if (orientation() == Qt::Horizontal)
		setSpan(row, column, model()->rowCount(), 1);
	else
		setSpan(row, column, 1, model()->columnCount());
}

QModelIndex MultiHeaderView::columnSpanIndex(const QModelIndex& index) const
{
	const auto* tblModel = model();
	const int   curRow   = index.row();
	const int   curCol   = index.column();
	int         i        = curCol;
	while (i >= 0)
	{
		QModelIndex spanIndex = tblModel->index(curRow, i);
		QVariant    span      = spanIndex.data(GridTableHeaderModel::ColumnSpanRole);
		if (span.isValid() && spanIndex.column() + span.toInt() - 1 >= curCol)
			return spanIndex;
		i--;
	}
	return {};
}

QModelIndex MultiHeaderView::rowSpanIndex(const QModelIndex& index) const
{
	const auto* tblModel = model();
	const int   curRow   = index.row();
	const int   curCol   = index.column();
	int         i        = curRow;
	while (i >= 0)
	{
		QModelIndex spanIndex = tblModel->index(i, curCol);
		QVariant    span      = spanIndex.data(GridTableHeaderModel::RowSpanRole);
		if (span.isValid() && spanIndex.row() + span.toInt() - 1 >= curRow)
			return spanIndex;
		i--;
	}
	return {};
}

int MultiHeaderView::columnSpanSize(const int row, const int from, const int spanCount) const
{
	const auto* tblModel = model();
	int         span     = 0;
	for (int i = from; i < from + spanCount; ++i)
	{
		QSize cellSize  = tblModel->index(row, i).data(Qt::SizeHintRole).toSize();
		span           += cellSize.width();
	}
	return span;
}

int MultiHeaderView::rowSpanSize(const int column, const int from, const int spanCount) const
{
	const auto* tblModel = model();
	int         span     = 0;
	for (int i = from; i < from + spanCount; ++i)
	{
		QSize cellSize  = tblModel->index(i, column).data(Qt::SizeHintRole).toSize();
		span           += cellSize.height();
	}
	return span;
}

int MultiHeaderView::getSectionRange(QModelIndex& index, int* beginSection, int* endSection) const
{
	QModelIndex colSpanIdx = columnSpanIndex(index);
	QModelIndex rowSpanIdx = rowSpanIndex(index);

	if (colSpanIdx.isValid())
	{
		int colSpanFrom = colSpanIdx.column();
		int colSpanCnt  = colSpanIdx.data(GridTableHeaderModel::ColumnSpanRole).toInt();

		int colSpanTo = colSpanFrom + colSpanCnt - 1;
		if (orientation() == Qt::Horizontal)
		{
			*beginSection = colSpanFrom;
			*endSection   = colSpanTo;
			index         = colSpanIdx;
			return colSpanCnt;
		}

		QVariant subRowSpanData = colSpanIdx.data(GridTableHeaderModel::RowSpanRole);
		if (subRowSpanData.isValid())
		{
			int subRowSpanFrom = colSpanIdx.row();
			int subRowSpanCnt  = subRowSpanData.toInt();
			int subRowSpanTo   = subRowSpanFrom + subRowSpanCnt - 1;
			*beginSection      = subRowSpanFrom;
			*endSection        = subRowSpanTo;
			index              = colSpanIdx;
			return subRowSpanCnt;
		}
	}

	if (rowSpanIdx.isValid())
	{
		int rowSpanFrom = rowSpanIdx.row();
		int rowSpanCnt  = rowSpanIdx.data(GridTableHeaderModel::RowSpanRole).toInt();
		int rowSpanTo   = rowSpanFrom + rowSpanCnt - 1;
		if (orientation() == Qt::Vertical)
		{
			*beginSection = rowSpanFrom;
			*endSection   = rowSpanTo;
			index         = rowSpanIdx;
			return rowSpanCnt;
		}

		QVariant subColSpanData = rowSpanIdx.data(GridTableHeaderModel::ColumnSpanRole);
		if (subColSpanData.isValid())
		{
			int subColSpanFrom = rowSpanIdx.column();
			int subColSpanCnt  = subColSpanData.toInt();
			int subColSpanTo   = subColSpanFrom + subColSpanCnt - 1;
			*beginSection      = subColSpanFrom;
			*endSection        = subColSpanTo;
			index              = rowSpanIdx;
			return subColSpanCnt;
		}
	}

	return 0;
}

void MultiHeaderView::mousePressEvent(QMouseEvent* event)
{
	QHeaderView::mousePressEvent(event);
	QModelIndex index = indexAt(event->pos());

	if (index.isValid())
	{
		int beginSection = (orientation() == Qt::Horizontal) ? index.column() : index.row();
		;
		int endSection = beginSection;
		if (getSectionRange(index, &beginSection, &endSection) > 0)
			emit sectionPressed(beginSection, endSection);
		else
			emit sectionPressed(beginSection, endSection);
	}
}

void MultiHeaderView::onSectionResized(const int logicalIndex, const int oldSize, const int newSize)
{
	Q_UNUSED(oldSize)
	auto* tblModel = model();

	const int level = (orientation() == Qt::Horizontal) ? tblModel->rowCount() : tblModel->columnCount();
	int       pos   = sectionViewportPosition(logicalIndex);
	int       xx    = (orientation() == Qt::Horizontal) ? pos : 0;
	int       yy    = (orientation() == Qt::Horizontal) ? 0 : pos;
	QRect     sectionRect(xx, yy, 0, 0);
	for (int i = 0; i < level; ++i)
	{
		QModelIndex cellIndex = (orientation() == Qt::Horizontal) ? tblModel->index(i, logicalIndex) : tblModel->index(logicalIndex, i);
		QSize       cellSize  = cellIndex.data(Qt::SizeHintRole).toSize();
		// set position of cell
		if (orientation() == Qt::Horizontal)
		{
			sectionRect.setTop(rowSpanSize(logicalIndex, 0, i));
			cellSize.setWidth(newSize);
		}
		else
		{
			sectionRect.setLeft(columnSpanSize(logicalIndex, 0, i));
			cellSize.setHeight(newSize);
		}
		tblModel->setData(cellIndex, cellSize, Qt::SizeHintRole);

		QModelIndex colSpanIdx = columnSpanIndex(cellIndex);
		QModelIndex rowSpanIdx = rowSpanIndex(cellIndex);

		if (colSpanIdx.isValid())
		{
			int colSpanFrom = colSpanIdx.column();
			if (orientation() == Qt::Horizontal)
				sectionRect.setLeft(sectionViewportPosition(colSpanFrom));
			else
			{
				sectionRect.setLeft(columnSpanSize(logicalIndex, 0, colSpanFrom));
			}
		}
		if (rowSpanIdx.isValid())
		{
			int rowSpanFrom = rowSpanIdx.row();
			if (orientation() == Qt::Vertical)
				sectionRect.setTop(sectionViewportPosition(rowSpanFrom));
			else
				sectionRect.setTop(rowSpanSize(logicalIndex, 0, rowSpanFrom));
		}
		QRect rToUpdate(sectionRect);
		rToUpdate.setWidth(viewport()->width() - sectionRect.left());
		rToUpdate.setHeight(viewport()->height() - sectionRect.top());
		viewport()->update(rToUpdate.normalized());
	}
}
