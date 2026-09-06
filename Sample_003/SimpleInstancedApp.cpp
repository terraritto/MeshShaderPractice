#include "SimpleInstancedApp.h"
#include "MeshShaderPractice/Base/App/ImguiManager.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

#include "MeshShaderPractice/External/Imgui/imgui.h"
#include "MeshShaderPractice/External/Imgui/imgui_impl_win32.h"
#include "MeshShaderPractice/External/Imgui/imgui_impl_dx12.h"

bool SimpleInstancedApp::OnInitialize()
{
    HRESULT hr = S_OK;

    auto device = GraphicsProxy::GetD3D12Device();

    m_graphicsQueue = GraphicsProxy::GetGraphicsQueue();

    // RootSignature
    {
        // Root Param
        std::vector<D3D12_ROOT_PARAMETER> params;
        params.push_back(PARAM_CONSTANT{ D3D12_SHADER_VISIBILITY_ALL, 18, 0, 0}); // 18 = (sizeof(XMMATRIX) + sizeof(UINT) * 2) / 4
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_MESH, 0, 0 });
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_MESH, 1, 0 });
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_MESH, 2, 0 });
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_MESH, 3, 0 });
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_MESH, 4, 0 });

        // Root signature
        D3D12_ROOT_SIGNATURE_DESC desc;
        desc.NumParameters = static_cast<UINT>(params.size());
        desc.pParameters = params.data();
        desc.NumStaticSamplers = 0;
        desc.pStaticSamplers = nullptr;
        desc.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
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
        rsDesc.CullMode = D3D12_CULL_MODE_BACK;
        rsDesc.FrontCounterClockwise = TRUE;
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
        m_pso.SetReloadPathAS(L"Sample_003/Resource/Shader/SimpleAS.hlsl");
        m_pso.SetReloadPathPS(L"Sample_003/Resource/Shader/SimplePS.hlsl");
        m_pso.SetReloadPathMS(L"Sample_003/Resource/Shader/SimpleMS.hlsl");

        // Depth Stencil Operate
        D3D12_DEPTH_STENCILOP_DESC stencilOpDesc = {};
        stencilOpDesc.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        stencilOpDesc.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        stencilOpDesc.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        stencilOpDesc.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

        // Depth Stencil
        D3D12_DEPTH_STENCIL_DESC stencilDesc = {};
        stencilDesc.DepthEnable = TRUE;
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

    m_graphicsCommandList.Reset();
    auto command = m_graphicsCommandList.GetD3D12CommandList();

    // setup model
    //std::string path = "Sample_003/Resource/Model/icon_no_aitsu_2026.obj";
    std::string path = "Sample_003/Resource/Model/bunny.obj";
    if (!m_model.Initialize(command, path))
    {
        ELOGA("Error : Model::Initialize() Failed.");
        return false;
    }

    // setup instances
    {
        m_instances.resize(InstanceColsNum * InstanceRowsNum);

        // Calculate max span
        XMFLOAT3 span = m_model.GetBounds().GetSpan();
        float maxSpan = std::max(span.x, span.z);

        // per instance
        float instanceSpanX = 2.0f * maxSpan;
        float instanceSpanZ = 4.0f * maxSpan;

        // total span
        float totalSpanX = InstanceColsNum * instanceSpanX;
        float totalSpanZ = InstanceRowsNum * instanceSpanZ;

        // Calculate Position
        for (auto j = 0u; j < InstanceRowsNum; ++j)
        {
            for (auto i = 0u; i < InstanceColsNum; ++i)
            {
                float x = i * instanceSpanX - (totalSpanX / 2.0f) + instanceSpanX / 2.0f;
                float y = 0.0f;
                float z = j * instanceSpanZ - (totalSpanZ / 2.0f) - 2.15f * instanceSpanZ;

                uint32_t index = j * InstanceColsNum + i;
                m_instances[index] = DirectX::XMMatrixTranslation(x, y, z);
            }
        }

        // Write Buffer
        m_instancesBuffer.Initialize(command, m_instances.size(), sizeof(XMMATRIX), m_instances.data());
    }

    command->Close();

    auto graphicsQueue = m_graphicsQueue.lock();
    if (!graphicsQueue) { return false; }

    ID3D12CommandList* commandLists[] = { command };
    graphicsQueue->Execute(1, commandLists);

    // create wait point
    m_frameWaitPoint = graphicsQueue->Signal();

    // wait to finish command complete
    GraphicsProxy::FrameSync();

    return true;
}

bool SimpleInstancedApp::OnTerminate()
{
    m_cameraCB.clear();

    m_model.Terminate();
    m_instancesBuffer.Terminate();

    m_pso.Terminate();
    m_rootSignature.Terminate();

    return true;
}

void SimpleInstancedApp::OnUpdate(double deltaTime)
{
    // Calculate max span
    XMFLOAT3 span = m_model.GetBounds().GetSpan();
    float maxSpan = std::max(span.x, span.z);

    // per instance
    float instanceSpanX = 2.0f * maxSpan;
    float instanceSpanZ = 4.0f * maxSpan;

    // total span
    float totalSpanX = InstanceColsNum * instanceSpanX;
    float totalSpanZ = InstanceRowsNum * instanceSpanZ;

    // Calculate Position
    for (auto j = 0u; j < InstanceRowsNum; ++j)
    {
        for (auto i = 0u; i < InstanceColsNum; ++i)
        {
            float x = i * instanceSpanX - (totalSpanX / 2.0f) + instanceSpanX / 2.0f;
            float y = 0.0f;
            float z = j * instanceSpanZ - (totalSpanZ / 2.0f) - 2.15f * instanceSpanZ;

            uint32_t index = j * InstanceColsNum + i;
            double angle = GetGlobalRelativeTime();
            m_instances[index] = 
                DirectX::XMMatrixRotationY(angle) * DirectX::XMMatrixTranslation(x, y, z);
        }
    }

    // GetQuery
    D3D12_QUERY_DATA_PIPELINE_STATISTICS1 pipelineStatistics = {};
    if (GraphicsProxy::HasQuery())
    {
        void* pointer = nullptr;
        GraphicsProxy::GetQueryResource()->Map(0, nullptr, &pointer);
        memcpy(&pipelineStatistics, pointer, sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS1));
        GraphicsProxy::GetQueryResource()->Unmap(0, nullptr);
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Profile");
    ImGui::Text(std::format("CInvocations: {}", pipelineStatistics.CInvocations).c_str()); ImGui::NextColumn();
    ImGui::Text(std::format("CPrimitives: {}", pipelineStatistics.CPrimitives).c_str()); ImGui::NextColumn();
    ImGui::Text(std::format("PSInvocations: {}", pipelineStatistics.PSInvocations).c_str()); ImGui::NextColumn();
    ImGui::Text(std::format("ASInvocations: {}", pipelineStatistics.ASInvocations).c_str()); ImGui::NextColumn();
    ImGui::Text(std::format("MSInvocations: {}", pipelineStatistics.MSInvocations).c_str()); ImGui::NextColumn();
    ImGui::Text(std::format("MSPrimitives: {}", pipelineStatistics.MSPrimitives).c_str()); ImGui::NextColumn();
    ImGui::End();
}

void SimpleInstancedApp::OnRender()
{
    uint32_t index = GetCurrentBackBufferIndex();

    const UINT instanceCount = static_cast<UINT>(m_instances.size());

    // for camera
    constexpr auto fovY = DirectX::XMConvertToRadians(37.5f);
    auto aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
    XMMATRIX view = m_camera.GetView();
    XMMATRIX proj = DirectX::XMMatrixPerspectiveFovRH(fovY, aspect, 0.1f, 1000.0f);
    proj = view * proj;

    auto graphicsQueue = m_graphicsQueue.lock();
    if (!graphicsQueue) { return; }

    m_graphicsCommandList.Reset();
    auto command = m_graphicsCommandList.GetD3D12CommandList();

    // update instance
    GraphicsProxy::UpdateBuffer(command, m_instancesBuffer.GetResource(), m_instances.data());
    m_instancesBuffer.ChangeState(command, D3D12_RESOURCE_STATE_GENERIC_READ);

    // set RootSignature/PSO
    command->SetGraphicsRootSignature(m_rootSignature.Get());
    command->SetPipelineState(m_pso.Get());

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

    // set descriptor heap table
    GraphicsProxy::BeginQuery(command);
    for (auto i = 0u; i < m_model.GetMeshCount(); i++)
    {
        auto mesh = m_model.GetMesh(i).lock();
        UINT meshletCount = mesh->GetMeshletCount();

        //camera
        command->SetGraphicsRoot32BitConstants(0, 16, &proj, 0);
        command->SetGraphicsRoot32BitConstants(0, 1, &instanceCount, 16);
        command->SetGraphicsRoot32BitConstants(0, 1, &meshletCount, 17);

        // meshlet
        command->SetGraphicsRootShaderResourceView(1, mesh->GetPositions().GetGpuAddress());
        command->SetGraphicsRootShaderResourceView(2, mesh->GetMeshlets().GetGpuAddress());
        command->SetGraphicsRootShaderResourceView(3, mesh->GetUniqueVertexIndices().GetGpuAddress());
        command->SetGraphicsRootShaderResourceView(4, mesh->GetPrimitiveIndices().GetGpuAddress());
        command->SetGraphicsRootShaderResourceView(5, m_instancesBuffer.GetGpuAddress());

        // Draw
        UINT threadGroupCountX = static_cast<UINT>((meshletCount * instanceCount) / 32) + 1;
        command->DispatchMesh(threadGroupCountX, 1, 1);
    }
    GraphicsProxy::EndQuery(command);

    // ResolveQuery
    GraphicsProxy::ResolveQuery(command);

    // Imgui
    ImguiManager::Instance().Draw(command);

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
