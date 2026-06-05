#include "pdf.h"

#include <QBuffer>
#include <QIODevice>
#include <QImage>

#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-image.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-page.h>

#include "fnd/FindPair.h"
#include "fnd/algorithm.h"

#include "log.h"

using namespace HomeCompa::Pdf;
using namespace HomeCompa;

namespace
{

constexpr auto DPI            = 300;
constexpr auto MAX_IMAGE_SIZE = 1440;

constexpr std::pair<poppler::image::format_enum, QImage::Format> PIXEL_FORMATS[] {
	{    poppler::image::format_mono,       QImage::Format_Mono },
    {   poppler::image::format_rgb24,     QImage::Format_RGB888 },
    {  poppler::image::format_argb32,     QImage::Format_ARGB32 },
	{   poppler::image::format_gray8, QImage::Format_Grayscale8 },
    {   poppler::image::format_bgr24,     QImage::Format_BGR888 },
    { poppler::image::format_invalid,    QImage::Format_Invalid },
};

}

namespace HomeCompa::Pdf
{

QByteArray GetCover(QIODevice& stream)
{
	const auto data = stream.readAll();

	const std::unique_ptr<const poppler::document> document(poppler::document::load_from_raw_data(data.constData(), static_cast<int>(data.size())));
	if (!document)
	{
		PLOGW << "cannot load document";
		return {};
	}

	if (document->pages() == 0)
	{
		PLOGW << "no pages found";
		return {};
	}

	const std::unique_ptr<const poppler::page> page(document->create_page(0));
	if (!page)
	{
		PLOGW << "cannot create page";
		return {};
	}

	const poppler::page_renderer renderer;

	const auto img         = renderer.render_page(page.get(), DPI, DPI);
	const auto pixelFormat = FindSecond(PIXEL_FORMATS, img.format(), QImage::Format_Invalid);
	if (pixelFormat == QImage::Format_Invalid)
		return {};

	auto   width = img.width(), height = img.height();
	QImage image(width, height, pixelFormat);

	const auto* imgData = img.const_data();
	for (int h = 0, szW = width * image.pixelFormat().channelCount(); h < height; ++h, imgData += img.bytes_per_row())
		memcpy(image.scanLine(h), imgData, szW);

	Util::FixSize(width, height, MAX_IMAGE_SIZE);

	image = image.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

	QByteArray result;
	{
		QBuffer buffer(&result);
		buffer.open(QIODevice::WriteOnly);
		if (!image.save(&buffer, "png"))
			return {};
	}

	return result;
}

} // namespace HomeCompa::Pdf
