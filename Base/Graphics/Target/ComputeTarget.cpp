#include "ComputeTarget.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Graphics/Target/ComputeTarget.h"
#include "MeshShaderPractice/Base/Util.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

ComputeTarget::ComputeTarget()
{
}

ComputeTarget::~ComputeTarget()
{
    Terminate();
}

bool ComputeTarget::Initialize(const TargetDesc* desc, uint32_t stride)
{
    if (desc == nullptr)
    {
        ELOGA("Error: ComputeTarget Invalid Argument.");
        return false;
    }

    if (desc->m_samplerDesc.Count > 1)
    {
        ELOGA("Error: ComputeTarget Invalid Resource Dimension.");
        return false;
    }

    HRESULT hr = S_OK;

    {
        D3D12_HEAP_PROPERTIES properties =
        {
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            1,
            1
        };

        D3D12_TEXTURE_LAYOUT layout = desc->m_dimension == D3D12_RESOURCE_DIMENSION_BUFFER ? D3D12_TEXTURE_LAYOUT_ROW_MAJOR : D3D12_TEXTURE_LAYOUT_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc =
        {
            desc->m_dimension,
            desc->m_alignment,
            desc->m_width,
            desc->m_height,
            desc->m_depthOrArraySize,
            desc->m_mipLevels,
            desc->m_format,
            desc->m_samplerDesc,
            layout,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        };

        D3D12MA::Allocator* allocator = GraphicsProxy::GetD3D12MA();
        if (allocator != nullptr)
        {
            D3D12MA::ALLOCATION_DESC allocationDesc = {};
            allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

            ComPtr<D3D12MA::Allocation> allocation = nullptr;
            hr = allocator->CreateResource
            (
                &allocationDesc,
                &resourceDesc,
                desc->m_initState,
                nullptr,
                &allocation,
                IID_PPV_ARGS(m_resource.GetAddressOf())
            );

            if (FAILED(hr))
            {
                ELOGA("Error: D3D12MA::Allocator::CrateResource() Failed. errcode = 0x%x", hr);
                return false;
            }

            m_allocationHolder.Attach(allocation);
        }
        else
        {
            hr = GraphicsProxy::GetD3D12Device()->CreateCommittedResource
            (
                &properties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                desc->m_initState,
                nullptr,
                IID_PPV_ARGS(m_resource.GetAddressOf())
            );
            if (FAILED(hr))
            {
                ELOGA("Error: ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
                return false;
            }
        }
    }

    unsigned int mostDetailedMip = 0u;

    // prepare SRV/DSV view
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = desc->m_format;
    srvDesc.Format = desc->m_format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (desc->m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.FirstWSlice = 0;
        uavDesc.Texture3D.MipSlice = 0;
        uavDesc.Texture3D.WSize = desc->m_depthOrArraySize;

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MipLevels = desc->m_mipLevels;
        srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
        srvDesc.Texture3D.MostDetailedMip = mostDetailedMip;
    }
    else if (desc->m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        // for Texture2D Settings
        if (desc->m_depthOrArraySize > 1)
        {
            // Array
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray.ArraySize = desc->m_depthOrArraySize;
            uavDesc.Texture2DArray.FirstArraySlice = 0;
            uavDesc.Texture2DArray.MipSlice = 0;
            uavDesc.Texture2DArray.PlaneSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.ArraySize = desc->m_depthOrArraySize;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.MipLevels = 0;
            srvDesc.Texture2DArray.PlaneSlice = 0;
            srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            srvDesc.Texture2DArray.MostDetailedMip = mostDetailedMip;
        }
        else
        {
            // Default
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0;
            uavDesc.Texture2D.PlaneSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc->m_mipLevels;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;
        }
    }
    else if (desc->m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
    {
        if (desc->m_depthOrArraySize > 1)
        {
            // MS
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            uavDesc.Texture1DArray.ArraySize = desc->m_depthOrArraySize;
            uavDesc.Texture1DArray.FirstArraySlice = 0;
            uavDesc.Texture1DArray.MipSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            srvDesc.Texture1DArray.ArraySize = desc->m_depthOrArraySize;
            srvDesc.Texture1DArray.FirstArraySlice = 0;
            srvDesc.Texture1DArray.MipLevels = desc->m_mipLevels;
            srvDesc.Texture1DArray.MostDetailedMip = mostDetailedMip;
            srvDesc.Texture1DArray.ResourceMinLODClamp = 0;
        }
        else
        {
            // Default
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uavDesc.Texture1D.MipSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            srvDesc.Texture1D.MipLevels = desc->m_mipLevels;
            srvDesc.Texture1D.MostDetailedMip = mostDetailedMip;
            srvDesc.Texture1D.ResourceMinLODClamp = 0;
        }
    }
    else if (desc->m_dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        uint64_t elements = stride == 0 ? desc->m_width : desc->m_width / stride;

        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.CounterOffsetInBytes = 0;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = UINT(elements);
        uavDesc.Buffer.StructureByteStride = stride;
        uavDesc.Buffer.Flags = stride == 0 ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
        uavDesc.Buffer.CounterOffsetInBytes = 0;

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = UINT(elements);
        srvDesc.Buffer.StructureByteStride = stride;
        srvDesc.Buffer.Flags = (stride == 0) ? D3D12_BUFFER_SRV_FLAG_RAW: D3D12_BUFFER_SRV_FLAG_NONE;
    }

    auto device = GraphicsProxy::GetD3D12Device();
    assert(device != nullptr);

    // Create UAV
    auto uavHandle = GraphicsProxy::GetResourceDescriptorHeap()->Allocate(1);
    if (!uavHandle.IsValid())
    {
        ELOGA("Error: DescriptorHeap::Alloc() Failed");
        return false;
    }

    m_uavHolder = DescriptorHolder(DescriptorHolder::HEAP_RES, uavHandle);
    device->CreateUnorderedAccessView(m_resource.Get(), nullptr, &uavDesc, GetCpuHandleUAV());

    // Create SRV
    auto handleSRV = GraphicsProxy::GetResourceDescriptorHeap()->Allocate(1);
    if (!handleSRV.IsValid())
    {
        ELOGA("Error: DescriptorHeap::Alloc() Failed");
        return false;
    }

    m_srvHolder = DescriptorHolder(DescriptorHolder::HEAP_RES, handleSRV);
    device->CreateShaderResourceView(m_resource.Get(), &srvDesc, GetCpuHandleSRV());

    // save
    memcpy(&m_desc, desc, sizeof(m_desc));
    m_prevState = m_desc.m_initState;
    m_stride = stride;

    return true;
}

bool ComputeTarget::Initialize(ColorTarget& target)
{
    m_resource = target.GetResource();

    TargetDesc desc = target.GetDesc();

    // prepare SRV/DSV view
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = desc.m_format;
    srvDesc.Format = desc.m_format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (desc.m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.FirstWSlice = 0;
        uavDesc.Texture3D.MipSlice = 0;
        uavDesc.Texture3D.WSize = desc.m_depthOrArraySize;

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MipLevels = desc.m_mipLevels;
        srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
        srvDesc.Texture3D.MostDetailedMip = 0;
    }
    else if (desc.m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        // for Texture2D Settings
        if (desc.m_depthOrArraySize > 1)
        {
            // Array
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray.ArraySize = desc.m_depthOrArraySize;
            uavDesc.Texture2DArray.FirstArraySlice = 0;
            uavDesc.Texture2DArray.MipSlice = 0;
            uavDesc.Texture2DArray.PlaneSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.ArraySize = desc.m_depthOrArraySize;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.MipLevels = 0;
            srvDesc.Texture2DArray.PlaneSlice = 0;
            srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            srvDesc.Texture2DArray.MostDetailedMip = 0;
        }
        else
        {
            // Default
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0;
            uavDesc.Texture2D.PlaneSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.m_mipLevels;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Texture2D.MostDetailedMip = 0;
        }
    }
    else if (desc.m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
    {
        if (desc.m_depthOrArraySize > 1)
        {
            // MS
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            uavDesc.Texture1DArray.ArraySize = desc.m_depthOrArraySize;
            uavDesc.Texture1DArray.FirstArraySlice = 0;
            uavDesc.Texture1DArray.MipSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            srvDesc.Texture1DArray.ArraySize = desc.m_depthOrArraySize;
            srvDesc.Texture1DArray.FirstArraySlice = 0;
            srvDesc.Texture1DArray.MipLevels = desc.m_mipLevels;
            srvDesc.Texture1DArray.MostDetailedMip = 0;
            srvDesc.Texture1DArray.ResourceMinLODClamp = 0;
        }
        else
        {
            // Default
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uavDesc.Texture1D.MipSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            srvDesc.Texture1D.MipLevels = desc.m_mipLevels;
            srvDesc.Texture1D.MostDetailedMip = 0;
            srvDesc.Texture1D.ResourceMinLODClamp = 0;
        }
    }
    else if (desc.m_dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        // Not Support
        assert(false);
        ELOGA("Error: Not Support Buffer Type for ColorBuffer.");
        return false;
    }

    auto device = GraphicsProxy::GetD3D12Device();
    assert(device != nullptr);

    // Create UAV
    auto uavHandle = GraphicsProxy::GetResourceDescriptorHeap()->Allocate(1);
    if (!uavHandle.IsValid())
    {
        ELOGA("Error: DescriptorHeap::Alloc() Failed");
        return false;
    }

    m_uavHolder = DescriptorHolder(DescriptorHolder::HEAP_RES, uavHandle);
    device->CreateUnorderedAccessView(m_resource.Get(), nullptr, &uavDesc, GetCpuHandleUAV());

    // Create SRV
    auto handleSRV = GraphicsProxy::GetResourceDescriptorHeap()->Allocate(1);
    if (!handleSRV.IsValid())
    {
        ELOGA("Error: DescriptorHeap::Alloc() Failed");
        return false;
    }

    m_srvHolder = DescriptorHolder(DescriptorHolder::HEAP_RES, handleSRV);
    device->CreateShaderResourceView(m_resource.Get(), &srvDesc, GetCpuHandleSRV());

    // save
    memcpy(&m_desc, &desc, sizeof(m_desc));
    m_prevState = m_desc.m_initState;

    return true;
}

void ComputeTarget::Terminate()
{
    m_allocationHolder.Reset();
    m_uavHolder.Reset();
    m_srvHolder.Reset();

    ID3D12Resource* resource = m_resource.Detach();
    GraphicsProxy::Dispose(resource);

    memset(&m_desc, 0, sizeof(m_desc));
    m_stride = 0;
}

bool ComputeTarget::Resize(uint32_t width, uint32_t height, uint16_t depth)
{
    TargetDesc desc = m_desc;
    uint32_t stride = m_stride;
    Terminate();

    // resize
    desc.m_width = width;
    desc.m_height = height;
    desc.m_depthOrArraySize = depth;
    return Initialize(&desc, stride);
}

void ComputeTarget::ChangeState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES nextState)
{
    // change state unnecessary.
    if (nextState == m_prevState) { return; }

    // barrier
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_resource.Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = m_prevState;
    barrier.Transition.StateAfter = nextState;
    cmdList->ResourceBarrier(1, &barrier);

    // change current state.
    m_prevState = nextState;
}

void ComputeTarget::SetName(LPCWSTR tag)
{
    m_resource->SetName(tag);
}

void ComputeTarget::UAVBarrier(ID3D12GraphicsCommandList* commandList)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = m_resource.Get();

    commandList->ResourceBarrier(1, &barrier);
}

ID3D12Resource* ComputeTarget::GetResource() const
{
    return m_resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE ComputeTarget::GetCpuHandleUAV() const
{
    return m_uavHolder.GetCpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE ComputeTarget::GetGpuHandleUAV() const
{
    return m_uavHolder.GetGpuHandle();
}

uint32_t ComputeTarget::GetBindlessIndexUAV() const
{
    return m_uavHolder.GetIndex();
}

D3D12_CPU_DESCRIPTOR_HANDLE ComputeTarget::GetCpuHandleSRV() const
{
    return m_srvHolder.GetCpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE ComputeTarget::GetGpuHandleSRV() const
{
    return m_srvHolder.GetGpuHandle();
}

uint32_t ComputeTarget::GetBindlessIndexSRV() const
{
    return m_srvHolder.GetIndex();
}

TargetDesc ComputeTarget::GetDesc() const
{
    return m_desc;
}

uint32_t ComputeTarget::GetStride() const
{
    return m_stride;
}

D3D12_RESOURCE_STATES ComputeTarget::GetState() const
{
    return m_prevState;
}
