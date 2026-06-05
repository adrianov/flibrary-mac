#include "DiskCache.h"

#include <QDateTime>

#include "database/interface/IDatabase.h"
#include "database/interface/IQuery.h"

namespace HomeCompa::Util
{

qint64 DiskCache::nowSeconds()
{
	return QDateTime::currentSecsSinceEpoch();
}

std::optional<QByteArray> DiskCache::Get(const QString& ns, const QString& key) const
{
	if (!tryOpen())
		return std::nullopt;

	std::lock_guard lock(m_mutex);

	std::string bytes;
	{
		const auto select = m_db->CreateQuery("SELECT value FROM cache WHERE ns = ? AND key = ?");
		select->Bind(0, ns);
		select->Bind(1, key);
		select->Execute();
		if (select->Eof())
			return std::nullopt;
		bytes = select->Get<std::string>(0);
	}

	const auto touch = m_db->CreateQuery("UPDATE cache SET last_access = ? WHERE ns = ? AND key = ?");
	touch->Bind(0, nowSeconds());
	touch->Bind(1, ns);
	touch->Bind(2, key);
	touch->Execute();

	return QByteArray(bytes.data(), static_cast<int>(bytes.size()));
}

void DiskCache::Put(const QString& ns, const QString& key, QByteArray value)
{
	if (!tryOpen())
		return;

	std::lock_guard lock(m_mutex);

	{
		const auto size   = static_cast<long long>(value.size());
		const auto access = nowSeconds();
		const auto upsert = m_db->CreateQuery(
			"INSERT INTO cache (ns, key, value, size, last_access) VALUES (?, ?, ?, ?, ?) "
			"ON CONFLICT (ns, key) DO UPDATE SET value = excluded.value, size = excluded.size, last_access = excluded.last_access"
		);
		upsert->Bind(0, ns);
		upsert->Bind(1, key);
		upsert->BindString(2, std::string_view { value.constData(), static_cast<size_t>(value.size()) });
		upsert->Bind(3, size);
		upsert->Bind(4, access);
		upsert->Execute();
	}

	trim(ns);
}

void DiskCache::removeUnlocked(const QString& ns, const QString& key)
{
	const auto del = m_db->CreateQuery("DELETE FROM cache WHERE ns = ? AND key = ?");
	del->Bind(0, ns);
	del->Bind(1, key);
	del->Execute();
}

void DiskCache::Remove(const QString& ns, const QString& key)
{
	if (!tryOpen())
		return;

	std::lock_guard lock(m_mutex);
	removeUnlocked(ns, key);
}

void DiskCache::ClearNamespace(const QString& ns)
{
	if (!tryOpen())
		return;

	std::lock_guard lock(m_mutex);

	const auto del = m_db->CreateQuery("DELETE FROM cache WHERE ns = ?");
	del->Bind(0, ns);
	del->Execute();
}

qint64 DiskCache::sumBytes(const QString& ns) const
{
	const auto query = ns.isEmpty() ? m_db->CreateQuery("SELECT COALESCE(SUM(size), 0) FROM cache") : m_db->CreateQuery("SELECT COALESCE(SUM(size), 0) FROM cache WHERE ns = ?");
	if (!ns.isEmpty())
		query->Bind(0, ns);
	query->Execute();
	return query->Eof() ? 0 : query->GetLong(0);
}

void DiskCache::trim(const QString& ns)
{
	const auto evict = [&](const QString& scope, const qint64 cap) {
		if (cap <= 0)
			return;

		while (sumBytes(scope) > cap)
		{
			QString oldestNs;
			QString oldestKey;
			auto oldest = scope.isEmpty()
				? m_db->CreateQuery("SELECT ns, key FROM cache ORDER BY last_access ASC LIMIT 1")
				: m_db->CreateQuery("SELECT key FROM cache WHERE ns = ? ORDER BY last_access ASC LIMIT 1");
			if (!scope.isEmpty())
				oldest->Bind(0, scope);
			oldest->Execute();
			if (oldest->Eof())
				break;

			if (scope.isEmpty())
			{
				oldestNs  = oldest->GetQString(0);
				oldestKey = oldest->GetQString(1);
			}
			else
			{
				oldestNs  = scope;
				oldestKey = oldest->GetQString(0);
			}
			oldest.reset();

			removeUnlocked(oldestNs, oldestKey);
		}
	};

	if (m_maxNamespaceBytes > 0)
		evict(ns, m_maxNamespaceBytes);
	if (m_maxTotalBytes > 0)
		evict(QString {}, m_maxTotalBytes);
}

qint64 DiskCache::TotalBytes() const
{
	if (!tryOpen())
		return 0;

	std::lock_guard lock(m_mutex);
	return sumBytes({});
}

qint64 DiskCache::NamespaceBytes(const QString& ns) const
{
	if (!tryOpen())
		return 0;

	std::lock_guard lock(m_mutex);
	return sumBytes(ns);
}

} // namespace HomeCompa::Util
