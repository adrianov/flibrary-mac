#include "AnnotationCacheCodec.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "interface/constants/ExportStat.h"
#include "interface/logic/IAnnotationController.h"
#include "AnnotationCacheCodecItem.h"

namespace HomeCompa::Flibrary
{

namespace
{

constexpr auto CODEC_VERSION = 3;

} // namespace

QByteArray AnnotationCacheCodec::EncodeDatabase(const AnnotationDbCache& data)
{
	QJsonArray exportStats;
	for (const auto& [type, dates] : data.exportStatistics)
	{
		QJsonArray dateList;
		for (const auto& dt : dates)
			dateList.append(dt.toString(Qt::ISODate));
		exportStats.append(QJsonObject { { "type", static_cast<int>(type) }, { "dates", dateList } });
	}

	QJsonArray reviews;
	for (const auto& review : data.reviews)
	{
		reviews.append(QJsonObject {
			{ "time", review.time.toString(Qt::ISODate) },
			{ "name", review.name },
			{ "text", review.text },
		});
	}

	const QJsonObject root {
		{ "v", CODEC_VERSION },
		{ "book", AnnotationCacheItemCodec::ToJson(*data.book) },
		{ "series", AnnotationCacheItemCodec::ToJson(*data.series) },
		{ "authors", AnnotationCacheItemCodec::ToJson(*data.authors) },
		{ "genres", AnnotationCacheItemCodec::ToJson(*data.genres) },
		{ "groups", AnnotationCacheItemCodec::ToJson(*data.groups) },
		{ "keywords", AnnotationCacheItemCodec::ToJson(*data.keywords) },
		{ "folder", AnnotationCacheItemCodec::ToJson(*data.folder) },
		{ "update", AnnotationCacheItemCodec::ToJson(*data.update) },
		{ "sourceLib", data.sourceLib },
		{ "exportStatistics", exportStats },
		{ "reviews", reviews },
	};

	return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

std::optional<AnnotationDbCache> AnnotationCacheCodec::DecodeDatabase(const QByteArray& bytes)
{
	const auto doc = QJsonDocument::fromJson(bytes);
	if (!doc.isObject() || doc.object().value("v").toInt() != CODEC_VERSION)
		return std::nullopt;

	const auto root = doc.object();
	AnnotationDbCache data;
	data.book      = AnnotationCacheItemCodec::FromJsonField(root, "book", ItemType::Books);
	data.series    = AnnotationCacheItemCodec::FromJsonField(root, "series");
	data.authors   = AnnotationCacheItemCodec::FromJsonField(root, "authors");
	data.genres    = AnnotationCacheItemCodec::FromJsonField(root, "genres");
	data.groups    = AnnotationCacheItemCodec::FromJsonField(root, "groups");
	data.keywords  = AnnotationCacheItemCodec::FromJsonField(root, "keywords");
	data.folder    = AnnotationCacheItemCodec::FromJsonField(root, "folder");
	data.update    = AnnotationCacheItemCodec::FromJsonField(root, "update");
	data.sourceLib = root.value("sourceLib").toString();

	for (const auto item : root.value("exportStatistics").toArray())
	{
		if (!item.isObject())
			continue;
		const auto obj   = item.toObject();
		const auto type  = static_cast<ExportStat::Type>(obj.value("type").toInt());
		std::vector<QDateTime> dates;
		for (const auto dt : obj.value("dates").toArray())
			dates.emplace_back(QDateTime::fromString(dt.toString(), Qt::ISODate));
		data.exportStatistics.emplace_back(type, std::move(dates));
	}

	for (const auto item : root.value("reviews").toArray())
	{
		if (!item.isObject())
			continue;
		const auto obj = item.toObject();
		data.reviews.emplace_back(
			QDateTime::fromString(obj.value("time").toString(), Qt::ISODate),
			obj.value("name").toString(),
			obj.value("text").toString()
		);
	}

	return data;
}

} // namespace HomeCompa::Flibrary
