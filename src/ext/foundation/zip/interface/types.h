#pragma once

#include <qmetatype.h>

namespace HomeCompa::ZipDetails
{

constexpr auto INVALID_INDEX = std::numeric_limits<size_t>::max();

enum class PropertyId
{
	CompressionLevel,
	CompressionMethod,
	SolidArchive,
	ThreadsCount,
};

enum class CompressionLevel
{
	None    = 0, ///< Copy mode (no compression)
	Fastest = 1, ///< Fastest compressing
	Fast    = 3, ///< Fast compressing
	Normal  = 5, ///< Normal compressing
	Max     = 7, ///< Maximum compressing
	Ultra   = 9 ///< Ultra compressing
};

enum class CompressionMethod
{
	Copy,
	Deflate,
	Deflate64,
	BZip2,
	Lzma,
	Lzma2,
	Ppmd
};

}

Q_DECLARE_METATYPE(HomeCompa::ZipDetails::CompressionLevel)
Q_DECLARE_METATYPE(HomeCompa::ZipDetails::CompressionMethod)
