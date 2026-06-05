#pragma once

#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"

#include "export/logging.h"

class QCommandLineParser;
class QString;

namespace HomeCompa::Log
{

class LOGGING_EXPORT LoggingInitializer final
{
	NON_COPY_MOVABLE(LoggingInitializer)

public:
	static QString AddLogFileOption(QCommandLineParser& parser, const QString& defaultPath);

public:
	explicit LoggingInitializer(const QString& path);
	~LoggingInitializer();

private:
	struct Impl;
	PropagateConstPtr<Impl> m_impl;
};

}
