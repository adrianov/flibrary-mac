#include "ProgressController.h"

#include <atomic>
#include <memory>

#include "fnd/observable.h"

#include "interface/logic/IProgressController.h"

#include "settings/UiTimer.h"
#include "util/FunctorExecutionForwarder.h"

#include "log.h"

using namespace HomeCompa;
using namespace Flibrary;

namespace
{

class ProgressItem final : public IProgressController::IProgressItem
{
	NON_COPY_MOVABLE(ProgressItem)
public:
	class IObserver // NOLINT(cppcoreguidelines-special-member-functions)
	{
	public:
		virtual ~IObserver()                      = default;
		virtual void OnIncremented(int64_t value) = 0;
		virtual void OnDestroyed()                = 0;
	};

public:
	ProgressItem(std::atomic_bool& stopped, IObserver& observer, const int64_t maximum)
		: m_stopped(stopped)
		, m_observer(observer)
		, m_maximum(maximum)
	{
	}

	~ProgressItem() override
	{
		m_observer.OnIncremented(m_maximum - m_value);
		m_observer.OnDestroyed();
	}

private: // IProgressController::IProgressItem
	void Increment(int64_t value) override
	{
		value    = std::min(value, m_maximum - m_value);
		m_value += value;
		if (value)
			m_observer.OnIncremented(value);
	}

	bool IsStopped() const noexcept override
	{
		return m_stopped;
	}

private:
	std::atomic_bool& m_stopped;
	IObserver&        m_observer;
	const int64_t     m_maximum;
	int64_t           m_value { 0 };
};

} // namespace

struct ProgressController::Impl final
	: std::enable_shared_from_this<Impl>
	, Observable<IProgressController::IObserver>
	, ProgressItem::IObserver
{
	std::atomic_bool                stopped { false };
	std::atomic_int64_t             globalMaximum { 0 };
	std::atomic_int64_t             count { 0 };
	std::atomic_int64_t             globalValue { 0 };
	Util::FunctorExecutionForwarder forwarder;

	void NotifyMainThread(std::function<void(Impl&)> action)
	{
		const auto weak = weak_from_this();
		forwarder.Forward([weak, action = std::move(action)] {
			if (const auto self = weak.lock())
				action(*self);
		});
	}

	void OnIncremented(const int64_t value) override
	{
		if (stopped.load(std::memory_order_relaxed))
			return;

		globalValue.fetch_add(value, std::memory_order_relaxed);
		NotifyMainThread([](Impl& self) {
			if (!self.stopped.load(std::memory_order_relaxed))
				self.Perform(&IProgressController::IObserver::OnValueChanged);
		});
	}

	void OnDestroyed() override
	{
		if (count.fetch_sub(1, std::memory_order_acq_rel) != 1)
			return;

		NotifyMainThread([](Impl& self) {
			self.globalMaximum.store(0, std::memory_order_relaxed);
			self.globalValue.store(0, std::memory_order_relaxed);
			self.Perform(&IProgressController::IObserver::OnStartedChanged);
		});
	}
};

ProgressController::ProgressController()
	: m_impl { std::make_shared<Impl>() }
{
	PLOGV << "ProgressController created";
}

ProgressController::~ProgressController()
{
	PLOGV << "ProgressController destroyed";
}

bool ProgressController::IsStarted() const noexcept
{
	return m_impl->count.load(std::memory_order_relaxed) != 0;
}

void ProgressController::RegisterObserver(IObserver* observer)
{
	m_impl->Register(observer);
}

void ProgressController::UnregisterObserver(IObserver* observer)
{
	m_impl->Unregister(observer);
}

std::unique_ptr<IProgressController::IProgressItem> ProgressController::Add(const int64_t value)
{
	const auto justStarted = m_impl->count.load(std::memory_order_relaxed) == 0;
	m_impl->count.fetch_add(1, std::memory_order_relaxed);
	m_impl->globalMaximum.fetch_add(value, std::memory_order_relaxed);
	if (justStarted)
	{
		m_impl->stopped.store(false, std::memory_order_relaxed);
		m_impl->NotifyMainThread([](Impl& self) {
			self.Perform(&IProgressController::IObserver::OnStartedChanged);
		});
	}
	return std::make_unique<ProgressItem>(m_impl->stopped, *m_impl, value);
}

double ProgressController::GetValue() const noexcept
{
	const auto maximum = m_impl->globalMaximum.load(std::memory_order_relaxed);
	if (maximum == 0)
		return 0;

	return static_cast<double>(m_impl->globalValue.load(std::memory_order_relaxed)) / static_cast<double>(maximum);
}

void ProgressController::Stop()
{
	m_impl->stopped.store(true, std::memory_order_relaxed);
	m_impl->Perform(&IProgressController::IObserver::OnStop);
}
