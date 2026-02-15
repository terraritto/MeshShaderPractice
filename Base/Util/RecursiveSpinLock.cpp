#include "RecursiveSpinLock.h"
#include <cassert>
#include <thread>

void RecursiveSpinLock::Lock()
{
	std::hash<std::thread::id> hasher;
	size_t hashId = hasher(std::this_thread::get_id());

	while (!Acquire(hashId))
	{
		_mm_pause();
	}

	assert(m_counter + 1 <= UINT64_MAX);
	m_counter++;
}

void RecursiveSpinLock::Unlock()
{
	std::hash<std::thread::id> hasher;
	size_t hashId = hasher(std::this_thread::get_id());
	assert(m_owner == hashId);

	// decrement recursive count.
	m_counter--;

	// if zero, unlock.
	if (m_counter == 0)
	{
		size_t defaultId = 0;
		m_owner.store(defaultId, std::memory_order_relaxed);
		m_state.store(0, std::memory_order_release);
	}
}

bool RecursiveSpinLock::TryLock()
{
	std::hash<std::thread::id> hasher;
	size_t hashId = hasher(std::this_thread::get_id());

	// if locked before, same owner thread only can lock 
	auto lockHashId = m_owner.load();
	if (lockHashId > 0 && m_counter > 0 && hashId != lockHashId)
	{
		return false; // failed.
	}

	if (Acquire(hashId))
	{
		assert(m_counter + 1 <= UINT64_MAX);
		m_counter++;
		return true; // lock
	}

	return false; // failed
}
bool RecursiveSpinLock::Acquire(size_t hashId)
{
	// get flag and set flag
	uint32_t old = 0;
	m_state.compare_exchange_weak(old, 1u, std::memory_order_acquire);

	// if lock isn't get, set owner.
	if (old == 0)
	{
		assert(m_counter == 0); // must not set
		m_owner.store(hashId, std::memory_order_relaxed);
		return true; // lock
	}

	// get lock and same thread, recursive count increment and finish.
	if (old == 1u && m_owner.load(std::memory_order_relaxed) == hashId)
	{
		assert(m_counter > 0);
		return true; // lock success.
	}

	return false; // failed
}
