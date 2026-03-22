#include "GraphicsProxy.h"
#include <filesystem>
#include <assert.h>
#include "MeshShaderPractice/Base/Graphics/GraphicsDevice.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

bool GraphicsProxy::Initialize(const DeviceDesc& desc)
{
    return GraphicsDevice::Instance().Initialize(desc);
}

void GraphicsProxy::Terminate()
{
    GraphicsDevice::Instance().Terminate();
}

void GraphicsProxy::WaitIdle()
{
    GraphicsDevice::Instance().WaitIdle();
}

void GraphicsProxy::FrameSync()
{
    GraphicsDevice::Instance().FrameSync();
}

void GraphicsProxy::SetDescriptorHeaps(ID3D12GraphicsCommandList* command)
{
    GraphicsDevice::Instance().SetDescriptorHeaps(command);
}

std::weak_ptr<CommandQueue> GraphicsProxy::GetGraphicsQueue()
{
    return GraphicsDevice::Instance().GetGraphicsQueue();
}

std::weak_ptr<CommandQueue> GraphicsProxy::GetComputeQueue()
{
    return GraphicsDevice::Instance().GetComputeQueue();
}

std::weak_ptr<CommandQueue> GraphicsProxy::GetCopyQueue()
{
    return GraphicsDevice::Instance().GetCopyQueue();
}

std::weak_ptr<CommandQueue> GraphicsProxy::GetVideoProcessQueue()
{
    return GraphicsDevice::Instance().GetVideoProcessQueue();
}

std::weak_ptr<CommandQueue> GraphicsProxy::GetVideoEncodeQueue()
{
    return GraphicsDevice::Instance().GetVideoEncodeQueue();
}

std::weak_ptr<CommandQueue> GraphicsProxy::GetVideoDecodeQueue()
{
    return GraphicsDevice::Instance().GetVideoDecodeQueue();
}

ID3D12Device8* GraphicsProxy::GetD3D12Device()
{
    return GraphicsDevice::Instance().GetDevice();
}

IDXGIFactory7* GraphicsProxy::GetDXGIFactory()
{
    return GraphicsDevice::Instance().GetFactory();
}

D3D12MA::Allocator* GraphicsProxy::GetD3D12MA()
{
    return GraphicsDevice::Instance().GetD3D12MA();
}

DescriptorHeap* GraphicsProxy::GetRtvDescriptorHeap()
{
    return GraphicsDevice::Instance().GetRtvDescriptorHeap();
}

DescriptorHeap* GraphicsProxy::GetDsvDescriptorHeap()
{
    return GraphicsDevice::Instance().GetDsvDescriptorHeap();
}

DescriptorHeap* GraphicsProxy::GetResourceDescriptorHeap()
{
    return GraphicsDevice::Instance().GetResourceDescriptorHeap();
}

DescriptorHeap* GraphicsProxy::GetSamplerDescriptorHeap()
{
    return GraphicsDevice::Instance().GetSamplerDescriptorHeap();
}

void GraphicsProxy::GetDisplayInfo(DXGI_FORMAT format, std::vector<DisplayInfo>& result)
{
    return GraphicsDevice::Instance().GetDisplayInfo(format, result);
}

bool GraphicsProxy::IsSupportGpuUploadHeap()
{
    return GraphicsDevice::Instance().IsSupportGpuUploadHeap();
}

bool GraphicsProxy::IsUseMeshlet()
{
    return GraphicsDevice::Instance().IsUseMeshlet();
}

void GraphicsProxy::UpdateSubResources(ID3D12GraphicsCommandList* commandList, ID3D12Resource* dstResource, uint32_t subResourceCount, uint32_t subResourceOffset, const D3D12_SUBRESOURCE_DATA* subResources)
{
    // Invalid data settings.
    if (commandList == nullptr || dstResource == nullptr || subResources == nullptr || subResourceCount == 0)
    {
        return;
    }

    auto device = GetD3D12Device();
    auto dstDesc = dstResource->GetDesc();

    // resource desc
    D3D12_RESOURCE_DESC uploadDesc =
    {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        GetRequiredIntermediateSize(device, &dstDesc, subResourceOffset, subResourceCount),
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        {1,0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_NONE
    };
    
    // properties
    D3D12_HEAP_PROPERTIES properties =
    {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        1,
        1
    };

    // Create Resource
    ID3D12Resource* srcResource = nullptr;
    HRESULT hr = device->CreateCommittedResource
    (
        &properties,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&srcResource)
    );
    if (FAILED(hr))
    {
        ELOGA("Error: ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
        return;
    }

    // Create Command
    {
        uint32_t count = subResourceCount;
        
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts; layouts.resize(count);
        std::vector<UINT> rows; rows.resize(count);
        std::vector<UINT64> rowSizeInBytes; rowSizeInBytes.resize(count);

        UINT64 requiredSize = 0;
        device->GetCopyableFootprints
        (
            &dstDesc,
            subResourceOffset,
            count,
            0,
            layouts.data(), // subresource layout
            rows.data(), // sobresource row
            rowSizeInBytes.data(), // subresoure size in bytes
            &requiredSize // TotalBytes
        );

        // Mapping for source resource
        BYTE* data = nullptr;
        hr = srcResource->Map(0, nullptr, reinterpret_cast<void**>(&data));
        if (FAILED(hr))
        {
            ELOGA("Error: ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
            srcResource->Release();
            return;
        }

        for (auto i = 0u; i < count; ++i)
        {
            D3D12_MEMCPY_DEST dstData = {};
            dstData.pData = data + layouts[i].Offset;
            dstData.RowPitch = layouts[i].Footprint.RowPitch;
            dstData.SlicePitch = SIZE_T(layouts[i].Footprint.RowPitch) * SIZE_T(rows[i]);

            // write memory
            CopySubresource
            (
                &dstData,
                &subResources[i],
                SIZE_T(rowSizeInBytes[i]),
                rows[i],
                layouts[i].Footprint.Depth
            );
        }

        // unmapping
        srcResource->Unmap(0, nullptr);

        // Push Command for copy from source to dst.
        if (dstDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            // Copy for Buffer
            commandList->CopyBufferRegion
            (
                dstResource,
                0,
                srcResource,
                layouts[0].Offset,
                layouts[0].Footprint.Width
            );
        }
        else
        {
            for (auto i = 0u; i < count; ++i)
            {
                D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
                dstLocation.pResource = dstResource;
                dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dstLocation.SubresourceIndex = i + subResourceOffset;

                D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
                srcLocation.pResource = srcResource;
                srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                srcLocation.PlacedFootprint = layouts[i];

                // Copy for Texture
                commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
            }
        }
    }

    // delete source resource.
    Dispose(srcResource);
}

void GraphicsProxy::UpdateBuffer(ID3D12GraphicsCommandList* commandList, ID3D12Resource* dstResource, const void* srcResource)
{
    // Invalid data settings.
    if (commandList == nullptr || srcResource == nullptr || dstResource == nullptr)
    {
        return;
    }

    auto desc = dstResource->GetDesc();

    D3D12_SUBRESOURCE_DATA resource = {};
    resource.pData = srcResource;
    resource.RowPitch = desc.Width;
    resource.SlicePitch = desc.Width;

    // Update
    UpdateSubResources(commandList, dstResource, 1, 0, &resource);
}

void GraphicsProxy::UAVBarrier(ID3D12GraphicsCommandList* command, ID3D12Resource* resource)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource;

    command->ResourceBarrier(1, &barrier);
}

bool GraphicsProxy::CompileShader(const std::wstring fileName, const std::wstring profile, ComPtr<IDxcBlob>& byte, ComPtr<IDxcBlob>& errorBlob)
{
    if (fileName.empty() || profile.empty())
    {
        return false;
    }

    using namespace std::filesystem;

    HRESULT hr;

    // reference
    // https://simoncoenen.com/blog/programming/graphics/DxcCompiling
    ComPtr<IDxcUtils> utils;
    hr = ::DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf()));

    ComPtr<IDxcBlobEncoding> source;
    hr = utils->LoadFile(fileName.c_str(), nullptr, &source);

    if (source == NULL || FAILED(hr))
    {
        throw std::runtime_error("shader not found");
    }

    ComPtr<IDxcCompiler3> compiler;
    hr = ::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.GetAddressOf()));

    // construct agrument
    std::vector<LPCWSTR> arguments;
    // entry point
    arguments.push_back(L"-E");
    arguments.push_back(L"main");

    // profile (Example: ps_6_0,ms_6_5)
    arguments.push_back(L"-T");
    arguments.push_back(profile.c_str());

    arguments.push_back(DXC_ARG_DEBUG); //-Zi
    arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL0); // -O0

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = source->GetBufferPointer();
    sourceBuffer.Size = source->GetBufferSize();
    sourceBuffer.Encoding = 0;

    ComPtr<IDxcResult> compileResult;

    // default include handler
    ComPtr<IDxcIncludeHandler> includeHandler;
    hr = utils->CreateDefaultIncludeHandler(&includeHandler);

    hr = compiler->Compile(&sourceBuffer, arguments.data(), (UINT)arguments.size(), *includeHandler.GetAddressOf(), IID_PPV_ARGS(compileResult.GetAddressOf()));

    // error process
    ComPtr<IDxcBlobUtf8> errors{};
    ComPtr<IDxcBlobUtf16> outputName{};
    hr = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
    if ((errors && errors->GetStringLength() > 0) || FAILED(hr))
    {
        const char* errorText = static_cast<const char*>(errors->GetBufferPointer());
        OutputDebugStringA(errorText);
        return false;
    }

    compileResult->GetStatus(&hr);

    if (SUCCEEDED(hr))
    {
        compileResult->GetResult(&byte);
    }
    else
    {
        compileResult->GetErrorBuffer(
            reinterpret_cast<IDxcBlobEncoding**>(errorBlob.GetAddressOf())
        );
    }

    return SUCCEEDED(hr);
}

/*
void GraphicsProxy::DisposeDescriptor(std::weak_ptr<Descriptor>& resource)
{
    GraphicsDevice::Instance().Dispose(resource, DEFAULT_WEAK_LIFE_TIME);
}
*/

void GraphicsProxy::ClearDisposer()
{
    GraphicsDevice::Instance().ClearDisposer();
}

UINT64 GraphicsProxy::GetRequiredIntermediateSize(ID3D12Device* device, D3D12_RESOURCE_DESC* desc, UINT firstSubResource, UINT subResourceCount) noexcept
{
    UINT64 requiredSize = 0;
    device->GetCopyableFootprints
    (
        desc,
        firstSubResource,
        subResourceCount,
        0,
        nullptr,
        nullptr,
        nullptr,
        &requiredSize // Get TotalBytes
    );
    return requiredSize;
}

void GraphicsProxy::CopySubresource(const D3D12_MEMCPY_DEST* dst, const D3D12_SUBRESOURCE_DATA* src, SIZE_T rowSizeInBytes, UINT rowCount, UINT sliceCount) noexcept
{
    for (auto z = 0u; z < sliceCount; ++z)
    {
        // calculate slice
        auto dstSlice = static_cast<BYTE*>(dst->pData) + dst->SlicePitch * z;
        auto srcSlice = static_cast<const BYTE*>(src->pData) + src->SlicePitch * LONG_PTR(z);

        // copy memory
        for (auto y = 0u; y < rowCount; ++y)
        {
            memcpy(dstSlice + dst->RowPitch * y, srcSlice + src->RowPitch * LONG_PTR(y), rowSizeInBytes);
        }
    }
}

