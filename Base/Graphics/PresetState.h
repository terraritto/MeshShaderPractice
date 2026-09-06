#pragma once
#include <d3d12.h>

class PresetState
{
public:
	// rasterizer desc
	static const D3D12_RASTERIZER_DESC m_cullNone;
	static const D3D12_RASTERIZER_DESC m_wireFrame;

	// depth stencil op desc
	static const D3D12_DEPTH_STENCILOP_DESC m_stencilDefault;

	// depth stencil desc
	static const D3D12_DEPTH_STENCIL_DESC m_depthReadWrite;
	static const D3D12_DEPTH_STENCIL_DESC m_depthReadOnly;

	// RT blend desc
	static const D3D12_RENDER_TARGET_BLEND_DESC m_rtBlendOpaque;
	static const D3D12_RENDER_TARGET_BLEND_DESC m_rtBlendAlphaBlend;

	// blend desc
	static const D3D12_BLEND_DESC m_opaque;
	static const D3D12_BLEND_DESC m_alphaBlend;

	// static sampler desc
	static const D3D12_STATIC_SAMPLER_DESC m_staticSamplers[11];
};