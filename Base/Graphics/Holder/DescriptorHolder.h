#pragma once
#include <d3d12.h>
#include "MeshShaderPractice/Base/Util/OffsetAllocator.h"

class DescriptorHolder
{
public:
	enum HEAP_TYPE
	{
		HEAP_NONE,
		HEAP_RTV,		// Render Target View
		HEAP_DSV,		// Depth Stencil View
		HEAP_RES,		// CBV/SRV/UAV
		HEAP_SMP		// Sampler
	};

public:
	DescriptorHolder();
	explicit DescriptorHolder(HEAP_TYPE heapType, OffsetHandle handle);
	~DescriptorHolder();

	void Reset();

	// Getter
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const;
	uint32_t GetIndex() const;
	bool IsValid() const;

private:
	HEAP_TYPE m_heapType;
	OffsetHandle m_handle;
};