#pragma once

#include "zip.h"

#include "export/util.h"

namespace HomeCompa::Util
{

struct ExtractedBook
{
	long long   id;
	QString     folder;
	QString     file;
	int64_t     size;
	QString     author;
	QString     series;
	int         seqNumber;
	QString     title;
	QString     genre;
	QStringList genreTree;
	long long   libId;
	QString     lang;

	struct Author
	{
		QString firstName;
		QString middleName;
		QString lastName;
	};

	Author authorFull;

	QString dstFileName;
};

using ExtractedBooks = std::vector<ExtractedBook>;

} // namespace HomeCompa::Util

namespace HomeCompa::Util::Remove
{

struct Book
{
	long long id;
	QString   folder;
	QString   file;
};

using Books = std::vector<Book>;

using AllFilesItem = std::tuple<std::vector<QString>, std::shared_ptr<Zip::ProgressCallback>>;
using AllFiles     = std::unordered_map<QString, AllFilesItem>;
UTIL_EXPORT AllFiles CollectBookFiles(Books& books, const std::function<std::shared_ptr<Zip::ProgressCallback>()>& progressProvider);
UTIL_EXPORT AllFiles CollectImageFiles(const AllFiles& bookFiles, const QString& collectionFolder, const std::function<std::shared_ptr<Zip::ProgressCallback>()>& progressProvider);
UTIL_EXPORT void     RemoveFiles(AllFiles& allFiles, const QString& collectionFolder);

}
