#include "VertexBuffer.h"
#include <cassert>
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

VertexBuffer::VertexBuffer()
    : m_isSrv(false)
{
	memset(&m_view, 0, sizeof(m_view));
}

VertexBuffer::~VertexBuffer()
{
	Terminate();
}

bool VertexBuffer::Initialize(uint64_t size, uint32_t stride, bool isSrv)
{
	auto device = GraphicsProxy::GetD3D12Device();

	if (device == nullptr || size == 0 || stride == 0)
	{
		ELOGA("Error: VertexBuffer Invalid Argument.");
		return false;
	}

    D3D12_HEAP_TYPE heapType = GraphicsProxy::IsSupportGpuUploadHeap()
        ? D3D12_HEAP_TYPE_GPU_UPLOAD
        : D3D12_HEAP_TYPE_UPLOAD;

    // heap property
    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type = heapType;
    prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    prop.CreationNodeMask = 1;
    prop.VisibleNodeMask = 1;

    // resource
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;

    // allocate
    D3D12MA::Allocator* allocator = GraphicsProxy::GetD3D12MA();
    if (allocator != nullptr)
    {
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = heapType;

        ComPtr<D3D12MA::Allocation> allocation = nullptr;

        HRESULT hr = allocator->CreateResource
        (
            &allocDesc,
            &desc,
            state,
            nullptr,
            allocation.GetAddressOf(),
            IID_PPV_ARGS(m_resource.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOGA("Error: D3D12MA::Allocator::CreateResource() Failed. errcode=0x%x", hr);
            return false;
        }

        m_holder.Attach(allocation);
    }
    else
    {


        HRESULT hr = device->CreateCommittedResource
        (
            &prop,
            flags,
            &desc,
            state,
            nullptr,
            IID_PPV_ARGS(m_resource.GetAddressOf())
        );
        if (FAILED(hr))
        {
            ELOGA("Error: ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
            return false;
        }
    }

    // set view
    m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
    m_view.SizeInBytes = UINT(size);
    m_view.StrideInBytes = stride;

    // SRV
    m_isSrv = isSrv;

    if (m_isSrv)
    {
        // alloc
        auto handleSRV = GraphicsProxy::GetResourceDescriptorHeap()->Allocate(1);
        if (!handleSRV.IsValid())
        {
            ELOGA("Error: DescriptorHeap::Alloc() Failed.");
            return false;
        }

        // resource
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = size / stride;
        srvDesc.Buffer.StructureByteStride = stride;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        m_srvHolder = DescriptorHolder(DescriptorHolder::HEAP_RES, handleSRV);
        device->CreateShaderResourceView(m_resource.Get(), &srvDesc, GetCpuHandleSRV());
    }

    return true;
}

void VertexBuffer::Terminate()
{
    ID3D12Resource* resource = m_resource.Detach();
    GraphicsProxy::Dispose(resource);
    memset(&m_view, 0, sizeof(m_view));
    m_srvHolder.Reset();
    m_holder.Reset();
}

void* VertexBuffer::Map()
{
    if (m_resource.Get() == nullptr)
    {
        return nullptr;
    }

    void* pointer = nullptr;
    HRESULT hr = m_resource->Map(0, nullptr, &pointer);
    if (FAILED(hr))
    {
        ELOGA("Error: ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
        return nullptr;
    }

    return pointer;
}

void VertexBuffer::UnMap()
{
    if (m_resource.Get() == nullptr)
    {
        return;
    }

    m_resource->Unmap(0, nullptr);
}

void VertexBuffer::SetDebugName(LPCWSTR tag)
{
    if (m_resource)
    {
        m_resource->SetName(tag);
    }
}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetView() const
{
	return m_view;
}

ID3D12Resource* VertexBuffer::GetResource() const
{
	return m_resource.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS VertexBuffer::GetGpuAddress() const
{
    D3D12_GPU_VIRTUAL_ADDRESS result = {};
    if (m_resource.Get() != nullptr)
    {
        result = m_resource->GetGPUVirtualAddress();
    }
    return result;
}

D3D12_CPU_DESCRIPTOR_HANDLE VertexBuffer::GetCpuHandleSRV() const
{
    assert(m_isSrv);
    return m_srvHolder.GetCpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE VertexBuffer::GetGpuHandleSRV() const
{
    assert(m_isSrv);
    return m_srvHolder.GetGpuHandle();
}
