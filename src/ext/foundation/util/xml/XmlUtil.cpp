#include "XmlUtil.h"

#include "QtTypes.h"

namespace HomeCompa::Util
{

QByteArray RemoveDocType(QByteArray bytesSrc)
{
	auto bytes = std::move(bytesSrc);

	const auto begin = bytes.indexOf("<!DOCTYPE");
	if (begin < 0)
		return bytes;

	const auto end = bytes.indexOf('>', begin + 1);
	if (end < 0)
		return bytes;

	Erase(bytes, std::next(bytes.begin(), begin), std::next(bytes.begin(), end + 1));
	return bytes;
}

}
