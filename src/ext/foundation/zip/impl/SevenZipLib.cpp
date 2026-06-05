#include "SevenZipLib.h"

#include <memory>
#include <mutex>

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include "bit7z/bit7zlibrary.hpp"

namespace HomeCompa::ZipDetails::SevenZip
{

namespace
{

bit7z::tstring LibPath()
{
#ifdef _WIN32
	return QDir::cleanPath(QCoreApplication::applicationDirPath() + "/7z.dll").toStdWString();
#elif defined(__APPLE__)
	return QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../Frameworks/7z.so").toStdString();
#else
	const auto bundled = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../lib/7z.so");
	if (QFile::exists(bundled))
		return bundled.toStdString();
	return "/usr/lib/p7zip/7z.so";
#endif
}

} // namespace

const bit7z::Bit7zLibrary& GetSevenZipLib()
{
	static std::once_flag once;
	static std::unique_ptr<bit7z::Bit7zLibrary> lib;

	std::call_once(once, [] {
		lib = std::make_unique<bit7z::Bit7zLibrary>(LibPath());
	});

	return *lib;
}

} // namespace HomeCompa::ZipDetails::SevenZip
