#pragma once

#include <functional>

#include <QAbstractItemModel>
#include <QList>
#include <QModelIndex>
#include <QString>

#include "interface/logic/IDataItem.h"
#include "interface/logic/ILogicFactory.h"
#include "interface/logic/IInpxGenerator.h"
#include "interface/logic/IScriptController.h"
#include "interface/logic/ITreeViewController.h"
#include "interface/logic/IDatabaseUser.h"
#include "interface/logic/IDataProvider.h"
#include "interface/ui/IUiFactory.h"

#include "extract/BooksExtractor.h"
#include "settings/ISettings.h"

namespace HomeCompa::Flibrary
{

struct SendSettings
{
	QString ext;
	bool    tempFolder { false };
	bool    createFillTemplateConverterParameter { false };
};

struct BooksContextMenuExportHost
{
	std::weak_ptr<const ILogicFactory>       logicFactory;
	std::shared_ptr<const ISettings>         settings;
	std::shared_ptr<const IDatabaseUser>     databaseUser;
	std::shared_ptr<const IBookInfoProvider> dataProvider;
	std::shared_ptr<const IUiFactory>        uiFactory;
};

using BooksContextMenuCallback = std::function<void(IDataItem::Ptr)>;

void CreateSendMenu(
	const IDataItem::Ptr&                          root,
	ITreeViewController::RequestContextMenuOptions options,
	const IScriptController::Scripts&              scripts,
	const QString&                                 fileName
);

void SendAsImpl(
	const BooksContextMenuExportHost& host,
	QAbstractItemModel*               model,
	const QModelIndex&                index,
	const QList<QModelIndex>&         indexList,
	IDataItem::Ptr                    item,
	BooksContextMenuCallback          callback,
	BooksExtractor::Extract           f,
	const SendSettings&               sendSettings = {}
);

void Send(
	const BooksContextMenuExportHost& host,
	QAbstractItemModel*               model,
	const QModelIndex&                index,
	const QList<QModelIndex>&         indexList,
	IDataItem::Ptr                    item,
	BooksContextMenuCallback          callback,
	BooksExtractor::Extract           f,
	const QString&                    outputFileNameTemplate,
	bool                              dstFolderRequired,
	const SendSettings&               sendSettings
);

void SendAsInpxImpl(
	const BooksContextMenuExportHost& host,
	QAbstractItemModel*               model,
	const QModelIndex&                index,
	const QList<QModelIndex>&         indexList,
	IDataItem::Ptr                    item,
	BooksContextMenuCallback          callback,
	void (IInpxGenerator::*extractorMethod)(QString, const std::vector<QString>&, const IBookInfoProvider&, IInpxGenerator::Callback)
);

} // namespace HomeCompa::Flibrary
