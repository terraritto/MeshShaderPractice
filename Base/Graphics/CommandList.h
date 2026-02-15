#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include "MeshShaderPractice/Base/Util.h"

class CommandList
{
public:
	CommandList();
	~CommandList();

	bool Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
	void Terminate();
	ID3D12GraphicsCommandList6* Reset();

	// Getter
	ID3D12CommandAllocator* GetD3D12CommandAllocator(uint8_t index) const;
	ID3D12GraphicsCommandList6* GetD3D12CommandList() const;
	uint8_t GetIndex() const;
	void SetName(LPCWSTR name);

private:
	std::array<ComPtr<ID3D12CommandAllocator>, 2> m_allocator;
	ComPtr<ID3D12GraphicsCommandList6> m_commandList;
	uint8_t m_index;
};