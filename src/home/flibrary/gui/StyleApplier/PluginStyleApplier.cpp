#include "PluginStyleApplier.h"

#include <QApplication>
#include <QStyleFactory>

#include "log.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace
{

#ifdef Q_OS_MACOS
constexpr auto MACOS_STYLE_FILE_NAME = ":/theme/style-macos.qss";
#endif

QString ResolvePluginStyleName(const QString& style)
{
#ifdef Q_OS_MACOS
	if (style.compare("windowsvista", Qt::CaseInsensitive) == 0 || style.compare("Windows", Qt::CaseInsensitive) == 0 || style.compare("Windows11", Qt::CaseInsensitive) == 0)
		return IStyleApplier::THEME_NAME_DEFAULT;
#endif
	return style;
}

} // namespace

PluginStyleApplier::PluginStyleApplier(std::shared_ptr<ISettings> settings)
	: AbstractThemeApplier(std::move(settings))
{
	PLOGV << "PluginStyleApplier created";
}

PluginStyleApplier::~PluginStyleApplier()
{
	PLOGV << "PluginStyleApplier destroyed";
}

IStyleApplier::Type PluginStyleApplier::GetType() const noexcept
{
	return Type::PluginStyle;
}

std::unique_ptr<Platform::DyLib> PluginStyleApplier::Set(QApplication& app) const
{
	auto style = ResolvePluginStyleName(m_settings->Get(THEME_NAME_KEY, THEME_NAME_DEFAULT));
	if (!QStyleFactory::keys().contains(style, Qt::CaseInsensitive))
		style = THEME_NAME_DEFAULT;

	QApplication::setStyle(style);

	auto stylesheet = ReadStyleSheet(STYLE_FILE_NAME);
#ifdef Q_OS_MACOS
	stylesheet += ReadStyleSheet(MACOS_STYLE_FILE_NAME);
#endif
	app.setStyleSheet(stylesheet);

	return {};
}
