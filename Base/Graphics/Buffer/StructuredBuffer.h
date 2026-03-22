#pragma once
#include <d3d12.h>
#include <stdint.h>
#include "MeshShaderPractice/Base/Util.h"
#include "MeshShaderPractice/Base/Graphics/Holder/AllocationHolder.h"
#include "MeshShaderPractice/Base/Graphics/Holder/DescriptorHolder.h"

class StructuredBuffer
{
public:
	StructuredBuffer();
	~StructuredBuffer();

	bool Initialize(uint64_t count, uint32_t stride, D3D12_RESOURCE_STATES state);
	bool Initialize(ID3D12GraphicsCommandList* command, uint64_t count, uint32_t stride, const void* initData);
	void Terminate();

	void UAVBarrier(ID3D12GraphicsCommandList* command);
	void ChangeState(ID3D12GraphicsCommandList* command, D3D12_RESOURCE_STATES state);

	// Setter
	void SetDebugName(LPCWSTR tag);

	// Getter
	ID3D12Resource* GetResource() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const;
	D3D12_RESOURCE_STATES GetState() const;

private:
	ComPtr<ID3D12Resource> m_resource;
	AllocationHolder m_holder;
	D3D12_RESOURCE_STATES m_state;
};
