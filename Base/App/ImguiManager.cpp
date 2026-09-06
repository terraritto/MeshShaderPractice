#include "ImguiManager.h"
#include <cassert>
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/External/Imgui/imgui.h"
#include "MeshShaderPractice/External/Imgui/imgui_impl_win32.h"
#include "MeshShaderPractice/External/Imgui/imgui_impl_dx12.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ImguiManager ImguiManager::m_instance;
std::vector<DescriptorHolder> ImguiManager::m_holders;

ImguiManager& ImguiManager::Instance()
{
	return m_instance;
}

void ImguiManager::Draw(ID3D12GraphicsCommandList* command)
{
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command);
}

bool ImguiManager::Initialize(HWND window, ImguiDesc desc)
{
    auto* resourceDescriptorHeap = GraphicsProxy::GetResourceDescriptorHeap();

    // init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui_ImplWin32_Init(window);

    ImGui_ImplDX12_InitInfo initInfo;
    initInfo.Device = GraphicsProxy::GetD3D12Device();
    initInfo.CommandQueue = GraphicsProxy::GetGraphicsQueue().lock()->GetQueue();
    initInfo.NumFramesInFlight = desc.numFrames;
    initInfo.RTVFormat = desc.rtvFormat;
    initInfo.DSVFormat = desc.dsvFormat;
    initInfo.SrvDescriptorHeap = resourceDescriptorHeap->GetHeap();
    initInfo.SrvDescriptorAllocFn = &ImguiManager::Allocate;
    initInfo.SrvDescriptorFreeFn = &ImguiManager::Free;
    ImGui_ImplDX12_Init(&initInfo);

    return true;
}

void ImguiManager::Terminate()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // release all imgui srv resource
    m_holders.clear();
}

bool ImguiManager::WindowProc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    return ImGui_ImplWin32_WndProcHandler(window, message, wp, lp);
}

bool ImguiManager::IsInput()
{
    if (ImGui::GetCurrentContext() == nullptr)
    {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

void ImguiManager::Allocate(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuDescHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuDescHandle)
{
    // allocate
    auto handleSRV = GraphicsProxy::GetResourceDescriptorHeap()->Allocate(1);
    assert(handleSRV.IsValid());

    DescriptorHolder holder = DescriptorHolder(DescriptorHolder::HEAP_RES, handleSRV);
    m_holders.push_back(holder);

    // set
    outCpuDescHandle->ptr = holder.GetCpuHandle().ptr;
    outGpuDescHandle->ptr = holder.GetGpuHandle().ptr;
}

void ImguiManager::Free(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle)
{
    auto iter = std::find_if(m_holders.begin(), m_holders.end(), [cpuDescHandle, gpuDescHandle](DescriptorHolder& holder)
        {
            bool isCpuEqual = holder.GetCpuHandle().ptr == cpuDescHandle.ptr;
            bool isGpuEqual = holder.GetGpuHandle().ptr == gpuDescHandle.ptr;
            return isCpuEqual && isGpuEqual;
        });

    if (iter != m_holders.end())
    {
        // free resource
        iter->Reset();
        m_holders.erase(iter);
    }
}
