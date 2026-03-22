#include "ConstantBuffer.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

ConstantBuffer::ConstantBuffer()
{
}

ConstantBuffer::~ConstantBuffer()
{
    Terminate();
}

bool ConstantBuffer::Initialize(uint64_t size)
{
    if (size == 0)
    {
        ELOGA("Error : Invalid Argument.");
        return false;
    }

    uint64_t rest = size % 256;
    if (rest != 0)
    {
        ELOGA("Error : ConstantBuffer must be 256 byte alignment!! (size %% 256) = %u", rest);
        return false;
    }

    auto device = GraphicsProxy::GetD3D12Device();

    //resource
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
    desc.Alignment = 0;
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

    m_size = size;

    return true;
}

void ConstantBuffer::Terminate()
{
    ID3D12Resource* resource = m_resource.Detach();
    GraphicsProxy::Dispose(resource);
    m_holder.Reset();
    m_size = 0;
}

void* ConstantBuffer::Map()
{
    if (m_resource.Get() == nullptr) { return nullptr; }

    void* data = nullptr;
    HRESULT hr = m_resource->Map(0, nullptr, &data);
    if (FAILED(hr))
    {
        ELOGA("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
        return nullptr;
    }
    return data;
}

void ConstantBuffer::UnMap()
{
    if (m_resource.Get() == nullptr) { return; }

    m_resource->Unmap(0, nullptr);
}

void ConstantBuffer::SetDebugName(LPCWSTR tag)
{
    if (m_resource.Get())
    {
        m_resource->SetName(tag);
    }
}

ID3D12Resource* ConstantBuffer::GetResource() const
{
    return m_resource.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetGpuAddress() const
{
    D3D12_GPU_VIRTUAL_ADDRESS result = {};
    if (m_resource.Get() != nullptr)
    {
        result = m_resource->GetGPUVirtualAddress();
    }
    return result;
}

uint64_t ConstantBuffer::GetSize() const
{
    return m_size;
}
