#pragma once

#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"
#include "fnd/observer.h"

#include "QtTypes.h"

#include "export/platform.h"

class QCoreApplication;

namespace HomeCompa::Platform
{

class PLATFORM_EXPORT NativeEventFilter
{
	NON_COPY_MOVABLE(NativeEventFilter)

public:
	class IObserver : public Observer
	{
	public:
		virtual void OnQueryEndSession(qintptr_t* result) = 0;
	};

public:
	explicit NativeEventFilter(QCoreApplication& app);
	~NativeEventFilter();

	void Register(IObserver* observer);
	void Unregister(IObserver* observer);

private:
	class Impl;
	PropagateConstPtr<Impl> m_impl;
};

} // namespace HomeCompa::Platform
