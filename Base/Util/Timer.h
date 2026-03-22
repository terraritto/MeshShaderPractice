#pragma once
#include <cstdint>
#include <Windows.h>
#include <profileapi.h>

class Timer
{
public:
	Timer();

	void Start();

	// Getter
	double GetRelativeTime();
	double GetElapsedSec();

private:
	int64_t m_ticksPerSec; // timer count num per one sec.
	double m_invTicksPerSec;
	int64_t m_startTime;
	int64_t m_elapsedTime;
};