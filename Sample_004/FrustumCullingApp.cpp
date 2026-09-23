#include "FrustumCullingApp.h"
#include "MeshShaderPractice/Base/App/ImguiManager.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Util/Logger.h"
#include "MeshShaderPractice/External/Imgui/imgui.h"
#include "MeshShaderPractice/External/Imgui/imgui_impl_win32.h"
#include "MeshShaderPractice/External/Imgui/imgui_impl_dx12.h"

struct alignas(256) TransformParam
{
    XMMATRIX m_viewProj;
    XMVECTOR m_planes[6];
    XMVECTOR m_debugPlanes[6];
    float m_useFrustum;
};

bool FrustumCullingApp::OnInitialize()
{
    HRESULT hr = S_OK;

    auto device = GraphicsProxy::GetD3D12Device();

    m_graphicsQueue = GraphicsProxy::GetGraphicsQueue();

    // RootSignature
    {
        // Root Param
        std::vector<D3D12_ROOT_PARAMETER> params;
        params.push_back(PARAM_CONSTANT{ D3D12_SHADER_VISIBILITY_ALL, 2, 0, 0 }); // 2 = (sizeof(UINT) * 2) / 4
        params.push_back(PARAM_CBV{ D3D12_SHADER_VISIBILITY_ALL, 1, 0 });
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_ALL, 0, 0 });
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_ALL, 1, 0 });
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_ALL, 2, 0 });
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_ALL, 3, 0 });
        params.push_back(PARAM_SRV{ D3D12_SHADER_VISIBILITY_ALL, 4, 0 });

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
        m_pso.SetReloadPathAS(L"Sample_004/Resource/Shader/SimpleAS.hlsl");
        m_pso.SetReloadPathPS(L"Sample_004/Resource/Shader/SimplePS.hlsl");
        m_pso.SetReloadPathMS(L"Sample_004/Resource/Shader/SimpleMS.hlsl");

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
    std::string path = "Sample_004/Resource/Model/bunny.obj";
    if (!m_model.Initialize(command, path))
    {
        ELOGA("Error : Model::Initialize() Failed.");
        return false;
    }

    // setup transform   
    if (!m_transformBuffer.Initialize(sizeof(TransformParam)))
    {
        ELOGA("Error : TransformBuffer Init Failed.");
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

    // Camera
    constexpr auto fovY = DirectX::XMConvertToRadians(37.5f);
    m_camera.SetFov(37.5f);
    m_camera.SetAspect(static_cast<float>(m_width) / static_cast<float>(m_height));

    // Debug
    {
        if (!m_shapeStates.Initialize(m_swapChainFormat, m_depthStencilFormat))
        {
            ELOGA("Error: ShapeState Init Failed.");
            return false;
        }

        XMMATRIX view = m_camera.GetView();
        XMMATRIX proj = m_camera.GetProjection();
        m_shapeStates.SetViewProj(view, proj);

        if (!m_frustumShape.Initialize(2.0f)) // [-1,1]
        {
            ELOGA("Error: Frustum Shape Init Failed.");
            return false;
        }
        m_frustumShape.SetColor(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.1f));
        m_frustumShape.SetWorld(DirectX::XMMatrixInverse(nullptr, view) * DirectX::XMMatrixInverse(nullptr, proj));
        CalculateFrustumPlanes(view, proj, m_debugPlanes);
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

bool FrustumCullingApp::OnTerminate()
{
    m_shapeStates.Terminate();
    m_frustumShape.Terminate();

    m_cameraCB.clear();

    m_model.Terminate();
    m_transformBuffer.Terminate();
    m_instancesBuffer.Terminate();

    m_pso.Terminate();
    m_rootSignature.Terminate();

    return true;
}

void FrustumCullingApp::OnUpdate(double deltaTime)
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

    {
        TransformParam param;

        // view/proj
        XMMATRIX view = m_camera.GetView();
        XMMATRIX proj = m_camera.GetProjection();
        XMMATRIX viewProj = view * proj;

        // frustum
        param.m_viewProj = viewProj;
        CalculateFrustumPlanes(view, proj, param.m_planes);
        for (int i = 0;i < 6; i++) { param.m_debugPlanes[i] = m_debugPlanes[i]; }
        param.m_useFrustum = m_isDebugFrustum ? 1.0f : 0.0f;

        // update transform
        m_transformBuffer.Swap();
        m_transformBuffer.Update(&param, sizeof(TransformParam));

        // update debug
        m_shapeStates.SetViewProj(view, proj);
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Profile");
    ImGui::Checkbox("Debug Frustum", &m_isDebugFrustum);
    ImGui::Separator();
    ImGui::Text(std::format("CInvocations: {}", pipelineStatistics.CInvocations).c_str()); ImGui::NextColumn();
    ImGui::Text(std::format("CPrimitives: {}", pipelineStatistics.CPrimitives).c_str()); ImGui::NextColumn();
    ImGui::Text(std::format("PSInvocations: {}", pipelineStatistics.PSInvocations).c_str()); ImGui::NextColumn();
    ImGui::Text(std::format("ASInvocations: {}", pipelineStatistics.ASInvocations).c_str()); ImGui::NextColumn();
    ImGui::Text(std::format("MSInvocations: {}", pipelineStatistics.MSInvocations).c_str()); ImGui::NextColumn();
    ImGui::Text(std::format("MSPrimitives: {}", pipelineStatistics.MSPrimitives).c_str()); ImGui::NextColumn();
    ImGui::End();
}

void FrustumCullingApp::OnRender()
{
    uint32_t index = GetCurrentBackBufferIndex();

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
        const UINT meshletCount = mesh->GetMeshletCount();
        const UINT instanceCount = static_cast<UINT>(m_instances.size());

        //camera
        command->SetGraphicsRoot32BitConstants(0, 1, &instanceCount, 0);
        command->SetGraphicsRoot32BitConstants(0, 1, &meshletCount, 1);

        // transform
        command->SetGraphicsRootConstantBufferView(1, m_transformBuffer.GetGpuAddress());

        // meshlet
        command->SetGraphicsRootShaderResourceView(2, mesh->GetPositions().GetGpuAddress());
        command->SetGraphicsRootShaderResourceView(3, mesh->GetMeshlets().GetGpuAddress());
        command->SetGraphicsRootShaderResourceView(4, mesh->GetUniqueVertexIndices().GetGpuAddress());
        command->SetGraphicsRootShaderResourceView(5, mesh->GetPrimitiveIndices().GetGpuAddress());
        
        // instance
        command->SetGraphicsRootShaderResourceView(6, m_instancesBuffer.GetGpuAddress());

        // Draw
        UINT threadGroupCountX = static_cast<UINT>((meshletCount * instanceCount) / 32) + 1;
        command->DispatchMesh(threadGroupCountX, 1, 1);
    }
    GraphicsProxy::EndQuery(command);

    // frustum
    if (m_isDebugFrustum)
    {
        m_shapeStates.ApplyWireframeState(command);
        m_frustumShape.Draw(command);
    }

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
