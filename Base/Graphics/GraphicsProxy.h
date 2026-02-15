#pragma once
#include <d3d12.h>
#include "MeshShaderPractice/Base/Graphics/GraphicsDevice.h"
#include "MeshShaderPractice/Base/Graphics/PipelineState.h"

class GraphicsProxy
{
public:
	static bool Initialize(const DeviceDesc& desc);
	static void Terminate();

	static void WaitIdle();
	static void FrameSync();

	// Setter
	static void SetDescriptorHeaps(ID3D12GraphicsCommandList* command);

	// Getter
	static std::weak_ptr<CommandQueue> GetGraphicsQueue();
	static std::weak_ptr<CommandQueue> GetComputeQueue();
	static std::weak_ptr<CommandQueue> GetCopyQueue();
	static std::weak_ptr<CommandQueue> GetVideoProcessQueue();
	static std::weak_ptr<CommandQueue> GetVideoEncodeQueue();
	static std::weak_ptr<CommandQueue> GetVideoDecodeQueue();
	static DescriptorHeap* GetRtvDescriptorHeap();
	static DescriptorHeap* GetDsvDescriptorHeap();
	static DescriptorHeap* GetResourceDescriptorHeap();
	static DescriptorHeap* GetSamplerDescriptorHeap();
	static ID3D12Device8* GetD3D12Device();
	static IDXGIFactory7* GetDXGIFactory();
	static D3D12MA::Allocator* GetD3D12MA();
	static void GetDisplayInfo(DXGI_FORMAT format, std::vector<DisplayInfo>& result);

	// Condition
	static bool IsSupportGpuUploadHeap();

	// Resource
	static void UpdateSubResources(ID3D12GraphicsCommandList* commandList, ID3D12Resource* dstResource, uint32_t subResourceCount, uint32_t subResourceOffset, const D3D12_SUBRESOURCE_DATA* subResources);
	static void UpdateBuffer(ID3D12GraphicsCommandList* commandList, ID3D12Resource* dstResource, const void* srcResource);
	// todo: when use texture, expand this function.
	//static void UpdateTexture(ID3D12GraphicsCommandList* commandList, ID3D12Resource* dstResource, const ResTexture* resTexture);
	static void UAVBarrier(ID3D12GraphicsCommandList* command, ID3D12Resource* resource);

	// PipelineState
	static bool CompileShader(const std::wstring fileName, const std::wstring profile, ComPtr<IDxcBlob>& byte, ComPtr<IDxcBlob>& errorBlob);

	// Disposer
	static void inline DisposeObject(ID3D12Object*& resource)
	{
		GraphicsDevice::Instance().Dispose(resource);
	}
	// static void DisposeDescriptor(std::weak_ptr<Descriptor>& resource);
	static void ClearDisposer();
	template<class T>
	static void inline Dispose(T*& ptr)
	{
		auto casted = reinterpret_cast<ID3D12Object*>(ptr);
		DisposeObject(casted);
	}

private:
	// Resource
	static UINT64 GetRequiredIntermediateSize(ID3D12Device* device, D3D12_RESOURCE_DESC* desc, UINT firstSubResource, UINT subResourceCount) noexcept;
	static void CopySubresource(const D3D12_MEMCPY_DEST* dst, const D3D12_SUBRESOURCE_DATA* src, SIZE_T rowSizeInBytes, UINT rowCount, UINT sliceCount) noexcept;
};