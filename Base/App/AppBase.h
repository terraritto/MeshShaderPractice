#pragma once
#define NOMINMAX
#include <Windows.h>
#include <iostream>
#include <format>
#include <vector>
#include <wrl.h>
#include <WinUser.h>
#include "MeshShaderPractice/Base/Graphics/Camera.h"
#include "MeshShaderPractice/Base/Graphics/CommandList.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Graphics/Target/ColorTarget.h"
#include "MeshShaderPractice/Base/Graphics/Target/DepthTarget.h"
#include "MeshShaderPractice/Base/Util/Timer.h"

class AppBase
{
public:
	AppBase();
	AppBase(LPCWSTR title, UINT width, UINT height, HICON icon, HMENU menu, HACCEL accel);

	// Run!!
	void Run();

	// Setter
	void SetWindowSize(UINT width, UINT height) { m_width = width;m_height = height; }

protected:
	bool Initialize();
	void Terminate();

	// Get Back Buffer Index of swap chain.
	uint32_t GetCurrentBackBufferIndex() const;

	// Execute Command and display.
	void Present(uint32_t syncInterval);

	// for time
	double GetGlobalRelativeTime();

protected:
	virtual bool OnInitialize() = 0;
	virtual bool OnTerminate() = 0;
	virtual void OnUpdate(double deltaTime) {}
	virtual void OnRender() = 0;

private:
	// Init
	bool InitializeWindow();
	bool InitializeD3D();

	// Term
	void TerminateWindow();
	void TerminateD3D();

	// Main Process
	void MainLoop();

	// Callback of window
	static LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp);

protected:
	// Window
	HINSTANCE					m_instance;
	HWND						m_window;
	LPCWSTR						m_title;
	HICON						m_icon;
	HMENU						m_menu;
	HACCEL						m_accel;
	bool						m_isCreateWindow;

	// Parameter
	UINT						m_width;
	UINT						m_height;
	float						m_aspectRatio;

	// D3D
	DeviceDesc					m_deviceDesc;
	uint32_t					m_swapChainCount;
	DXGI_FORMAT					m_swapChainFormat;
	DXGI_FORMAT					m_depthStencilFormat;
	float                       m_clearColor[4];
	float						m_clearDepth;
	float						m_clearStencil;
	uint32_t					m_multiSampleCount;
	uint32_t					m_multiSampleQuality;
	bool						m_isAllowTearing;

	D3D12_VIEWPORT				m_viewport;
	D3D12_RECT					m_scissorRect;

	CommandList					m_graphicsCommandList;
	CommandList					m_copyCommandList;

	// Resource
	std::vector<ColorTarget>	m_colorTarget;
	DepthTarget					m_depthTarget;

	// Utility
	int							m_prevCursorX;
	int							m_prevCursorY;
	Camera						m_camera;

private:
	// D3D
	ComPtr<IDXGISwapChain4> m_swapChain;

	// Utility
	Timer						m_timer;
};