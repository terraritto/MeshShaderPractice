#pragma once
#include <dxgi1_6.h>
#include "MeshShaderPractice/Base/Graphics/Holder/AllocationHolder.h"
#include "MeshShaderPractice/Base/Graphics/Holder/DescriptorHolder.h"
#include "MeshShaderPractice/Base/Graphics/Target/TargetDesc.h"
#include "MeshShaderPractice/Base/Util.h"

class ColorTarget
{
public:
	ColorTarget();
	~ColorTarget();

	bool Initialize(const TargetDesc* desc);
	bool Initialize(IDXGISwapChain* swapChain, uint32_t backBufferIndex, bool sRGB);
	void Terminate();

	bool Resize(uint32_t width, uint32_t height);

	void ChangeState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES nextState);

	// Setter
	void SetName(LPCWSTR tag);

	// Getter
	ID3D12Resource* GetResource() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleRTV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleSRV() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleSRV() const;
	uint32_t GetBindlessIndexSRV() const;
	TargetDesc GetDesc() const;
	D3D12_RESOURCE_STATES GetState() const;
	bool IsSRGB() const;

private:
	ComPtr<ID3D12Resource> m_resource;
	AllocationHolder m_allocationHolder;
	DescriptorHolder m_rtvHolder;
	DescriptorHolder m_srvHolder;
	TargetDesc m_desc;
	D3D12_RESOURCE_STATES m_prevState = D3D12_RESOURCE_STATE_COMMON;
};