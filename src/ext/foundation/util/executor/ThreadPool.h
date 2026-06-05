#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <ranges>
#include <thread>

namespace HomeCompa::Util
{

template <typename T = size_t>
class ThreadPool
{
public:
	struct Initializer
	{
		unsigned int             threadCount { std::thread::hardware_concurrency() };
		size_t                   maxQueueSize { std::numeric_limits<size_t>::max() };
		std::function<T(size_t)> contextGetter { [](size_t) {
			return T {};
		} };
	};

public:
	explicit ThreadPool(const Initializer& initializer = {})
		: m_maxQueueSize { initializer.maxQueueSize }
	{
		m_contexts.reserve(initializer.threadCount);
		std::ranges::transform(std::views::iota(decltype(initializer.threadCount) { 0 }, initializer.threadCount), std::back_inserter(m_contexts), [&](const auto n) {
			return initializer.contextGetter(n);
		});
		m_threads.reserve(initializer.threadCount);
		std::ranges::transform(std::views::iota(decltype(initializer.threadCount) { 0 }, initializer.threadCount), std::back_inserter(m_threads), [&](const auto n) {
			return std::jthread(std::bind_front(&ThreadPool::work, this, std::ref(m_contexts[n])));
		});
	}

	void enqueue(std::function<void(T&)> task)
	{
		{
			std::unique_lock lock(m_tasksGuard);
			m_condition.wait(lock, [this] {
				return m_tasks.size() < m_maxQueueSize;
			});
			m_tasks.emplace(std::move(task));
		}

		m_condition.notify_one();
	}

	std::vector<T> wait()
	{
		{
			std::unique_lock lock(m_tasksGuard);
			m_condition.wait(lock, [this] {
				return m_tasks.empty();
			});
		}

		m_threads.clear();

		auto contexts = std::move(m_contexts);
		return contexts;
	}

private:
	std::function<void(T&)> getTask(const std::stop_token& stop)
	{
		auto task = getTaskImpl(stop);
		m_condition.notify_all();

		return task;
	}

	std::function<void(T&)> getTaskImpl(const std::stop_token& stop)
	{
		std::unique_lock lock(m_tasksGuard);
		if (!m_condition.wait(lock, stop, [this] {
				return !m_tasks.empty();
			}))
			return {};

		auto task = std::move(m_tasks.front());
		m_tasks.pop();

		return task;
	}

	void work(T& t, const std::stop_token stop)
	{
		while (!stop.stop_requested())
			if (const auto task = getTask(stop))
				task(t);
	}

private:
	const size_t                        m_maxQueueSize;
	std::queue<std::function<void(T&)>> m_tasks;
	std::mutex                          m_tasksGuard;
	std::condition_variable_any         m_condition;
	std::vector<T>                      m_contexts;
	std::vector<std::jthread>           m_threads;
};

} // namespace HomeCompa::Util
