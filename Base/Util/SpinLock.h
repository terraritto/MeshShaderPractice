#pragma once
#include <atomic>

class SpinLock
{
public:
	void Lock();
	void Unlock();
	bool TryLock();

private:
	std::atomic<uint32_t> m_state = {};
};