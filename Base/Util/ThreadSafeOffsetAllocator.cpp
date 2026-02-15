#include "ThreadSafeOffsetAllocator.h"
#include "MeshShaderPractice/Base/Util/ScopedLock.h"

void ThreadSafeOffsetAllocator::Initialize(uint32_t size, uint32_t maxAllocatableCount)
{
	ScopedLock<SpinLock> locker(m_lock);
	m_allocator.Initialize(size, maxAllocatableCount);
}

void ThreadSafeOffsetAllocator::Terminate()
{
	ScopedLock<SpinLock> locker(m_lock);
	m_allocator.Terminate();
}

void ThreadSafeOffsetAllocator::Reset()
{
	ScopedLock<SpinLock> locker(m_lock);
	m_allocator.Reset();
}

OffsetHandle ThreadSafeOffsetAllocator::Allocate(uint32_t size)
{
	ScopedLock<SpinLock> locker(m_lock);
	return m_allocator.Allocate(size);
}

OffsetHandle ThreadSafeOffsetAllocator::Allocate(uint32_t size, uint32_t alignment)
{
	uint32_t alignSize = (size + (alignment - 1)) & ~(alignment - 1);
	return Allocate(alignSize);
}

void ThreadSafeOffsetAllocator::Free(OffsetHandle& handle)
{
	ScopedLock<SpinLock> locker(m_lock);
	m_allocator.Free(handle);
}

uint32_t ThreadSafeOffsetAllocator::GetUsedSize() const
{
	return m_allocator.GetUsedSize();
}

uint32_t ThreadSafeOffsetAllocator::GetFreeSize() const
{
	return m_allocator.GetFreeSize();
}