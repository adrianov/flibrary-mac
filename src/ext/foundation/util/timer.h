#pragma once

#include <chrono>

#include "fnd/NonCopyMovable.h"

#include "log.h"

namespace HomeCompa::Util
{

class Timer
{
	NON_COPY_MOVABLE(Timer)

public:
	explicit Timer(QString process)
		: m_t { std::chrono::high_resolution_clock::now() }
		, m_process { std::move(process) }
	{
		PLOGI << m_process << " started";
	}

	~Timer()
	{
		PLOGI << m_process << " done for " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_t).count() << " ms";
	}

private:
	const std::chrono::high_resolution_clock::time_point m_t;
	const QString                                        m_process;
};

}
