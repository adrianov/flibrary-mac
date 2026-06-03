#include "ReaderOpenPath.h"

#include <QFileInfo>

#ifdef Q_OS_MACOS
#include "util/Fb2EpubConverter.h"
#endif

namespace HomeCompa::Flibrary
{

namespace
{

bool IsFb2Path(const QString& path)
{
	const auto ext = QFileInfo(path).suffix();
	return ext.compare(QStringLiteral("fb2"), Qt::CaseInsensitive) == 0 || ext.compare(QStringLiteral("fbd"), Qt::CaseInsensitive) == 0;
}

} // namespace

QString PrepareReaderFile(const QString& extractedPath)
{
#ifdef Q_OS_MACOS
	if (!IsFb2Path(extractedPath))
		return extractedPath;

	const auto epubPath = Util::EpubPathForFb2(extractedPath);
	if (Util::ConvertFb2ToEpub(extractedPath, epubPath))
		return epubPath;
#endif

	return extractedPath;
}

} // namespace HomeCompa::Flibrary
