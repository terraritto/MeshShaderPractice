#include "StructuredBuffer.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

StructuredBuffer::StructuredBuffer()
    : m_resource()
    , m_state(D3D12_RESOURCE_STATE_COMMON)
{
}

StructuredBuffer::~StructuredBuffer()
{
    Terminate();
}

bool StructuredBuffer::Initialize(uint64_t count, uint32_t stride, D3D12_RESOURCE_STATES state)
{
    uint64_t size = count * stride;
    uint64_t rest = size % 4;
    if (rest != 0)
    {
        ELOGA("Error : StructuredBuffer must be 4byte alignment., (size %% 4) = %u", rest);
        return false;
    }

    auto device = GraphicsProxy::GetD3D12Device();
    if (device == nullptr || count == 0 || stride == 0)
    {
        ELOGA("Error : Invalid Argument.");
        return false;
    }

    D3D12_HEAP_PROPERTIES property = {};
    property.Type = D3D12_HEAP_TYPE_DEFAULT;
    property.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    property.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    property.VisibleNodeMask = 1;
    property.CreationNodeMask = 1;

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

    D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;

    D3D12MA::Allocator* allocator = GraphicsProxy::GetD3D12MA();
    if (allocator != nullptr)
    {
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

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
            &property,
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

    m_state = state;

    return true;
}

bool StructuredBuffer::Initialize(ID3D12GraphicsCommandList* command, uint64_t count, uint32_t stride, const void* initData)
{
    if (GraphicsProxy::IsSupportGpuUploadHeap() == false)
    {
        uint64_t size = count * stride;
        uint64_t rest = size % 4;
        if (rest != 0)
        {
            ELOGA("Error : StructuredBuffer must be 4byte alignment., (size %% 4) = %u", rest);
            return false;
        }

        auto device = GraphicsProxy::GetD3D12Device();
        if (device == nullptr || count == 0 || stride == 0)
        {
            ELOGA("Error : Invalid Argument.");
            return false;
        }

        D3D12_HEAP_PROPERTIES property = {};
        property.Type = D3D12_HEAP_TYPE_GPU_UPLOAD;
        property.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        property.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        property.VisibleNodeMask = 1;
        property.CreationNodeMask = 1;

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

        D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;

        D3D12MA::Allocator* allocator = GraphicsProxy::GetD3D12MA();
        if (allocator != nullptr)
        {
            D3D12MA::ALLOCATION_DESC allocDesc = {};
            allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

            ComPtr<D3D12MA::Allocation> allocation = nullptr;

            HRESULT hr = allocator->CreateResource
            (
                &allocDesc,
                &desc,
                D3D12_RESOURCE_STATE_COMMON,
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
                &property,
                flags,
                &desc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(m_resource.GetAddressOf())
            );
            if (FAILED(hr))
            {
                ELOGA("Error: ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
                return false;
            }
        }

        m_state = D3D12_RESOURCE_STATE_COMMON;
    }
    else
    {
        if (!Initialize(count, stride, D3D12_RESOURCE_STATE_COMMON))
        {
            return false;
        }
    }

    // update
    GraphicsProxy::UpdateBuffer(command, m_resource.Get(), initData);

    // barrier
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_resource.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
        barrier.Transition.Subresource = 0;

        command->ResourceBarrier(1, &barrier);
    }

    m_state = D3D12_RESOURCE_STATE_GENERIC_READ;

    return true;
}

void StructuredBuffer::Terminate()
{
    ID3D12Resource* resource = m_resource.Detach();
    GraphicsProxy::Dispose(resource);
    m_holder.Reset();
}

void StructuredBuffer::UAVBarrier(ID3D12GraphicsCommandList* command)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = m_resource.Get();

    command->ResourceBarrier(1, &barrier);
}

void StructuredBuffer::ChangeState(ID3D12GraphicsCommandList* command, D3D12_RESOURCE_STATES state)
{
    if (m_state == state) { return; }

    D3D12_RESOURCE_STATES prev = m_state;
    D3D12_RESOURCE_STATES next = state;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_resource.Get();
    barrier.Transition.StateBefore = prev;
    barrier.Transition.StateAfter = next;

    command->ResourceBarrier(1, &barrier);

    m_state = next;
}

void StructuredBuffer::SetDebugName(LPCWSTR tag)
{
    if (m_resource) { m_resource->SetName(tag); }
}

ID3D12Resource* StructuredBuffer::GetResource() const
{
    return m_resource.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS StructuredBuffer::GetGpuAddress() const
{
    D3D12_GPU_VIRTUAL_ADDRESS result = {};
    if (m_resource.Get() != nullptr)
    {
        result = m_resource->GetGPUVirtualAddress();
    }

    return result;
}

D3D12_RESOURCE_STATES StructuredBuffer::GetState() const
{
    return m_state;
}
