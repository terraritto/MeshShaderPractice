#pragma once
#include "MeshShaderPractice/Base/Graphics/Holder/DescriptorHolder.h"

struct ImGui_ImplDX12_InitInfo;

struct ImguiDesc
{
	DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
	int numFrames = 2;
};

class ImguiManager
{
public:
	static ImguiManager& Instance();
	
	// draw instruction
	void Draw(ID3D12GraphicsCommandList* command);

protected:
	// for AppBase
	bool Initialize(HWND window, ImguiDesc desc);
	void Terminate();
	static bool WindowProc(HWND window, UINT message, WPARAM wp, LPARAM lp);
	static bool IsInput();

private:
	// provide alloc/free for imgui. 
	static void Allocate(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuDescHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuDescHandle);
	static void Free(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescHandle);

private:
	// imgui descriptor list
	static std::vector<DescriptorHolder> m_holders;

private:
	static ImguiManager m_instance;
	friend class AppBase;
};
