#include "NativeEventFilter.h"

using namespace HomeCompa::Platform;

class NativeEventFilter::Impl
{
};

NativeEventFilter::NativeEventFilter(QCoreApplication& /*app*/)
{
}

NativeEventFilter::~NativeEventFilter() = default;

void NativeEventFilter::Register(IObserver* /*observer*/)
{
}

void NativeEventFilter::Unregister(IObserver* /*observer*/)
{
}
