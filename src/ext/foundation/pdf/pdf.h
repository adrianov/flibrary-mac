#pragma once

#include "export/flipdf.h"

class QByteArray;
class QIODevice;

namespace HomeCompa::Pdf
{

FLIPDF_EXPORT QByteArray GetCover(QIODevice& stream);

}
