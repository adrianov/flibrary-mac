#include "NativeEventFilter.h"

#include <Windows.h>

#include <iomanip>

#include <QAbstractNativeEventFilter>
#include <QCoreApplication>

#include "fnd/observable.h"

#include "QtTypes.h"
#include "log.h"

using namespace HomeCompa::Platform;

class NativeEventFilter::Impl final
	: QAbstractNativeEventFilter
	, public Observable<IObserver>
{
	NON_COPY_MOVABLE(Impl)

public:
	explicit Impl(QCoreApplication& app)
		: m_app { app }
	{
		m_app.installNativeEventFilter(this);
	}

	~Impl() override
	{
		m_app.removeNativeEventFilter(this);
	}

private: // QAbstractNativeEventFilter
	bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr_t* result) override
	{
		const auto* msg = static_cast<MSG*>(message);
		if (eventType != "windows_generic_MSG" || msg->message != WM_QUERYENDSESSION)
			return false;

		//		PLOGI << "WM_QUERYENDSESSION: " << "0x" << std::setfill('0') << std::setw(sizeof(msg->lParam) * 2) << std::hex << msg->lParam;

		Perform(&IObserver::OnQueryEndSession, result);

		return false;
	}

private:
	QCoreApplication& m_app;
};

NativeEventFilter::NativeEventFilter(QCoreApplication& app)
	: m_impl(app)
{
}

NativeEventFilter::~NativeEventFilter() = default;

void NativeEventFilter::Register(IObserver* observer)
{
	m_impl->Register(observer);
}

void NativeEventFilter::Unregister(IObserver* observer)
{
	m_impl->Unregister(observer);
}
