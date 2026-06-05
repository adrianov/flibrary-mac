#include "hashbook.h"

// clang-format off
#include <QBuffer>
#include <QPixmap>
#include "QtTypes.h"

#include "canny.h"
// clang-format on

#include <set>

#include "fnd/ScopedCall.h"

#include "xml/SaxParser.h"
#include "xml/XmlUtil.h"

#include "ImageUtil.h"
#include "StrUtil.h"
#include "log.h"
#include "parser.h"

#define BOOK_HASH_PARSER_ITEMS_X_MACRO \
	BOOK_HASH_PARSER_ITEM(fb2)         \
	BOOK_HASH_PARSER_ITEM(epub)

namespace HomeCompa::Util::BookHash
{

#define BOOK_HASH_PARSER_ITEM(NAME) std::unique_ptr<BookHash::IParser> create_##NAME##_parser(QIODevice& stream);
BOOK_HASH_PARSER_ITEMS_X_MACRO
#undef BOOK_HASH_PARSER_ITEM

}

using namespace HomeCompa::Util::BookHash;
using namespace HomeCompa::Util;
using namespace HomeCompa;
using namespace cimg_library;

namespace
{

CImg<float> GetDctMatrix(const int N)
{
	const auto  n = static_cast<float>(N);
	CImg<float> matrix(N, N, 1, 1, 1 / std::sqrt(n));
	const auto  c1 = std::sqrt(2.0f / n);
	for (int x = 0; x < N; ++x)
		for (int y = 1; y < N; ++y)
			matrix(x, y) = c1 * static_cast<float>(std::cos((cimg::PI / 2.0 / N) * y * (2.0 * x + 1)));
	return matrix;
}

const CImg<float> DCT   = GetDctMatrix(32);
const CImg<float> DCT_T = DCT.get_transpose();
const CImg<float> MEAN_FILTER(7, 7, 1, 1, 1);

uint64_t GetPHash(const ImageHashItem& item)
{
	auto pixmap = Decode(item.body);
	if (pixmap.isNull())
		return 0;

	auto       image    = pixmap.toImage();
	const auto hasAlpha = image.pixelFormat().alphaUsage() == QPixelFormat::UsesAlpha;
	image.convertTo(hasAlpha ? QImage::Format_RGBA8888 : QImage::Format_Grayscale8);

	auto          data = new uint8_t[static_cast<size_t>(image.width()) * image.height()];
	CImg<uint8_t> img(data, image.width(), image.height(), 1, 1, true);
	img._is_shared = false;

	if (hasAlpha)
	{
		auto* dst = img.data();
		for (auto h = 0, szh = image.height(), szw = image.width(); h < szh; ++h)
		{
			const auto* src = image.scanLine(h);
			for (auto w = 0; w < szw; ++w, ++dst, src += 4)
				*dst = static_cast<uint8_t>(std::lround((0.299 * src[0] + 0.587 * src[1] + 0.114 * src[2]) * src[3] / 255.0 + (255.0 - src[3])));
		}
	}
	else
	{
		auto* dst = img.data();
		for (auto h = 0, szh = image.height(), szw = image.width(); h < szh; ++h, dst += szw)
			memcpy(dst, image.scanLine(h), szw);
	}

	const Canny canny;
	const auto  cropRect = canny.Process(img);
	static_assert(sizeof(cropRect) == sizeof(uint64_t));

	if (cropRect.width() > img.width() / 2 && cropRect.height() > img.height() / 2)
		img.crop(cropRect.left, cropRect.top, cropRect.right - 1, cropRect.bottom - 1);

	const auto resized = img.get_convolve(MEAN_FILTER).resize(32, 32);
	const auto dct     = (DCT * resized * DCT_T).crop(1, 1, 8, 8);

#ifndef NDEBUG
	QString          str;
	const ScopedCall strGuard([&] {
		PLOGV << item.file << ": " << str;
	});
#endif

	return std::accumulate(dct._data, dct._data + 64, uint64_t { 0 }, [&, median = dct.median()](const uint64_t init, const float value) {
		auto result = init << 1;
		if (value > median)
			result |= 1;

#ifndef NDEBUG
		str.append(value > median ? "1" : "0");
#endif

		return result;
	});
}

using ParserCreator = std::unique_ptr<IParser> (*)(QIODevice& stream);

constexpr std::pair<const char*, ParserCreator> PARSERS[] {
#define BOOK_HASH_PARSER_ITEM(NAME) { "" #NAME, create_##NAME##_parser },
	BOOK_HASH_PARSER_ITEMS_X_MACRO
#undef BOOK_HASH_PARSER_ITEM
};

std::unique_ptr<IParser> CreateParser(BookHashItem& bookHashItem, QIODevice& stream)
{
	const auto it = std::ranges::find_if(PARSERS, [&](const auto& item) {
		return bookHashItem.file.endsWith(item.first, Qt::CaseInsensitive);
	});
	assert(it != std::end(PARSERS));
	return std::invoke(it->second, std::ref(stream));
}

struct HistComparer
{
	using ItemType = std::pair<size_t, QString>;

	bool operator()(const ItemType& lhs, const ItemType& rhs) const
	{
		return ToComparable(lhs) > ToComparable(rhs);
	}

private:
	static ItemType ToComparable(const ItemType& item)
	{
		auto result   = item;
		result.first |= (1llu << (32 + std::min(static_cast<int>(item.second.length()), 8)));
		return result;
	}
};

std::pair<std::vector<std::pair<size_t, QString>>, size_t> GetHashValues(const Hist& hist)
{
	std::set<std::pair<size_t, QString>, HistComparer> counter(HistComparer {});
	std::ranges::transform(hist, std::inserter(counter, counter.begin()), [](const auto& item) {
		return std::make_pair(item.second, item.first);
	});

	return std::make_pair(
		counter | std::views::take(10) | std::ranges::to<std::vector<std::pair<size_t, QString>>>(),
		std::accumulate(hist.cbegin(), hist.cend(), size_t { 0 }, [](const auto init, const auto& item) {
			return init + item.first.length() * item.second;
		})
	);
}

struct HtmlParser final : private SaxParser
{
	std::unordered_map<QString, size_t> hist;

	HtmlParser(QIODevice& input, QCryptographicHash& md5)
		: SaxParser(input, 512)
		, m_md5 { md5 }
	{
		Parse();
	}

private: // SaxParser
	bool OnCharacters(const QString&, const QString& value) override
	{
		auto valueCopy = value;

		PrepareTitle(valueCopy);
		for (auto&& word : valueCopy.split(' ', Qt::SkipEmptyParts))
		{
			UpdateHash(word);

			RemoveIf(word, [](const QChar ch) {
				const auto category = ch.category();
				return category < QChar::Letter_Lowercase || category > QChar::Letter_Other;
			});
			if (word.isEmpty())
				continue;

			++hist[word];
		}
		return true;
	}

private:
	void UpdateHash(QString value) const
	{
		RemoveIf(value, [](const QChar ch) {
			return ch.category() != QChar::Letter_Lowercase;
		});
		m_md5.addData(value.toUtf8());
	}

private:
	QCryptographicHash& m_md5;
};

} // namespace

namespace HomeCompa::Util
{

void SetHash(ImageHashItem& item, QCryptographicHash& cryptographicHash)
{
	cryptographicHash.reset();
	cryptographicHash.addData(item.body);
	item.hash  = QString::fromUtf8(cryptographicHash.result().toHex());
	item.pHash = GetPHash(item);
	item.body.clear();
#ifdef ADDITIONAL_LOG_ENABLED
	PLOGV << item.file;
#endif
}

CalculateHashResult CalculateHash(Hist& hist)
{
	QCryptographicHash cryptographicHash { QCryptographicHash::Md5 };

	cryptographicHash.reset();
	auto hashValues = GetHashValues(hist);
	for (const auto& word : hashValues.first | std::views::values)
		cryptographicHash.addData(word.toUtf8());

	auto       hash  = QString::fromUtf8(cryptographicHash.result().toHex());
	const auto count = hist.size();
	hist.clear();

	return { .hashValues = std::move(hashValues.first), .hash = std::move(hash), .count = count, .size = hashValues.second };
}

void ParseBookHash(BookHashItem& bookHashItem, QCryptographicHash& cryptographicHash)
{
	QBuffer buffer(&bookHashItem.body);
	buffer.open(QIODevice::ReadOnly);
	const auto parser        = CreateParser(bookHashItem, buffer);
	bookHashItem.parseResult = parser->GetResult();

	if (auto cover = parser->GetCover(); !cover.file.isEmpty())
		bookHashItem.cover = std::move(cover);

	if (auto images = parser->GetImages(); !images.empty())
		bookHashItem.images = std::move(images);

	if (!bookHashItem.cover.body.isEmpty())
		SetHash(bookHashItem.cover, cryptographicHash);

	std::ranges::for_each(bookHashItem.images, [&](auto& item) {
		SetHash(item, cryptographicHash);
	});
	std::ranges::sort(bookHashItem.images, {}, [](const auto& item) {
		auto       ok     = false;
		const auto number = item.file.toLongLong(&ok);
		return ok ? QString("%1").arg(number, 16, 10, QChar { '0' }) : item.file;
	});
	bookHashItem.body.clear();
}

Hist CollectHistogram(QByteArray body, QCryptographicHash& md5)
{
	body = RemoveDocType(std::move(body));
	QBuffer buffer(&body);
	buffer.open(QIODevice::ReadOnly);
	HtmlParser parser(buffer, md5);

	auto result = std::move(parser.hist);
	return result;
}

} // namespace HomeCompa::Util
