#include "ReaderExtract.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "platform/FileUtil.h"
#include "util/ImageRestore.h"

#include "log.h"
#include "zip.h"

using namespace HomeCompa;

namespace HomeCompa::Flibrary
{

namespace
{

constexpr auto READER_KEY = "Reader/%1";

QByteArray ReadBookBytes(QIODevice& stream, const QString& archive, const QString& entryName, const ISettings& settings)
{
	if (entryName.endsWith(".epub", Qt::CaseInsensitive))
		return stream.readAll();

	return Util::PrepareToExport(stream, archive, entryName, settings);
}

} // namespace

void ExtractBookForReading(
	const ISettings&                            settings,
	const ILogicFactory::ITemporaryDir&         temporaryDir,
	const QString&                                archive,
	QString&                                    fileName,
	QString&                                    error,
	const std::shared_ptr<const ILogicFactory>& logicFactory
)
{
	try
	{
		const Zip  zip(archive);
		const auto stream       = zip.Read(fileName);
		const auto settingsStub = logicFactory->CreateSettingsStub();

		if (!fileName.endsWith(".epub", Qt::CaseInsensitive) && Zip::IsArchive(Platform::RemoveIllegalPathCharacters(fileName)))
		{
			const Zip   subZip(stream->GetStream());
			const auto  fileList = subZip.GetFileNameList();
			QStringList filesWithReader;
			for (const auto& archiveFileName : fileList)
			{
				if (auto ext = QFileInfo(archiveFileName).suffix(); !ext.isEmpty())
				{
					auto key = QString(READER_KEY).arg(ext);
					if (auto reader = settings.Get(key).toString(); !reader.isEmpty())
						filesWithReader << archiveFileName;
				}

				auto       fileNameDst = temporaryDir.filePath(archiveFileName);
				const auto dir         = QFileInfo(fileNameDst).dir();
				if (!dir.exists() && !dir.mkpath("."))
				{
					PLOGE << "Cannot create " << dir.dirName();
					continue;
				}

				if (QFile file(fileNameDst); file.open(QIODevice::WriteOnly))
				{
					const auto subStream = subZip.Read(archiveFileName);
					file.write(ReadBookBytes(subStream->GetStream(), archive, archiveFileName, *settingsStub));
				}
			}

			if (fileList.size() == 1)
				fileName = temporaryDir.filePath(fileList.front());
			else if (filesWithReader.size() == 1)
				fileName = temporaryDir.filePath(filesWithReader.front());
			else
				fileName.clear();
		}
		else
		{
			auto fileNameDst = temporaryDir.filePath(Platform::RemoveIllegalPathCharacters(fileName));
			if (QFile file(fileNameDst); file.open(QIODevice::WriteOnly))
				file.write(ReadBookBytes(stream->GetStream(), archive, fileName, *settingsStub));

			fileName = std::move(fileNameDst);
		}
	}
	catch (const std::exception& ex)
	{
		error = ex.what();
	}
}

} // namespace HomeCompa::Flibrary
