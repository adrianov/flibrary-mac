#include "BookUtil.h"

#include <QFileInfo>

#include "Constant.h"

namespace HomeCompa::Util::Remove
{

AllFiles CollectBookFiles(Books& books, const std::function<std::shared_ptr<Zip::ProgressCallback>()>& progressProvider)
{
	AllFiles result;
	for (auto&& [id, folder, file] : books)
	{
		auto [it, ok]           = result.try_emplace(std::move(folder), AllFilesItem {});
		auto& [files, progress] = it->second;
		files.emplace_back(std::move(file));
		if (ok)
			progress = progressProvider();
	}
	return result;
}

auto GetFolderPath(const QString& collectionFolder, const QString& name)
{
	return QString("%1/%2").arg(collectionFolder, name);
}

AllFiles CollectImageFiles(const AllFiles& bookFiles, const QString& collectionFolder, const std::function<std::shared_ptr<Zip::ProgressCallback>()>& progressProvider)
{
	static constexpr std::pair<const char*, const char*> imageTypes[] {
		{ Global::COVERS,  "" },
		{ Global::IMAGES, "/" },
	};

	AllFiles result;
	for (const auto& [folder, archiveItem] : bookFiles)
	{
		const QFileInfo fileInfo(folder);
		for (const auto& [type, replacedExt] : imageTypes)
		{
			const auto imageFolderName = QString("%1/%2.zip").arg(type, fileInfo.completeBaseName());
			if (QFile::exists(GetFolderPath(collectionFolder, imageFolderName)))
			{
				auto& [files, progress] = result[imageFolderName];
				std::ranges::transform(std::get<0>(archiveItem), std::back_inserter(files), [=](const QString& file) {
					return QFileInfo(file).completeBaseName() + replacedExt;
				});
				progress = progressProvider();
			}
		}
	}

	return result;
}

void RemoveFiles(AllFiles& allFiles, const QString& collectionFolder)
{
	std::map toCleanup(std::make_move_iterator(allFiles.begin()), std::make_move_iterator(allFiles.end()));
	for (auto&& [folder, archiveItem] : toCleanup)
	{
		const auto archive = GetFolderPath(collectionFolder, folder);
		if ([&] {
				auto&& [files, progressCallback] = archiveItem;
				Zip zip(archive, Zip::Format::Auto, true, std::move(progressCallback));
				zip.Remove(files);
				return zip.GetFileNameList().isEmpty();
			}())
		{
			QFile::remove(archive);
		}
	}
}

} // namespace HomeCompa::Util::Remove
