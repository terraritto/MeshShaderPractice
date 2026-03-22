#include "Timer.h"

Timer::Timer()
	: m_startTime(0)
	, m_elapsedTime(0)
{
	LARGE_INTEGER qwTicksPerSec = { 0 };
	QueryPerformanceFrequency(&qwTicksPerSec);

	m_ticksPerSec = qwTicksPerSec.QuadPart;
	m_invTicksPerSec = 1.0 / static_cast<double>(m_ticksPerSec);
}

void Timer::Start()
{
	LARGE_INTEGER qwTime = { 0 };
	QueryPerformanceCounter(&qwTime);

	// initialize time
	m_startTime = qwTime.QuadPart;
	m_elapsedTime = qwTime.QuadPart;
}

double Timer::GetRelativeTime()
{
	LARGE_INTEGER qwTime = { 0 };
	QueryPerformanceCounter(&qwTime);

	return (qwTime.QuadPart - m_startTime) * m_invTicksPerSec;
}

double Timer::GetElapsedSec()
{
	LARGE_INTEGER qwTime = { 0 };
	QueryPerformanceCounter(&qwTime);

	auto elapsedTime = (qwTime.QuadPart - m_elapsedTime) * m_invTicksPerSec;
	m_elapsedTime = qwTime.QuadPart;

	// clamp
	if (elapsedTime < 0)
	{
		elapsedTime = 0.0;
	}

	return elapsedTime;
}
