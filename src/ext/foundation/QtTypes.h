#pragma once

#include <ranges>
#include <set>
#include <unordered_set>

#include <QCryptographicHash>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

class QEnterEvent;

using qsizetype_t   = qsizetype;
using qintptr_t     = qintptr;
using enter_event_t = QEnterEvent;

template <typename T>
inline void RemoveIf(QString& value, T&& functor)
{
	value.removeIf(std::forward<T>(functor));
}

inline QString First(const QString& string, const qsizetype n)
{
	return string.first(n);
}

inline QString Last(const QString& string, const qsizetype n)
{
	return string.last(n);
}

template <typename T>
inline void Erase(QString& string, const T& begin, const T& end)
{
	string.erase(begin, end);
}

template <typename T>
inline void Erase(QString& string, const T& it)
{
	string.erase(it);
}

inline int TypeId(const QVariant& var)
{
	return var.typeId();
}

inline void AddData(QCryptographicHash& hash, const char* data, qint64 length)
{
	hash.addData(QByteArrayView { data, length });
}

inline void Erase(QByteArray& bytes, const QByteArray::iterator begin, const QByteArray::iterator end)
{
	bytes.erase(begin, end);
}

inline void Resize(QStringList& list, const qsizetype_t size)
{
	list.resize(size);
}

	#define BEGIN_FILTER_CHANGE beginFilterChange()

	#define END_FILTER_CHANGE endFilterChange(Direction::Rows)

	#define QT_CONST const

#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)

using qsizetype_t   = int;
using qintptr_t     = long;
using enter_event_t = QEvent;

template <typename T>
inline void RemoveIf(QString& value, T&& functor)
{
	std::ranges::transform(value, value.begin(), std::forward<T>(functor));
}

inline QString First(const QString& string, const qsizetype n)
{
	return string.mid(0, n);
}

inline QString Last(const QString& string, const qsizetype n)
{
	return string.mid(string.length() - n);
}

template <typename T>
inline void Erase(QString& string, const T& begin, const T& end)
{
	string.mid(0, begin - string.begin()) + string.mid(end - string.begin(), string.length());
}

template <typename T>
inline void Erase(QString& string, const T& it)
{
	Erase(string, it, std::next(it));
}

inline int TypeId(const QVariant& var)
{
	return var.type();
}

inline void AddData(QCryptographicHash& hash, const char* data, qint64 length)
{
	hash.addData(data, length);
}

inline void Erase(QByteArray& bytes, const QByteArray::iterator begin, const QByteArray::iterator end)
{
	QByteArray result(bytes.constBegin(), std::distance(bytes.begin(), begin));
	result.append(bytes.constBegin() + std::distance(bytes.begin(), end), std::distance(end, bytes.end()));
	bytes = std::move(result);
}

inline void Resize(QStringList& list, const qsizetype_t size)
{
	while (list.size() > size)
		list.pop_back();
	while (list.size() < size)
		list << QString {};
}

	#define BEGIN_FILTER_CHANGE (void)0

	#define END_FILTER_CHANGE invalidateFilter()

	#define QT_CONST

Q_DECLARE_METATYPE(std::set<QString>)
Q_DECLARE_METATYPE(std::unordered_set<QString>)

#else
	#error unsupported qt version
#endif
