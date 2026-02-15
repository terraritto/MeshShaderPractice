#pragma once
#include <d3d12.h>
#include <memory>
#include <vector>
#include <list>
#include "MeshShaderPractice/Base/Util/ThreadSafeOffsetAllocator.h"

class Descriptor;

class DescriptorHeap : public std::enable_shared_from_this<DescriptorHeap>
{
public:
	DescriptorHeap();
	~DescriptorHeap();

	// Alloc/Free
	OffsetHandle Allocate(uint32_t count);
	void Free(OffsetHandle& handle);

	// Getter
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU(const OffsetHandle& handle, uint32_t offset = 0) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU(const OffsetHandle& handle, uint32_t offset = 0) const;

	uint32_t GetUsedCount() const;
	uint32_t GetFreeCount() const;

	// Synchronize GPU
	void FrameSync();

private:
	bool Initialize(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC* desc);
	void Terminate();

private:
	static constexpr uint32_t DEFAULT_FRAME_COUNT = 4;

private:
	ID3D12DescriptorHeap* m_heap;
	ThreadSafeOffsetAllocator m_allocator;
	SpinLock m_spinLock;
	std::vector<std::list<OffsetHandle>> m_disposeList;
	uint32_t m_incrementSize;

private:
	friend class GraphicsDevice;
};