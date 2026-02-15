#include "DescriptorHeap.h"
#include <cassert>
#include "MeshShaderPractice/Base/Util/Logger.h"
#include "MeshShaderPractice/Base/Util/ScopedLock.h"

DescriptorHeap::DescriptorHeap()
	: m_heap(nullptr)
	, m_incrementSize(0)
{
}

DescriptorHeap::~DescriptorHeap()
{
	Terminate();
}

bool DescriptorHeap::Initialize(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC* desc)
{
	if (device == nullptr || desc == nullptr)
	{
		return false;
	}

	HRESULT hr = device->CreateDescriptorHeap(desc, IID_PPV_ARGS(&m_heap));
	if (FAILED(hr))
	{
		ELOGA("Error: ID3D12Device::CreateDescriptorHeap() Failed. errcode = 0x%x", hr);
		return false;
	}

	// Init Allocator
	m_allocator.Initialize(desc->NumDescriptors, desc->NumDescriptors);

	// Get increment size
	m_incrementSize = device->GetDescriptorHandleIncrementSize(desc->Type);

	m_disposeList.resize(DEFAULT_FRAME_COUNT);

	return true;
}

void DescriptorHeap::Terminate()
{ 
	{
		ScopedLock<SpinLock> lock{ m_spinLock };

		// free Offset Handle
		for (std::list<OffsetHandle>& list : m_disposeList)
		{
			auto iter = list.begin();
			while (iter != list.end())
			{
				auto handle = *iter;
				if (handle.IsValid())
				{
					m_allocator.Free(handle);
				}
				iter = list.erase(iter);
			}
			list.clear();
		}
	}

	m_allocator.Terminate();
	
	if (m_heap != nullptr)
	{
		m_heap->Release();
		m_heap = nullptr;
	}
}

OffsetHandle DescriptorHeap::Allocate(uint32_t count)
{
	ScopedLock<SpinLock> lock{ m_spinLock };
	return m_allocator.Allocate(count);
}

void DescriptorHeap::Free(OffsetHandle& handle)
{
	if (!handle.IsValid()) { return; }

	ScopedLock<SpinLock> lock{ m_spinLock };
	m_disposeList.front().push_back(handle);
	handle = OffsetHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetHandleCPU(const OffsetHandle& handle, uint32_t offset) const
{
	assert(offset < handle.GetSize());
	D3D12_CPU_DESCRIPTOR_HANDLE result = m_heap->GetCPUDescriptorHandleForHeapStart();
	result.ptr += SIZE_T((handle.GetOffset() + offset) * m_incrementSize);
	return result;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetHandleGPU(const OffsetHandle& handle, uint32_t offset) const
{
	assert(offset < handle.GetSize());
	D3D12_GPU_DESCRIPTOR_HANDLE result = m_heap->GetGPUDescriptorHandleForHeapStart();
	result.ptr += SIZE_T((handle.GetOffset() + offset) * m_incrementSize);
	return result;
}

uint32_t DescriptorHeap::GetUsedCount() const
{
	return m_allocator.GetUsedSize();
}

uint32_t DescriptorHeap::GetFreeCount() const
{
	return m_allocator.GetFreeSize();
}

void DescriptorHeap::FrameSync()
{
	ScopedLock<SpinLock> lock{ m_spinLock };

	// rotate to move from head to tail.
	std::rotate(m_disposeList.begin(), (++m_disposeList.begin()), m_disposeList.end());

	// 1 process free 1 list.
	std::list<OffsetHandle>& list = m_disposeList.front();
	auto iter = list.begin();
	while (iter != list.end())
	{
		auto handle = *iter;
		if (handle.IsValid())
		{
			m_allocator.Free(handle);
			iter = list.erase(iter);
		}
	}
	list.clear();
}
