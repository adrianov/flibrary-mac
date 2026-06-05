#include "app.h"

#include <QCoreApplication>
#include <QFile>
#include <QString>

#include "platform/PlatformUtil.h"

#include "log.h"

namespace HomeCompa::Util
{

namespace
{

constexpr InstallerDescription MODES[] {
#define INSTALLER_MODE_ITEM(NAME, EXT, INSTALLABLE) { InstallerType::NAME, #NAME, EXT, INSTALLABLE },
	INSTALLER_MODE_ITEMS_X_MACRO
#undef INSTALLER_MODE_ITEM
};

const InstallerDescription& GetDefaultInstallerDescription()
{
	const auto it = std::ranges::find(MODES, Platform::GetDefaultInstallerSuffix(), [&](const auto& item) {
		return item.ext;
	});
	assert(it != std::end(MODES));
	PLOGD << "Installer mode: " << it->name;

	return *it;
}

}

const InstallerDescription& GetInstallerDescription()
{
	const auto fileNamePortable = QString("%1/installer_mode").arg(QCoreApplication::applicationDirPath());
	QFile      file(QString("%1/installer_mode").arg(QCoreApplication::applicationDirPath()));
	if (!file.open(QIODevice::ReadOnly))
		return GetDefaultInstallerDescription();

	const auto bytes = QString::fromUtf8(file.readAll());
	PLOGD << "Installer mode: " << bytes;
	const auto it = std::ranges::find_if(MODES, [&](const auto& item) {
		return bytes.startsWith(item.name, Qt::CaseInsensitive);
	});
	return it != std::end(MODES) ? *it : GetDefaultInstallerDescription();
}

} // namespace HomeCompa::Util
