#include "AppBase.h"
#include <d3dcommon.h>
#include "MeshShaderPractice/Base/Graphics/DescriptorHeap.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

#ifndef WINDOW_CLASS_NAME
#define WINDOW_CLASS_NAME	TEXT("testApplicationClass")
#endif

AppBase::AppBase()
	: AppBase(L"TestApplication", 960, 540, nullptr, nullptr, nullptr)
{
}

AppBase::AppBase(LPCWSTR title, UINT width, UINT height, HICON icon, HMENU menu, HACCEL accel)
	// Window
	: m_instance(nullptr)
	, m_window(nullptr)
	, m_title(title)
	, m_icon(icon)
	, m_menu(menu)
	, m_accel(accel)
	, m_isCreateWindow(true)
	// Parameter
	, m_width(width)
	, m_height(height)
	, m_aspectRatio(static_cast<float>(width)/static_cast<float>(height))
	// D3D
	, m_swapChainCount(2)
	, m_swapChainFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
	, m_depthStencilFormat(DXGI_FORMAT_D32_FLOAT)
	, m_multiSampleCount(1)
	, m_multiSampleQuality(0)
	, m_isAllowTearing(false)
	, m_viewport()
	, m_scissorRect()
	// Utility
	, m_prevCursorX(0)
	, m_prevCursorY(0)
	// private D3D
	, m_swapChain(nullptr)
{
	// Color Clear
	m_clearColor[0] = 0.392156899f;
	m_clearColor[1] = 0.584313750f;
	m_clearColor[2] = 0.929411829f;
	m_clearColor[3] = 1.000000000f;

	// DepthStencil Clear
	m_clearDepth = 1.0f;
	m_clearStencil = 0.0f;

	// default device desc
	m_deviceDesc.EnableDebug = true;
	m_deviceDesc.EnableDRED = true;
	m_deviceDesc.EnableBreakOnError = true;
	m_deviceDesc.EnableBreakOnWarning = true;
	m_deviceDesc.IsUseMeshlet = true;
	m_deviceDesc.MaxColorTargetCount = 128;
	m_deviceDesc.MaxDepthTargetCount = 128;
	m_deviceDesc.MaxSamplerCount = 128;
	m_deviceDesc.MaxShaderResourceCount = 4096;
}

void AppBase::Run()
{
	if (Initialize())
	{
		MainLoop();
	}

	Terminate();
}

bool AppBase::Initialize()
{
	// initialize COM Library
	HRESULT hr = CoInitialize(nullptr);
	if (FAILED(hr))
	{
		ELOGA("Error: Com Lib Initialize Failed.");
		return false;
	}

	// set security level for COM Lib
	hr = CoInitializeSecurity
	(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE,
		NULL
	);
	if (FAILED(hr))
	{
		DLOGA("Error: Com Lib initialize security failed.");
		return false;
	}

	// Create Window
	if (m_isCreateWindow && !InitializeWindow())
	{
		std::cout << std::format("Error: InitializeWindow() Failed.") << std::endl;
		return false;
	}

	if (!InitializeD3D())
	{
		std::cout << std::format("Error: InitializeD3D() Failed.") << std::endl;
		return false;
	}

	if (!OnInitialize())
	{
		std::cout << std::format("Error: OnInitialize() Failed.") << std::endl;
		return false;
	}

	// Display Window
	ShowWindow(m_window, SW_SHOWNORMAL);
	UpdateWindow(m_window);

	// Set focus
	SetFocus(m_window);

	// Init Timer
	m_timer.Start();

	return true;
}

void AppBase::Terminate()
{
	GraphicsProxy::WaitIdle();

	OnTerminate();
	TerminateD3D();
	if (m_isCreateWindow)
	{
		TerminateWindow();
	}

	CoUninitialize();
}

bool AppBase::InitializeWindow()
{
	// Instance HandleÇê∂ê¨
	HINSTANCE instance = GetModuleHandle(nullptr);
	if (!instance)
	{
		std::cout << std::format("Error: GetModuleHandle() Failed.") << std::endl;
		return false;
	}

	// if icon is nullptr, load default icon.
	if (m_icon == nullptr)
	{
		// find icon and set
		WCHAR path[MAX_PATH];
		GetModuleFileNameW(NULL, path, MAX_PATH);
		m_icon = ExtractIconW(instance, path, 0);

		// not found icon case
		if (m_icon == nullptr)
		{
			m_icon = LoadIcon(instance, IDI_APPLICATION);
		}
	}

	// ägí£ÉEÉBÉìÉhÉEÉNÉâÉX
	WNDCLASSEXW wc;
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProcedure;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = instance;
	wc.hIcon = m_icon;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = WINDOW_CLASS_NAME;
	wc.hIconSm = m_icon;

	// register window class
	if (!RegisterClassExW(&wc))
	{
		std::cout << std::format("Error: RegisterClassEx() Failed.") << std::endl;
		return false;
	}

	m_instance = instance;

	// adjust WindowRect
	RECT rc = { 0,0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rc, style, FALSE);

	// windowÇÃê∂ê¨
	m_window = CreateWindowW
	(
		WINDOW_CLASS_NAME,
		m_title,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		m_menu,
		m_instance,
		this
	);
	if (!m_window)
	{
		std::cout << std::format("Error: CreateWindowW() Failed.") << std::endl;
		return false;
	}

	return true;
}

bool AppBase::InitializeD3D()
{
	HRESULT hr = S_OK;

	if (!GraphicsProxy::Initialize(m_deviceDesc))
	{
		ELOGA("Error : GraphicsProxy::Initialize() Failed.");
		return false;
	}

	if (m_isCreateWindow)
	{
		DXGI_FORMAT format = GetNoSRGBFormat(m_swapChainFormat);

		std::vector<DisplayInfo> infos;
		GraphicsProxy::GetDisplayInfo(format, infos);

		bool isDetect = false;
		DisplayInfo supportedInfo = {};

		// find display info for found lists.
		for (const DisplayInfo& info : infos)
		{
			if (info.m_width == m_width && info.m_height == m_height)
			{
				isDetect = true;
				supportedInfo = info;
				break;
			}
		}

		// if selected resolution isn't supported, find near resolution.
		if (!isDetect)
		{
			for (const DisplayInfo& info : infos)
			{
				if (m_width <= info.m_width && m_height <= info.m_height)
				{
					isDetect = true;
					supportedInfo = info;
					break;
				}
			}
		}

		if (!isDetect)
		{
			ELOGA("Error: Not Found Supported Resolution.");
			return false;
		}

		// Get WindowSize
		RECT rc;
		GetClientRect(m_window, &rc);
		UINT w = rc.right - rc.left, h = rc.bottom - rc.top;
		m_width = w; m_height = h;

		// Calculate aspect
		m_aspectRatio = static_cast<float>(supportedInfo.m_width) / static_cast<float>(supportedInfo.m_height);

		// SwapChain
		{
			DXGI_SWAP_CHAIN_DESC1 desc = {};
			desc.Width = m_width;
			desc.Height = m_height;
			desc.Format = format;
			desc.Stereo = FALSE;
			desc.SampleDesc.Count = m_multiSampleCount;
			desc.SampleDesc.Quality = m_multiSampleQuality;
			desc.BufferCount = m_swapChainCount;
			desc.Scaling = DXGI_SCALING_STRETCH;
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			desc.Flags = m_isAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenDesc = {};
			fullScreenDesc.RefreshRate = supportedInfo.m_refreshRate;
			fullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			fullScreenDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
			fullScreenDesc.Windowed = TRUE;

			// Create
			ComPtr<IDXGISwapChain1> swapChain;
			ID3D12CommandQueue* queue = GraphicsProxy::GetGraphicsQueue().lock()->GetQueue();
			hr = GraphicsProxy::GetDXGIFactory()->CreateSwapChainForHwnd
			(
				queue, 
				m_window,
				&desc,
				&fullScreenDesc,
				nullptr,
				swapChain.GetAddressOf()
			);
			if (FAILED(hr))
			{
				ELOGA("Error: IDXGIFactory::CreateSwapChain() Failed. errcode=0x%x", hr);
				return false;
			}

			if (m_isAllowTearing)
			{
				// disable ALT+Enter fullscreen command.
				GraphicsProxy::GetDXGIFactory()->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER);
			}

			// cast IDXGISwapChain4
			hr = swapChain->QueryInterface(m_swapChain.GetAddressOf());
			if (FAILED(hr))
			{
				m_swapChain.Reset();
				ELOGA("Error: IDXGISwapChain4 Conversion Failed.");
				return false;
			}
			else
			{
				wchar_t name[] = L"defaultSwapChain4\0";
				m_swapChain->SetPrivateData(WKPDID_D3DDebugObjectNameW, sizeof(name), name);

				// todo: write HDR Output Check
			}
		}

		// ColorTarget
		{
			m_colorTarget.resize(m_swapChainCount);

			for (auto i = 0u; i < m_swapChainCount; ++i)
			{
				if (!m_colorTarget[i].Initialize(m_swapChain.Get(), i, IsSRGBFormat(m_swapChainFormat)))
				{
					ELOGA("Error: ColorTarget::Initialize() Failed.");
					return false;
				}

				m_colorTarget[i].SetName(L"DefaultColorTarget");
			}
		}

		// Depth
		{
			TargetDesc desc = {};
			desc.m_dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.m_alignment = 0;
			desc.m_width = m_width;
			desc.m_height = m_height;
			desc.m_depthOrArraySize = 1;
			desc.m_mipLevels = 1;
			desc.m_format = m_depthStencilFormat;
			desc.m_samplerDesc.Count = 1;
			desc.m_samplerDesc.Quality = 0;
			desc.m_initState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			desc.m_clearDepth = m_clearDepth;
			desc.m_clearStencil = m_clearStencil;

			if (!m_depthTarget.Initialize(&desc))
			{
				ELOGA("Error: DepthTarget::Initialize() Failed.");
				return false;
			}

			m_depthTarget.SetName(L"DefaultDepthTarget");
		}

		// Viewport
		m_viewport.Width = FLOAT(m_width);
		m_viewport.Height = FLOAT(m_height);
		m_viewport.MinDepth = 0.0f;
		m_viewport.MaxDepth = 1.0f;
		m_viewport.TopLeftX = 0;
		m_viewport.TopLeftY = 0;

		// Scissor
		m_scissorRect.left = 0;
		m_scissorRect.right = m_width;
		m_scissorRect.top = 0;
		m_scissorRect.bottom = m_height;
	}

	// CommandList (and CommandAllocator)
	if (!m_graphicsCommandList.Initialize(GraphicsProxy::GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		ELOGA("Error: CommandList::Init() failed.");
		return false;
	}

	m_graphicsCommandList.SetName(L"DefaultGraphicsCommandList");

	if (!m_copyCommandList.Initialize(GraphicsProxy::GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_COPY))
	{
		ELOGA("Error: CommandList::Init() failed.");
		return false;
	}

	m_copyCommandList.SetName(L"DefaultCopyCommandList");

	return true;
}

void AppBase::TerminateWindow()
{
	// ìoò^âèú
	if (m_instance != nullptr)
	{
		UnregisterClassW(WINDOW_CLASS_NAME, m_instance);
	}

	if (m_accel) { DestroyAcceleratorTable(m_accel); }
	if (m_menu) { DestroyMenu(m_menu); }
	if (m_icon) { DestroyIcon(m_icon); }

	// ãÛÇ…Ç∑ÇÈ
	m_title = nullptr;
	m_instance = nullptr;
	m_window = nullptr;
	m_icon = nullptr;
	m_menu = nullptr;
	m_accel = nullptr;
}

void AppBase::TerminateD3D()
{
	if (m_isCreateWindow)
	{
		for (size_t i = 0; i < m_colorTarget.size(); ++i)
		{
			m_colorTarget[i].Terminate();
		}
		m_colorTarget.clear();

		m_depthTarget.Terminate();
		m_swapChain.Reset();
	}

	m_copyCommandList.Terminate();
	m_graphicsCommandList.Terminate();

	GraphicsProxy::Terminate();
}

void AppBase::MainLoop()
{
	MSG msg = { 0 };

	while (WM_QUIT != msg.message)
	{
		auto gotMsg = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);

		if (gotMsg)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			OnUpdate(m_timer.GetElapsedSec());
			OnRender();
		}
	}

}

LRESULT CALLBACK AppBase::WindowProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
	AppBase* instance = reinterpret_cast<AppBase*>(GetWindowLongPtr(window, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE:
		{
			auto createStruct = reinterpret_cast<LPCREATESTRUCT>(lp);
			SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));

			// enable drag and drop.
			DragAcceptFiles(window, TRUE);
		}
		break;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(window, &ps);
			EndPaint(window, &ps);
		}
		break;

	case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;

	default:
		break;
	}
	
	if (instance != nullptr)
	{
		// Camera Process
		switch (message)
		{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_XBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
		case WM_MOUSEMOVE:
			{
				int x = int(LOWORD(lp));
				int y = int(HIWORD(lp));

				int delta = 0;
				if (message == WM_MOUSEHWHEEL)
				{
					POINT pt;
					pt.x = x; pt.y = y;

					// Convert Client Axis
					ScreenToClient(window, &pt);
					x = pt.x; y = pt.y;

					delta += int(HIWORD(wp));
				}

				int state = int(LOWORD(wp));
				bool left = ((state & MK_LBUTTON) != 0);
				bool right = ((state & MK_RBUTTON) != 0);
				bool middle = ((state & MK_MBUTTON) != 0);

				Camera::Event args = {};

				if (left)
				{
					args.m_type = static_cast<uint32_t>(Camera::EventType::Rotate);
					args.m_rotateH = DirectX::XMConvertToRadians(-0.5f * (x - instance->m_prevCursorX));
					args.m_rotateV = DirectX::XMConvertToRadians(0.5f * (y - instance->m_prevCursorY));
					instance->m_camera.UpdateByEvent(args);
				}
				else if (right)
				{
					args.m_type = static_cast<uint32_t>(Camera::EventType::Dolly);
					args.m_dolly = DirectX::XMConvertToRadians(0.5f * (y - instance->m_prevCursorY));
					instance->m_camera.UpdateByEvent(args);
				}
				else if (middle)
				{
					args.m_type = static_cast<uint32_t>(Camera::EventType::Move);
					if (GetAsyncKeyState(VK_MENU) != 0)
					{
						args.m_moveX = DirectX::XMConvertToRadians(0.5f * (x - instance->m_prevCursorX));
						args.m_moveZ = DirectX::XMConvertToRadians(0.5f * (y - instance->m_prevCursorY));
					}
					else
					{
						args.m_moveX = DirectX::XMConvertToRadians(0.5f * (x - instance->m_prevCursorX));
						args.m_moveY = DirectX::XMConvertToRadians(0.5f * (y - instance->m_prevCursorY));
					}
					instance->m_camera.UpdateByEvent(args);
				}
				
				instance->m_prevCursorX = x; instance->m_prevCursorY = y;
			}
			break;
		}
	}

	return DefWindowProc(window, message, wp, lp);
}

uint32_t AppBase::GetCurrentBackBufferIndex() const
{
	if (m_swapChain.Get() == nullptr)
	{
		return 0;
	}

	return m_swapChain->GetCurrentBackBufferIndex();
}

void AppBase::Present(uint32_t syncInterval)
{
	if (!m_isCreateWindow || m_swapChain.Get() == nullptr)
	{
		return;
	}

	HRESULT hr = S_OK;
	hr = m_swapChain->Present(syncInterval, 0);
}

double AppBase::GetGlobalRelativeTime()
{
	return m_timer.GetRelativeTime();
}
