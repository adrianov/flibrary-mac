#pragma once

#include <optional>

#include <QByteArray>

#include "ArchiveParser.h"
#include "interface/logic/IAnnotationController.h"

namespace HomeCompa::Flibrary
{

struct AnnotationDbCache
{
	IDataItem::Ptr                               book;
	IDataItem::Ptr                               series;
	IDataItem::Ptr                               authors;
	IDataItem::Ptr                               genres;
	IDataItem::Ptr                               groups;
	IDataItem::Ptr                               keywords;
	IDataItem::Ptr                               folder;
	IDataItem::Ptr                               update;
	IAnnotationController::IDataProvider::ExportStatistics exportStatistics;
	IAnnotationController::IDataProvider::Reviews        reviews;
	QString                                      sourceLib;
};

struct AnnotationCacheCodec
{
	[[nodiscard]] static QByteArray EncodeArchive(const ArchiveParser::Data& data);
	[[nodiscard]] static std::optional<ArchiveParser::Data> DecodeArchive(const QByteArray& bytes);

	[[nodiscard]] static QByteArray EncodeDatabase(const AnnotationDbCache& data);
	[[nodiscard]] static std::optional<AnnotationDbCache> DecodeDatabase(const QByteArray& bytes);
};

} // namespace HomeCompa::Flibrary
