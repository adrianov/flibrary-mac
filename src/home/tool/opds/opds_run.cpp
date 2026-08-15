#include "opds_run.h"
#include "opds_setup.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QTimer>

#include "fnd/ScopedCall.h"

#include "Constant.h"

#include "interface/IServer.h"
#include "interface/constants/ProductConstant.h"
#include "interface/constants/SettingsConstant.h"
#include "interface/localization.h"
#include "interface/logic/ICollectionAutoUpdater.h"

#include "Hypodermic/Hypodermic.h"
#include "inpx/InpxConstant.h"
#include "logic/data/Genre.h"
#include "platform/NativeEventFilter.h"
#include "settings/ISettings.h"
#include "util/SortString.h"
#include "util/xml/Initializer.h"

#include "di_app.h"
#include "log.h"

#include "config/version.h"

using namespace HomeCompa;
using namespace HomeCompa::Flibrary;

namespace
{

class NativeEventFilterObserver final : public Platform::NativeEventFilter::IObserver
{
private:
	void OnQueryEndSession(long long*) override
	{
		QCoreApplication::exit();
	}
};

class CollectionAutoUpdaterObserver final : ICollectionAutoUpdater::IObserver
{
	NON_COPY_MOVABLE(CollectionAutoUpdaterObserver)

public:
	explicit CollectionAutoUpdaterObserver(ICollectionAutoUpdater& updater)
		: m_updater { updater }
	{
		m_updater.RegisterObserver(this);
	}

	~CollectionAutoUpdaterObserver() override
	{
		m_updater.UnregisterObserver(this);
	}

private:
	void OnCollectionUpdated() override
	{
		QTimer::singleShot(0, [] {
			QCoreApplication::exit(Global::RESTART_APP);
		});
	}

private:
	ICollectionAutoUpdater& m_updater;
};

} // namespace

namespace HomeCompa::Opds
{

int RunOpds(int argc, char* argv[])
{
	QGuiApplication app(argc, argv);
	QCoreApplication::setApplicationName(APP_ID);
	QCoreApplication::setApplicationVersion(PRODUCT_VERSION);
	Util::XMLPlatformInitializer xmlPlatformInitializer;

	QCommandLineParser parser;
	parser.setApplicationDescription(QString("%1 recodes images").arg(APP_ID));
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addOptions({
		{ COLLECTION_NAME, "Collection name", COLLECTION_NAME },
		{ DB_PATH, "Database path", DB_PATH },
		{ ARCHIVE_FOLDER, "Archives folder", ARCHIVE_FOLDER },
		{ ADDITIONAL_FOLDER, "Additional data folder (optional)", ADDITIONAL_FOLDER },
		{ INPX_PATH, "Index inpx file (optional)", INPX_PATH },
		{ Constant::OPDS_SERVER_COMMAND_STOP, "Stop server" },
	});
	parser.process(app);

	NativeEventFilterObserver   nativeEventFilterObserver;
	Platform::NativeEventFilter nativeEventFilter(app);
	const ScopedCall            nativeEventFilterRegisterGuard(
		[&] {
			nativeEventFilter.Register(&nativeEventFilterObserver);
		},
		[&] {
			nativeEventFilter.Unregister(&nativeEventFilterObserver);
		}
	);

	while (true)
	{
		std::shared_ptr<Hypodermic::Container> container;
		{
			Hypodermic::ContainerBuilder builder;
			DiInit(builder, container);
		}

		if (CheckForStop(parser, *container))
			return 0;

		SetCollection(parser, *container);
		auto settings = container->resolve<ISettings>();
		Genre::SetSortMode(*settings);

		std::shared_ptr<ICollectionAutoUpdater>        collectionAutoUpdater;
		std::unique_ptr<CollectionAutoUpdaterObserver> collectionAutoUpdaterObserver;
		if (settings->Get(Constant::Settings::PREFER_OPDS_AUTOUPDATE_COLLECTION, false))
		{
			collectionAutoUpdater         = container->resolve<ICollectionAutoUpdater>();
			collectionAutoUpdaterObserver = std::make_unique<CollectionAutoUpdaterObserver>(*collectionAutoUpdater);
		}

		Util::QStringWrapper::SetLocale(Loc::GetLocale(*settings));
		Loc::LoadLocales(*settings);
		const auto server = container->resolve<IServer>();

		if (const auto code = QCoreApplication::exec(); code != Global::RESTART_APP)
		{
			PLOGI << "App finished with " << code;
			return code;
		}
		PLOGI << "App restarted";
	}
}

}
