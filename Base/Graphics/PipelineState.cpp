#include "PipelineState.h"
#include "MeshShaderPractice/Base/Util/Logger.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"

template<typename InnerStructType, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename DefaultArg = InnerStructType>
class alignas(void*) PSSubObject
{
public:
    PSSubObject() noexcept
        : m_Type(Type)
        , m_Inner(DefaultArg())
    {
    }

    PSSubObject(InnerStructType const& value) noexcept
        : m_Type(Type)
        , m_Inner(value)
    {
    }

    PSSubObject& operator = (InnerStructType const& value) noexcept
    {
        m_Type = Type;
        m_Inner = value;
        return *this;
    }

    operator InnerStructType const& () const noexcept
    {
        return m_Inner;
    }

    operator InnerStructType& () noexcept
    {
        return m_Inner;
    }

    InnerStructType* operator&() noexcept
    {
        return &m_Inner;
    }

    InnerStructType const* operator&() const noexcept
    {
        return &m_Inner;
    }

private:
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_Type;
    InnerStructType                     m_Inner;
};

using PSS_ROOT_SIGNATURE = PSSubObject< ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE >;
using PSS_AS = PSSubObject< D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS >;
using PSS_MS = PSSubObject< D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS >;
using PSS_PS = PSSubObject< D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS >;
using PSS_BLEND = PSSubObject< D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND >;
using PSS_SAMPLE_MASK = PSSubObject< UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK >;
using PSS_RASTERIZER = PSSubObject< D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER >;
using PSS_DEPTH_STENCIL = PSSubObject< D3D12_DEPTH_STENCIL_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL >;
using PSS_RTV_FORMATS = PSSubObject< D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS >;
using PSS_DSV_FORMAT = PSSubObject< DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT >;
using PSS_SAMPLE_DESC = PSSubObject< DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC >;
using PSS_NODE_MASK = PSSubObject< UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK >;
using PSS_CACHED_PSO = PSSubObject< D3D12_CACHED_PIPELINE_STATE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO >;
using PSS_FLAGS = PSSubObject< D3D12_PIPELINE_STATE_FLAGS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS >;

// MeshShader—p‚ÌPS
struct MsPsoDesc
{
    PSS_ROOT_SIGNATURE      RootSignature;
    PSS_AS                  AS;
    PSS_MS                  MS;
    PSS_PS                  PS;
    PSS_BLEND               BlendState;
    PSS_SAMPLE_MASK         SampleMask;
    PSS_RASTERIZER          RasterizerState;
    PSS_DEPTH_STENCIL       DepthStencilState;
    PSS_RTV_FORMATS         RTVFormats;
    PSS_DSV_FORMAT          DSVFormat;
    PSS_SAMPLE_DESC         SampleDesc;
    PSS_NODE_MASK           NodeMask;
    PSS_CACHED_PSO          CachedPSO;
    PSS_FLAGS               Flags;

    MsPsoDesc() {}

    MsPsoDesc(const MESH_SHADER_PIPELINE_STATE_DESC* value)
    {
        RootSignature = value->pRootSignature;
        AS = value->AS;
        MS = value->MS;
        PS = value->PS;
        BlendState = value->BlendState;
        SampleMask = value->SampleMask;
        RasterizerState = value->RasterizerState;
        DepthStencilState = value->DepthStencilState;
        RTVFormats = value->RTVFormats;
        DSVFormat = value->DSVFormat;
        SampleDesc = value->SampleDesc;
        NodeMask = value->NodeMask;
        CachedPSO = value->CachedPSO;
        Flags = value->Flags;
    }
};

MeshShaderPipelineState::MeshShaderPipelineState()
{
}

MeshShaderPipelineState::~MeshShaderPipelineState()
{
    Terminate();
}

bool MeshShaderPipelineState::Initialize(const MESH_SHADER_PIPELINE_STATE_DESC* desc)
{
    auto device = GraphicsProxy::GetD3D12Device();
    HRESULT hr = S_OK;

    // shader model check
    {
        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_5 };
        hr = device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
        if (FAILED(hr) || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_5))
        {
            ELOGA("Error: Shader Model 6.5 not Supported.");
            return false;
        }
    }

    // Mesh Shader Support Check
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
        hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features));
        if (FAILED(hr) || (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED))
        {
            ELOGA("Error: Mesh Shader not Supported.");
            return false;
        }
    }

    m_desc = *desc;

    // Shader Compile
    ForceReload();
    m_dirty = false;

    MsPsoDesc msPsoDesc{ &m_desc };

    D3D12_PIPELINE_STATE_STREAM_DESC pssDesc = {};
    pssDesc.pPipelineStateSubobjectStream = &msPsoDesc;
    pssDesc.SizeInBytes = sizeof(msPsoDesc);

    // Create Pipeline State
    hr = device->CreatePipelineState(&pssDesc, IID_PPV_ARGS(m_state.GetAddressOf()));
    if (FAILED(hr))
    {
        ELOGA("Error: ID3D12Device::CreatePipelineState() Failed. errcode = 0x%x", hr);
        return false;
    }

    return true;
}

void MeshShaderPipelineState::Terminate()
{
    m_reloadState.Reset();
    m_state.Reset();
}

void MeshShaderPipelineState::OnReload(const std::wstring& fullpath)
{
    if (fullpath.empty()) { return; }

    ComPtr<IDxcBlob> errBlob;
    if (m_as.m_path == fullpath)
    {
        if (GraphicsProxy::CompileShader(m_as.m_path, L"as_6_5", m_as.m_blob, errBlob))
        {
            m_desc.AS.pShaderBytecode = m_as.m_blob->GetBufferPointer();
            m_desc.AS.BytecodeLength = m_as.m_blob->GetBufferSize();
            m_dirty = true;
        }
    }

    if (m_ms.m_path == fullpath)
    {
        if (GraphicsProxy::CompileShader(m_ms.m_path, L"ms_6_5", m_ms.m_blob, errBlob))
        {
            m_desc.MS.pShaderBytecode = m_ms.m_blob->GetBufferPointer();
            m_desc.MS.BytecodeLength = m_ms.m_blob->GetBufferSize();
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
}

void MeshShaderPipelineState::ForceReload()
{
    bool isChanged = false;

    ComPtr<IDxcBlob> errBlob;
    if (GraphicsProxy::CompileShader(m_as.m_path, L"as_6_5", m_as.m_blob, errBlob))
    {
        m_desc.AS.pShaderBytecode = m_as.m_blob->GetBufferPointer();
        m_desc.AS.BytecodeLength = m_as.m_blob->GetBufferSize();
        isChanged = true;
    }

    if (GraphicsProxy::CompileShader(m_ms.m_path, L"ms_6_5", m_ms.m_blob, errBlob))
    {
        m_desc.MS.pShaderBytecode = m_ms.m_blob->GetBufferPointer();
        m_desc.MS.BytecodeLength = m_ms.m_blob->GetBufferSize();
        isChanged = true;
    }

    if (GraphicsProxy::CompileShader(m_ps.m_path, L"ps_6_5", m_ps.m_blob, errBlob))
    {
        m_desc.PS.pShaderBytecode = m_ps.m_blob->GetBufferPointer();
        m_desc.PS.BytecodeLength = m_ps.m_blob->GetBufferSize();
        isChanged = true;
    }

    if (isChanged)
    {
        m_dirty = true;
    }
}

void MeshShaderPipelineState::SetState(ID3D12GraphicsCommandList* command)
{
    if (m_dirty) { Recreate(); }

    if (m_reloadState.Get() != nullptr)
    {
        command->SetPipelineState(m_reloadState.Get());
        return;
    }

    command->SetPipelineState(m_state.Get());
}

void MeshShaderPipelineState::SetReloadPathAS(const std::wstring& value)
{
    m_as.m_path = value;
}

void MeshShaderPipelineState::SetReloadPathMS(const std::wstring& value)
{
    m_ms.m_path = value;
}

void MeshShaderPipelineState::SetReloadPathPS(const std::wstring& value)
{
    m_ps.m_path = value;
}

ID3D12PipelineState* MeshShaderPipelineState::Get() const
{
    return m_state.Get();
}

void MeshShaderPipelineState::Recreate()
{
    if (!m_dirty) { return; }

    auto device = GraphicsProxy::GetD3D12Device();
    HRESULT hr = S_OK;

    MsPsoDesc msPsoDesc{ &m_desc };

    D3D12_PIPELINE_STATE_STREAM_DESC pssDesc = {};
    pssDesc.SizeInBytes = sizeof(msPsoDesc);
    pssDesc.pPipelineStateSubobjectStream = &msPsoDesc;

    // Create Pipeline State
    ID3D12PipelineState* pipelineState = nullptr;
    hr = device->CreatePipelineState(&pssDesc, IID_PPV_ARGS(m_state.GetAddressOf()));
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
