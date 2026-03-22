#pragma once
#include <d3d12.h>
#include <stdint.h>
#include "MeshShaderPractice/Base/Util.h"
#include "MeshShaderPractice/Base/Graphics/Holder/AllocationHolder.h"
#include "MeshShaderPractice/Base/Graphics/Holder/DescriptorHolder.h"

class ConstantBuffer
{
public:
	ConstantBuffer();
	~ConstantBuffer();

	bool Initialize(uint64_t size);
	void Terminate();

	// memory mapping
	void* Map();
	void UnMap();

	template<class T>
	T* MapAs()
	{
		return static_cast<T*>(Map());
	}

	// Setter
	void SetDebugName(LPCWSTR tag);

	// Getter
	ID3D12Resource* GetResource() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const;
	uint64_t GetSize() const;

private:
	ComPtr<ID3D12Resource> m_resource;
	AllocationHolder m_holder;
	uint64_t m_size = 0;
};