#include "ProcessWrapper.h"

namespace HomeCompa::Util
{

bool RunSystem(const QString& command, const QString& parameters, const QString& cwd, const bool wait)
{
	const auto cmdLine = QString("/D /C %1 %2").arg(command, parameters);
	return RunProcess("cmd.exe", cmdLine, cwd, wait);
}

} // namespace HomeCompa::Util
