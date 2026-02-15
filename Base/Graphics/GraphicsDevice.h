#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <array>
#include "MeshShaderPractice/Base/Graphics/CommandQueue.h"
#include "MeshShaderPractice/Base/Graphics/DescriptorHeap.h"
#include "MeshShaderPractice/Base/Graphics/Buffer/VertexBuffer.h"
#include "MeshShaderPractice/Base/Util.h"
#include "MeshShaderPractice/Base/Util/SpinLock.h"
#include "MeshShaderPractice/Base/Graphics/Disposer.h"
#include "MeshShaderPractice/Base/Graphics/WeakDisposer.h"
#include "MeshShaderPractice/External/D3D12MA/D3D12MemAlloc.h"

class GraphicsProxy;

enum COMMAND_SIGNATURE_TYPE
{
	COMMAND_SIGNATURE_TYPE_DRAW,
	COMMAND_SIGNATURE_TYPE_DRAW_INDEXED,
	COMMAND_SIGNATURE_TYPE_DISPATCH,
	COMMAND_SIGNATURE_TYPE_DISPATCH_RAYS,
	COMMAND_SIGNATURE_TYPE_DISPATCH_MESH,
	MAX_COUNT_COMMAND_SIGNATURE_TYPE
};

struct DisplayInfo
{
	uint32_t m_width;
	uint32_t m_height;
	DXGI_RATIONAL m_refreshRate;
};

struct DeviceDesc
{
	uint32_t MaxShaderResourceCount;
	uint32_t MaxSamplerCount;
	uint32_t MaxColorTargetCount;
	uint32_t MaxDepthTargetCount;
	bool EnableDebug = false;
	bool EnableDRED = true;
	bool EnableBreakOnWarning = false;
	bool EnableBreakOnError = true;
};

class GraphicsDevice
{
protected:
	static GraphicsDevice& Instance();

	bool Initialize(const DeviceDesc& deviceDesc);
	void Terminate();

	// Getter
	ID3D12Device8* GetDevice() const;
	IDXGIFactory7* GetFactory() const;
	D3D12MA::Allocator* GetD3D12MA() const;
	std::weak_ptr<CommandQueue> GetGraphicsQueue() const;
	std::weak_ptr<CommandQueue> GetComputeQueue() const;
	std::weak_ptr<CommandQueue> GetCopyQueue() const;
	std::weak_ptr<CommandQueue> GetVideoDecodeQueue() const;
	std::weak_ptr<CommandQueue> GetVideoProcessQueue() const;
	std::weak_ptr<CommandQueue> GetVideoEncodeQueue() const;
	DescriptorHeap* GetRtvDescriptorHeap();
	DescriptorHeap* GetDsvDescriptorHeap();
	DescriptorHeap* GetResourceDescriptorHeap();
	DescriptorHeap* GetSamplerDescriptorHeap();
	void GetDisplayInfo(DXGI_FORMAT format, std::vector<DisplayInfo>& result);
	bool IsSupportGpuUploadHeap() const;

	// operator
	ID3D12Device8* operator->() const;

	// descriptor heap
	void SetDescriptorHeaps(ID3D12GraphicsCommandList* commandList);

	// wait gpu
	void WaitIdle();

	// Disposer
	void Dispose(ID3D12Object*& resource);
	// void Dispose(std::weak_ptr<Descriptor>& resource, uint8_t lifeTime);
	void FrameSync();
	void ClearDisposer();

protected:
	const VertexBuffer& GetQuadVB() const { return m_quadVb; }

	ID3D12CommandSignature* GetCommandSignature(COMMAND_SIGNATURE_TYPE type) const
	{
		return m_commandSignatures[type].Get();
	}

private:
	struct QuadVertex
	{
		float m_position[2];
		float m_texCoord[2];

		QuadVertex(float x, float y, float u, float v)
		{
			m_position[0] = x; m_position[1] = y;
			m_texCoord[0] = u; m_texCoord[1] = v;
		}
	};

private:
	// debug
	ComPtr<ID3D12Debug3> m_debug;

	// IDXGI
	ComPtr<IDXGIFactory7> m_factory;
	ComPtr<IDXGIAdapter1> m_adapter;
	ComPtr<IDXGIOutput6> m_output;
	ComPtr<ID3D12Device14> m_device;
	ComPtr<ID3D12InfoQueue> m_infoQueue;
	ComPtr<D3D12MA::Allocator> m_allocator;
	DescriptorHeap m_rtvHeap;
	DescriptorHeap m_dsvHeap;
	DescriptorHeap m_samplerHeap;
	DescriptorHeap m_resourceHeap;
	std::shared_ptr<CommandQueue> m_graphicsQueue;
	std::shared_ptr<CommandQueue> m_computeQueue;
	std::shared_ptr<CommandQueue> m_copyQueue;
	std::shared_ptr<CommandQueue> m_videoDecodeQueue;
	std::shared_ptr<CommandQueue> m_videoProcessQueue;
	std::shared_ptr<CommandQueue> m_videoEncodeQueue;
	std::array<ComPtr<ID3D12CommandSignature>, MAX_COUNT_COMMAND_SIGNATURE_TYPE> m_commandSignatures;

	bool m_isSupportGpuUploadHeap;

	VertexBuffer m_quadVb;
	std::shared_ptr<SpinLock> m_spinLock; // todo: delete?

	Disposer<ID3D12Object> m_objectDisposer;
	// WeakDisposer<Descriptor> m_descriptorDisposer;

private:
	static GraphicsDevice m_instance;

private:
	friend class GraphicsProxy; // Access Allowed Proxy Only.
};