#include "PluginStyleApplier.h"

#include <QApplication>
#include <QStyleFactory>

#include "log.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace
{

QString ResolvePluginStyleName(const QString& style)
{
#ifdef Q_OS_MACOS
	(void)style;
	return IStyleApplier::THEME_NAME_DEFAULT;
#else
	return style;
#endif
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

#ifdef Q_OS_MACOS
	app.setStyleSheet(ReadStyleSheet(":/theme/style-macos.qss"));
#else
	app.setStyleSheet(ReadStyleSheet(STYLE_FILE_NAME));
#endif

	return {};
}
