#pragma once

#include <functional>

#include "export/util.h"

namespace HomeCompa::Util
{

class UTIL_EXPORT FunctorExecutionForwarder
{
public:
	using FunctorType = std::function<void()>;
	void Forward(FunctorType f) const;
};

} // namespace HomeCompa::Util
