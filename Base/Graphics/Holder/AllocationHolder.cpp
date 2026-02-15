#include "AllocationHolder.h"
#include "MeshShaderPractice/External/D3D12MA/D3D12MemAlloc.h"

AllocationHolder::AllocationHolder()
	: m_allocation(nullptr)
{
}

AllocationHolder::AllocationHolder(ComPtr<D3D12MA::Allocation> value)
	: m_allocation(value)
{
}

AllocationHolder::~AllocationHolder()
{
	m_allocation.Reset();
}

void AllocationHolder::Reset()
{
	m_allocation.Reset();
}

void AllocationHolder::Attach(ComPtr<D3D12MA::Allocation> value)
{
	m_allocation.Reset();
	m_allocation = value;
}

ComPtr<D3D12MA::Allocation> AllocationHolder::Detach()
{
	ComPtr<D3D12MA::Allocation> allocation = m_allocation;
	m_allocation.Reset();
	return allocation;
}
