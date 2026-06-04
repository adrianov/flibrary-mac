#pragma once

#include <QString>

namespace HomeCompa::Flibrary
{

QString ReadExtractDir(long long bookId);

QString PrepareReaderFile(const QString& extractedPath, long long bookId = 0);

} // namespace HomeCompa::Flibrary
