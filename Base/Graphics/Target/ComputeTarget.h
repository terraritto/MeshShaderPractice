#pragma once
#include <dxgi1_6.h>
#include "MeshShaderPractice/Base/Graphics/Holder/AllocationHolder.h"
#include "MeshShaderPractice/Base/Graphics/Holder/DescriptorHolder.h"
#include "MeshShaderPractice/Base/Graphics/Target/ColorTarget.h"
#include "MeshShaderPractice/Base/Graphics/Target/TargetDesc.h"
#include "MeshShaderPractice/Base/Util.h"

class ComputeTarget
{
public:
	ComputeTarget();
	~ComputeTarget();

	bool Initialize(const TargetDesc* desc, uint32_t stride = 0);
	bool Initialize(ColorTarget& target);
	void Terminate();

	bool Resize(uint32_t width, uint32_t height = 1, uint16_t depth = 1);

	void ChangeState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES nextState);

	// Setter
	void SetName(LPCWSTR tag);
	void UAVBarrier(ID3D12GraphicsCommandList* commandList);

	// Getter
	ID3D12Resource* GetResource() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleUAV() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleUAV() const;
	uint32_t GetBindlessIndexUAV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleSRV() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleSRV() const;
	uint32_t GetBindlessIndexSRV() const;
	TargetDesc GetDesc() const;
	uint32_t GetStride() const;
	D3D12_RESOURCE_STATES GetState() const;

private:
	ComPtr<ID3D12Resource> m_resource;
	AllocationHolder m_allocationHolder;
	DescriptorHolder m_uavHolder;
	DescriptorHolder m_srvHolder;
	TargetDesc m_desc;
	D3D12_RESOURCE_STATES m_prevState = D3D12_RESOURCE_STATE_COMMON;
	uint32_t m_stride = 0;
};