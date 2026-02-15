#pragma once
#include <atomic>
#include <memory>
#include "SpinLock.h"

template<class T>
class ScopedLock
{
public:
	ScopedLock(T& value)
		: m_lock(value)
	{
		m_lock.Lock();
	}

	~ScopedLock()
	{
		m_lock.Unlock();
	}

private:
	T& m_lock;
};