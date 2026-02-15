#pragma once
#include <d3d12.h>
#include <stdint.h>
#include "MeshShaderPractice/Base/Util.h"

class Fence
{
public:
	Fence();
	~Fence();

	bool Initialize(ID3D12Device* device);
	void Terminate();

	// wait until fence value reached specific value.
	void Wait(UINT64 fenceValue, uint32_t msec = INFINITE_VALUE);

	ID3D12Fence* GetRowFence() const;

public:
	static constexpr uint32_t IGNORE_VALUE = 0;
	static constexpr uint32_t INFINITE_VALUE = 0xFFFFFFFF;

private:
	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_handle;
};