#pragma once
#include <d3d12.h>
#include <stdint.h>
#include "MeshShaderPractice/Base/Util.h"

class CommandQueue;

class WaitPoint
{
public:
	WaitPoint();
	~WaitPoint();
	WaitPoint& operator=(const WaitPoint& value);

	// check that fence and fence value is valid
	bool IsValid() const;

private:
	UINT64 m_fenceValue = 0;
	ID3D12Fence* m_fence = nullptr;

private:
	friend class CommandQueue;
};