#include "StrUtil.h"

#include <QString>

namespace HomeCompa::Platform
{

QString PathToString(const std::filesystem::path& path)
{
	return QString::fromStdString(path);
}

std::filesystem::path StringToPath(const QString& string)
{
	return std::filesystem::path { string.toStdString() };
}

}
