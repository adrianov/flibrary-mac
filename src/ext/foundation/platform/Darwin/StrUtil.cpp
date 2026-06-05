#include "StrUtil.h"

#include <QString>

namespace HomeCompa::Platform
{

QString PathToString(const std::filesystem::path& path)
{
	const auto& native = path.native();
	return QString::fromUtf8(native.data(), static_cast<qsizetype>(native.size()));
}

std::filesystem::path StringToPath(const QString& string)
{
	const auto utf8 = string.toUtf8();
	return std::filesystem::path(utf8.constData(), utf8.constData() + utf8.size());
}

}
