#include "CommandQueue.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

CommandQueue::CommandQueue()
    : m_fence()
    , m_queue()
    , m_counter(1)
    , m_fenceValue(0)
{
}

CommandQueue::~CommandQueue()
{
    Terminate();
}

bool CommandQueue::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
    if (device == nullptr)
    {
        ELOGA("Error : CommandQueue Device not found.");
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC desc =
    {
        type,
        0,
        D3D12_COMMAND_QUEUE_FLAG_NONE
    };

    HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_queue.GetAddressOf()));
    if (FAILED(hr))
    {
        ELOGA("Error : ID3D12Device::CreateCommandQueue() Failed. errcodes = 0x%x", hr);
        return false;
    }
    m_queue->SetName(L"MyQueue");

    if (!m_fence.Initialize(device))
    {
        ELOGA("Error : Fence::Init() Failed.");
        return false;
    }

    m_isExecuted = false;
    m_fenceValue = 1;

    return true;
}

void CommandQueue::Terminate()
{
    m_queue.Reset();
    m_fence.Terminate();
    m_isExecuted = false;
    m_fenceValue = 0;
}

void CommandQueue::Execute(uint32_t count, ID3D12CommandList** commandList)
{
    if (count == 0 || commandList == nullptr)
    {
        return;
    }

    m_queue->ExecuteCommandLists(count, commandList);
    m_isExecuted = true;
}

WaitPoint CommandQueue::Signal()
{
    WaitPoint result;

    // set fence to update it from GPU.
    // when GPU process all command, fence is updated.
    const auto fence = m_fenceValue;
    HRESULT hr = m_queue->Signal(m_fence.GetRowFence(), fence);
    if (FAILED(hr))
    {
        ELOGA("Error : Fence::Init() Failed.");
        return result;
    }

    // change fence value(CPU side).
    m_fenceValue++;

    result.m_fenceValue = fence;
    result.m_fence = m_fence.GetRowFence();

    return result;

}

bool CommandQueue::Wait(const WaitPoint& value)
{
    // Wait for fence updated from GPU.
    // this function is used to sync command queue execute order.
    HRESULT hr = m_queue->Wait(value.m_fence, value.m_fenceValue);
    if (FAILED(hr))
    {
        ELOGA("Error : ID3D12CommandQueue::Wait() Failed.");
        return false;
    }
    return true;
}

void CommandQueue::Sync(const WaitPoint& value, uint32_t msec)
{
    // wait for fence specified time or fence updated.
    if (!m_isExecuted) { return; }
    m_fence.Wait(value.m_fenceValue, msec);
}

ID3D12CommandQueue* CommandQueue::GetQueue() const
{
    return m_queue.Get();
}

bool CommandQueue::Create(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, std::shared_ptr<CommandQueue>& result)
{
    if (device == nullptr)
    {
        ELOGA("Error : CommandQueue Device not found.");
        return false;
    }

    class Impl : public CommandQueue {
    public:
        Impl() : CommandQueue() {}
    };

    auto queue = std::make_shared<Impl>();
    if (!queue)
    {
        ELOGA("Error : CommandQueue out of memory.");
        return false;
    }

    if (!queue->Initialize(device, type))
    {
        queue.reset();
        ELOGA("Error : CommandQueue::Init() Failed.");
        return false;
    }

    result = queue;

    return true;
}
