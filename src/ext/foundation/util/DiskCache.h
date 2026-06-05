#pragma once

#include <cstdint>
#include <mutex>
#include <optional>

#include <QByteArray>
#include <QString>

#include "export/util.h"

namespace HomeCompa::DB
{
class IDatabase;
}

namespace HomeCompa::Util
{

// Single-file SQLite key-value store with LRU eviction and optional per-namespace caps.
class DiskCache
{
public:
	explicit DiskCache(QString dbPath, qint64 maxTotalBytes = 1024LL * 1024 * 1024, qint64 maxNamespaceBytes = 0);
	~DiskCache();

	DiskCache(const DiskCache&)            = delete;
	DiskCache& operator=(const DiskCache&) = delete;

	[[nodiscard]] std::optional<QByteArray> Get(const QString& ns, const QString& key) const;
	void                                    Put(const QString& ns, const QString& key, QByteArray value);
	void                                    Remove(const QString& ns, const QString& key);
	void                                    ClearNamespace(const QString& ns);

	[[nodiscard]] qint64 TotalBytes() const;
	[[nodiscard]] qint64 NamespaceBytes(const QString& ns) const;

private:
	[[nodiscard]] bool tryOpen() const;
	void trim(const QString& ns);
	void removeUnlocked(const QString& ns, const QString& key);
	qint64 sumBytes(const QString& ns) const;
	static qint64 nowSeconds();

	QString                                        m_path;
	qint64                                         m_maxTotalBytes;
	qint64                                         m_maxNamespaceBytes;
	mutable std::mutex                                  m_mutex;
	mutable std::once_flag                              m_openOnce;
	mutable std::unique_ptr<HomeCompa::DB::IDatabase>   m_db;
	mutable bool                                        m_openFailed { false };
};

} // namespace HomeCompa::Util
