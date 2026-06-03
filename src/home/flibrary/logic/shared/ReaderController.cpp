#include "ReaderController.h"

#include <random>

#include "database/interface/ICommand.h"
#include "database/interface/IDatabase.h"
#include "database/interface/IQuery.h"
#include "database/interface/ITransaction.h"

#include "interface/constants/ExportStat.h"
#include "interface/localization.h"

#include "util/IExecutor.h"

#include "shared/ReaderExtract.h"
#include "shared/ReaderLaunch.h"

#include "log.h"

using namespace HomeCompa::Flibrary;
using namespace HomeCompa;

namespace
{

constexpr auto DEFAULT_FOLDER_KEY = "Preferences/Export/readFolder";

}

struct ReaderController::Impl
{
	std::weak_ptr<const ILogicFactory>                        logicFactory;
	std::shared_ptr<ISettings>                                settings;
	PropagateConstPtr<ICollectionController, std::shared_ptr> collectionController;
	PropagateConstPtr<IUiFactory, std::shared_ptr>            uiFactory;
	PropagateConstPtr<IDatabaseUser, std::shared_ptr>         databaseUser;

	mutable std::random_device rd {};
	mutable std::mt19937       mt { rd() };

	Impl(
		const std::shared_ptr<const ILogicFactory>& logicFactory,
		std::shared_ptr<ISettings>                  settings,
		std::shared_ptr<ICollectionController>      collectionController,
		std::shared_ptr<IUiFactory>                 uiFactory,
		std::shared_ptr<IDatabaseUser>              databaseUser
	)
		: logicFactory { logicFactory }
		, settings { std::move(settings) }
		, collectionController { std::move(collectionController) }
		, uiFactory { std::move(uiFactory) }
		, databaseUser { std::move(databaseUser) }
	{
	}

	void Read(std::shared_ptr<ILogicFactory::ITemporaryDir> temporaryDir, QString fileName, const QString& error, const long long bookId = 0) const
	{
		LaunchConfiguredReader(*settings, *uiFactory, std::move(temporaryDir), std::move(fileName), error, bookId);
	}
};

ReaderController::ReaderController(
	const std::shared_ptr<const ILogicFactory>& logicFactory,
	std::shared_ptr<ISettings>                  settings,
	std::shared_ptr<ICollectionController>      collectionController,
	std::shared_ptr<IUiFactory>                 uiFactory,
	std::shared_ptr<IDatabaseUser>              databaseUser
)
	: m_impl(logicFactory, std::move(settings), std::move(collectionController), std::move(uiFactory), std::move(databaseUser))
{
	PLOGV << "ReaderController created";
}

ReaderController::~ReaderController()
{
	PLOGV << "ReaderController destroyed";
}

void ReaderController::Read(long long id) const
{
	std::shared_ptr executor = ILogicFactory::Lock(m_impl->logicFactory)->GetExecutor();

	const auto& executorRef = *executor;
	executorRef({ "Get archive and file names", [this, id, executor = std::move(executor)]() mutable {
					 const auto db = m_impl->databaseUser->Database();
					 {
						 const auto transaction = db->CreateTransaction();
						 const auto command     = transaction->CreateQuery(ExportStat::INSERT_QUERY);
						 command->Bind(0, id);
						 command->Bind(1, static_cast<int>(ExportStat::Type::Read));
						 command->Execute();
						 transaction->Commit();
					 }

					 const auto query = db->CreateQuery("select f.FolderTitle, b.FileName||b.Ext from Books b join Folders f on f.FolderID = b.FolderID where b.BookID = ?");
					 query->Bind(0, id);
					 query->Execute();
					 assert(!query->Eof());

					 QString folderName = query->Get<const char*>(0), fileName = query->Get<const char*>(1);
					 auto    archive = QString("%1/%2").arg(m_impl->collectionController->GetActiveCollection().GetFolder(), folderName);
					 QString error;
					 auto    temporaryDir = [this] {
						 const auto logicFactory = ILogicFactory::Lock(m_impl->logicFactory);
						 if (const auto folder = m_impl->settings->Get(DEFAULT_FOLDER_KEY); folder.isValid())
							 return logicFactory->CreateTemporaryDir(folder.toString());
						 return logicFactory->CreateTemporaryDir(true);
					 }();
					 ExtractBookForReading(*m_impl->settings, *temporaryDir, archive, fileName, error, ILogicFactory::Lock(m_impl->logicFactory));
					 return [this, executor = std::move(executor), fileName = std::move(fileName), temporaryDir = std::move(temporaryDir), error(std::move(error)), id](size_t) mutable {
						 m_impl->Read(std::move(temporaryDir), std::move(fileName), error, id);
						 executor.reset();
					 };
				 } });
}

void ReaderController::ReadRandomBook(QString lang) const
{
	m_impl->databaseUser->Execute({ "Get books for language", [this, lang = std::move(lang)]() {
									   std::function<void(size_t)> result { [](size_t) {
									   } };
									   const auto                  db    = m_impl->databaseUser->Database();
									   const auto                  query = db->CreateQuery("select b.BookID from Books b join Folders f on f.FolderID = b.FolderID where b.Lang = ?");
									   query->Bind(0, lang.toStdString());
									   std::vector<long long> allBooks;
									   for (query->Execute(); !query->Eof(); query->Next())
										   allBooks.emplace_back(query->Get<long long>(0));

									   if (allBooks.empty())
										   return result;

									   std::uniform_int_distribution<size_t> distribution(0, allBooks.size() - 1);
									   const auto                            n = distribution(m_impl->mt);

									   result = [this, id = allBooks[n]](size_t) mutable {
										   Read(id);
									   };

									   return result;
								   } });
}
