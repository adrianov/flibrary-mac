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

QJsonObject PublishInfoToJson(const ArchiveParser::Data::PublishInfo& info)
{
	return {
		{ "publisher", info.publisher },
		{ "city", info.city },
		{ "year", info.year },
		{ "isbn", info.isbn },
	};
}

ArchiveParser::Data::PublishInfo PublishInfoFromJson(const QJsonObject& obj)
{
	return {
		.publisher = obj.value("publisher").toString(),
		.city       = obj.value("city").toString(),
		.year       = obj.value("year").toString(),
		.isbn       = obj.value("isbn").toString(),
	};
}

QJsonArray StringListToJson(const std::vector<QString>& list)
{
	QJsonArray arr;
	for (const auto& s : list)
		arr.append(s);
	return arr;
}

std::vector<QString> StringListFromJson(const QJsonArray& arr)
{
	std::vector<QString> list;
	list.reserve(arr.size());
	for (const auto v : arr)
		list.emplace_back(v.toString());
	return list;
}

} // namespace

QByteArray AnnotationCacheCodec::EncodeArchive(const ArchiveParser::Data& data)
{
	QJsonArray covers;
	for (const auto& cover : data.covers)
	{
		covers.append(QJsonObject {
			{ "name", cover.name },
			{ "bytes", QString::fromLatin1(cover.bytes.toBase64()) },
		});
	}

	const QJsonObject root {
		{ "v", CODEC_VERSION },
		{ "annotation", data.annotation },
		{ "epigraph", data.epigraph },
		{ "epigraphAuthor", data.epigraphAuthor },
		{ "language", data.language },
		{ "sourceLanguage", data.sourceLanguage },
		{ "keywords", StringListToJson(data.keywords) },
		{ "content", AnnotationCacheItemCodec::ToJson(*data.content) },
		{ "description", AnnotationCacheItemCodec::ToJson(*data.description) },
		{ "translators", AnnotationCacheItemCodec::ToJson(*data.translators) },
		{ "publishInfo", PublishInfoToJson(data.publishInfo) },
		{ "error", data.error },
		{ "textSize", static_cast<qint64>(data.textSize) },
		{ "wordCount", static_cast<qint64>(data.wordCount) },
		{ "covers", covers },
	};

	return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

std::optional<ArchiveParser::Data> AnnotationCacheCodec::DecodeArchive(const QByteArray& bytes)
{
	const auto doc = QJsonDocument::fromJson(bytes);
	if (!doc.isObject() || doc.object().value("v").toInt() != CODEC_VERSION)
		return std::nullopt;

	const auto root = doc.object();
	ArchiveParser::Data data;
	data.annotation       = root.value("annotation").toString();
	data.epigraph         = root.value("epigraph").toString();
	data.epigraphAuthor   = root.value("epigraphAuthor").toString();
	data.language         = root.value("language").toString();
	data.sourceLanguage   = root.value("sourceLanguage").toString();
	data.keywords         = StringListFromJson(root.value("keywords").toArray());
	data.content          = AnnotationCacheItemCodec::FromJsonField(root, "content");
	data.description      = AnnotationCacheItemCodec::FromJsonField(root, "description");
	data.translators      = AnnotationCacheItemCodec::FromJsonField(root, "translators");
	data.publishInfo      = PublishInfoFromJson(root.value("publishInfo").toObject());
	data.error            = root.value("error").toString();
	data.textSize         = static_cast<size_t>(root.value("textSize").toInteger());
	data.wordCount        = static_cast<size_t>(root.value("wordCount").toInteger());

	for (const auto cover : root.value("covers").toArray())
	{
		if (!cover.isObject())
			continue;
		const auto obj = cover.toObject();
		data.covers.emplace_back(obj.value("name").toString(), QByteArray::fromBase64(obj.value("bytes").toString().toLatin1()));
	}

	return data;
}

} // namespace HomeCompa::Flibrary
