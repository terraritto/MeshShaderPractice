#include "DescriptorHolder.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"

DescriptorHolder::DescriptorHolder()
	: m_heapType(HEAP_NONE)
	, m_handle()
{
}

DescriptorHolder::DescriptorHolder(DescriptorHolder::HEAP_TYPE heapType, OffsetHandle handle)
	: m_heapType(heapType)
	, m_handle(handle)
{
}

DescriptorHolder::~DescriptorHolder()
{
	Reset();
}

void DescriptorHolder::Reset()
{
	if (!m_handle.IsValid()) { return; }

	switch (m_heapType)
	{
	case HEAP_RTV: GraphicsProxy::GetRtvDescriptorHeap()->Free(m_handle); break;
	case HEAP_DSV: GraphicsProxy::GetDsvDescriptorHeap()->Free(m_handle); break;
	case HEAP_RES: GraphicsProxy::GetResourceDescriptorHeap()->Free(m_handle); break;
	case HEAP_SMP: GraphicsProxy::GetSamplerDescriptorHeap()->Free(m_handle); break;
	default: break;
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHolder::GetCpuHandle() const
{
	D3D12_CPU_DESCRIPTOR_HANDLE result = {};

	if (m_handle.IsValid())
	{
		switch (m_heapType)
		{
		case HEAP_RTV: result = GraphicsProxy::GetRtvDescriptorHeap()->GetHandleCPU(m_handle); break;
		case HEAP_DSV: result = GraphicsProxy::GetDsvDescriptorHeap()->GetHandleCPU(m_handle); break;
		case HEAP_RES: result = GraphicsProxy::GetResourceDescriptorHeap()->GetHandleCPU(m_handle); break;
		case HEAP_SMP: result = GraphicsProxy::GetSamplerDescriptorHeap()->GetHandleCPU(m_handle); break;
		default: break;
		}
	}
	return result;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHolder::GetGpuHandle() const
{
	D3D12_GPU_DESCRIPTOR_HANDLE result = {};

	if (m_handle.IsValid())
	{
		switch (m_heapType)
		{
		case HEAP_RES: result = GraphicsProxy::GetResourceDescriptorHeap()->GetHandleGPU(m_handle); break;
		case HEAP_SMP: result = GraphicsProxy::GetSamplerDescriptorHeap()->GetHandleGPU(m_handle); break;
		default: break;
		}
	}
	return result;
}

uint32_t DescriptorHolder::GetIndex() const
{
	return m_handle.GetOffset();
}

bool DescriptorHolder::IsValid() const
{
	return m_handle.IsValid();
}
