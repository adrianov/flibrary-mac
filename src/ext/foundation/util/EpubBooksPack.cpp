// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// EPUB repackaging for Apple Books (macOS), used in FLibrary.
//
#include "EpubBooksPack.h"

#include <QFile>

#include "EpubZipPack.h"

namespace HomeCompa::Util
{

#ifndef Q_OS_MACOS

bool IsBooksCompatibleEpub(const QString&)
{
	return true;
}

bool RepackEpubForBooks(const QString& inputPath, const QString& outputPath)
{
	return QFile::exists(inputPath) && QFile::remove(outputPath) && QFile::copy(inputPath, outputPath);
}

bool RepackEpubBytesForBooks(const QByteArray& epubBytes, const QString& outputPath)
{
	QFile output(outputPath);
	return output.open(QIODevice::WriteOnly | QIODevice::Truncate) && output.write(epubBytes) == epubBytes.size();
}

#else

bool IsBooksCompatibleEpub(const QString& epubPath)
{
	QFile file(epubPath);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	return IsBooksCompatibleEpubBytes(file.readAll());
}

bool RepackEpubForBooks(const QString& inputPath, const QString& outputPath)
{
	if (inputPath.isEmpty() || outputPath.isEmpty())
		return false;

	QFile input(inputPath);
	if (!input.open(QIODevice::ReadOnly))
		return false;

	return RepackEpubBytesForBooks(input.readAll(), outputPath);
}

bool RepackEpubBytesForBooks(const QByteArray& epubBytes, const QString& outputPath)
{
	if (epubBytes.isEmpty() || outputPath.isEmpty())
		return false;

	const auto members = ReadEpubMembers(epubBytes);
	if (members.empty())
		return false;

	const auto packed = RepackEpubMembersForBooks(members);
	if (packed.isEmpty())
		return false;

	QFile output(outputPath);
	return output.open(QIODevice::WriteOnly | QIODevice::Truncate) && output.write(packed) == packed.size();
}

#endif

} // namespace HomeCompa::Util
