#pragma once

#include "export/util.h"

namespace HomeCompa::Util
{

#define INSTALLER_MODE_ITEMS_X_MACRO           \
    INSTALLER_MODE_ITEM(exe, "exe", true)      \
    INSTALLER_MODE_ITEM(msi, "msi", true)      \
    INSTALLER_MODE_ITEM(portable, "7z", false) \
    INSTALLER_MODE_ITEM(deb, "deb", false)     \
	INSTALLER_MODE_ITEM(xz, "xz", false)

enum class InstallerType
{
#define INSTALLER_MODE_ITEM(NAME, _1, _2) NAME,
	INSTALLER_MODE_ITEMS_X_MACRO
#undef INSTALLER_MODE_ITEM
};

struct InstallerDescription
{
	InstallerType type;
	const char*   name;
	const char*   ext;
	bool          installable;
};

UTIL_EXPORT const InstallerDescription& GetInstallerDescription();

}
