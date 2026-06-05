#pragma once

#include "fnd/NonCopyMovable.h"

#include "export/flidjvu.h"

class QByteArray;
class QIODevice;

namespace HomeCompa::DjVu
{

FLIDJVU_EXPORT QByteArray GetCover(QIODevice& stream);

class FLIDJVU_EXPORT Initializer
{
	NON_COPY_MOVABLE(Initializer)

public:
	Initializer();
	~Initializer();
};

}
