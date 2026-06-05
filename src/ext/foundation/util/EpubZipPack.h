// Copyright (c) 2026 Peter Adrianov
// SPDX-License-Identifier: MIT
// In-memory EPUB ZIP packing (macOS), used in FLibrary.
//
#pragma once

#include "export/util.h"

#include <QByteArray>
#include <QString>
#include <utility>
#include <vector>

namespace HomeCompa::Util
{

using EpubMember = std::pair<QString, QByteArray>;

[[nodiscard]] UTIL_EXPORT bool IsBooksCompatibleEpubBytes(const QByteArray& epubBytes);

[[nodiscard]] UTIL_EXPORT std::vector<EpubMember> ReadEpubMembers(const QByteArray& epubBytes);

[[nodiscard]] UTIL_EXPORT QByteArray RepackEpubMembersForBooks(const std::vector<EpubMember>& members);

} // namespace HomeCompa::Util
