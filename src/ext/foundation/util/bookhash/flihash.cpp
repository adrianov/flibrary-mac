#include "flihash.h"

#include <ranges>
#include <set>
#include <unordered_set>

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "fnd/EnumBitmask.h"
#include "fnd/FindPair.h"

#include "Constant.h"
#include "hashbook.h"
#include "hashxml.h"
#include "log.h"
#include "zip.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

enum class CompareResult
{
	None   = 0,
	Left   = 1 << 0,
	Right  = 1 << 1,
	Images = 1 << 2,
	All    = Left | Right
};

}

ENABLE_BITMASK_OPERATORS(CompareResult);

namespace
{

constexpr auto KEY_FOLDER    = "folder";
constexpr auto KEY_FILE      = "file";
constexpr auto KEY_TITLE     = "title";
constexpr auto KEY_ID        = "id";
constexpr auto KEY_HASH      = "hash";
constexpr auto KEY_PHASH     = "phash";
constexpr auto KEY_COVER     = "cover";
constexpr auto KEY_IMAGES    = "images";
constexpr auto KEY_HISTOGRAM = "histogram";
constexpr auto KEY_WORD      = "word";
constexpr auto KEY_COUNT     = "count";

using ImageHash   = std::pair<uint64_t, QString>;
using ImageHashes = std::unordered_multimap<uint64_t, QString>;

BookHashItem GetHash_7z(const QString& path, const QString& file)
{
	QCryptographicHash md5 { QCryptographicHash::Md5 };
	auto               bookHashItem = BookHashItemProvider(path).Get(file);
	ParseBookHash(bookHashItem, md5);
	return bookHashItem;
}

BookHashItem GetHash_xml(const QString& path, const QString& file)
{
	return ParseXmlHash(path, file);
}

std::unique_ptr<Zip> GetZip(const QFileInfo& fileInfo, const char* type)
{
	const auto zipPath = fileInfo.dir().absoluteFilePath(QString("%1/%2.zip").arg(type, fileInfo.completeBaseName()));
	return QFile::exists(zipPath) ? std::make_unique<Zip>(zipPath) : std::unique_ptr<Zip> {};
}

CompareResult CompareTexts(QStringList& result, const HashParseResult& lhs, const HashParseResult& rhs)
{
	if (lhs.hashText == rhs.hashText)
		return (result << "texts are equal"), CompareResult::None;

	result << QString("texts are different: %1 vs %2").arg(lhs.hashText, rhs.hashText);
	std::ranges::transform(std::views::zip(lhs.hashValues, rhs.hashValues), std::back_inserter(result), [](const auto& item) {
		const auto& lhsItem = std::get<0>(item);
		const auto& rhsItem = std::get<1>(item);
		return QString("%1 %2 \t %3 %4").arg(lhsItem.first).arg(lhsItem.second).arg(rhsItem.first).arg(rhsItem.second);
	});

	return CompareResult::All;
}

CompareResult FromHammingDistance(const int hammingDistance) noexcept
{
	static constexpr auto HAMMING_DISTANCE_THRESHOLD = 16;
	return hammingDistance == 0 ? CompareResult::None : hammingDistance <= HAMMING_DISTANCE_THRESHOLD ? CompareResult::Images : CompareResult::All;
}

CompareResult CompareCovers(QStringList& result, const ImageHashItem& lhs, const ImageHashItem& rhs)
{
	if (lhs.hash == rhs.hash)
	{
		if (!lhs.hash.isEmpty())
			result << "covers are equal";
		return CompareResult::None;
	}

	if (lhs.hash.isEmpty())
		return (result << QString("left: no cover")), CompareResult::Right;

	if (rhs.hash.isEmpty())
		return (result << QString("right: no cover")), CompareResult::Left;

	const auto hammingDistance    = std::popcount(lhs.pHash ^ rhs.pHash);
	const auto imageCompareResult = FromHammingDistance(hammingDistance);
	result << QString("covers are %4: %1 vs %2, Hamming distance: %3")
				  .arg(lhs.pHash, 16, 16, QChar { '0' })
				  .arg(rhs.pHash, 16, 16, QChar { '0' })
				  .arg(hammingDistance)
				  .arg(imageCompareResult == CompareResult::All ? "different" : "probably the same");

	return imageCompareResult;
}

QString GetComparable(const QString& str)
{
	bool       ok     = false;
	const auto result = str.toLongLong(&ok);
	return ok ? QString("%1").arg(result, 16, 10, QChar { '0' }) : str;
}

CompareResult CompareImageHashes(std::multimap<QString, QString>& fileItems, const ImageHashes& lhs, const ImageHashes& rhs)
{
	CompareResult compareResult = CompareResult::None;

	auto lIds = lhs | std::views::values | std::ranges::to<std::unordered_set<QString>>();
	auto rIds = rhs | std::views::values | std::ranges::to<std::unordered_set<QString>>();

	if (!(lhs.empty() || rhs.empty()))
	{
		std::multimap<int, std::pair<ImageHash, ImageHash>> distances;
		for (const auto& l : lhs)
			for (const auto& r : rhs)
				distances.emplace(std::popcount(l.first ^ r.first), std::make_pair(l, r));

		for (const auto& [l, r] : distances | std::views::values)
		{
			if (!lIds.contains(l.second) || !rIds.contains(r.second))
				continue;

			lIds.erase(l.second);
			rIds.erase(r.second);

			const auto hammingDistance     = std::popcount(l.first ^ r.first);
			const auto imageCompareResult  = FromHammingDistance(hammingDistance);
			compareResult                 |= imageCompareResult;
			fileItems.emplace(
				GetComparable(l.second),
				QString("images are %6: %1: %4 vs %2: %5, Hamming distance: %3")
					.arg(l.second, r.second)
					.arg(hammingDistance)
					.arg(l.first, 16, 16, QChar { '0' })
					.arg(r.first, 16, 16, QChar { '0' })
					.arg(imageCompareResult == CompareResult::All ? "different" : "probably the same")
			);
		}
	}

	const auto notFound = [&](const bool reverse, const QString& id) {
		compareResult |= reverse ? CompareResult::Right : CompareResult::Left;
		return std::make_pair(GetComparable(id), QString("pair not found for %1 %2").arg(reverse ? "right" : "left", id));
	};

	std::ranges::transform(lIds, std::inserter(fileItems, fileItems.end()), std::bind_front(notFound, false));
	std::ranges::transform(rIds, std::inserter(fileItems, fileItems.end()), std::bind_front(notFound, true));

	return compareResult;
}

CompareResult CompareImages(QStringList& result, const ImageHashItems& lhs, const ImageHashItems& rhs)
{
	if (lhs.empty() && rhs.empty())
		return (result << "no images"), CompareResult::None;

	std::multimap<QString, QString> fileItems;
	ImageHashes                     lpHashes, rpHashes;

	auto lIt = lhs.cbegin(), rIt = rhs.cbegin();
	while (lIt != lhs.cend() && rIt != rhs.cend())
	{
		if (lIt->hash < rIt->hash)
		{
			lpHashes.emplace(lIt->pHash, lIt->file);
			++lIt;
			continue;
		}

		if (lIt->hash > rIt->hash)
		{
			rpHashes.emplace(rIt->pHash, rIt->file);
			++rIt;
			continue;
		}
		fileItems.emplace(GetComparable(lIt->file), QString("%1 and %2 are equal: %3").arg(lIt->file, rIt->file).arg(lIt->hash));
		++lIt;
		++rIt;
	}

	const auto transform = [](const auto& item) {
		return std::make_pair(item.pHash, item.file);
	};
	std::transform(lIt, lhs.cend(), std::inserter(lpHashes, lpHashes.end()), transform);
	std::transform(rIt, rhs.cend(), std::inserter(rpHashes, rpHashes.end()), transform);

	if (lpHashes.empty() && rpHashes.empty())
		return (result << "images are equal"), CompareResult::None;

	const auto compareResult = CompareImageHashes(fileItems, lpHashes, rpHashes);
	std::ranges::move(std::move(fileItems) | std::views::values, std::back_inserter(result));

	return compareResult;
}

} // namespace

struct BookHashItemProvider::Impl
{
	Zip             zip;
	const QFileInfo fileInfo;

	std::unique_ptr<Zip> coversZip { GetZip(fileInfo, Global::COVERS) };
	std::unique_ptr<Zip> imagesZip { GetZip(fileInfo, Global::IMAGES) };

	std::set<QString> covers { (coversZip ? coversZip->GetFileNameList() : QStringList {}) | std::ranges::to<std::set<QString>>() };
	std::set<QString> images { (imagesZip ? imagesZip->GetFileNameList() : QStringList {}) | std::ranges::to<std::set<QString>>() };

	explicit Impl(const QString& path)
		: zip(path)
		, fileInfo(path)
	{
	}
};

BookHashItemProvider::BookHashItemProvider(const QString& path)
	: m_impl(path)
{
}

BookHashItemProvider::~BookHashItemProvider() = default;

QStringList BookHashItemProvider::GetFiles() const
{
	return m_impl->zip.GetFileNameList();
}

BookHashItem BookHashItemProvider::Get(const QString& file) const
{
	BookHashItem bookHashItem { .folder = m_impl->fileInfo.fileName(), .file = file, .body = m_impl->zip.Read(file)->GetStream().readAll() };

	const auto baseName = QFileInfo(file).completeBaseName();
	if (m_impl->coversZip && m_impl->covers.contains(baseName))
		bookHashItem.cover = { QString {}, m_impl->coversZip->Read(baseName)->GetStream().readAll() };

	if (m_impl->imagesZip)
		std::ranges::transform(
			std::ranges::equal_range(
				m_impl->images,
				baseName + "/",
				{},
				[n = baseName.length() + 1](const QString& item) {
					return QStringView { item.begin(), std::next(item.begin(), n) };
				}
			),
			std::back_inserter(bookHashItem.images),
			[&](const QString& item) {
				return ImageHashItem { item.split("/").back(), m_impl->imagesZip->Read(item)->GetStream().readAll() };
			}
		);

	return bookHashItem;
}

namespace HomeCompa::Util
{

BookHashItem GetHash(const QString& path, const QString& file)
{
	static constexpr std::pair<const char*, BookHashItem (*)(const QString&, const QString&)> parsers[] {
#define ITEM(NAME) { #NAME, &GetHash_##NAME }
		ITEM(xml),
#undef ITEM
	};

	auto bookHashItem = FindSecond(parsers, QFileInfo(path).suffix().toLower().toStdString().data(), &GetHash_7z, PszComparer {})(path, file);
	bookHashItem.body.clear();

	return bookHashItem;
}

std::ostream& operator<<(std::ostream& stream, const BookHashItem& bookHashItem)
{
	auto result = QStringList() << QString("%1/%2, %3").arg(bookHashItem.folder, bookHashItem.file, bookHashItem.parseResult.title) << QString("full hash: %1").arg(bookHashItem.parseResult.id)
	                            << QString("top-10 hash: %1").arg(bookHashItem.parseResult.hashText);
	std::ranges::transform(bookHashItem.parseResult.hashValues, std::back_inserter(result), [](const auto& item) {
		return QString("%1: %2").arg(item.first).arg(item.second);
	});
	result << QString("cover%1").arg(
		bookHashItem.cover.hash.isEmpty() ? QString { " not found" } : QString(" hash: %1, pHash: %2").arg(bookHashItem.cover.hash).arg(bookHashItem.cover.pHash, 16, 16, QChar { '0' })
	);
	std::ranges::transform(bookHashItem.images, std::back_inserter(result), [](const auto& item) {
		return QString("image %1 hash: %2, pHash: %3").arg(item.file, item.hash).arg(item.pHash, 16, 16, QChar { '0' });
	});
	return stream << result.join('\n').toStdString();
}

QByteArray Serialize(const BookHashItem& bookHashItem)
{
	QJsonArray images;
	for (const auto& image : bookHashItem.images)
		images.append(
			QJsonObject {
				{ KEY_ID, image.file },
				{ KEY_HASH, image.hash },
				{ KEY_PHASH, QString("%1").arg(image.pHash, 16, 16, QChar { '0' }) },
        }
		);

	QJsonArray histogram;
	for (const auto& [count, word] : bookHashItem.parseResult.hashValues)
		histogram.append(
			QJsonObject {
				{ KEY_COUNT, static_cast<int>(count) },
				{  KEY_WORD,                    word },
        }
		);
	QJsonObject obj {
		{    KEY_FOLDER,               bookHashItem.folder },
        {      KEY_FILE,                 bookHashItem.file },
        {        KEY_ID,       bookHashItem.parseResult.id },
        {      KEY_HASH, bookHashItem.parseResult.hashText },
		{     KEY_TITLE,    bookHashItem.parseResult.title },
        { KEY_HISTOGRAM,              std::move(histogram) },
	};
	if (!bookHashItem.cover.hash.isEmpty())
		obj.insert(
			KEY_COVER,
			QJsonObject {
				{ KEY_HASH, bookHashItem.cover.hash },
				{ KEY_PHASH, QString("%1").arg(bookHashItem.cover.pHash, 16, 16, QChar { '0' }) },
        }
		);
	if (!images.isEmpty())
		obj.insert(KEY_IMAGES, std::move(images));

	return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

BookHashItem Deserialize(const QByteArray& bytes)
{
	QJsonParseError jsonParseError;
	auto            doc = QJsonDocument::fromJson(bytes, &jsonParseError);
	if (jsonParseError.error != QJsonParseError::NoError)
	{
		PLOGE << jsonParseError.errorString() << ":\n" << QString::fromUtf8(bytes);
		return {};
	}

	if (!doc.isObject())
	{
		PLOGE << "doc must be an object:\n" << QString::fromUtf8(bytes);
		return {};
	}

	const auto obj = doc.object();

	return {
		.folder = obj.value(KEY_FOLDER).toString(),
		.file   = obj.value(KEY_FILE).toString(),
		.cover  = [&]() -> ImageHashItem {
			const auto value = obj.value(KEY_COVER);
			if (value.isNull() || !value.isObject())
				return {};

			const auto object = value.toObject();
			return ImageHashItem { .hash = object.value(KEY_HASH).toString(), .pHash = object.value(KEY_PHASH).toString().toULongLong(nullptr, 16) };
										  }
			  (),
		.images = [&]() -> ImageHashItems {
			const auto value = obj.value(KEY_IMAGES);
			if (value.isNull() || !value.isArray())
				return {};

			return value.toArray() | std::views::transform([](const auto& item) {
					   const auto imageObj = item.toObject();
					   return ImageHashItem { .file = imageObj.value(KEY_ID).toString(), .hash = imageObj.value(KEY_HASH).toString(), .pHash = imageObj.value(KEY_PHASH).toString().toULongLong(nullptr, 16) };
				   })
		         | std::ranges::to<ImageHashItems>();
										  }
			  (),
		.parseResult = { .id         = obj.value(KEY_ID).toString(),
		                                  .title      = obj.value(KEY_TITLE).toString(),
		                                  .hashText   = obj.value(KEY_HASH).toString(),
		                                  .hashValues = [&]() -> TextHistogram {
							 const auto histogramValue = obj.value(KEY_HISTOGRAM);
							 if (histogramValue.isNull() || !histogramValue.isArray())
								 return {};
							 return histogramValue.toArray() | std::views::transform([](const auto& item) {
										const auto word = item.toObject();
										return std::make_pair(word.value(KEY_COUNT).toInt(), word.value(KEY_WORD).toString());
									})
		                          | std::ranges::to<TextHistogram>();
						 }() }
	};
}

QStringList Compare(const BookHashItem& lhs, const BookHashItem& rhs)
{
	QStringList result { QString("%1/%2 vs %3/%4:").arg(lhs.folder, lhs.file, rhs.folder, rhs.file) };

	auto compareResult  = CompareTexts(result, lhs.parseResult, rhs.parseResult);
	compareResult      |= CompareCovers(result, lhs.cover, rhs.cover);
	compareResult      |= CompareImages(result, lhs.images, rhs.images);

	result
		<< (compareResult == CompareResult::None                         ? "books are the same"
	        : compareResult == CompareResult::Images                     ? "books are probably the same"
	        : (compareResult & CompareResult::All) == CompareResult::All ? "books are different"
	        : !!(compareResult | CompareResult::Left)                    ? QString("left%1 includes right").arg(!!(compareResult | CompareResult::Images) ? " probably" : "")
	        : !!(compareResult | CompareResult::Right)                   ? QString("right%1 includes left").arg(!!(compareResult | CompareResult::Images) ? " probably" : "")
	                                                                     : (assert(false && "bad logic"), "wtf"));

	return result;
}

} // namespace HomeCompa::Util
