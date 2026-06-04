#include "AnnotationController.h"

#include <atomic>
#include <functional>
#include <random>
#include <ranges>

#include <QTimer>

#include "fnd/EnumBitmask.h"
#include "fnd/observable.h"

#include "database/interface/IDatabase.h"
#include "database/interface/IQuery.h"

#include "interface/localization.h"
#include "interface/logic/IJokeRequester.h"
#include "interface/logic/IProgressController.h"

#include "data/DataItem.h"
#include "database/DatabaseUtil.h"
#include "util/language.h"

#include "AnnotationBookCache.h"
#include "AnnotationControllerFormat.h"
#include "AnnotationDbLoader.h"
#include "ArchiveDescriptionFallback.h"
#include "ArchiveParser.h"
#include "Constant.h"
#include "log.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace
{

enum class Ready
{
	None     = 0,
	Database = 1 << 0,
	Archive  = 1 << 1,
	All      = None | Database | Archive
};

class JokeRequesterClientImpl : virtual public IJokeRequester::IClient
{
public:
	static std::shared_ptr<IClient> Create(IClient& impl)
	{
		return std::make_shared<JokeRequesterClientImpl>(impl);
	}

public:
	explicit JokeRequesterClientImpl(IClient& impl)
		: m_impl(impl)
	{
	}

private: // IJokeRequester::IClient
	void OnError(const QString& api, const QString& error) override
	{
		m_impl.OnError(api, error);
	}

	void OnTextReceived(const QString& value) override
	{
		m_impl.OnTextReceived(value);
	}

	void OnImageReceived(const QByteArray& value) override
	{
		m_impl.OnImageReceived(value);
	}

private:
	IClient& m_impl;
};

} // namespace

ENABLE_BITMASK_OPERATORS(Ready);

class AnnotationController::Impl final
	: public Observable<IObserver>
	, public IDataProvider
	, IProgressController::IObserver
	, IJokeRequester::IClient
	, IFilterProvider::IObserver
{
	NON_COPY_MOVABLE(Impl)

public:
	Impl(
		const std::shared_ptr<const ILogicFactory>&  logicFactory,
		std::shared_ptr<const ISettings>             settings,
		std::shared_ptr<const ICollectionProvider>   collectionProvider,
		std::shared_ptr<const IJokeRequesterFactory> jokeRequesterFactory,
		std::shared_ptr<const IDatabaseUser>         databaseUser,
		std::shared_ptr<IFilterProvider>             filterProvider
	)
		: m_logicFactory { logicFactory }
		, m_settings { std::move(settings) }
		, m_collectionProvider { std::move(collectionProvider) }
		, m_jokeRequesterFactory { std::move(jokeRequesterFactory) }
		, m_databaseUser { std::move(databaseUser) }
		, m_filterProvider { std::move(filterProvider) }
		, m_executor { logicFactory->GetExecutor() }
		, m_jokeRequesterClientImpl(JokeRequesterClientImpl::Create(*this))
		, m_book { NavigationItem::Create() }
		, m_series { NavigationItem::Create() }
		, m_authors { NavigationItem::Create() }
		, m_genres { NavigationItem::Create() }
		, m_groups { NavigationItem::Create() }
		, m_keywords { NavigationItem::Create() }
		, m_folder { NavigationItem::Create() }
		, m_update { NavigationItem::Create() }
	{
		m_jokeTimer.setSingleShot(true);
		m_jokeTimer.setInterval(std::chrono::seconds(1));
		QObject::connect(&m_jokeTimer, &QTimer::timeout, [this] {
			RequestJoke();
		});
		m_filterProvider->RegisterObserver(this);
	}

	~Impl() override
	{
		m_filterProvider->UnregisterObserver(this);
		m_stopping = true;
		++m_preloadGeneration;
		++m_extractGeneration;
		DetachArchiveParserProgress();
		if (auto parser = m_archiveParser.lock())
			parser->Stop();
		if (m_executor)
			m_executor->Stop();
	}

public:
	void SetCurrentBookId(QString bookId, const bool extractNow, std::vector<QString> preloadBookIds)
	{
		DetachArchiveParserProgress();
		if (auto parser = m_archiveParser.lock())
			parser->Stop();

		++m_preloadGeneration;
		m_pendingPreloadIds = std::move(preloadBookIds);

		m_currentBookId = std::move(bookId);
		if (m_currentBookId.isEmpty())
		{
			Perform(&IAnnotationController::IObserver::OnAnnotationRequested);
			if (!m_jokeRequesters.empty())
				m_jokeTimer.start();
			return;
		}

		++m_extractGeneration;
		RequestSessionCache();
		(void)extractNow;
	}

	void ShowJokes(const IJokeRequesterFactory::Implementation impl, const bool value) noexcept
	{
		if (value)
			m_jokeRequesters.emplace_back(impl, m_jokeRequesterFactory->Create(impl));
		else
			std::erase_if(m_jokeRequesters, [impl](const auto& item) {
				return item.first == impl;
			});
	}

	void ShowReviews(const bool value) noexcept
	{
		m_showReviews = value;
		if (!m_currentBookId.isEmpty())
		{
			++m_extractGeneration;
			RequestSessionCache();
		}
	}

	void EmitAnnotationChanged()
	{
		if (!m_book || m_book->GetId().isEmpty() || m_book->GetId() != m_currentBookId)
			return;

		Perform(&IAnnotationController::IObserver::OnAnnotationChanged, std::cref(*this));
		StartPendingPreload();
	}

	void StartPendingPreload()
	{
		if (m_pendingPreloadIds.empty() || m_currentBookId.isEmpty())
			return;

		auto ids = std::move(m_pendingPreloadIds);
		PreloadBooks(std::move(ids));
	}

	void PreloadBooks(std::vector<QString> bookIds)
	{
		const auto generation  = m_preloadGeneration;
		const auto filter      = FilterEnabled();
		const auto showReviews = m_showReviews;

		if (bookIds.size() > 20)
			bookIds.resize(20);
		if (bookIds.empty())
			return;

		for (const auto& bookId : bookIds)
		{
			(*m_executor)(
				{ "Preload book annotation",
				  [this, bookId, generation, filter, showReviews] -> std::function<void(size_t)> {
					  if (m_stopping || generation != m_preloadGeneration || bookId.isEmpty() || bookId == m_currentBookId || m_bookCache.HasDatabase(bookId, filter, showReviews))
						  return [](size_t) {};

					  const auto db = m_databaseUser->Database();
					  if (!db)
						  return [](size_t) {};

					  auto book = AnnotationDbLoader::CreateBook(*db, bookId.toLongLong());
					  if (!book || book->GetId().isEmpty() || m_stopping || generation != m_preloadGeneration)
						  return [](size_t) {};

					  if (m_stopping || generation != m_preloadGeneration || m_bookCache.HasDatabase(bookId, filter, showReviews))
						  return [](size_t) {};

					  m_bookCache.PutDatabase(
						  bookId,
						  filter,
						  showReviews,
						  AnnotationDbLoader::Build(*db, book, filter, showReviews, *m_collectionProvider, *m_settings)
					  );
					  return [](size_t) {};
				  } },
				50,
				false
			);
		}
	}

private: // IDataProvider
	[[nodiscard]] const IDataItem& GetBook() const noexcept override
	{
		return *m_book;
	}

	[[nodiscard]] const IDataItem& GetSeries() const noexcept override
	{
		return *m_series;
	}

	[[nodiscard]] const IDataItem& GetAuthors() const noexcept override
	{
		return *m_authors;
	}

	[[nodiscard]] const IDataItem& GetGenres() const noexcept override
	{
		return *m_genres;
	}

	[[nodiscard]] const IDataItem& GetGroups() const noexcept override
	{
		return *m_groups;
	}

	[[nodiscard]] const IDataItem& GetKeywords() const noexcept override
	{
		return *m_keywords;
	}

	[[nodiscard]] const IDataItem& GetFolder() const noexcept override
	{
		return *m_folder;
	}

	[[nodiscard]] const IDataItem& GetUpdate() const noexcept override
	{
		return *m_update;
	}

	[[nodiscard]] const QString& GetError() const noexcept override
	{
		return m_archiveData.error;
	}

	[[nodiscard]] const QString& GetAnnotation() const noexcept override
	{
		return m_archiveData.annotation;
	}

	[[nodiscard]] const QString& GetEpigraph() const noexcept override
	{
		return m_archiveData.epigraph;
	}

	[[nodiscard]] const QString& GetEpigraphAuthor() const noexcept override
	{
		return m_archiveData.epigraphAuthor;
	}

	[[nodiscard]] const QString& GetLanguage() const noexcept override
	{
		return m_archiveData.language;
	}

	[[nodiscard]] const QString& GetSourceLanguage() const noexcept override
	{
		return m_archiveData.sourceLanguage;
	}

	[[nodiscard]] const QString& GetSourceLibrary() const noexcept override
	{
		return m_sourceLib;
	}

	[[nodiscard]] const std::vector<QString>& GetFb2Keywords() const noexcept override
	{
		return m_archiveData.keywords;
	}

	[[nodiscard]] const Covers& GetCovers() const noexcept override
	{
		return m_archiveData.covers;
	}

	[[nodiscard]] size_t GetTextSize() const noexcept override
	{
		return m_archiveData.textSize;
	}

	[[nodiscard]] size_t GetWordCount() const noexcept override
	{
		return m_archiveData.wordCount;
	}

	[[nodiscard]] IDataItem::Ptr GetContent() const noexcept override
	{
		return m_archiveData.content;
	}

	[[nodiscard]] IDataItem::Ptr GetDescription() const noexcept override
	{
		return m_archiveData.description;
	}

	[[nodiscard]] IDataItem::Ptr GetTranslators() const noexcept override
	{
		return m_archiveData.translators;
	}

	[[nodiscard]] const QString& GetPublisher() const noexcept override
	{
		return m_archiveData.publishInfo.publisher;
	}

	[[nodiscard]] const QString& GetPublishCity() const noexcept override
	{
		return m_archiveData.publishInfo.city;
	}

	[[nodiscard]] const QString& GetPublishYear() const noexcept override
	{
		return m_archiveData.publishInfo.year;
	}

	[[nodiscard]] const QString& GetPublishIsbn() const noexcept override
	{
		return m_archiveData.publishInfo.isbn;
	}

	[[nodiscard]] const ExportStatistics& GetExportStatistics() const noexcept override
	{
		return m_exportStatistics;
	}

	[[nodiscard]] const Reviews& GetReviews() const noexcept override
	{
		return m_reviews;
	}

	[[nodiscard]] std::vector<IDataItem::Flags> GetFlags(const NavigationMode navigationMode, const std::vector<QString>& ids) const override
	{
		return m_filterProvider->GetFlags(navigationMode, ids);
	}

private: // IProgressController::IObserver
	void OnStartedChanged() override
	{
		Perform(&IAnnotationController::IObserver::OnArchiveParserProgress, 0);
	}

	void OnValueChanged() override
	{
		if (const auto progressController = m_archiveParserProgressController.lock())
			Perform(&IAnnotationController::IObserver::OnArchiveParserProgress, static_cast<int>(std::lround(progressController->GetValue() * 100)));
	}

	void OnStop() override
	{
		Perform(&IAnnotationController::IObserver::OnArchiveParserProgress, 100);
	}

private: // IJokeRequester::IClient
	void OnError(const QString& api, const QString& error) override
	{
		Perform(&IAnnotationController::IObserver::OnJokeErrorOccured, std::cref(api), std::cref(error));
	}

	void OnTextReceived(const QString& value) override
	{
		Perform(&IAnnotationController::IObserver::OnJokeTextChanged, std::cref(value));
	}

	void OnImageReceived(const QByteArray& value) override
	{
		Perform(&IAnnotationController::IObserver::OnJokeImageChanged, std::cref(value));
	}

private: // IFilterProvider::IObserver
	void OnFilterBooksChanged() override
	{
		if (m_currentBookId.isEmpty())
			return;

		++m_extractGeneration;
		RequestSessionCache();
	}

private:
	[[nodiscard]] bool FilterEnabled() const
	{
		return m_filterProvider->IsFilterEnabled();
	}

	void ApplyDbData(const AnnotationDbCache& data)
	{
		m_book             = data.book;
		m_series           = data.series;
		m_authors          = data.authors;
		m_genres           = data.genres;
		m_groups           = data.groups;
		m_keywords         = data.keywords;
		m_folder           = data.folder;
		m_update           = data.update;
		m_exportStatistics = data.exportStatistics;
		m_reviews          = data.reviews;
		m_sourceLib        = data.sourceLib;
	}

	[[nodiscard]] AnnotationDbCache SnapshotDbData() const
	{
		return {
			m_book,
			m_series,
			m_authors,
			m_genres,
			m_groups,
			m_keywords,
			m_folder,
			m_update,
			m_exportStatistics,
			m_reviews,
			m_sourceLib,
		};
	}

	void ApplyCachedBook(const ArchiveParser::Data& archived, const AnnotationDbCache& dbData)
	{
		DetachArchiveParserProgress();
		if (auto parser = m_archiveParser.lock())
			parser->Stop();
		m_archiveData = archived;
		ApplyDbData(dbData);
		m_ready = Ready::All;
		EmitAnnotationChanged();
	}

	void RequestSessionCache(std::function<void()> onMiss = {})
	{
		if (m_currentBookId.isEmpty())
			return;

		const auto bookId      = m_currentBookId;
		const auto filter      = FilterEnabled();
		const auto showReviews = m_showReviews;

		(*m_executor)(
			{ "Load annotation cache",
			  [this, bookId, filter, showReviews, onMiss = std::move(onMiss)]() mutable {
				  const auto archived = m_bookCache.Archive(bookId);
				  const auto dbData   = m_bookCache.Database(bookId, filter, showReviews);
				  return [this, bookId, onMiss = std::move(onMiss), archived = std::move(archived), dbData = std::move(dbData)](size_t) mutable {
					  if (bookId != m_currentBookId)
						  return;

					  if (archived && dbData && dbData->book && !dbData->book->GetId().isEmpty())
					  {
						  ApplyCachedBook(*archived, *dbData);
						  return;
					  }

					  const bool hasDb = dbData && dbData->book && !dbData->book->GetId().isEmpty();
					  if (archived || hasDb)
					  {
						  LoadBookAndExtract(std::move(archived), hasDb ? std::move(dbData) : std::nullopt);
						  return;
					  }

					  if (onMiss)
						  onMiss();
					  else
						  LoadBookAndExtract();
				  };
			  } },
			1,
			true
		);
	}

	void LoadBookAndExtract(
		std::optional<ArchiveParser::Data> archived   = std::nullopt,
		std::optional<AnnotationDbCache>     dbData     = std::nullopt
	)
	{
		const auto generation = m_extractGeneration;
		auto       db         = m_databaseUser->Database();
		if (!db || m_currentBookId.isEmpty())
			return;

		m_databaseUser->Execute(
			{ "Get database book info",
		      [&, db = std::move(db), id = m_currentBookId.toLongLong(), generation, archived = std::move(archived), dbData = std::move(dbData)] {
				  return [this, generation, book = AnnotationDbLoader::CreateBook(*db, id), archived = std::move(archived), dbData = std::move(dbData)](size_t) mutable {
					  if (generation != m_extractGeneration || book->GetId() != m_currentBookId)
						  return;

					  if (archived || dbData)
						  ContinueExtractInfo(std::move(book), generation, std::move(archived), std::move(dbData));
					  else
						  BeginExtractInfo(std::move(book));
				  };
			  } },
			1
		);
	}

	void BeginExtractInfo(IDataItem::Ptr book)
	{
		if (book->GetId() != m_currentBookId)
			return;

		ContinueExtractInfo(std::move(book), m_extractGeneration, std::nullopt, std::nullopt);
	}

	void ContinueExtractInfo(
		IDataItem::Ptr                       book,
		const uint64_t                       generation,
		std::optional<ArchiveParser::Data> archived,
		std::optional<AnnotationDbCache>     dbData
	)
	{
		const auto bookId = book->GetId();
		if (generation != m_extractGeneration || bookId != m_currentBookId)
			return;

		m_ready = Ready::None;
		if (!archived)
			m_archiveData = {};
		else
			m_archiveData = *archived;

		if (dbData)
			ApplyDbData(*dbData);

		if (archived)
			m_ready |= Ready::Archive;
		if (dbData)
			m_ready |= Ready::Database;

		if (m_ready == Ready::All)
		{
			EmitAnnotationChanged();
			return;
		}

		if (!archived && !dbData)
		{
			ExtractArchiveInfo(book, generation);
			ExtractDatabaseInfo(std::move(book), generation);
			return;
		}

		if (!archived)
		{
			const auto archiveBook = (m_book && !m_book->GetId().isEmpty()) ? m_book : book;
			if (!archiveBook || archiveBook->GetId().isEmpty())
				return;
			ExtractArchiveInfo(archiveBook, generation);
			return;
		}

		ExtractDatabaseInfo(std::move(book), generation);
	}

	void DetachArchiveParserProgress()
	{
		if (const auto progressController = m_archiveParserProgressController.lock())
			progressController->UnregisterObserver(this);
	}

	void ExtractArchiveInfo(IDataItem::Ptr book, const uint64_t generation)
	{
		DetachArchiveParserProgress();

		auto parser     = ILogicFactory::Lock(m_logicFactory)->CreateArchiveParser();
		m_archiveParser = parser;

		(*m_executor)({ "Get archive book info",
		              [this, book = std::move(book), parser = std::move(parser), generation]() mutable {
						  const auto progressController = parser->GetProgressController();
						  progressController->RegisterObserver(this);
						  m_archiveParserProgressController = progressController;
						  auto data                         = parser->Parse(*book);
						  return [this, book = std::move(book), data = std::move(data), parser = std::move(parser), generation](size_t) mutable {
							  if (generation != m_extractGeneration || book->GetId() != m_currentBookId || m_archiveParser.lock() != parser)
								  return;

							  m_archiveData = std::move(data);
							  ArchiveDescriptionFallback::Apply(m_archiveData);
							  m_ready      |= Ready::Archive;
							  DetachArchiveParserProgress();
							  m_bookCache.PutArchive(book->GetId(), m_archiveData);

							  if (m_ready == Ready::All)
								  EmitAnnotationChanged();
						  };
					  } });
	}

	void ExtractDatabaseInfo(IDataItem::Ptr book, const uint64_t generation)
	{
		m_databaseUser->Execute(
			{ "Get database book additional info",
		      [this, book = std::move(book), generation]() mutable -> std::function<void(size_t)> {
				  const auto db = m_databaseUser->Database();
				  if (!db || !book || book->GetId().isEmpty())
					  return [](size_t) {};

				  return [this, generation, cache = AnnotationDbLoader::Build(*db, book, FilterEnabled(), m_showReviews, *m_collectionProvider, *m_settings)](size_t) mutable {
					  if (generation != m_extractGeneration || cache.book->GetId() != m_currentBookId)
						  return;

					  ApplyDbData(cache);
					  m_ready |= Ready::Database;
					  m_bookCache.PutDatabase(cache.book->GetId(), FilterEnabled(), m_showReviews, cache);
					  if (m_ready == Ready::All)
						  EmitAnnotationChanged();
				  };
			  } },
			3
		);
	}

	void RequestJoke()
	{
		if (m_jokeRequesters.empty())
			return;

		std::uniform_int_distribution<size_t> distribution(0, m_jokeRequesters.size() - 1);
		const auto                            index = distribution(m_mt);
		m_jokeRequesters[index].second->Request(m_jokeRequesterClientImpl);
	}

private:
	std::weak_ptr<const ILogicFactory>           m_logicFactory;
	std::shared_ptr<const ISettings>             m_settings;
	std::shared_ptr<const ICollectionProvider>   m_collectionProvider;
	std::shared_ptr<const IJokeRequesterFactory> m_jokeRequesterFactory;
	std::shared_ptr<const IDatabaseUser>         m_databaseUser;

	PropagateConstPtr<IFilterProvider, std::shared_ptr> m_filterProvider;

	std::vector<std::pair<IJokeRequesterFactory::Implementation, PropagateConstPtr<IJokeRequester, std::shared_ptr>>> m_jokeRequesters;

	PropagateConstPtr<Util::IExecutor>       m_executor;
	std::shared_ptr<IJokeRequester::IClient> m_jokeRequesterClientImpl;

	QString              m_currentBookId;
	std::vector<QString> m_pendingPreloadIds;

	Ready    m_ready { Ready::None };
	uint64_t              m_extractGeneration { 0 };
	uint64_t              m_preloadGeneration { 0 };
	std::atomic<bool>     m_stopping { false };

	std::weak_ptr<IProgressController> m_archiveParserProgressController;
	ArchiveParser::Data                m_archiveData;

	IDataItem::Ptr m_book;
	IDataItem::Ptr m_series;
	IDataItem::Ptr m_authors;
	IDataItem::Ptr m_genres;
	IDataItem::Ptr m_groups;
	IDataItem::Ptr m_keywords;
	IDataItem::Ptr m_folder;
	IDataItem::Ptr m_update;
	QString        m_sourceLib;

	ExportStatistics m_exportStatistics;
	Reviews          m_reviews;

	std::weak_ptr<ArchiveParser> m_archiveParser;

	AnnotationBookCache m_bookCache;

	bool   m_showReviews { true };
	QTimer m_jokeTimer;

	std::random_device m_rd;
	std::mt19937       m_mt { m_rd() };
};

AnnotationController::AnnotationController(
	const std::shared_ptr<const ILogicFactory>&  logicFactory,
	std::shared_ptr<const ISettings>             settings,
	std::shared_ptr<const ICollectionProvider>   collectionProvider,
	std::shared_ptr<const IJokeRequesterFactory> jokeRequesterFactory,
	std::shared_ptr<const IDatabaseUser>         databaseUser,
	std::shared_ptr<IFilterProvider>             filterProvider
)
	: m_impl(logicFactory, std::move(settings), std::move(collectionProvider), std::move(jokeRequesterFactory), std::move(databaseUser), std::move(filterProvider))
{
	PLOGV << "AnnotationController created";
}

AnnotationController::~AnnotationController()
{
	PLOGV << "AnnotationController destroyed";
}

void AnnotationController::SetCurrentBookId(QString bookId, const bool extractNow, std::vector<QString> preloadBookIds)
{
	m_impl->SetCurrentBookId(std::move(bookId), extractNow, std::move(preloadBookIds));
}

void AnnotationController::PreloadBooks(std::vector<QString> bookIds)
{
	m_impl->PreloadBooks(std::move(bookIds));
}

QString AnnotationController::CreateAnnotation(const IDataProvider& dataProvider, const IStrategy& strategy) const
{
	return CreateBookAnnotation(dataProvider, strategy);
}

void AnnotationController::ShowJokes(const IJokeRequesterFactory::Implementation impl, const bool value)
{
	m_impl->ShowJokes(impl, value);
}

void AnnotationController::ShowReviews(const bool value)
{
	m_impl->ShowReviews(value);
}

void AnnotationController::RegisterObserver(IObserver* observer)
{
	m_impl->Register(observer);
}

void AnnotationController::UnregisterObserver(IObserver* observer)
{
	m_impl->Unregister(observer);
}
