#include "ProcessWrapper.h"

namespace HomeCompa::Util
{

bool RunSystem(const QString& command, const QString& parameters, const QString& cwd, const bool wait)
{
	const auto shellCmd = parameters.isEmpty() ? command : QString("%1 %2").arg(command, parameters);
	return RunProcess("/bin/sh", QString("-c \"%1\"").arg(shellCmd), cwd, wait);
}

} // namespace HomeCompa::Util
