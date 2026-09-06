#pragma once
#include <d3d12.h>
#include "MeshShaderPractice/Base/Graphics/Buffer/ConstantBuffer.h"
#include "MeshShaderPractice/Base/Graphics/Holder/AllocationHolder.h"
#include "MeshShaderPractice/Base/Graphics/RootSignature.h"
#include "MeshShaderPractice/Base/Graphics/PS/GraphicsPipelineState.h"

class ShapeStates
{
public:
	ShapeStates();
	~ShapeStates();

	bool Initialize(DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat);
	void Terminate();

	// setter
	void SetViewProj(const XMMATRIX& view, const XMMATRIX& proj);
	
	// getter
	D3D12_GPU_VIRTUAL_ADDRESS GetBufferAddress() const;

	// Change State
	void ApplyOpaqueState(ID3D12GraphicsCommandList* command);
	void ApplyTranslucentState(ID3D12GraphicsCommandList* command);
	void ApplyWireframeState(ID3D12GraphicsCommandList* command);

public:
	static const uint32_t CBV0 = 0;
	static const uint32_t CBV1 = 1;
	static const uint32_t CBV2 = 2;
	static const uint32_t CONSTANTS3 = 3;
	static const uint32_t SRV0 = 4;
	static const uint32_t SRV1 = 5;

private:
	// D3D
	RootSignature m_rootSignature;
	GraphicsPipelineState m_opaqueState;
	GraphicsPipelineState m_translucentState;
	GraphicsPipelineState m_wireframeState;
	
	// Camera
	ConstantBuffer m_cameraBuffer;
	uint8_t m_bufferIndex = 0;

	// Matrix
	XMMATRIX m_view;
	XMMATRIX m_proj;
};