#pragma once
#include <d3d12.h>
#include <cstdint>

struct TargetDesc
{
	D3D12_RESOURCE_DIMENSION m_dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
	uint64_t m_alignment = 0;
	uint64_t m_width = 0;
	uint32_t m_height = 1;
	uint16_t m_depthOrArraySize = 1;
	uint16_t m_mipLevels = 1;
	DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
	DXGI_SAMPLE_DESC m_samplerDesc = { 1,0 };
	D3D12_RESOURCE_STATES m_initState = D3D12_RESOURCE_STATE_COMMON;
	float m_clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float m_clearDepth = 1.0f;
	uint8_t m_clearStencil = 0;
};
