#include "BooksExtractorWrite.h"

#include <filesystem>

#include <QBuffer>
#include <QFileInfo>
#include <QTemporaryDir>

#include "interface/logic/ILogicFactory.h"
#include "interface/constants/SettingsConstant.h"

#include "Constant.h"
#include "platform/StrUtil.h"
#include "util/ImageRestore.h"

#include "log.h"
#include "zip.h"

#ifdef Q_OS_MACOS
#include "util/EpubBooksPack.h"
#include "util/Fb2EpubConverter.h"
#include "util/Fb2Format.h"
#endif

using namespace HomeCompa::Flibrary;
using namespace HomeCompa;

namespace
{

bool Write(const QByteArray& input, const std::filesystem::path& path)
{
	QFile output(Platform::PathToString(path));
	return output.open(QIODevice::WriteOnly) && output.write(input) == input.size();
}

bool Archive(const QByteArray& input, const std::filesystem::path& path, QString fileName, std::shared_ptr<Zip::ProgressCallback> zipProgressCallback)
{
	try
	{
		Zip  zip(Platform::PathToString(path), Zip::Format::Zip, false, std::move(zipProgressCallback));
		auto zipFiles = Zip::CreateZipFileController();
		zipFiles->AddFile(std::move(fileName), input, QDateTime::currentDateTime());
		zip.Write(*zipFiles);
		return true;
	}
	catch (const std::exception& ex)
	{
		PLOGE << ex.what();
	}
	return false;
}

bool Unpack(QByteArray& input, const std::filesystem::path& path)
{
	try
	{
		QBuffer buffer(&input);
		buffer.open(QIODevice::ReadOnly);
		Zip zip(buffer);

		std::filesystem::path folder = path;
		folder.replace_extension("");
		std::error_code ec;
		if (!create_directories(folder, ec))
		{
			PLOGE << ec.message() << " (" << ec.value() << ")";
			return false;
		}

		return std::ranges::all_of(zip.GetFileNameList(), [&](const auto& fileName) {
			const auto fullPath = folder / Platform::StringToPath(fileName);
			std::filesystem::create_directories(fullPath.parent_path());
			QFile      output(Platform::PathToString(fullPath));
			const auto fileSize = zip.GetFileSize(fileName);
			return output.open(QIODevice::WriteOnly) && output.write(zip.Read(fileName)->GetStream().readAll()) == static_cast<qint64>(fileSize);
		});
	}
	catch (const std::exception& ex)
	{
		PLOGE << ex.what();
	}

	return Write(input, path);
}

std::pair<bool, std::filesystem::path> WriteFile(
	const ISettings&                       settings,
	QIODevice&                             input,
	const QString&                         folder,
	const Util::ExtractedBook&             book,
	IProgressController::IProgressItem&    progress,
	std::shared_ptr<Zip::ProgressCallback> zipProgressCallback,
	BooksExtractorWrite::IExportHelper&    exportHelper,
	const BooksExtractorWrite::WriteMode   mode
)
{
	auto            result = std::make_pair(false, std::filesystem::path {});
	const QFileInfo dstFileInfo(book.dstFileName);
	if (const auto dstDir = dstFileInfo.absolutePath(); !(QDir().exists(dstDir) || QDir().mkpath(dstDir)))
		return result;

	result.second = Platform::StringToPath(QDir::toNativeSeparators(book.dstFileName));

	exportHelper.CheckPath(result.second);

	if (exists(result.second))
		if (!remove(result.second))
			return assert(false), result;

	result.first = [&] {
		auto bytes = PrepareToExport(input, folder, book.file, settings, exportHelper.GetMetadataReplacement(book));
		switch (mode)
		{
			case BooksExtractorWrite::WriteMode::AsIs:
				return Write(bytes, result.second);
			case BooksExtractorWrite::WriteMode::Archive:
				return Archive(bytes, result.second, dstFileInfo.completeBaseName() + "." + QFileInfo(book.file).suffix(), std::move(zipProgressCallback));
			case BooksExtractorWrite::WriteMode::Unpack:
				return Unpack(bytes, result.second);
			case BooksExtractorWrite::WriteMode::Epub:
#ifdef Q_OS_MACOS
				if (Util::IsFb2Suffix(QFileInfo(book.file).suffix()))
				{
					const Util::Fb2ToEpubOptions options { .archiveFolder = folder, .bookFile = book.file, .settings = &settings };
					return Util::ConvertFb2BytesToEpub(bytes, book.file, Platform::PathToString(result.second), &options);
				}
				if (Util::IsEpubSuffix(QFileInfo(book.file).suffix()))
					return Util::RepackEpubBytesForBooks(bytes, Platform::PathToString(result.second));
				return false;
#else
				return false;
#endif
			default: // NOLINT(clang-diagnostic-covered-switch-default)
				return assert(false && "unexpected mode"), false;
		}
	}();
	progress.Increment(1);

	return result;
}

} // namespace

namespace HomeCompa::Flibrary::BooksExtractorWrite
{

std::filesystem::path Process(
	const ISettings&                       settings,
	const std::filesystem::path&           archiveFolder,
	const Util::ExtractedBook&             book,
	IProgressController::IProgressItem&    progress,
	std::shared_ptr<Zip::ProgressCallback> zipProgressCallback,
	IExportHelper&                         exportHelper,
	const WriteMode                        mode
)
{
	if (progress.IsStopped())
		return {};

	const auto folder = QDir::fromNativeSeparators(Platform::PathToString(archiveFolder / Platform::StringToPath(book.folder)));
	if (!QFile::exists(folder))
		throw std::runtime_error((folder + ": archive not found").toStdString());

	const Zip  zip(folder);
	const auto stream = zip.Read(book.file);
	auto [ok, path]   = WriteFile(settings, stream->GetStream(), folder, book, progress, std::move(zipProgressCallback), exportHelper, mode);
	if (!ok && exists(path))
		remove(path);

	return ok ? path : std::filesystem::path {};
}

void Process(
	const ISettings&                    settings,
	const std::filesystem::path&        archiveFolder,
	const QString&                      dstFolder,
	DB::IDatabase&                      db,
	const Util::ExtractedBook&          book,
	IProgressController::IProgressItem& progress,
	IExportHelper&                      exportHelper,
	const IScriptController&            scriptController,
	IScriptController::Commands         commands
)
{
	const auto needFile   = std::ranges::any_of(commands, [](const auto& command) {
		return IScriptController::HasMacro(command.args, IScriptController::Macro::SourceFile);
	});
	const auto sourceFile = needFile ? Process(settings, archiveFolder, book, progress, {}, exportHelper, WriteMode::AsIs) : std::filesystem::path {};

	std::ranges::sort(commands, {}, [](const IScriptController::Command& command) {
		return command.number;
	});
	for (auto command : commands)
	{
		if (needFile)
			IScriptController::SetMacro(command.args, IScriptController::Macro::SourceFile, QDir::toNativeSeparators(Platform::PathToString(sourceFile)));
		IScriptController::SetMacro(command.args, IScriptController::Macro::UserDestinationFolder, QDir::toNativeSeparators(dstFolder));
		ILogicFactory::FillScriptTemplate(db, command.args, book);

		if (progress.IsStopped() || !scriptController.Execute(command))
			return;
	}
}

} // namespace HomeCompa::Flibrary::BooksExtractorWrite
