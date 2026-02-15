#pragma once
#include <d3d12.h>
#include <string>
#include <vector>
#include "MeshShaderPractice/Base/Util.h"

struct MESH_SHADER_PIPELINE_STATE_DESC
{
	ID3D12RootSignature*			pRootSignature;
	D3D12_SHADER_BYTECODE			AS;
	D3D12_SHADER_BYTECODE			MS;
	D3D12_SHADER_BYTECODE			PS;
	D3D12_BLEND_DESC				BlendState;
	UINT							SampleMask;
	D3D12_RASTERIZER_DESC			RasterizerState;
	D3D12_DEPTH_STENCIL_DESC		DepthStencilState;
	D3D12_RT_FORMAT_ARRAY			RTVFormats;
	DXGI_FORMAT						DSVFormat;
	DXGI_SAMPLE_DESC				SampleDesc;
	UINT							NodeMask;
	D3D12_CACHED_PIPELINE_STATE		CachedPSO;
	D3D12_PIPELINE_STATE_FLAGS		Flags;
};

struct ShaderInfo
{
	std::wstring m_path;
	ComPtr<IDxcBlob> m_blob;
};

class MeshShaderPipelineState
{
public:
	MeshShaderPipelineState();
	~MeshShaderPipelineState();

	bool Initialize(const MESH_SHADER_PIPELINE_STATE_DESC* desc);
	void Terminate();

	void OnReload(const std::wstring& fullpath);
	void ForceReload();

	// Setter
	void SetState(ID3D12GraphicsCommandList* command);
	void SetReloadPathAS(const std::wstring& value);
	void SetReloadPathMS(const std::wstring& value);
	void SetReloadPathPS(const std::wstring& value);

	// Getter
	ID3D12PipelineState* Get() const;

private:
	void Recreate();

private:
	ComPtr<ID3D12PipelineState> m_state;
	MESH_SHADER_PIPELINE_STATE_DESC m_desc = {};
	ComPtr<ID3D12PipelineState> m_reloadState;
	ShaderInfo m_as;
	ShaderInfo m_ms;
	ShaderInfo m_ps;
	bool m_dirty = false;
};
