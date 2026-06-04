#pragma once

#include <QString>

namespace HomeCompa
{

class ISettings;

}

namespace HomeCompa::Util
{

struct Fb2ToEpubOptions;

}

namespace HomeCompa::Flibrary
{

QString ReadExtractDir(long long bookId);

QString PrepareReaderFile(
	const QString&               extractedPath,
	long long                    bookId              = 0,
	const ISettings*             settings            = nullptr,
	const Util::Fb2ToEpubOptions* epubOptions        = nullptr
);

} // namespace HomeCompa::Flibrary
