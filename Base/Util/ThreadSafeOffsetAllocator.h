#pragma once
#include "MeshShaderPractice/Base/Util/OffsetAllocator.h"
#include "MeshShaderPractice/Base/Util/SpinLock.h"

class ThreadSafeOffsetAllocator
{
public:
	ThreadSafeOffsetAllocator() = default;

	void Initialize(uint32_t size, uint32_t maxAllocatableCount = 128 * 1024);
	void Terminate();
	void Reset();

	// Alloc/Free
	OffsetHandle Allocate(uint32_t size);
	OffsetHandle Allocate(uint32_t size, uint32_t alignment);
	void Free(OffsetHandle& handle);

	uint32_t GetUsedSize() const;
	uint32_t GetFreeSize() const;

private:
	SpinLock m_lock;
	OffsetAllocator m_allocator;
};