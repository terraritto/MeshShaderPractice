#pragma once
#include "MeshShaderPractice/Base/Util.h" 

namespace D3D12MA { class Allocation; }

class AllocationHolder
{
public:
	AllocationHolder();
	explicit AllocationHolder(ComPtr<D3D12MA::Allocation> value);
	~AllocationHolder();

	void Reset();
	
	void Attach(ComPtr<D3D12MA::Allocation> value);
	ComPtr<D3D12MA::Allocation> Detach();

private:
	ComPtr<D3D12MA::Allocation> m_allocation = nullptr;
};