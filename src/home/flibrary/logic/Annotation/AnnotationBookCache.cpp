#include "AnnotationBookCache.h"

#include <QDir>
#include <QStandardPaths>

namespace HomeCompa::Flibrary
{

namespace
{

constexpr qint64 MAX_TOTAL_BYTES      = 1024LL * 1024 * 1024;
constexpr qint64 MAX_ANN_NAMESPACE    = 768LL * 1024 * 1024;

} // namespace

QString AnnotationBookCache::DefaultPath()
{
	const auto dir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath(QStringLiteral("FLibrary"));
	QDir().mkpath(dir);
	return QDir(dir).filePath(QStringLiteral("cache.db"));
}

AnnotationBookCache::AnnotationBookCache(QString dbPath)
	: m_disk { dbPath.isEmpty() ? DefaultPath() : std::move(dbPath), MAX_TOTAL_BYTES, MAX_ANN_NAMESPACE }
{
}

QString AnnotationBookCache::DbKey(const QString& bookId, const bool filterEnabled, const bool showReviews, const QString& locale)
{
	return QStringLiteral("%1|%2|%3|%4").arg(bookId).arg(filterEnabled).arg(showReviews).arg(locale);
}

std::optional<ArchiveParser::Data> AnnotationBookCache::Archive(const QString& bookId) const
{
	if (bookId.isEmpty())
		return std::nullopt;

	const auto bytes = m_disk.Get(NS_ARCHIVE, bookId);
	if (!bytes)
		return std::nullopt;

	return AnnotationCacheCodec::DecodeArchive(*bytes);
}

void AnnotationBookCache::PutArchive(QString bookId, const ArchiveParser::Data& data)
{
	if (bookId.isEmpty())
		return;

	m_disk.Put(NS_ARCHIVE, bookId, AnnotationCacheCodec::EncodeArchive(data));
}

std::optional<AnnotationDbCache> AnnotationBookCache::Database(const QString& bookId, const bool filterEnabled, const bool showReviews, const QString& locale) const
{
	if (bookId.isEmpty())
		return std::nullopt;

	const auto bytes = m_disk.Get(NS_DATABASE, DbKey(bookId, filterEnabled, showReviews, locale));
	if (!bytes)
		return std::nullopt;

	return AnnotationCacheCodec::DecodeDatabase(*bytes);
}

void AnnotationBookCache::PutDatabase(QString bookId, const bool filterEnabled, const bool showReviews, const QString& locale, const AnnotationDbCache& data)
{
	if (bookId.isEmpty())
		return;

	m_disk.Put(NS_DATABASE, DbKey(bookId, filterEnabled, showReviews, locale), AnnotationCacheCodec::EncodeDatabase(data));
}

bool AnnotationBookCache::HasDatabase(const QString& bookId, const bool filterEnabled, const bool showReviews, const QString& locale) const
{
	if (bookId.isEmpty())
		return false;

	const auto db = Database(bookId, filterEnabled, showReviews, locale);
	return db && db->book && !db->book->GetId().isEmpty();
}

bool AnnotationBookCache::IsCached(const QString& bookId, const bool filterEnabled, const bool showReviews, const QString& locale) const
{
	if (bookId.isEmpty() || !m_disk.Get(NS_ARCHIVE, bookId))
		return false;

	return HasDatabase(bookId, filterEnabled, showReviews, locale);
}

} // namespace HomeCompa::Flibrary
