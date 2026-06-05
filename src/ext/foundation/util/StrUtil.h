#pragma once

#include <QHash>
#include <QString>

#include "export/util.h"

class QString;

namespace HomeCompa::Util
{

UTIL_EXPORT QString& SimplifyTitle(QString& value);
UTIL_EXPORT QString& PrepareTitle(QString& value);

template <typename T>
T ToInteger(const QStringView value, T defaultValue = 0)
{
	if (value.isEmpty())
		return defaultValue;

	bool       ok     = false;
	const auto result = value.toLongLong(&ok);
	return ok ? static_cast<T>(result) : defaultValue;
}

struct WStringHash
{
	using is_transparent = int;

	[[nodiscard]] size_t operator()(const QString& txt) const
	{
		return GetHashImpl(txt);
	}

	[[nodiscard]] size_t operator()(const QStringView& txt) const
	{
		return GetHashImpl(txt);
	}

private:
	static size_t GetHashImpl(const QStringView& txt)
	{
		const auto txtLower = txt.toString().toLower();
		const auto result   = std::hash<QString>()(txtLower);
		return result;
	}
};

struct WStringEqualTo
{
	template <class U, class V>
	[[nodiscard]] constexpr auto operator()(const U& lhs, const V& rhs) const
	{
		return lhs.compare(rhs, Qt::CaseInsensitive) == 0;
	}

	using is_transparent = int;
};

} // namespace HomeCompa::Util
