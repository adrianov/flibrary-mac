#pragma once

#include <memory>

#include <QStringList>

#include "ProgressCallback.h"

class QVariant;

namespace HomeCompa
{

class IZipFileProvider;

}

namespace HomeCompa::ZipDetails
{

class IFile;

class IZip // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	virtual ~IZip()                                                                    = default;
	[[nodiscard]] virtual QStringList      GetFileNameList() const                     = 0;
	[[nodiscard]] virtual size_t           GetFileSize(const QString& filename) const  = 0;
	[[nodiscard]] virtual const QDateTime& GetFileTime(const QString& filename) const  = 0;
	[[nodiscard]] virtual size_t           GetFileIndex(const QString& filename) const = 0;

	[[nodiscard]] virtual std::unique_ptr<IFile>                  Read(const QString& filename) const = 0;
	[[nodiscard]] virtual std::unordered_map<QString, QByteArray> ReadAll() const                     = 0;

	virtual void SetProperty(PropertyId id, QVariant value)     = 0;
	virtual bool Write(const IZipFileProvider& zipFileProvider) = 0;

	virtual bool Remove(const std::vector<QString>& fileNames) = 0;
};

}
