#pragma once

#include "util/Fb2EpubParser.h"

#include <QByteArray>
#include <QString>

namespace Fb2EpubTest
{

[[nodiscard]] bool EpubHasMember(const QString& epubPath, const QString& member);
[[nodiscard]] QByteArray ReadEpubMember(const QString& epubPath, const QString& member);

[[nodiscard]] bool CheckParsed(const HomeCompa::Util::ParsedFb2& parsed);
[[nodiscard]] bool CheckArchiveFileName();
[[nodiscard]] bool CheckArchiveEpubExport(const QString& archivePath, const QString& bookFile, const QString& epubPath);
[[nodiscard]] bool CheckGluedWords();
[[nodiscard]] bool CheckWarningSpacing(const QString& fb2Path);
[[nodiscard]] bool CheckEmphasisParenSpacing(const QString& fb2Path);
[[nodiscard]] bool CheckFootnoteAfterSentence(const QString& fb2Path);
[[nodiscard]] bool CheckFootnotePunctSpacing(const QString& fb2Path);
[[nodiscard]] bool CheckInlineWordBoundary(const QString& fb2Path);
[[nodiscard]] bool CheckFootnoteEmphasis(const QString& fb2Path);
[[nodiscard]] bool CheckDropCap(const QString& fb2Path);
[[nodiscard]] bool CheckInlineImage(const QString& fb2Path);
[[nodiscard]] bool CheckSpecialTitle(const QString& fb2Path);
[[nodiscard]] bool CheckCoverOverride(const QString& fb2Path);
[[nodiscard]] bool CheckPoem(const QString& fb2Path);
[[nodiscard]] bool CheckEpubRepack();

} // namespace Fb2EpubTest
