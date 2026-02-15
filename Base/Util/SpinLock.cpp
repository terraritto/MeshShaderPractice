#include "SpinLock.h"

void SpinLock::Lock()
{
	// acquire lock
	uint32_t old = 0;
	while (!m_state.compare_exchange_weak(old, 1u, std::memory_order_acquire))
	{
		_mm_pause();
	}
}

void SpinLock::Unlock()
{
	// unlock for released acquired lock resource.
	m_state.store(0, std::memory_order_release);
}

bool SpinLock::TryLock()
{
	uint32_t old = 0;
	return m_state.compare_exchange_weak(old, 1u);
}