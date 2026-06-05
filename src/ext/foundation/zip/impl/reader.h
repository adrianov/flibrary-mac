#pragma once

#include <memory>

namespace bit7z
{

class BitInputArchive;

}

namespace HomeCompa::ZipDetails
{

class IFile;

}

namespace HomeCompa::ZipDetails::SevenZip
{

struct FileItem;

namespace File
{

std::unique_ptr<IFile> Read(const bit7z::BitInputArchive& zip, const FileItem& fileItem);

};

}
