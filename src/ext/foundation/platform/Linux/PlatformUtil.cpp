#include "PlatformUtil.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

namespace HomeCompa::Platform
{

PlatformType GetPlatformType() noexcept
{
	return PlatformType::Linux;
}

QString GetPlatformName()
{
	return "Linux";
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
	return "deb";
}

void RevealInFileManager(const QString& path)
{
	const QFileInfo info(path);
	if (!info.exists())
		return;

	const auto target = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
	QDesktopServices::openUrl(QUrl::fromLocalFile(target));
}

} // namespace HomeCompa::Platform
