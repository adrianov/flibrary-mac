#include "DiskCache.h"

#include <stdexcept>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "database/factory/Factory.h"
#include "database/interface/IDatabase.h"
#include "database/interface/IQuery.h"
#include "log.h"

namespace HomeCompa::Util
{

namespace
{

constexpr auto SCHEMA_VERSION = 1;

constexpr auto CREATE_META = R"(
CREATE TABLE IF NOT EXISTS meta (
	key   TEXT NOT NULL PRIMARY KEY,
	value TEXT NOT NULL
))";

constexpr auto CREATE_CACHE = R"(
CREATE TABLE IF NOT EXISTS cache (
	ns          TEXT NOT NULL,
	key         TEXT NOT NULL,
	value       BLOB NOT NULL,
	size        INTEGER NOT NULL,
	last_access INTEGER NOT NULL,
	PRIMARY KEY (ns, key)
))";

constexpr auto CREATE_INDEX = "CREATE INDEX IF NOT EXISTS ix_cache_lru ON cache (ns, last_access)";

void Exec(DB::IDatabase& db, const char* sql)
{
	const auto query = db.CreateQuery(sql);
	query->Execute();
}

void InitSchema(DB::IDatabase& db)
{
	Exec(db, CREATE_META);
	Exec(db, CREATE_CACHE);
	Exec(db, CREATE_INDEX);
	Exec(db, "PRAGMA journal_mode = WAL");
	Exec(db, "PRAGMA synchronous = NORMAL");
	Exec(db, "PRAGMA busy_timeout = 3000");

	bool schemaVersionExists = false;
	{
		const auto version = db.CreateQuery("SELECT value FROM meta WHERE key = 'schema_version'");
		version->Execute();
		schemaVersionExists = !version->Eof();
	}
	if (!schemaVersionExists)
	{
		const auto insert = db.CreateQuery("INSERT INTO meta (key, value) VALUES ('schema_version', ?)");
		insert->Bind(0, SCHEMA_VERSION);
		insert->Execute();
	}
}

std::unique_ptr<DB::IDatabase> OpenCacheDb(const QString& path)
{
	const auto connection = std::string("path=") + path.toStdString() + ";flag=CREATE";
	auto       db         = DB::Factory::Create(DB::Factory::Impl::Sqlite, connection);
	if (!db)
		return nullptr;

	InitSchema(*db);
	return db;
}

} // namespace

DiskCache::DiskCache(QString dbPath, const qint64 maxTotalBytes, const qint64 maxNamespaceBytes)
	: m_path { std::move(dbPath) }
	, m_maxTotalBytes { maxTotalBytes }
	, m_maxNamespaceBytes { maxNamespaceBytes }
{
}

DiskCache::~DiskCache() = default;

bool DiskCache::tryOpen() const
{
	if (m_openFailed)
		return false;

	std::call_once(m_openOnce, [this] {
		QDir().mkpath(QFileInfo(m_path).absolutePath());

		auto open = [&]() -> bool {
			try
			{
				m_db = OpenCacheDb(m_path);
				return m_db != nullptr;
			}
			catch (const std::exception& e)
			{
				PLOGW << "DiskCache: cannot open " << m_path.toStdString() << ": " << e.what();
				m_db.reset();
				return false;
			}
		};

		if (open())
			return;

		if (QFile::exists(m_path))
		{
			QFile::remove(m_path);
			if (open())
			{
				PLOGW << "DiskCache: recreated " << m_path.toStdString();
				return;
			}
		}

		m_openFailed = true;
	});

	return m_db != nullptr;
}

} // namespace HomeCompa::Util
