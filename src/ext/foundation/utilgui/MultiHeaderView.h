#pragma once

#include <QHeaderView>

#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"

#include "export/utilgui.h"

namespace HomeCompa::Util
{

class UTILGUI_EXPORT MultiHeaderView final : public QHeaderView
{
	Q_OBJECT
	NON_COPY_MOVABLE(MultiHeaderView)

public:
	MultiHeaderView(Qt::Orientation orientation, int rows, int columns, QWidget* parent = nullptr);
	~MultiHeaderView() override;

	void setCellLabel(int row, int column, const QString& label);
	void setCellBackgroundColor(int row, int column, const QColor& color);
	void setCellForegroundColor(int row, int column, const QColor& color);

	QModelIndex indexAt(const QPoint& pos);
	void        paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;
	QSize       sectionSizeFromContents(int logicalIndex) const override;

	void setRowHeight(int row, int height);
	void setColumnWidth(int col, int width);

	void setSpan(int row, int column, int rowSpanCount, int columnSpanCount);
	void setSpan(int row, int column);

	QModelIndex columnSpanIndex(const QModelIndex& index) const;
	QModelIndex rowSpanIndex(const QModelIndex& index) const;

	int columnSpanSize(int row, int from, int spanCount) const;
	int rowSpanSize(int column, int from, int spanCount) const;
	int getSectionRange(QModelIndex& index, int* beginSection, int* endSection) const;

private: // QWidget
	void mousePressEvent(QMouseEvent* event) override;

private:
	void onSectionResized(int logicalIndex, int oldSize, int newSize);

signals:
	void sectionPressed(int from, int to);

private:
	struct Impl;
	PropagateConstPtr<Impl> m_impl;
};

} // namespace HomeCompa::Util
