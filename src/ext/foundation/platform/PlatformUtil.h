#pragma once

#include "export/platform.h"

class QRect;
class QString;

namespace HomeCompa::Platform
{

enum class PlatformType
{
	Windows,
	Linux,
	MacOS
};

PLATFORM_EXPORT PlatformType GetPlatformType() noexcept;
PLATFORM_EXPORT QString      GetPlatformName();

PLATFORM_EXPORT bool    IsRegisteredExtension(const QString& extension);
PLATFORM_EXPORT void    SetKeyboardLayout(const QString& locale);
PLATFORM_EXPORT void    SetKeyboardLayoutId(const QString& id);
PLATFORM_EXPORT QString GetKeyboardLayoutId();

PLATFORM_EXPORT bool IsAppAddedToAutostart(const QString& key);
PLATFORM_EXPORT void AddToAutostart(const QString& key, const QString& path);
PLATFORM_EXPORT void RemoveFromAutostart(const QString& key);

PLATFORM_EXPORT QString GetDefaultInstallerSuffix();

PLATFORM_EXPORT void RevealInFileManager(const QString& path);

}
