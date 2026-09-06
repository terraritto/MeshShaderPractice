#pragma once
#include "MeshShaderPractice/Base/Util.h"

class GraphicsPipelineState
{
public:
	GraphicsPipelineState();
	~GraphicsPipelineState();

	bool Initialize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);
	void Terminate();

	void OnReload(const std::wstring& fullpath);
	void ForceReload();

	// Setter
	void SetState(ID3D12GraphicsCommandList* command);
	void SetReloadPathVS(const std::wstring& value);
	void SetReloadPathPS(const std::wstring& value);
	void SetReloadPathHS(const std::wstring& value);
	void SetReloadPathDS(const std::wstring& value);
	void SetReloadPathGS(const std::wstring& value);

	// Getter
	ID3D12PipelineState* Get() const;

private:
	void Recreate();

private:
	ComPtr<ID3D12PipelineState> m_state;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_desc = {};
	ComPtr<ID3D12PipelineState> m_reloadState;
	ShaderInfo m_vs;
	ShaderInfo m_ps;
	ShaderInfo m_hs;
	ShaderInfo m_ds;
	ShaderInfo m_gs;
	bool m_dirty = false;
};
