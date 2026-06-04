#pragma once

#include <optional>

#include <QString>

#include "AnnotationCacheCodec.h"
#include "ArchiveParser.h"

#include "util/DiskCache.h"

namespace HomeCompa::Flibrary
{

// Persistent book annotation cache (cache.db).
class AnnotationBookCache
{
public:
	explicit AnnotationBookCache(QString dbPath = {});

	[[nodiscard]] std::optional<ArchiveParser::Data> Archive(const QString& bookId) const;
	void                                             PutArchive(QString bookId, const ArchiveParser::Data& data);

	[[nodiscard]] std::optional<AnnotationDbCache> Database(const QString& bookId, bool filterEnabled, bool showReviews) const;
	void                                         PutDatabase(QString bookId, bool filterEnabled, bool showReviews, const AnnotationDbCache& data);
	[[nodiscard]] bool                           HasDatabase(const QString& bookId, bool filterEnabled, bool showReviews) const;

	[[nodiscard]] bool IsCached(const QString& bookId, bool filterEnabled, bool showReviews) const;

	[[nodiscard]] static QString DefaultPath();

private:
	static QString DbKey(const QString& bookId, bool filterEnabled, bool showReviews);

	static constexpr auto NS_ARCHIVE = "ann/archive";
	static constexpr auto NS_DATABASE = "ann/db";

	Util::DiskCache m_disk;
};

} // namespace HomeCompa::Flibrary
