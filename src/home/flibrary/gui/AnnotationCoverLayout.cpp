#include "AnnotationCoverLayout.h"

#include <QAbstractButton>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QSvgRenderer>

#include "util/ImageRestore.h"
#include "util/ImageUtil.h"

#include "log.h"
#include "widgets/ClickableLabel.h"

using namespace HomeCompa;

namespace HomeCompa::Flibrary
{

namespace
{

class CoverTitleLabel final : public QLabel
{
public:
	explicit CoverTitleLabel(QWidget* parent = nullptr)
		: QLabel(parent)
	{
	}

private:
	void paintEvent(QPaintEvent* /*event*/) override
	{
		static constexpr auto off = 6;
		QPainter              painter(this);
		QPainterPath          path;
		path.addText(off, font().pointSize() + off, font(), text());
		painter.setRenderHints(QPainter::Antialiasing);
		painter.strokePath(path, QPen(palette().color(QPalette::Window), 2));
		painter.fillPath(path, QBrush(palette().color(QPalette::Text)));
		resize(path.boundingRect().size().toSize().width() + off * 2, path.boundingRect().size().toSize().height() + off * 2);
	}
};

enum CoverButtonType
{
	CoverPrevious,
	CoverHome,
	CoverNext,
};

} // namespace

bool SaveCoverImage(QString& fileName, const QByteArray& bytes)
{
	const auto [recoded, mediaType] = Util::Recode(bytes);
	if (const QFileInfo fileInfo(fileName); fileInfo.suffix().isEmpty())
		fileName += QString(mediaType) == Util::IMAGE_PNG ? ".png" : ".jpg";

	if (QFile::exists(fileName))
	{
		PLOGW << fileName << " already exists and will be overwritten";
	}

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		PLOGE << "Cannot open to write into " << fileName;
		return false;
	}

	if (file.write(recoded) != recoded.size())
	{
		PLOGW << "Write incomplete into " << fileName;
		return false;
	}

	return true;
}

CoverFitResult FitCoverToArea(ClickableLabel& cover, const QByteArray& bytes, int maxWidth, int maxHeight)
{
	auto imgWidth  = maxWidth;
	auto imgHeight = maxHeight;

	if (auto pixmap = Util::Decode(bytes); !pixmap.isNull())
	{
		if (imgHeight * pixmap.width() > pixmap.height() * imgWidth)
			imgHeight = pixmap.height() * imgWidth / pixmap.width();
		else
			imgWidth = pixmap.width() * imgHeight / pixmap.height();

		cover.setFixedSize(imgWidth, imgHeight);
		cover.setPixmap(pixmap.scaled(imgWidth, imgHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		return { { imgWidth, imgHeight }, true };
	}

	QSvgRenderer renderer(QString(":/icons/unsupported-image.svg"));
	const auto   defaultSize = renderer.defaultSize();
	imgWidth                 = imgHeight * defaultSize.width() / defaultSize.height();
	auto         pixmap      = QPixmap(imgWidth, imgHeight);
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	renderer.render(&painter);
	cover.setFixedSize(imgWidth, imgHeight);
	cover.setPixmap(pixmap);
	return { { imgWidth, imgHeight }, true };
}

void LayoutCoverNav(const std::vector<QAbstractButton*>& buttons, QLabel& titleLabel, const QString& title, int imgWidth, int imgHeight, int /*coverIndex*/)
{
	titleLabel.setText(title);

	const QFontMetrics metrics(titleLabel.font());
	const auto         height = 3 * metrics.lineSpacing() / 2;
	const QSize        size { height, height };
	const auto         top = imgHeight - height - height / 8;

	if (buttons.size() > CoverPrevious)
		buttons[CoverPrevious]->setGeometry(QRect { QPoint { height / 8, top }, size });
	if (buttons.size() > CoverNext)
		buttons[CoverNext]->setGeometry(QRect { QPoint { imgWidth - height - height / 8, top }, size });
	if (buttons.size() > CoverHome)
		buttons[CoverHome]->setGeometry(QRect { QPoint { (imgWidth - height) / 2, top }, size });

	titleLabel.move(height / 8, height / 16);
}

QLabel* CreateCoverTitleLabel(QWidget* parent)
{
	return new CoverTitleLabel(parent);
}

} // namespace HomeCompa::Flibrary
