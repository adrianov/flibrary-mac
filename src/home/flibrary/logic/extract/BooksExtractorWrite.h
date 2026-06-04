#pragma once

#include <filesystem>
#include <memory>

#include <QString>

#include "database/interface/IDatabase.h"
#include "interface/logic/IProgressController.h"
#include "interface/logic/IScriptController.h"
#include "settings/ISettings.h"
#include "util/BookUtil.h"
#include "zip.h"

namespace HomeCompa::Flibrary::BooksExtractorWrite
{

enum class WriteMode
{
	AsIs,
	Archive,
	Unpack,
	Epub,
};

class IExportHelper // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	virtual ~IExportHelper()                                                                                         = default;
	virtual void                                       CheckPath(std::filesystem::path& path)                        = 0;
	virtual std::unique_ptr<const Util::ExtractedBook> GetMetadataReplacement(const Util::ExtractedBook& book) const = 0;
};

std::filesystem::path Process(
	const ISettings&                       settings,
	const std::filesystem::path&           archiveFolder,
	const Util::ExtractedBook&             book,
	IProgressController::IProgressItem&    progress,
	std::shared_ptr<Zip::ProgressCallback> zipProgressCallback,
	IExportHelper&                         exportHelper,
	WriteMode                              mode
);

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
);

} // namespace HomeCompa::Flibrary::BooksExtractorWrite
