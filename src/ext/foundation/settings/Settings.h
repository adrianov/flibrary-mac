#pragma once

#include <memory>

#include "ISettings.h"

#include "export/settings.h"

namespace HomeCompa::SettingsFactory
{

class AbstractSettings : virtual public ISettings
{
};

SETTINGS_EXPORT std::unique_ptr<AbstractSettings> CreateStub();
SETTINGS_EXPORT std::unique_ptr<AbstractSettings> Create(const QString& fileName);
SETTINGS_EXPORT std::unique_ptr<AbstractSettings> Create(const QString& organization, const QString& application);
SETTINGS_EXPORT std::unique_ptr<AbstractSettings> CreateDecorator(std::shared_ptr<ISettings> impl, std::unordered_map<QString, QVariant> replacement = {});

} // namespace HomeCompa::SettingsFactory
