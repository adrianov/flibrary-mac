#include "FileUtil.h"

#include <QRegularExpression>

namespace HomeCompa::Platform
{

QString RemoveIllegalPathCharacters(QString str)
{
	str.remove(QRegularExpression(R"([/\\<>:"\|\?\*\t])"));

	while (!str.isEmpty() && str.endsWith('.'))
		str.chop(1);

	return str.simplified();
}

}
