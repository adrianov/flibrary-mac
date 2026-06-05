#pragma once

#include <utility>

#include <QString>

#include "export/util.h"

class QByteArray;
class QImage;
class QPixmap;

namespace HomeCompa::Util
{

inline constexpr auto IMAGE_JPEG = "image/jpeg";
inline constexpr auto IMAGE_PNG  = "image/png";
inline constexpr auto JPEG       = "jpeg";
inline constexpr auto PNG        = "png";

UTIL_EXPORT QImage  HasAlpha(const QImage& image, const char* data);
UTIL_EXPORT QPixmap Decode(const QByteArray& bytes);
UTIL_EXPORT std::pair<QByteArray, const char*> Recode(const QByteArray& bytes);
UTIL_EXPORT std::pair<QByteArray, const char*> Encode(const QImage& image, const QString& format = {});
UTIL_EXPORT bool                               IsImage(const QString& fileName);

}
