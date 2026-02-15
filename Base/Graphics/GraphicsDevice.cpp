#include "GraphicsDevice.h"
#include <algorithm>
#include "MeshShaderPractice/Base/Util/Logger.h"
#include "MeshShaderPractice/Base/Util/ScopedLock.h"
#include "GraphicsProxy.h"

GraphicsDevice GraphicsDevice::m_instance;

GraphicsDevice& GraphicsDevice::Instance()
{
	return m_instance;
}

bool GraphicsDevice::Initialize(const DeviceDesc& deviceDesc)
{
	m_spinLock = std::make_shared<SpinLock>();
	if (!m_spinLock)
	{
		ELOGA("Error: SpinLock can't be allocated.");
		return false;
	}

	HRESULT hr = S_OK;
	if (deviceDesc.EnableDebug)
	{
		// DebugLayer‚ðON
		ComPtr<ID3D12Debug> debug;
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()));

		if (SUCCEEDED(hr))
		{
			hr = debug->QueryInterface(IID_PPV_ARGS(m_debug.GetAddressOf()));
			if (SUCCEEDED(hr))
			{
				m_debug->EnableDebugLayer();
				m_debug->SetEnableGPUBasedValidation(TRUE);
			}

			ComPtr<ID3D12Debug5> debug5;
			hr = debug->QueryInterface(IID_PPV_ARGS(debug5.GetAddressOf()));
			if (SUCCEEDED(hr))
			{
				debug5->SetEnableAutoName(TRUE);
			}
		}
	}

	//DRED
	if (deviceDesc.EnableDRED)
	{
		ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dred;
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(dred.GetAddressOf()));
		if (SUCCEEDED(hr))
		{
			dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			dred->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		}
	}

	// DXGI Factory
	{
		uint32_t flags = 0;
		if (deviceDesc.EnableDebug) { flags |= DXGI_CREATE_FACTORY_DEBUG; }

		ComPtr<IDXGIFactory6> factory;

		hr = CreateDXGIFactory(IID_PPV_ARGS(factory.GetAddressOf()));
		if (FAILED(hr))
		{

			ELOGA("Error: CreateDXGIFactory() Failed. errcode=0x%x", hr);
			return false;
		}

		hr = factory->QueryInterface(IID_PPV_ARGS(m_factory.GetAddressOf()));
		if (FAILED(hr))
		{

			ELOGA("Error: QueryInterface() Failed. errcode=0x%x", hr);
			return false;
		}
	}

	// DXGI Adapter
	{
		ComPtr<IDXGIAdapter1> adapter;
		for (int adapterId = 0;
			DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapterByGpuPreference(adapterId, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()));
			adapterId++)
		{
			DXGI_ADAPTER_DESC1 desc;
			hr = adapter->GetDesc1(&desc);
			if (FAILED(hr)) { continue; }

			// avoid software
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { continue; }

			hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr);
			if (SUCCEEDED(hr))
			{
				if (m_adapter == nullptr) { m_adapter = adapter; }

				ComPtr<IDXGIOutput> output;
				hr = adapter->EnumOutputs(0, output.GetAddressOf());
				if (FAILED(hr)) { continue; }

				hr = output->QueryInterface(IID_PPV_ARGS(m_output.GetAddressOf()));
				if (SUCCEEDED(hr)) { break; }
			}
		}

	}

	// Device
	{
		ComPtr<ID3D12Device> device;
		hr = D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOGA("Error: D3D12CreateDevice() Failed. errcode = 0x%x", hr);
			return false;
		}

		hr = device->QueryInterface(IID_PPV_ARGS(m_device.GetAddressOf()));
		if (FAILED(hr))
		{
			ELOGA("Error: QueryInterface() Failed. errcode = 0x%x", hr);
			return false;
		}
		m_device->SetName(L"MyDevice");

		// convert ID3D12InfoQueue
		if (deviceDesc.EnableDebug)
		{
			hr = m_device->QueryInterface(IID_PPV_ARGS(m_infoQueue.GetAddressOf()));
			if (SUCCEEDED(hr))
			{
				// Break is occurred by happened error.
				if (deviceDesc.EnableBreakOnError)
				{
					m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
				}

				// Break is occurred by happened warning.
				if (deviceDesc.EnableBreakOnWarning)
				{
					m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
				}

				// deny message ID list
				D3D12_MESSAGE_ID denyIds[] =
				{
					D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
					D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
				};

				// deny message severity list
				D3D12_MESSAGE_SEVERITY denySeverities[] =
				{
					D3D12_MESSAGE_SEVERITY_INFO
				};

				D3D12_INFO_QUEUE_FILTER filter = {};
				filter.DenyList.NumIDs = _countof(denyIds);
				filter.DenyList.pIDList = denyIds;
				filter.DenyList.NumSeverities = _countof(denySeverities);
				filter.DenyList.pSeverityList = denySeverities;

				m_infoQueue->PushStorageFilter(&filter);
			}
		}
	}

	// D3D12MA
	{
		D3D12MA::ALLOCATOR_DESC desc = {};
		desc.Flags = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
		desc.pDevice = m_device.Get();
		desc.pAdapter = m_adapter.Get();
		hr = D3D12MA::CreateAllocator(&desc, m_allocator.GetAddressOf());
		if (FAILED(hr))
		{
			ELOGA("Error: D3D12MA::CreateAllocator() Failed. errcode=0x%x", hr);
			return false;
		}
	}

	// todo: summarize
	// CBV/SRV/UAV descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = deviceDesc.MaxShaderResourceCount;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (!m_resourceHeap.Initialize(m_device.Get(), &desc))
		{
			ELOGA("Error: CBV/SRV/UAV DescriptorHeap::Init() Failed.");
			return false;
		}
	}

	// static sampler descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = deviceDesc.MaxSamplerCount;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (!m_samplerHeap.Initialize(m_device.Get(), &desc))
		{
			ELOGA("Error: Sampler DescriptorHeap::Init() Failed.");
			return false;
		}
	}

	// RTV descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = deviceDesc.MaxColorTargetCount;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		if (!m_rtvHeap.Initialize(m_device.Get(), &desc))
		{
			ELOGA("Error: RTV DescriptorHeap::Init() Failed.");
			return false;
		}
	}

	// DSV descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = deviceDesc.MaxColorTargetCount;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		if (!m_dsvHeap.Initialize(m_device.Get(), &desc))
		{
			ELOGA("Error: RTV DescriptorHeap::Init() Failed.");
			return false;
		}
	}

	// CommandQueue
	{
		// todo: summarize
		// Direct
		bool isCreateCommandQueue = CommandQueue::Create
		(
			m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, m_graphicsQueue
		);
		if (!isCreateCommandQueue)
		{
			ELOGA("Error: GraphicsQueue CommandQueue::Create() Failed.");
			return false;
		}

		// Compute
		isCreateCommandQueue = CommandQueue::Create
		(
			m_device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeQueue
		);
		if (!isCreateCommandQueue)
		{
			ELOGA("Error: ComputeQueue CommandQueue::Create() Failed.");
			return false;
		}

		// Copy
		isCreateCommandQueue = CommandQueue::Create
		(
			m_device.Get(), D3D12_COMMAND_LIST_TYPE_COPY, m_copyQueue
		);
		if (!isCreateCommandQueue)
		{
			ELOGA("Error: CopyQueue CommandQueue::Create() Failed.");
			return false;
		}

		// Video Decode
		isCreateCommandQueue = CommandQueue::Create
		(
			m_device.Get(), D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE, m_videoDecodeQueue
		);
		if (!isCreateCommandQueue)
		{
			ELOGA("Error: VideoDecodeQueue CommandQueue::Create() Failed.");
			return false;
		}

		// Video Process
		isCreateCommandQueue = CommandQueue::Create
		(
			m_device.Get(), D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS, m_videoProcessQueue
		);
		if (!isCreateCommandQueue)
		{
			ELOGA("Error: VideoProcessQueue CommandQueue::Create() Failed.");
			return false;
		}

		// Video Encode
		isCreateCommandQueue = CommandQueue::Create
		(
			m_device.Get(), D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE, m_videoEncodeQueue
		);
		if (!isCreateCommandQueue)
		{
			ELOGA("Error: VideoEncodeQueue CommandQueue::Create() Failed.");
			return false;
		}
	}

	// Quad(use example: postprocess)
	{
		QuadVertex vertices[] =
		{
			QuadVertex(-1.0f,  1.0f, 0.0f, 0.0f),
			QuadVertex( 3.0f,  1.0f, 2.0f, 0.0f),
			QuadVertex(-1.0f, -3.0f, 0.0f, 2.0f),
		};

		auto size = sizeof(vertices);
		auto stride = uint32_t(sizeof(vertices[0]));

		if (!m_quadVb.Initialize(size, stride))
		{
			ELOGA("Error : VertexBuffer::Init() Failed.");
			return false;
		}

		auto dst = m_quadVb.MapAs<QuadVertex>();
		memcpy(dst, vertices, size);
		m_quadVb.UnMap();
	}

	// CommandSignature
	{
		// todo: summarize
		// Indirect Draw
		{
			D3D12_INDIRECT_ARGUMENT_DESC argument = {};
			argument.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

			D3D12_COMMAND_SIGNATURE_DESC desc = {};
			desc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
			desc.NumArgumentDescs = 1;
			desc.pArgumentDescs = &argument;

			hr = m_device->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(m_commandSignatures[COMMAND_SIGNATURE_TYPE_DRAW].GetAddressOf()));
			if (FAILED(hr))
			{
				ELOGA("Error: ID3D12Device::CreateCommandSignature() Failed. errcode = 0x%x", hr);
				return false;
			}
		}

		// Indirect Draw Indexed
		{
			D3D12_INDIRECT_ARGUMENT_DESC argument = {};
			argument.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

			D3D12_COMMAND_SIGNATURE_DESC desc = {};
			desc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
			desc.NumArgumentDescs = 1;
			desc.pArgumentDescs = &argument;

			hr = m_device->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(m_commandSignatures[COMMAND_SIGNATURE_TYPE_DRAW_INDEXED].GetAddressOf()));
			if (FAILED(hr))
			{
				ELOGA("Error: ID3D12Device::CreateCommandSignature() Failed. errcode = 0x%x", hr);
				return false;
			}
		}

		// Dispatch draw
		{
			D3D12_INDIRECT_ARGUMENT_DESC argument = {};
			argument.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

			D3D12_COMMAND_SIGNATURE_DESC desc = {};
			desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
			desc.NumArgumentDescs = 1;
			desc.pArgumentDescs = &argument;

			hr = m_device->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(m_commandSignatures[COMMAND_SIGNATURE_TYPE_DISPATCH].GetAddressOf()));
			if (FAILED(hr))
			{
				ELOGA("Error: ID3D12Device::CreateCommandSignature() Failed. errcode = 0x%x", hr);
				return false;
			}
		}

		// Dispatch Ray
		{
			D3D12_INDIRECT_ARGUMENT_DESC argument = {};
			argument.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;

			D3D12_COMMAND_SIGNATURE_DESC desc = {};
			desc.ByteStride = sizeof(D3D12_DISPATCH_RAYS_DESC);
			desc.NumArgumentDescs = 1;
			desc.pArgumentDescs = &argument;

			hr = m_device->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(m_commandSignatures[COMMAND_SIGNATURE_TYPE_DISPATCH_RAYS].GetAddressOf()));
			if (FAILED(hr))
			{
				ELOGA("Error: ID3D12Device::CreateCommandSignature() Failed. errcode = 0x%x", hr);
				return false;
			}
		}

		// Dispatch Mesh
		{
			D3D12_INDIRECT_ARGUMENT_DESC argument = {};
			argument.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;

			D3D12_COMMAND_SIGNATURE_DESC desc = {};
			desc.ByteStride = sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);
			desc.NumArgumentDescs = 1;
			desc.pArgumentDescs = &argument;

			hr = m_device->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(m_commandSignatures[COMMAND_SIGNATURE_TYPE_DISPATCH_MESH].GetAddressOf()));
			if (FAILED(hr))
			{
				ELOGA("Error: ID3D12Device::CreateCommandSignature() Failed. errcode = 0x%x", hr);
				return false;
			}
		}
	}

	// Check Gpu upload heap support
	m_isSupportGpuUploadHeap = false;
	D3D12_FEATURE_DATA_D3D12_OPTIONS16 options = {};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options, sizeof(options));
	if (SUCCEEDED(hr))
	{
		m_isSupportGpuUploadHeap = options.GPUUploadHeapSupported;
	}

	return true;
}

void GraphicsDevice::Terminate()
{
	WaitIdle();

	for (int i = 0; i < MAX_COUNT_COMMAND_SIGNATURE_TYPE; ++i)
	{
		if (m_commandSignatures[i].Get() != nullptr)
		{
			m_commandSignatures[i].Reset();
		}
	}

	m_quadVb.Terminate();

	m_objectDisposer.Clear();
	// m_descriptorDisposer.Clear();

	m_graphicsQueue.reset();
	m_computeQueue.reset();
	m_copyQueue.reset();
	m_videoDecodeQueue.reset();
	m_videoProcessQueue.reset();
	m_videoEncodeQueue.reset();

	m_rtvHeap.Terminate();
	m_dsvHeap.Terminate();
	m_resourceHeap.Terminate();
	m_samplerHeap.Terminate();

	m_allocator.Reset();

	m_output.Reset();
	m_device.Reset();
	m_infoQueue.Reset();
	m_debug.Reset();
	m_adapter.Reset();
	m_factory.Reset();
}

ID3D12Device8* GraphicsDevice::GetDevice() const
{
	return m_device.Get();
}

IDXGIFactory7* GraphicsDevice::GetFactory() const
{
	return m_factory.Get();
}

D3D12MA::Allocator* GraphicsDevice::GetD3D12MA() const
{
	return m_allocator.Get();
}

std::weak_ptr<CommandQueue> GraphicsDevice::GetGraphicsQueue() const
{
	return m_graphicsQueue;
}

std::weak_ptr<CommandQueue> GraphicsDevice::GetComputeQueue() const
{
	return m_computeQueue;
}

std::weak_ptr<CommandQueue> GraphicsDevice::GetCopyQueue() const
{
	return m_copyQueue;
}

std::weak_ptr<CommandQueue> GraphicsDevice::GetVideoDecodeQueue() const
{
	return m_videoDecodeQueue;
}

std::weak_ptr<CommandQueue> GraphicsDevice::GetVideoProcessQueue() const
{
	return m_videoProcessQueue;
}

std::weak_ptr<CommandQueue> GraphicsDevice::GetVideoEncodeQueue() const
{
	return m_videoEncodeQueue;
}

DescriptorHeap* GraphicsDevice::GetRtvDescriptorHeap()
{
	return &m_rtvHeap;
}

DescriptorHeap* GraphicsDevice::GetDsvDescriptorHeap()
{
	return &m_dsvHeap;
}

DescriptorHeap* GraphicsDevice::GetResourceDescriptorHeap()
{
	return &m_resourceHeap;
}

DescriptorHeap* GraphicsDevice::GetSamplerDescriptorHeap()
{
	return &m_samplerHeap;
}

void GraphicsDevice::GetDisplayInfo(DXGI_FORMAT format, std::vector<DisplayInfo>& result)
{
	if (!m_output) { return; }
	
	UINT count = 0;
	HRESULT hr = m_output->GetDisplayModeList(format, DXGI_ENUM_MODES_SCALING, &count, nullptr);
	if (FAILED(hr) || count == 0) { return; }

	std::vector<DXGI_MODE_DESC> descList;
	descList.resize(count);

	hr = m_output->GetDisplayModeList(format, DXGI_ENUM_MODES_SCALING, &count, descList.data());
	if (FAILED(hr)) { return; }

	result.resize(count);
	for (size_t i = 0; i < result.size(); ++i)
	{
		result[i].m_width = descList[i].Width;
		result[i].m_height = descList[i].Height;
		result[i].m_refreshRate = descList[i].RefreshRate;
	}

	// sort bigger resolution.
	std::sort(result.begin(), result.end(), [](const DisplayInfo& lhs, const DisplayInfo& rhs)
		{
			auto refreshRateLhs = static_cast<double>(lhs.m_refreshRate.Numerator) / static_cast<double>(lhs.m_refreshRate.Denominator);
			auto refreshRateRhs = static_cast<double>(rhs.m_refreshRate.Numerator) / static_cast<double>(rhs.m_refreshRate.Denominator);
			return std::tie(lhs.m_width, lhs.m_height, refreshRateLhs) > std::tie(rhs.m_width, rhs.m_height, refreshRateRhs);
		});
}

bool GraphicsDevice::IsSupportGpuUploadHeap() const
{
	return m_isSupportGpuUploadHeap;
}

ID3D12Device8* GraphicsDevice::operator->() const
{
	return m_device.Get();
}

void GraphicsDevice::SetDescriptorHeaps(ID3D12GraphicsCommandList* commandList)
{
	if (commandList == nullptr) { return; }
	
	// can't use SetDescriptorHeaps for CopyCommandList.
	if (commandList->GetType() == D3D12_COMMAND_LIST_TYPE_COPY)
	{
		return;
	}

	ID3D12DescriptorHeap* heaps[] =
	{
		m_resourceHeap.m_heap,
		m_samplerHeap.m_heap
	};

	commandList->SetDescriptorHeaps(2, heaps);
}

void GraphicsDevice::WaitIdle()
{
	// todo: summarize
	if (m_graphicsQueue)
	{
		WaitPoint waitPoint = m_graphicsQueue->Signal();
		m_graphicsQueue->Sync(waitPoint);
	}

	if (m_computeQueue)
	{
		WaitPoint waitPoint = m_computeQueue->Signal();
		m_computeQueue->Sync(waitPoint);
	}

	if (m_copyQueue)
	{
		WaitPoint waitPoint = m_copyQueue->Signal();
		m_copyQueue->Sync(waitPoint);
	}

	if (m_videoDecodeQueue)
	{
		WaitPoint waitPoint = m_videoDecodeQueue->Signal();
		m_videoDecodeQueue->Sync(waitPoint);
	}

	if (m_videoEncodeQueue)
	{
		WaitPoint waitPoint = m_videoEncodeQueue->Signal();
		m_videoEncodeQueue->Sync(waitPoint);
	}

	if (m_videoProcessQueue)
	{
		WaitPoint waitPoint = m_videoProcessQueue->Signal();
		m_videoProcessQueue->Sync(waitPoint);
	}
}

void GraphicsDevice::Dispose(ID3D12Object*& resource)
{
	m_objectDisposer.Push(resource);
}

/*
void GraphicsDevice::Dispose(std::weak_ptr<Descriptor>& resource, uint8_t lifeTime)
{
	m_descriptorDisposer.Push(resource, lifeTime);
}
*/

void GraphicsDevice::FrameSync()
{
	m_objectDisposer.FrameSync();

	m_rtvHeap.FrameSync();
	m_dsvHeap.FrameSync();
	m_resourceHeap.FrameSync();
	m_samplerHeap.FrameSync();
}

void GraphicsDevice::ClearDisposer()
{
	m_objectDisposer.Clear();
	// m_descriptorDisposer.Clear();
}
