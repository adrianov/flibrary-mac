#include "opds_setup.h"

#include <QCommandLineParser>
#include <QEventLoop>

#include "interface/constants/ProductConstant.h"
#include "interface/constants/SettingsConstant.h"
#include "interface/logic/ICollectionController.h"
#include "interface/logic/IOpdsController.h"

#include "Hypodermic/Hypodermic.h"
#include "inpx/InpxConstant.h"
#include "logic/Collection/CollectionImpl.h"
#include "settings/ISettings.h"

using namespace HomeCompa;
using namespace HomeCompa::Flibrary;

namespace
{

class CollectionControllerObserver final : public ICollectionsObserver
{
public:
	explicit CollectionControllerObserver(QEventLoop& eventLoop)
		: m_eventLoop { eventLoop }
	{
	}

private:
	void OnActiveCollectionChanged() override
	{
		m_eventLoop.exit();
	}

	void OnNewCollectionCreating(bool) override
	{
	}

private:
	QEventLoop& m_eventLoop;
};

} // namespace

namespace HomeCompa::Opds
{

bool CheckForStop(const QCommandLineParser& parser, Hypodermic::Container& container)
{
	if (!parser.isSet(Constant::OPDS_SERVER_COMMAND_STOP))
		return false;
	const auto opdsController = container.resolve<IOpdsController>();
	if (opdsController->IsRunning())
		return opdsController->Stop(), true;
	return false;
}

void SetCollection(const QCommandLineParser& parser, Hypodermic::Container& container)
{
	const auto collectionController = container.resolve<ICollectionController>();
	auto       name                 = parser.isSet(COLLECTION_NAME) ? parser.value(COLLECTION_NAME) : QString {};
	if (name.isEmpty())
	{
		if (collectionController->ActiveCollectionExists())
			return;
		throw std::runtime_error("Active collection not found");
	}

	const auto& collections = collectionController->GetCollections();
	if (const auto it = std::ranges::find(
			collections,
			name,
			[](const auto& collection) {
				return collection->name;
			}
		);
	    it != collections.end())
	{
		if (collectionController->ActiveCollectionExists() && (*it)->id == collectionController->GetActiveCollectionId())
			return;
		return collectionController->SetActiveCollection((*it)->id);
	}

	if (!parser.isSet(DB_PATH))
		throw std::invalid_argument("Database path required");
	if (!parser.isSet(ARCHIVE_FOLDER))
		throw std::invalid_argument("Archive folder required");

	QEventLoop                   eventLoop;
	CollectionControllerObserver observer(eventLoop);
	const auto                   collection = collectionController->CreateCollection(std::move(name), parser.value(DB_PATH), parser.value(ARCHIVE_FOLDER), parser.value(ADDITIONAL_FOLDER), parser.value(INPX_PATH));
	collectionController->RegisterObserver(&observer);
	collectionController->CreateCollection(*collection);
	eventLoop.exec();
	collectionController->UnregisterObserver(&observer);
	container.resolve<ISettings>()->Set(Constant::Settings::PREFER_OPDS_AUTOUPDATE_COLLECTION, true);
}

}
