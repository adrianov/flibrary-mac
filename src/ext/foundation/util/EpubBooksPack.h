// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// EPUB repackaging for Apple Books (macOS), used in FLibrary.
//
#pragma once

#include "export/util.h"

#include <QByteArray>
#include <QString>

namespace HomeCompa::Util
{

[[nodiscard]] UTIL_EXPORT bool IsBooksCompatibleEpub(const QString& epubPath);

[[nodiscard]] UTIL_EXPORT bool RepackEpubForBooks(const QString& inputPath, const QString& outputPath);

[[nodiscard]] UTIL_EXPORT bool RepackEpubBytesForBooks(const QByteArray& epubBytes, const QString& outputPath);

} // namespace HomeCompa::Util
