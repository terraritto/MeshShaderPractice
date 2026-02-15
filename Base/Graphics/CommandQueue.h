#pragma once
#include <memory>
#include "MeshShaderPractice/Base/Graphics/Fence.h"
#include "MeshShaderPractice/Base/Graphics/WaitPoint.h"

class CommandQueue
{
public:
	static bool Create(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, std::shared_ptr<CommandQueue>& result);

	// Execute Command
	void Execute(uint32_t count, ID3D12CommandList** commandList);

	// update fence
	WaitPoint Signal();

	// set wait point for GPU
	bool Wait(const WaitPoint& value);

	// wait for completed command in CPU
	void Sync(const WaitPoint& value, uint32_t msec = Fence::INFINITE_VALUE);

	ID3D12CommandQueue* GetQueue() const;

protected:
	CommandQueue();
	~CommandQueue();

private:

	bool Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
	void Terminate();

private:
	Fence m_fence;
	ComPtr<ID3D12CommandQueue> m_queue;
	std::atomic<uint32_t> m_counter;
	std::atomic<bool> m_isExecuted;
	UINT64 m_fenceValue;
};