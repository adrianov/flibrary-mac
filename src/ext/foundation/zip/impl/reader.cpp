#include "reader.h"

#include <QBuffer>

#include "fnd/ScopedCall.h"

#include "bit7z/bitinputarchive.hpp"
#include "zip/interface/file.h"
#include "zip/interface/stream.h"

#include "FileItem.h"
#include "QtTypes.h"
#include "log.h"

namespace HomeCompa::ZipDetails::SevenZip
{

namespace
{

class StreamImpl final : public Stream
{
public:
	StreamImpl(const bit7z::BitInputArchive& zip, const FileItem& fileItem)
		: m_outStream(&m_bytes)
	{
		const ScopedCall ioDeviceGuard(
			[this] {
				m_outStream.open(QIODevice::WriteOnly);
			},
			[this] {
				m_outStream.close();
			}
		);
		std::vector<std::byte> buffer;
		zip.extractTo(buffer, fileItem.index);
		m_bytes = QByteArray { reinterpret_cast<char*>(buffer.data()), static_cast<qsizetype_t>(buffer.size()) };
	}

private: // Stream
	QIODevice& GetStream() override
	{
		m_buffer = std::make_unique<QBuffer>(&m_bytes);
		m_buffer->open(QIODevice::ReadOnly);
		return *m_buffer;
	}

private:
	QBuffer                  m_outStream;
	QByteArray               m_bytes;
	std::unique_ptr<QBuffer> m_buffer;
};

class FileReader final : virtual public IFile
{
public:
	FileReader(const bit7z::BitInputArchive& zip, const FileItem& fileItem)
		: m_zip { zip }
		, m_fileItem { fileItem }
	{
	}

private: // IFile
	std::unique_ptr<Stream> Read() override
	{
		try
		{
			return std::make_unique<StreamImpl>(m_zip, m_fileItem);
		}
		catch (const std::exception& ex)
		{
			PLOGE << m_fileItem.name << " read error: " << ex.what();
			throw;
		}
		catch (...)
		{
			PLOGE << m_fileItem.name << " read error: unknown error";
			throw;
		}
	}

	std::unique_ptr<Stream> Write() override
	{
		throw std::runtime_error("Cannot write with reader");
	}

private:
	const bit7z::BitInputArchive& m_zip;
	const FileItem&               m_fileItem;
};

} // namespace

namespace File
{

std::unique_ptr<IFile> Read(const bit7z::BitInputArchive& zip, const FileItem& fileItem)
{
	return std::make_unique<FileReader>(zip, fileItem);
}

}

} // namespace HomeCompa::ZipDetails::SevenZip
