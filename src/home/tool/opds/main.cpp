#include <QFileInfo>
#include <QStandardPaths>

#ifdef Q_OS_MACOS
#include <cstdlib>
#endif

#include "logging/init.h"
#include "log.h"
#include "opds_run.h"

#include "config/version.h"

using namespace HomeCompa;
using namespace HomeCompa::Opds;

int main(const int argc, char* argv[])
{
#ifdef Q_OS_MACOS
	setenv("ICU_DATA", (QFileInfo(QString::fromUtf8(argv[0])).absolutePath() + "/../Resources").toUtf8().constData(), 1);
#endif
	Log::LoggingInitializer logging(QString("%1/%2.%3.log").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation), COMPANY_ID, APP_ID));
	PLOGI << QString("%1 started").arg(APP_ID);

	try
	{
		return RunOpds(argc, argv);
	}
	catch (const std::exception& ex)
	{
		PLOGE << ex.what();
	}
	catch (...)
	{
		PLOGE << "Unknown error";
	}

	return 1;
}
