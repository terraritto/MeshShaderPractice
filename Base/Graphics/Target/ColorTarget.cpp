#include "ColorTarget.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Util.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

ColorTarget::ColorTarget()
{
}

ColorTarget::~ColorTarget()
{
    Terminate();
}

bool ColorTarget::Initialize(const TargetDesc* desc)
{
    if (desc == nullptr) 
    {
        ELOGA("Error: ColorTarget Invalid Argument."); 
        return false; 
    }
    
    if (desc->m_dimension == D3D12_RESOURCE_DIMENSION_BUFFER) 
    { 
        ELOGA("Error: ColorTarget Invalid Resource Dimension."); 
        return false; 
    }

    HRESULT hr = S_OK;

    {
        DXGI_FORMAT format = GetNoSRGBFormat(desc->m_format);

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
            format,
            desc->m_samplerDesc,
            D3D12_TEXTURE_LAYOUT_UNKNOWN,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
        };

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = format;
        for (int i = 0; i < _countof(clearValue.Color); i++)
        {
            clearValue.Color[i] = desc->m_clearColor[i];
        }

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
    }

    unsigned int mostDetailedMip = 0u;
       
    // prepare SRV/RTV view
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = desc->m_format;
    srvDesc.Format = desc->m_format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (desc->m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        // for Texture3D Settings
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        rtvDesc.Texture3D.FirstWSlice = 0;
        rtvDesc.Texture3D.MipSlice = 0;
        rtvDesc.Texture3D.WSize = desc->m_depthOrArraySize;

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MipLevels = desc->m_mipLevels;
        srvDesc.Texture3D.MostDetailedMip = mostDetailedMip;
        srvDesc.Texture3D.ResourceMinLODClamp = 0;
    }
    else if (desc->m_dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        if (desc->m_depthOrArraySize > 1)
        {
            // for Texture2DArray Settings
            if (desc->m_samplerDesc.Count > 1)
            {
                // MS
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                rtvDesc.Texture2DMSArray.ArraySize = desc->m_depthOrArraySize;
                rtvDesc.Texture2DMSArray.FirstArraySlice = 0;

                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                srvDesc.Texture2DMSArray.ArraySize = desc->m_depthOrArraySize;
                srvDesc.Texture2DMSArray.FirstArraySlice = 0;
            }
            else
            {
                // Default
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.ArraySize = desc->m_depthOrArraySize;
                rtvDesc.Texture2DArray.FirstArraySlice = 0;
                rtvDesc.Texture2DArray.MipSlice = 0;
                rtvDesc.Texture2DArray.PlaneSlice = 0;

                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.ArraySize = desc->m_depthOrArraySize;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.MipLevels = 0;
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
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                // Default
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;

                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 0;
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
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            rtvDesc.Texture1DArray.ArraySize = desc->m_depthOrArraySize;
            rtvDesc.Texture1DArray.FirstArraySlice = 0;
            rtvDesc.Texture1DArray.MipSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            srvDesc.Texture1DArray.ArraySize = desc->m_depthOrArraySize;
            srvDesc.Texture1DArray.FirstArraySlice = 0;
            srvDesc.Texture1DArray.MipLevels = desc->m_mipLevels;
            srvDesc.Texture1DArray.MostDetailedMip = mostDetailedMip;
            srvDesc.Texture1DArray.ResourceMinLODClamp = 0;
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

    // Create RTV
    auto handleRTV = GraphicsProxy::GetRtvDescriptorHeap()->Allocate(1);
    if (!handleRTV.IsValid())
    {
        ELOGA("Error: DescriptorHeap::Alloc() Failed");
        return false;
    }

    m_rtvHolder = DescriptorHolder(DescriptorHolder::HEAP_RTV, handleRTV);
    device->CreateRenderTargetView(m_resource.Get(), &rtvDesc, GetCpuHandleRTV());

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

bool ColorTarget::Initialize(IDXGISwapChain* swapChain, uint32_t backBufferIndex, bool sRGB)
{
    HRESULT hr = S_OK;
    
    hr = swapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(m_resource.GetAddressOf()));
    if (FAILED(hr))
    {
        ELOGA("Error: IDXGISwapChain::GetBuffer() Failed. errcode=0x%x", hr);
        return false;
    }

    D3D12_RESOURCE_DESC desc = m_resource->GetDesc();
    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        ELOGA("Error: ColorTarget Invalid Resource Dimension.");
        return false;
    }

    if (sRGB)
    {
        desc.Format = GetSRGBFormat(desc.Format);
    }

    unsigned int mostDetailedMip = 0u;

    // prepare SRV/RTV view
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = desc.Format;
    srvDesc.Format = desc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (desc.DepthOrArraySize > 1)
    {
        // for Texture2DArray Settings
        if (desc.SampleDesc.Count >= 1)
        {
            // MS
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            rtvDesc.Texture2DMSArray.ArraySize = desc.DepthOrArraySize;
            rtvDesc.Texture2DMSArray.FirstArraySlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            srvDesc.Texture2DMSArray.ArraySize = desc.DepthOrArraySize;
            srvDesc.Texture2DMSArray.FirstArraySlice = 0;
        }
        else
        {
            // Default
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
            rtvDesc.Texture2DArray.FirstArraySlice = 0;
            rtvDesc.Texture2DArray.MipSlice = 0;
            rtvDesc.Texture2DArray.PlaneSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
            srvDesc.Texture2DArray.PlaneSlice = 0;
            srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            srvDesc.Texture2DArray.MostDetailedMip = mostDetailedMip;
        }
    }
    else
    {
        // for Texture2D Settings
        if (desc.SampleDesc.Count > 1)
        {
            // MS
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            // Default
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;
            rtvDesc.Texture2D.PlaneSlice = 0;

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;
        }
    }

    auto device = GraphicsProxy::GetD3D12Device();
    assert(device != nullptr);

    // Create RTV
    auto handleRTV = GraphicsProxy::GetRtvDescriptorHeap()->Allocate(1);
    if (!handleRTV.IsValid())
    {
        ELOGA("Error: DescriptorHeap::Alloc() Failed");
        return false;
    }

    m_rtvHolder = DescriptorHolder(DescriptorHolder::HEAP_RTV, handleRTV);
    device->CreateRenderTargetView(m_resource.Get(), &rtvDesc, GetCpuHandleRTV());

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
    m_prevState = m_desc.m_initState;

    return true;
}

void ColorTarget::Terminate()
{
    m_allocationHolder.Reset();
    m_rtvHolder.Reset();
	m_srvHolder.Reset();

    ID3D12Resource* resource = m_resource.Detach();
    GraphicsProxy::Dispose(resource);

    memset(&m_desc, 0, sizeof(m_desc));
}

bool ColorTarget::Resize(uint32_t width, uint32_t height)
{
    TargetDesc desc = m_desc;
    Terminate();

    // resize
    desc.m_width = width;
    desc.m_height = height;
    return Initialize(&desc);
}

void ColorTarget::ChangeState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES nextState)
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

void ColorTarget::SetName(LPCWSTR tag)
{
    m_resource->SetName(tag);
}

ID3D12Resource* ColorTarget::GetResource() const
{
    return m_resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE ColorTarget::GetCpuHandleRTV() const
{
    return m_rtvHolder.GetCpuHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE ColorTarget::GetCpuHandleSRV() const
{
    return m_srvHolder.GetCpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE ColorTarget::GetGpuHandleSRV() const
{
    return m_srvHolder.GetGpuHandle();
}

uint32_t ColorTarget::GetBindlessIndexSRV() const
{
    return m_srvHolder.GetIndex();
}

TargetDesc ColorTarget::GetDesc() const
{
    return m_desc;
}

D3D12_RESOURCE_STATES ColorTarget::GetState() const
{
    return m_prevState;
}

bool ColorTarget::IsSRGB() const
{
    return IsSRGBFormat(m_desc.m_format);
}
