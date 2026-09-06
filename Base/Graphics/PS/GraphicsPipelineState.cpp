#include "GraphicsPipelineState.h"
#include "MeshShaderPractice/Base/Util/Logger.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"


GraphicsPipelineState::GraphicsPipelineState()
{
}

GraphicsPipelineState::~GraphicsPipelineState()
{
    Terminate();
}

bool GraphicsPipelineState::Initialize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc)
{
    auto device = GraphicsProxy::GetD3D12Device();

    m_desc = *desc;

    // Shader Compile
    ForceReload();
    m_dirty = false;

    // Create Pipeline State
    HRESULT hr = device->CreateGraphicsPipelineState(&m_desc, IID_PPV_ARGS(m_state.GetAddressOf()));
    if (FAILED(hr))
    {
        ELOGA("Error: ID3D12Device::CreateGraphicsPipelineState() Failed. errcode = 0x%x", hr);
        return false;
    }

    return true;
}

void GraphicsPipelineState::Terminate()
{
    m_reloadState.Reset();
    m_state.Reset();
}

void GraphicsPipelineState::OnReload(const std::wstring& fullpath)
{
    if (fullpath.empty()) { return; }

    ComPtr<IDxcBlob> errBlob;
    if (m_vs.m_path == fullpath)
    {
        if (GraphicsProxy::CompileShader(m_vs.m_path, L"vs_6_5", m_vs.m_blob, errBlob))
        {
            m_desc.VS.pShaderBytecode = m_vs.m_blob->GetBufferPointer();
            m_desc.VS.BytecodeLength = m_vs.m_blob->GetBufferSize();
            m_dirty = true;
        }
    }

    if (m_ps.m_path == fullpath)
    {
        if (GraphicsProxy::CompileShader(m_ps.m_path, L"ps_6_5", m_ps.m_blob, errBlob))
        {
            m_desc.PS.pShaderBytecode = m_ps.m_blob->GetBufferPointer();
            m_desc.PS.BytecodeLength = m_ps.m_blob->GetBufferSize();
            m_dirty = true;
        }
    }

    if (m_hs.m_path == fullpath)
    {
        if (GraphicsProxy::CompileShader(m_hs.m_path, L"hs_6_5", m_hs.m_blob, errBlob))
        {
            m_desc.HS.pShaderBytecode = m_hs.m_blob->GetBufferPointer();
            m_desc.HS.BytecodeLength = m_hs.m_blob->GetBufferSize();
            m_dirty = true;
        }
    }

    if (m_ds.m_path == fullpath)
    {
        if (GraphicsProxy::CompileShader(m_ds.m_path, L"ds_6_5", m_ds.m_blob, errBlob))
        {
            m_desc.DS.pShaderBytecode = m_ds.m_blob->GetBufferPointer();
            m_desc.DS.BytecodeLength = m_ds.m_blob->GetBufferSize();
            m_dirty = true;
        }
    }

    if (m_gs.m_path == fullpath)
    {
        if (GraphicsProxy::CompileShader(m_gs.m_path, L"gs_6_5", m_gs.m_blob, errBlob))
        {
            m_desc.GS.pShaderBytecode = m_gs.m_blob->GetBufferPointer();
            m_desc.GS.BytecodeLength = m_gs.m_blob->GetBufferSize();
            m_dirty = true;
        }
    }
}

void GraphicsPipelineState::ForceReload()
{
    bool isChanged = false;

    ComPtr<IDxcBlob> errBlob;
    if (GraphicsProxy::CompileShader(m_vs.m_path, L"vs_6_5", m_vs.m_blob, errBlob))
    {
        m_desc.VS.pShaderBytecode = m_vs.m_blob->GetBufferPointer();
        m_desc.VS.BytecodeLength = m_vs.m_blob->GetBufferSize();
        isChanged = true;
    }

    if (GraphicsProxy::CompileShader(m_ps.m_path, L"ps_6_5", m_ps.m_blob, errBlob))
    {
        m_desc.PS.pShaderBytecode = m_ps.m_blob->GetBufferPointer();
        m_desc.PS.BytecodeLength = m_ps.m_blob->GetBufferSize();
        isChanged = true;
    }

    if (GraphicsProxy::CompileShader(m_hs.m_path, L"hs_6_5", m_hs.m_blob, errBlob))
    {
        m_desc.HS.pShaderBytecode = m_hs.m_blob->GetBufferPointer();
        m_desc.HS.BytecodeLength = m_hs.m_blob->GetBufferSize();
        isChanged = true;
    }

    if (GraphicsProxy::CompileShader(m_ds.m_path, L"ds_6_5", m_ds.m_blob, errBlob))
    {
        m_desc.DS.pShaderBytecode = m_ds.m_blob->GetBufferPointer();
        m_desc.DS.BytecodeLength = m_ds.m_blob->GetBufferSize();
        isChanged = true;
    }

    if (GraphicsProxy::CompileShader(m_gs.m_path, L"gs_6_5", m_gs.m_blob, errBlob))
    {
        m_desc.GS.pShaderBytecode = m_gs.m_blob->GetBufferPointer();
        m_desc.GS.BytecodeLength = m_gs.m_blob->GetBufferSize();
        isChanged = true;
    }

    if (isChanged)
    {
        m_dirty = true;
    }
}

void GraphicsPipelineState::SetState(ID3D12GraphicsCommandList* command)
{
    if (command == nullptr) { return; }

    if (m_dirty) { Recreate(); }

    if (m_reloadState.Get() != nullptr)
    {
        command->SetPipelineState(m_reloadState.Get());
        return;
    }

    command->SetPipelineState(m_state.Get());
}

void GraphicsPipelineState::SetReloadPathVS(const std::wstring& value)
{
    m_vs.m_path = value;
}

void GraphicsPipelineState::SetReloadPathPS(const std::wstring& value)
{
    m_ps.m_path = value;
}

void GraphicsPipelineState::SetReloadPathHS(const std::wstring& value)
{
    m_hs.m_path = value;
}

void GraphicsPipelineState::SetReloadPathDS(const std::wstring& value)
{
    m_ds.m_path = value;
}

void GraphicsPipelineState::SetReloadPathGS(const std::wstring& value)
{
    m_gs.m_path = value;
}

ID3D12PipelineState* GraphicsPipelineState::Get() const
{
    return m_state.Get();
}

void GraphicsPipelineState::Recreate()
{
    if (!m_dirty) { return; }

    auto device = GraphicsProxy::GetD3D12Device();
    HRESULT hr = S_OK;

    // Create Pipeline State
    ID3D12PipelineState* pipelineState = nullptr;
    hr = device->CreateGraphicsPipelineState(&m_desc, IID_PPV_ARGS(m_state.GetAddressOf()));
    if (FAILED(hr))
    {
        ELOGA("Error: ID3D12Device::CreatePipelineState() Failed. errcode = 0x%x", hr);
        m_dirty = false;
        return;
    }

    // todo: shader compile

    auto pso = m_reloadState.Detach();
    GraphicsProxy::Dispose(pso);

    m_reloadState.Attach(pipelineState);

    m_dirty = false;
}
