#include "FunctorExecutionForwarder.h"

#include <QCoreApplication>
#include <QTimer>

namespace HomeCompa::Util
{

void FunctorExecutionForwarder::Forward(FunctorType f) const
{
	if (auto* app = QCoreApplication::instance())
		QTimer::singleShot(0, app, std::move(f));
	else
		f();
}

} // namespace HomeCompa::Util
