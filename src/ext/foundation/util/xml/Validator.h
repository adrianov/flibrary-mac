#pragma once

#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"

#include "export/util.h"

class QIODevice;
class QString;

namespace HomeCompa::Util
{

class UTIL_EXPORT XmlValidator
{
	NON_COPY_MOVABLE(XmlValidator)

public:
	XmlValidator();
	~XmlValidator();

public:
	QString Validate(QIODevice& input) const;

private:
	class Impl;
	PropagateConstPtr<Impl> m_impl;
};

}
