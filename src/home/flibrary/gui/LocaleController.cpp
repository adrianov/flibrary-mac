#include "LocaleController.h"

#include <QActionGroup>
#include <QApplication>
#include <QMenu>

#include "interface/constants/SettingsConstant.h"
#include "interface/localization.h"

#include "util/SortString.h"

#include "log.h"

namespace HomeCompa::Flibrary
{

namespace
{

constexpr auto NAME = "name";

void ApplyLocale(const QString& locale)
{
	Loc::LoadLocales(locale);
	Util::QStringWrapper::SetLocale(locale);

	if (auto* app = qobject_cast<QApplication*>(QCoreApplication::instance()))
	{
		QEvent event(QEvent::LanguageChange);
		for (auto* widget : app->allWidgets())
			QApplication::sendEvent(widget, &event);
	}
}

}

class LocaleController::Impl
{
public:
	explicit Impl(std::shared_ptr<ISettings> settings, std::shared_ptr<IUiFactory> uiFactory)
		: m_settings(std::move(settings))
		, m_uiFactory(std::move(uiFactory))
	{
		m_actionGroup.setExclusive(true);
	}

	void Setup(QMenu& menu)
	{
		const auto locales = Loc::GetLocales();
		if (locales.empty())
			return;

		if (locales.size() == 1)
			return Util::QStringWrapper::SetLocale(locales.front());

		const auto currentLocale = Loc::GetLocale(*m_settings);
		Util::QStringWrapper::SetLocale(currentLocale);
		for (const auto* locale : locales)
		{
			auto* action = menu.addAction(Loc::Tr(Loc::Ctx::LANG, locale), [&, locale] {
				SetLocale(locale);
			});
			action->setObjectName(locale);
			action->setProperty(NAME, QString(locale));
			m_actionGroup.addAction(action);
			action->setCheckable(true);
			action->setChecked(currentLocale == locale);
			action->setEnabled(currentLocale != locale);
		}
	}

	void RetranslateMenu()
	{
		for (auto* action : m_actionGroup.actions())
		{
			const auto locale = action->property(NAME).toString();
			action->setText(Loc::Tr(Loc::Ctx::LANG, locale.toUtf8().constData()));
		}
	}

private:
	void SetLocale(const QString& locale)
	{
		for (auto* action : m_actionGroup.actions())
		{
			const auto selected = action->property(NAME).toString() == locale;
			action->setChecked(selected);
			action->setEnabled(!selected);
		}

		m_settings->Set(Constant::Settings::LOCALE_KEY, locale);
		ApplyLocale(locale);
	}

private:
	PropagateConstPtr<ISettings, std::shared_ptr>  m_settings;
	PropagateConstPtr<IUiFactory, std::shared_ptr> m_uiFactory;
	QActionGroup                                   m_actionGroup { nullptr };
};

LocaleController::LocaleController(std::shared_ptr<ISettings> settings, std::shared_ptr<IUiFactory> uiFactory, QObject* parent)
	: QObject(parent)
	, m_impl(std::move(settings), std::move(uiFactory))
{
	PLOGV << "LocaleController created";
}

LocaleController::~LocaleController()
{
	PLOGV << "LocaleController destroyed";
}

void LocaleController::Setup(QMenu& menu)
{
	m_impl->Setup(menu);
}

void LocaleController::RetranslateMenu()
{
	m_impl->RetranslateMenu();
}

} // namespace HomeCompa::Flibrary
