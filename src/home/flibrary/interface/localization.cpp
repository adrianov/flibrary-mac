#include "localization.h"

#include <algorithm>
#include <ranges>

#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QTranslator>

#include "constants/SettingsConstant.h"
#include "settings/ISettings.h"

#include "log.h"

#include "config/locales.h"

namespace HomeCompa::Loc
{

namespace
{

std::vector<PropagateConstPtr<QTranslator>> g_translators;

uint64_t g_nextLocaleHandlerId { 1 };
std::vector<std::pair<uint64_t, std::function<void()>>> g_localeHandlers;

QString LocalesDir()
{
#ifdef Q_OS_MACOS
	return QCoreApplication::applicationDirPath() + "/../Resources/locales";
#else
	return QCoreApplication::applicationDirPath() + "/locales";
#endif
}

void UninstallTranslators()
{
	for (auto& translator : g_translators)
		if (translator)
			QCoreApplication::removeTranslator(translator.get());
	g_translators.clear();
}

} // namespace

QString Tr(const char* context, const char* str)
{
	return QCoreApplication::translate(context, str);
}

std::vector<const char*> GetLocales()
{
	const QDir dir = LocalesDir();

	std::vector<const char*> result;
	std::ranges::copy(
		LOCALES | std::views::filter([&](const auto* item) {
			return !dir.entryList(QStringList() << QString("*_%1.qm").arg(item), QDir::Files).isEmpty();
		}),
		std::back_inserter(result)
	);
	return result;
}

QString GetLocale(const ISettings& settings)
{
	if (auto locale = settings.Get(Flibrary::Constant::Settings::LOCALE_KEY).toString(); !locale.isEmpty())
		return locale;

	if (const auto it = std::ranges::find_if(
			LOCALES,
			[sysLocale = QLocale::system().name()](const char* item) {
				return sysLocale.startsWith(item);
			}
		);
	    it != std::cend(LOCALES))
		return *it;

	assert(!std::empty(Loc::LOCALES));
	return LOCALES[0];
}

const std::vector<PropagateConstPtr<QTranslator>>& LoadLocales(const ISettings& settings)
{
	return LoadLocales(GetLocale(settings));
}

const std::vector<PropagateConstPtr<QTranslator>>& LoadLocales(const QString& locale)
{
	UninstallTranslators();
	const QDir dir = LocalesDir();

	for (const auto& file : dir.entryList(QStringList() << QString("*_%1.qm").arg(locale), QDir::Files))
	{
		const auto fileName   = dir.absoluteFilePath(file);
		auto&      translator = g_translators.emplace_back();
		if (translator->load(fileName))
			QCoreApplication::installTranslator(translator.get());
		else
			PLOGE << "Cannot load " << fileName;
	}

	return g_translators;
}

uint64_t RegisterLocaleChangedHandler(std::function<void()> handler)
{
	const auto id = g_nextLocaleHandlerId++;
	g_localeHandlers.emplace_back(id, std::move(handler));
	return id;
}

void UnregisterLocaleChangedHandler(const uint64_t id)
{
	std::erase_if(g_localeHandlers, [id](const auto& item) {
		return item.first == id;
	});
}

void NotifyLocaleChanged()
{
	const auto handlers = g_localeHandlers;
	for (const auto& [_, handler] : handlers)
		handler();
}

} // namespace HomeCompa::Loc
