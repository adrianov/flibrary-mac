// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// FB2/EPUB conversion (macOS), used in FLibrary.
//

#pragma once

#include "export/util.h"

#include <QByteArray>
#include <QString>

namespace HomeCompa
{

class ISettings;

}

namespace HomeCompa::Util
{

struct EpubCover
{
	QByteArray data;
	QString    mime;
};

struct Fb2ToEpubOptions
{
	QString              archiveFolder;
	QString              bookFile;
	const ISettings*     settings { nullptr };
	const EpubCover*     coverOverride { nullptr };
};

[[nodiscard]] UTIL_EXPORT bool ConvertFb2ToEpub(const QString& fb2Path, const QString& epubPath, const Fb2ToEpubOptions* options = nullptr);

[[nodiscard]] UTIL_EXPORT bool ConvertFb2BytesToEpub(
	const QByteArray&      fb2Bytes,
	const QString&         fb2FileName,
	const QString&         epubPath,
	const Fb2ToEpubOptions* options = nullptr
);

[[nodiscard]] UTIL_EXPORT QString EpubPathForFb2(const QString& fb2Path);

} // namespace HomeCompa::Util
