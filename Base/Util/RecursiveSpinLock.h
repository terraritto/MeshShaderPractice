#pragma once
#include <atomic>

class RecursiveSpinLock
{
public:
	void Lock();
	void Unlock();
	bool TryLock();

private:
	bool Acquire(size_t hashId);

private:
	std::atomic<uint32_t> m_state = {};
	std::atomic<size_t> m_owner = {};
	uint64_t m_counter = 0;
};