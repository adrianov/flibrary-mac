#include "ReaderOpenPath.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#ifdef Q_OS_MACOS
#include "util/Fb2EpubConverter.h"
#include "util/Fb2Format.h"
#include "zip.h"
#endif

namespace HomeCompa::Flibrary
{

namespace
{

#ifdef Q_OS_MACOS

// Bump when EPUB output format changes so cached read copies are rebuilt.
constexpr auto EPUB_CACHE_VERSION = "v3";

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
	return byContent.isEmpty() ? Util::EpubPathForFb2(extractedPath) : byContent;
}

void RemoveLegacyCache(long long bookId)
{
	if (bookId <= 0)
		return;

	const auto dir = EpubCacheDir();
	for (const auto& suffix : { QString(), QStringLiteral(".v2") })
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
		if (!html.contains("epub:"))
			return true;

		return html.contains("xmlns:epub");
	}
	catch (...)
	{
		return false;
	}
}

#endif

} // namespace

QString PrepareReaderFile(const QString& extractedPath, const long long bookId)
{
#ifdef Q_OS_MACOS
	if (!Util::IsFb2Path(extractedPath))
		return extractedPath;

	RemoveLegacyCache(bookId);

	const auto epubPath = ResolveCachedEpubPath(extractedPath, bookId);
	if (epubPath.isEmpty())
		return extractedPath;

	if (QFile::exists(epubPath))
	{
		if (IsCachedEpubValid(epubPath))
			return epubPath;
		QFile::remove(epubPath);
	}

	if (Util::ConvertFb2ToEpub(extractedPath, epubPath))
		return epubPath;
#else
	(void)bookId;
#endif

	return extractedPath;
}

} // namespace HomeCompa::Flibrary
