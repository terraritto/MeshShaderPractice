#include "DepthTarget.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Util.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

DepthTarget::DepthTarget()
{
}

DepthTarget::~DepthTarget()
{
    Terminate();
}

bool DepthTarget::Initialize(const TargetDesc* desc)
{
    if (desc == nullptr)
    {
        ELOGA("Error: DepthTarget Invalid Argument.");
        return false;
    }

    if (desc->m_dimension == D3D12_RESOURCE_DIMENSION_BUFFER ||
        desc->m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        ELOGA("Error: DepthTarget Invalid Resource Dimension.");
        return false;
    }

    HRESULT hr = S_OK;
    DXGI_FORMAT format = GetResourceFormat(desc->m_format, false);

    {
        D3D12_HEAP_PROPERTIES properties =
        {
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            1,
            1
        };

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
            D3D12_TEXTURE_LAYOUT_UNKNOWN,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        };

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = desc->m_format;
        clearValue.DepthStencil.Depth = desc->m_clearDepth;
        clearValue.DepthStencil.Stencil = desc->m_clearStencil;

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
                &clearValue,
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
                &clearValue,
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
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = desc->m_format;
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (desc->m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        if (desc->m_depthOrArraySize > 1)
        {
            // for Texture2DArray Settings
            if (desc->m_samplerDesc.Count > 1)
            {
                // MS
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                dsvDesc.Texture2DMSArray.ArraySize = desc->m_depthOrArraySize;
                dsvDesc.Texture2DMSArray.FirstArraySlice = 0;

                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                srvDesc.Texture2DMSArray.ArraySize = desc->m_depthOrArraySize;
                srvDesc.Texture2DMSArray.FirstArraySlice = 0;
            }
            else
            {
                // Default
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                dsvDesc.Texture2DArray.ArraySize = desc->m_depthOrArraySize;
                dsvDesc.Texture2DArray.FirstArraySlice = 0;
                dsvDesc.Texture2DArray.MipSlice = 0;

                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.ArraySize = desc->m_depthOrArraySize;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.MipLevels = desc->m_mipLevels;
                srvDesc.Texture2DArray.PlaneSlice = 0;
                srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
                srvDesc.Texture2DArray.MostDetailedMip = mostDetailedMip;
            }
        }
        else
        {
            // for Texture2D Settings
            if (desc->m_samplerDesc.Count > 1)
            {
                // MS
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                // Default
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = 0;

                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = desc->m_mipLevels;
                srvDesc.Texture2D.PlaneSlice = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;
            }
        }
    }
    else if (desc->m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
    {
        if (desc->m_depthOrArraySize > 1)
        {
            // MS
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            dsvDesc.Texture1DArray.ArraySize = desc->m_depthOrArraySize;
            dsvDesc.Texture1DArray.FirstArraySlice = 0;
            dsvDesc.Texture1DArray.MipSlice = 0;

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
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
            dsvDesc.Texture1D.MipSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            srvDesc.Texture1D.MipLevels = desc->m_mipLevels;
            srvDesc.Texture1D.MostDetailedMip = mostDetailedMip;
            srvDesc.Texture1D.ResourceMinLODClamp = 0;
        }
    }
    else if (desc->m_dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        // Not Support
        assert(false);
        ELOGA("Error: Not Support Buffer Type for ColorBuffer.");
        return false;
    }

    auto device = GraphicsProxy::GetD3D12Device();
    assert(device != nullptr);

    // Create DSV
    auto dsvHandle = GraphicsProxy::GetDsvDescriptorHeap()->Allocate(1);
    if (!dsvHandle.IsValid())
    {
        ELOGA("Error: DescriptorHeap::Alloc() Failed");
        return false;
    }

    m_dsvHolder = DescriptorHolder(DescriptorHolder::HEAP_DSV, dsvHandle);
    device->CreateDepthStencilView(m_resource.Get(), &dsvDesc, GetCpuHandleDSV());

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

    return true;
}

void DepthTarget::Terminate()
{
    m_allocationHolder.Reset();
    m_dsvHolder.Reset();
    m_srvHolder.Reset();

    ID3D12Resource* resource = m_resource.Detach();
    GraphicsProxy::Dispose(resource);

    memset(&m_desc, 0, sizeof(m_desc));
}

bool DepthTarget::Resize(uint32_t width, uint32_t height)
{
    TargetDesc desc = m_desc;
    Terminate();

    // resize
    desc.m_width = width;
    desc.m_height = height;
    return Initialize(&desc);
}

void DepthTarget::ChangeState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES nextState)
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

void DepthTarget::SetName(LPCWSTR tag)
{
    m_resource->SetName(tag);
}

ID3D12Resource* DepthTarget::GetResource() const
{
    return m_resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthTarget::GetCpuHandleDSV() const
{
    return m_dsvHolder.GetCpuHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthTarget::GetCpuHandleSRV() const
{
    return m_srvHolder.GetCpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE DepthTarget::GetGpuHandleSRV() const
{
    return m_srvHolder.GetGpuHandle();
}

uint32_t DepthTarget::GetBindlessIndexSRV() const
{
    return m_srvHolder.GetIndex();
}

TargetDesc DepthTarget::GetDesc() const
{
    return m_desc;
}

D3D12_RESOURCE_STATES DepthTarget::GetState() const
{
    return m_prevState;
}
