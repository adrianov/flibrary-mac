#include "ReaderOpenPath.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#ifdef Q_OS_MACOS
#include "util/EpubBooksPack.h"
#include "util/Fb2EpubConverter.h"

#include "settings/ISettings.h"
#include "util/Fb2Format.h"
#include "zip.h"
#endif

namespace HomeCompa::Flibrary
{

QString ReadExtractDir(const long long bookId)
{
	const auto root = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(QStringLiteral("read"));
	const auto dir  = QDir(root).filePath(bookId > 0 ? QString::number(bookId) : QStringLiteral("misc"));
	QDir().mkpath(dir);
	return dir;
}

namespace
{

#ifdef Q_OS_MACOS

// Bump when EPUB output format changes so cached read copies are rebuilt.
constexpr auto EPUB_CACHE_VERSION = "v18";

QString EpubCacheDir()
{
	const auto dir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(QStringLiteral("epub"));
	QDir().mkpath(dir);
	return dir;
}

QString EpubCachePath(long long bookId)
{
	return QDir(EpubCacheDir()).filePath(QString("%1.%2.epub").arg(bookId).arg(EPUB_CACHE_VERSION));
}

QString EpubCachePathByContent(const QString& fb2Path)
{
	QFile file(fb2Path);
	if (!file.open(QIODevice::ReadOnly))
		return {};

	QCryptographicHash hash(QCryptographicHash::Sha256);
	if (!hash.addData(&file))
		return {};

	return QDir(EpubCacheDir()).filePath(QString("%1.%2.epub").arg(QString::fromLatin1(hash.result().toHex())).arg(EPUB_CACHE_VERSION));
}

QString ResolveCachedEpubPath(const QString& extractedPath, long long bookId)
{
	if (bookId > 0)
		return EpubCachePath(bookId);

	const auto byContent = EpubCachePathByContent(extractedPath);
	return byContent.isEmpty() ? QString {} : byContent;
}

bool IsEpubArchive(const QString& epubPath)
{
	try
	{
		const Zip zip(epubPath);
		return !zip.GetFileNameList().isEmpty();
	}
	catch (...)
	{
		return false;
	}
}

QString RepackEpubToCache(const QString& extractedPath, const long long bookId)
{
	const auto epubPath = ResolveCachedEpubPath(extractedPath, bookId);
	if (epubPath.isEmpty())
		return extractedPath;

	const QFileInfo srcInfo(extractedPath);
	const QFileInfo dstInfo(epubPath);
	if (srcInfo.canonicalFilePath() == dstInfo.canonicalFilePath())
		return extractedPath;

	if (!dstInfo.exists() || srcInfo.lastModified() > dstInfo.lastModified() || !Util::IsBooksCompatibleEpub(epubPath))
	{
		QFile::remove(epubPath);
		if (!Util::RepackEpubForBooks(extractedPath, epubPath) || !IsEpubArchive(epubPath))
			return extractedPath;
	}

	return epubPath;
}

void RemoveLegacyCache(long long bookId)
{
	if (bookId <= 0)
		return;

	const auto dir = EpubCacheDir();
	for (const auto& suffix :
	     { QString(), QStringLiteral(".v2"), QStringLiteral(".v3"), QStringLiteral(".v4"), QStringLiteral(".v5"), QStringLiteral(".v6"), QStringLiteral(".v7"), QStringLiteral(".v8"), QStringLiteral(".v9"), QStringLiteral(".v15"), QStringLiteral(".v16"), QStringLiteral(".v17") })
		QFile::remove(QDir(dir).filePath(QString("%1%2.epub").arg(bookId).arg(suffix)));
}

bool IsCachedEpubValid(const QString& epubPath)
{
	try
	{
		const Zip  zip(epubPath);
		const auto stream = zip.Read(QStringLiteral("OEBPS/content.xhtml"));
		if (!stream)
			return false;

		const auto html = stream->GetStream().readAll();
		return html.startsWith("<?xml")
		    && html.contains("xmlns:epub=\"http://www.idpf.org/2007/ops\"");
	}
	catch (...)
	{
		return false;
	}
}

#endif

} // namespace

QString PrepareReaderFile(
	const QString&                extractedPath,
	const long long               bookId,
	const ISettings*              settings,
	const Util::Fb2ToEpubOptions* epubOptions
)
{
#ifdef Q_OS_MACOS
	RemoveLegacyCache(bookId);

	if (Util::IsFb2Path(extractedPath))
	{
		const auto epubPath = [&] {
			if (bookId > 0)
				return EpubCachePath(bookId);

			const auto byContent = EpubCachePathByContent(extractedPath);
			return byContent.isEmpty() ? Util::EpubPathForFb2(extractedPath) : byContent;
		}();
		if (epubPath.isEmpty())
			return extractedPath;

		if (QFile::exists(epubPath))
		{
			if (IsCachedEpubValid(epubPath))
				return epubPath;
			QFile::remove(epubPath);
		}

		Util::Fb2ToEpubOptions options;
		if (epubOptions)
			options = *epubOptions;
		options.settings = settings ? settings : options.settings;

		if (Util::ConvertFb2ToEpub(extractedPath, epubPath, &options))
			return epubPath;

		return extractedPath;
	}

	if (Util::IsEpubPath(extractedPath))
		return RepackEpubToCache(extractedPath, bookId);
#else
	(void)bookId;
	(void)settings;
	(void)epubOptions;
#endif

	return extractedPath;
}

} // namespace HomeCompa::Flibrary
