#include "SimpleMeshShaderApp.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"

bool SimpleMeshShaderApp::OnInitialize()
{
    HRESULT hr = S_OK;

    auto device = GraphicsProxy::GetD3D12Device();

    m_graphicsQueue = GraphicsProxy::GetGraphicsQueue();

    // RootSignature
    {
        // Root Param
        std::vector<D3D12_ROOT_PARAMETER> params;
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_MESH, 0, 0 });
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_MESH, 1, 0 });

        // Root signature
        D3D12_ROOT_SIGNATURE_DESC desc;
        desc.NumParameters = params.size();
        desc.pParameters = params.data();
        desc.NumStaticSamplers = 0;
        desc.pStaticSamplers = nullptr;
        desc.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        if (m_rootSignature.Initialize(device, &desc) == false)
        {
            return false;
        }
    }

    // PSO
    {
        // RasterizerState
        D3D12_RASTERIZER_DESC rsDesc;
        rsDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rsDesc.CullMode = D3D12_CULL_MODE_NONE;
        rsDesc.FrontCounterClockwise = FALSE;
        rsDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rsDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rsDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rsDesc.DepthClipEnable = TRUE;
        rsDesc.MultisampleEnable = FALSE;
        rsDesc.AntialiasedLineEnable = FALSE;
        rsDesc.ForcedSampleCount = 0;
        rsDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // RT BlendState
        D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc =
        {
            FALSE, FALSE,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL
        };

        // BlendState
        D3D12_BLEND_DESC blendDesc;
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        {
            blendDesc.RenderTarget[i] = rtBlendDesc;
        }

        // Shader
        m_pso.SetReloadPathPS(L"Sample_001/Resource/Shader/SimplePS.hlsl");
        m_pso.SetReloadPathMS(L"Sample_001/Resource/Shader/SimpleMS.hlsl");

        // Depth Stencil Operate
        D3D12_DEPTH_STENCILOP_DESC stencilOpDesc = {};
        stencilOpDesc.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        stencilOpDesc.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        stencilOpDesc.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        stencilOpDesc.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

        // Depth Stencil
        D3D12_DEPTH_STENCIL_DESC stencilDesc = {};
        stencilDesc.DepthEnable = FALSE;
        stencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        stencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        stencilDesc.StencilEnable = FALSE;
        stencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        stencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        stencilDesc.FrontFace = stencilOpDesc;
        stencilDesc.BackFace = stencilOpDesc;

        // Render Target Format
        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0] = m_swapChainFormat;

        // Sample
        DXGI_SAMPLE_DESC sampleDesc;
        sampleDesc.Count = 1;
        sampleDesc.Quality = 0;

        // Pipeline State
        MESH_SHADER_PIPELINE_STATE_DESC psDesc = {};
        psDesc.pRootSignature = m_rootSignature.Get();
        psDesc.BlendState = blendDesc;
        psDesc.RasterizerState = rsDesc;
        psDesc.DepthStencilState = stencilDesc;
        psDesc.SampleMask = UINT_MAX;
        psDesc.RTVFormats = rtvFormats;
        psDesc.DSVFormat = m_depthStencilFormat;
        psDesc.SampleDesc = sampleDesc;
        psDesc.NodeMask = 0;
        psDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        if (!m_pso.Initialize(&psDesc))
        {
            return false;
        }
    }

    // Vertex Buffer
    {
        // Vertex
        struct Vertex
        {
            XMFLOAT3 Position;
            XMFLOAT4 Color;
        };

        Vertex vertices[] =
        {
            { {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        m_vertexBuffer.Initialize(sizeof(vertices), sizeof(Vertex), true);
        // Mapping
        void* data = m_vertexBuffer.Map();
        memcpy(data, vertices, sizeof(vertices));
        m_vertexBuffer.UnMap();
    }

    // Index Buffer
    {

        uint32_t indices[] =
        {
            0,1,2
        };

        m_indexBuffer.Initialize(sizeof(indices), true);
        void* data = m_indexBuffer.Map();
        memcpy(data, indices, sizeof(indices));
        m_indexBuffer.UnMap();
    }

    return true;
}

bool SimpleMeshShaderApp::OnTerminate()
{
    m_vertexBuffer.Terminate();
    m_indexBuffer.Terminate();

    m_pso.Terminate();
    m_rootSignature.Terminate();

    return true;
}

void SimpleMeshShaderApp::OnRender()
{
    auto graphicsQueue = m_graphicsQueue.lock();
    if (!graphicsQueue) { return; }

    uint32_t index = GetCurrentBackBufferIndex();

    m_graphicsCommandList.Reset();
    auto command = m_graphicsCommandList.GetD3D12CommandList();

    // set RootSignature/PSO
    command->SetGraphicsRootSignature(m_rootSignature.Get());
    command->SetPipelineState(m_pso.Get());

    // set descriptor heap table
    command->SetGraphicsRootShaderResourceView(0, m_vertexBuffer.GetGpuAddress());
    command->SetGraphicsRootShaderResourceView(1, m_indexBuffer.GetGpuAddress());

    // set viewport
    command->RSSetViewports(1, &m_viewport);

    // set scissor
    command->RSSetScissorRects(1, &m_scissorRect);

    // set resource barrier : Present -> RenderTarget
    m_colorTarget[index].ChangeState(command, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Get handle
    auto handleRTV = m_colorTarget[index].GetCpuHandleRTV();
    auto handleDSV = m_depthTarget.GetCpuHandleDSV();

    // set RenderTarget
    command->OMSetRenderTargets(1, &handleRTV, FALSE, &handleDSV);

    // Clear RenderTarget
    const FLOAT clearColor[] = { 0.39f, 0.58f, 0.92f, 1.0f };
    command->ClearRenderTargetView(handleRTV, m_clearColor, 0, nullptr);

    // Clear Depth Stencil View
    command->ClearDepthStencilView(handleDSV, D3D12_CLEAR_FLAG_DEPTH, m_clearDepth, m_clearStencil, 0, nullptr);

    // Draw
    command->DispatchMesh(1, 1, 1);

    // set resource barrier : RenderTarget -> Present
    m_colorTarget[index].ChangeState(command, D3D12_RESOURCE_STATE_PRESENT);

    // finish
    command->Close();

    // if it doesn't finish previous frame Command Execute, wait for finished.
    if (m_frameWaitPoint.IsValid())
    {
        graphicsQueue->Sync(m_frameWaitPoint);
    }

    // Execute
    ID3D12CommandList* commandLists[] = { command };
    graphicsQueue->Execute(1, commandLists);

    // create wait point
    m_frameWaitPoint = graphicsQueue->Signal();

    // display
    Present(0);

    // wait to finish command complete
    GraphicsProxy::FrameSync();
}
