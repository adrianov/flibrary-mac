#pragma once

#include <QByteArray>
#include <QSize>
#include <QString>
#include <vector>

class QAbstractButton;
class QLabel;
class QWidget;

namespace HomeCompa::Flibrary
{

class ClickableLabel;

struct CoverFitResult
{
	QSize size;
	bool  valid { false };
};

CoverFitResult FitCoverToArea(ClickableLabel& cover, const QByteArray& bytes, int maxWidth, int maxHeight);

void LayoutCoverNav(const std::vector<QAbstractButton*>& buttons, QLabel& titleLabel, const QString& title, int imgWidth, int imgHeight, int /*coverIndex*/);

bool SaveCoverImage(QString& fileName, const QByteArray& bytes);

QLabel* CreateCoverTitleLabel(QWidget* parent);

} // namespace HomeCompa::Flibrary
