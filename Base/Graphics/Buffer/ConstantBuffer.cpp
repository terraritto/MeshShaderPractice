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
    for (int i = 0; i < 2; i++)
    {
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
                IID_PPV_ARGS(m_resource[i].GetAddressOf()));
            if (FAILED(hr))
            {
                ELOGA("Error: D3D12MA::Allocator::CreateResource() Failed. errcode=0x%x", hr);
                return false;
            }

            m_holder[i].Attach(allocation);
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
                IID_PPV_ARGS(m_resource[i].GetAddressOf())
            );
            if (FAILED(hr))
            {
                ELOGA("Error: ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
                return false;
            }
        }
    }

    m_size = size;

    return true;
}

void ConstantBuffer::Terminate()
{
    for (int i = 0; i < 2; i++)
    {
        ID3D12Resource* resource = m_resource[i].Detach();
        GraphicsProxy::Dispose(resource);
        m_holder[i].Reset();
    }
    m_size = 0;
}

void ConstantBuffer::Update(const void* src, uint64_t size, uint64_t srcOffset, uint64_t dstOffset)
{
    auto dst = Map();
    auto copySize = (size > m_size) ? m_size : size;
    memcpy(dst, src, copySize);
    UnMap();
}

void ConstantBuffer::Swap()
{
    m_index = (m_index + 1) & 0x1;
}

void* ConstantBuffer::Map(uint32_t index)
{
    if (m_resource[index].Get() == nullptr) { return nullptr; }

    void* data = nullptr;
    HRESULT hr = m_resource[index]->Map(0, nullptr, &data);
    if (FAILED(hr))
    {
        ELOGA("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
        return nullptr;
    }
    return data;
}

void* ConstantBuffer::Map()
{
    return Map(m_index);
}

void ConstantBuffer::UnMap(uint32_t index)
{
    if (m_resource[index].Get() == nullptr) { return; }

    m_resource[index]->Unmap(0, nullptr);
}

void ConstantBuffer::UnMap()
{
    UnMap(m_index);
}

void ConstantBuffer::SetDebugName(LPCWSTR tag)
{
    for (int i = 0; i < 2; i++)
    {
        if (m_resource[i].Get())
        {
            m_resource[i]->SetName(tag);
        }
    }
}

ID3D12Resource* ConstantBuffer::GetResource() const
{
    return m_resource[m_index].Get();
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetGpuAddress() const
{
    D3D12_GPU_VIRTUAL_ADDRESS result = {};
    if (m_resource[m_index].Get() != nullptr)
    {
        result = m_resource[m_index]->GetGPUVirtualAddress();
    }
    return result;
}

uint64_t ConstantBuffer::GetSize() const
{
    return m_size;
}
