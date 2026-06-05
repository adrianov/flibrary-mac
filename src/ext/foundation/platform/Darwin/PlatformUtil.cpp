#include "PlatformUtil.h"

#include <QFileInfo>
#include <QProcess>

namespace HomeCompa::Platform
{

PlatformType GetPlatformType() noexcept
{
	return PlatformType::MacOS;
}

QString GetPlatformName()
{
	return "macOS";
}

bool IsRegisteredExtension(const QString& /*extension*/)
{
	return true;
}

void SetKeyboardLayout(const QString& /*locale*/)
{
	assert(false);
}

void SetKeyboardLayoutId(const QString& /*id*/)
{
	assert(false);
}

QString GetKeyboardLayoutId()
{
	return {};
}

bool IsAppAddedToAutostart(const QString& /*key*/)
{
	return false;
}

void AddToAutostart(const QString& /*key*/, const QString& /*path*/)
{
	assert(false);
}

void RemoveFromAutostart(const QString& /*key*/)
{
	assert(false);
}

QString GetDefaultInstallerSuffix()
{
	return "dmg";
}

void RevealInFileManager(const QString& path)
{
	const auto file = QFileInfo(path).absoluteFilePath();
	if (file.isEmpty() || !QFileInfo::exists(file))
		return;

	QProcess::startDetached(QStringLiteral("/usr/bin/open"), { QStringLiteral("-R"), file });
}

} // namespace HomeCompa::Platform
