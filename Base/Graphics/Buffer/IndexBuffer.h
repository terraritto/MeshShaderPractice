#pragma once
#include <d3d12.h>
#include <stdint.h>
#include "MeshShaderPractice/Base/Util.h"
#include "MeshShaderPractice/Base/Graphics/Holder/AllocationHolder.h"
#include "MeshShaderPractice/Base/Graphics/Holder/DescriptorHolder.h"

class IndexBuffer
{
public:
	IndexBuffer();
	~IndexBuffer();

	// isShortFormat: if use 16bit format, it is true. if 32bit, false.
	bool Initialize(uint64_t size, bool isSrv = false, bool isShortFormat = false);
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
	D3D12_INDEX_BUFFER_VIEW GetView() const;
	ID3D12Resource* GetResource() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleSRV() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleSRV() const;

private:
	ComPtr<ID3D12Resource> m_resource;
	AllocationHolder m_holder;
	D3D12_INDEX_BUFFER_VIEW m_view;

	// for srv
	DescriptorHolder m_srvHolder;
	bool m_isSrv;
};
