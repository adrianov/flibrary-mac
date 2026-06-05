#include "BooksContextMenuExport.h"

#include <ranges>
#include <unordered_map>
#include <unordered_set>

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QVariant>

#include "interface/constants/Enums.h"
#include "interface/constants/ModelRole.h"
#include "interface/constants/SettingsConstant.h"
#include "interface/localization.h"

#include "Constant.h"
#include "MenuItems.h"
#include "QtTypes.h"
#include "util/Fb2Format.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace
{

constexpr auto CONTEXT             = "BookContextMenu";
constexpr auto EXPORT              = QT_TRANSLATE_NOOP("BookContextMenu", "E&xport");
constexpr auto SEND_AS_ARCHIVE     = QT_TRANSLATE_NOOP("BookContextMenu", "As &zip archive");
constexpr auto SEND_AS_IS          = QT_TRANSLATE_NOOP("BookContextMenu", "As &original format");
constexpr auto SEND_AS_IS_FMT      = QT_TRANSLATE_NOOP("BookContextMenu", "As &original format (%1)");
constexpr auto SEND_AS_EPUB        = QT_TRANSLATE_NOOP("BookContextMenu", "As &epub");
constexpr auto UNPACK              = QT_TRANSLATE_NOOP("BookContextMenu", "&Unpack");
constexpr auto SEND_AS_INPX        = QT_TRANSLATE_NOOP("BookContextMenu", "As &inpx collection");
constexpr auto SEND_AS_SINGLE_INPX = QT_TRANSLATE_NOOP("BookContextMenu", "Generate inde&x file (*.inpx)");
constexpr auto SAME_NAMED_FILES           = QT_TRANSLATE_NOOP("BookContextMenu", "What to do with the same named files?");
constexpr auto SAME_NAMED_FILES_SKIP      = QT_TRANSLATE_NOOP("BookContextMenu", "Skip");
constexpr auto SAME_NAMED_FILES_OVERWRITE = QT_TRANSLATE_NOOP("BookContextMenu", "Overwrite");
constexpr auto SAME_NAMED_FILES_RENAME    = QT_TRANSLATE_NOOP("BookContextMenu", "Rename");
constexpr auto SAME_NAMED_FILES_CANCEL    = QT_TRANSLATE_NOOP("BookContextMenu", "Cancel");

TR_DEF

bool CheckUniqueFileNames(const BooksContextMenuExportHost& host, Util::ExtractedBooks& books)
{
	const auto whatTodo = [&] {
		std::unordered_set<QString> uniqueNames;
		if (std::ranges::any_of(books, [&](const auto& book) {
				return QFile::exists(book.dstFileName) || !uniqueNames.emplace(book.dstFileName).second;
			}))
			return host.uiFactory->ShowCustomDialog(
				QMessageBox::Question,
				Loc::Tr(Loc::Ctx::COMMON, Loc::WARNING),
				Tr(SAME_NAMED_FILES),
				{
					{      QMessageBox::AcceptRole,      Tr(SAME_NAMED_FILES_SKIP) },
					{ QMessageBox::DestructiveRole, Tr(SAME_NAMED_FILES_OVERWRITE) },
					{          QMessageBox::NoRole,    Tr(SAME_NAMED_FILES_RENAME) },
					{      QMessageBox::RejectRole,    Tr(SAME_NAMED_FILES_CANCEL) },
				},
				QMessageBox::AcceptRole
			);

		return QMessageBox::AcceptRole;
	}();

	if (whatTodo == QMessageBox::RejectRole)
		return true;

	if (whatTodo == QMessageBox::NoRole)
		return false;

	std::unordered_map<QString, size_t> unique;
	for (const auto& [book, index] : std::views::zip(books, std::views::iota(0)))
	{
		if (QFile::exists(book.dstFileName))
		{
			if (whatTodo == QMessageBox::DestructiveRole)
				QFile::remove(book.dstFileName);
			else
				continue;
		}

		auto [it, inserted] = unique.try_emplace(book.dstFileName, index);
		if (inserted)
			continue;

		if (whatTodo == QMessageBox::DestructiveRole)
			it->second = index;
	}

	const auto indices = unique | std::views::values | std::ranges::to<std::unordered_set<size_t>>();
	std::erase_if(books, [&, index = 0ULL](const auto&) mutable {
		return !indices.contains(index++);
	});

	return false;
}

} // namespace

namespace HomeCompa::Flibrary
{

void Send(
	const BooksContextMenuExportHost& host,
	QAbstractItemModel*               model,
	const QModelIndex&                index,
	const QList<QModelIndex>&         indexList,
	IDataItem::Ptr                    item,
	BooksContextMenuCallback          callback,
	BooksExtractor::Extract           f,
	const QString&                    outputFileNameTemplate,
	const bool                        dstFolderRequired,
	const SendSettings&               sendSettings
)
{
	auto dir = dstFolderRequired ? host.uiFactory->GetExistingDirectory(Constant::Settings::EXPORT_DIALOG_KEY, Loc::SELECT_SEND_TO_FOLDER) : QString();
	if (dstFolderRequired && dir.isEmpty())
		return callback(item);

	const auto logicFactory = ILogicFactory::Lock(host.logicFactory);

	auto books = logicFactory->GetExtractedBooks(model, index, indexList);

	const auto fillTemplateConverter = logicFactory->CreateFillTemplateConverter(sendSettings.createFillTemplateConverterParameter);

	std::shared_ptr<QTemporaryDir> tempDir;
	if (sendSettings.tempFolder)
		tempDir = std::make_shared<QTemporaryDir>();

	const auto db = host.databaseUser->Database();
	for (auto& book : books)
		fillTemplateConverter->Fill(*db, outputFileNameTemplate, book, tempDir ? tempDir->filePath("") : dir);

	if (!sendSettings.ext.isEmpty())
	{
		for (auto& book : books)
		{
			const QFileInfo fileInfo(book.dstFileName);
			book.dstFileName = fileInfo.dir().filePath(fileInfo.completeBaseName() + "." + sendSettings.ext);
		}
	}

	if (CheckUniqueFileNames(host, books))
		return callback(item);

	auto       ids       = books | std::views::transform([](const auto& book) {
				   return QString::number(book.id);
						   })
	                     | std::ranges::to<std::set<QString>>();
	auto       extractor = logicFactory->CreateBooksExtractor();
	const auto parameter = item->GetData(MenuItem::Column::Parameter);
	((*extractor)
	 .*f)(dir, parameter, std::move(books), [extractor, model, item = std::move(item), ids = std::move(ids), tempDir = std::move(tempDir), callback = std::move(callback)](const bool hasError) mutable {
		item->SetData(QString::number(hasError), MenuItem::Column::HasError);
		callback(item);
		model->setData({}, QVariant::fromValue(ids), Role::Uncheck);
		extractor.reset();
	});
}

void CreateSendMenu(
	const IDataItem::Ptr&                          root,
	const ITreeViewController::RequestContextMenuOptions options,
	const IScriptController::Scripts&              scripts,
	const QString&                                 fileName
)
{
	const auto& send = AddMenuItem(root, EXPORT, Tr(EXPORT));
	AddMenuItem(send, SEND_AS_ARCHIVE, Tr(SEND_AS_ARCHIVE), BooksMenuAction::SendAsArchive);

	const auto suffix   = QFileInfo(fileName).suffix();
	const auto asIsText = suffix.isEmpty() ? Tr(SEND_AS_IS) : Tr(SEND_AS_IS_FMT).arg(suffix);
	AddMenuItem(send, SEND_AS_IS, asIsText, BooksMenuAction::SendAsIs);

	const bool epubEnabled = Util::IsExportableEpubSource(fileName);
	if (const auto epubItem = AddMenuItem(send, SEND_AS_EPUB, Tr(SEND_AS_EPUB), BooksMenuAction::SendAsEpub); !epubEnabled)
		epubItem->SetData(QVariant(false).toString(), MenuItem::Column::Enabled);

	if (!!(options & ITreeViewController::RequestContextMenuOptions::IsArchive))
		AddMenuItem(send, UNPACK, Tr(UNPACK), BooksMenuAction::SendUnpack);

	if (!scripts.empty())
	{
		AddMenuItem(send)->SetData(QString::number(-1), MenuItem::Column::Parameter);
		for (const auto& script : scripts)
		{
			const auto& scriptItem = AddMenuItem(send, script.uid, script.name, BooksMenuAction::SendAsScript);
			scriptItem->SetData(script.uid, MenuItem::Column::Parameter);
		}
	}
	AddMenuItem(send)->SetData(QString::number(-1), MenuItem::Column::Parameter);
	AddMenuItem(send, SEND_AS_INPX, Tr(SEND_AS_INPX), BooksMenuAction::SendAsInpxCollection);
	AddMenuItem(send, SEND_AS_SINGLE_INPX, Tr(SEND_AS_SINGLE_INPX), BooksMenuAction::SendAsInpxFile);
}

void SendAsImpl(
	const BooksContextMenuExportHost& host,
	QAbstractItemModel*               model,
	const QModelIndex&                index,
	const QList<QModelIndex>&         indexList,
	IDataItem::Ptr                    item,
	BooksContextMenuCallback          callback,
	const BooksExtractor::Extract     f,
	const SendSettings&               sendSettings
)
{
	const auto outputFileNameTemplate = host.settings->Get(Constant::Settings::EXPORT_TEMPLATE_KEY, IScriptController::GetDefaultOutputFileNameTemplate());
	const auto dstFolderRequired      = IScriptController::HasMacro(outputFileNameTemplate, IScriptController::Macro::UserDestinationFolder);
	Send(host, model, index, indexList, std::move(item), std::move(callback), f, outputFileNameTemplate, dstFolderRequired, sendSettings);
}

void SendAsInpxImpl(
	const BooksContextMenuExportHost& host,
	QAbstractItemModel*               model,
	const QModelIndex&                index,
	const QList<QModelIndex>&         indexList,
	IDataItem::Ptr                    item,
	BooksContextMenuCallback          callback,
	void (IInpxGenerator::*extractorMethod)(QString, const std::vector<QString>&, const IBookInfoProvider&, IInpxGenerator::Callback)
)
{
	auto idList = ILogicFactory::Lock(host.logicFactory)->GetSelectedBookIds(model, index, indexList, { Role::Id });
	if (idList.empty())
		return;

	std::transform(std::next(idList.begin()), idList.end(), std::back_inserter(idList.front()), [](auto& id) {
		return std::move(id.front());
	});

	auto inpxName = host.uiFactory->GetSaveFileName(Constant::Settings::EXPORT_DIALOG_KEY, Loc::Tr(Loc::EXPORT, Loc::SELECT_INPX_FILE), Loc::Tr(Loc::EXPORT, Loc::SELECT_INPX_FILE_FILTER));
	if (inpxName.isEmpty())
		return callback(item);

	auto extractor = ILogicFactory::Lock(host.logicFactory)->CreateInpxGenerator();
	std::invoke(extractorMethod, *extractor, std::move(inpxName), idList.front(), *host.dataProvider, [extractor, item = std::move(item), callback = std::move(callback)](const bool hasError) mutable {
		item->SetData(QString::number(hasError), MenuItem::Column::HasError);
		callback(item);
		extractor.reset();
	});
}

} // namespace HomeCompa::Flibrary
