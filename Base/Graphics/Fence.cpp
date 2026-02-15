#include "Fence.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

Fence::Fence()
    : m_fence()
    , m_handle(nullptr)
{
}

Fence::~Fence()
{
    Terminate();
}

bool Fence::Initialize(ID3D12Device* device)
{
    if (device == nullptr)
    {
        ELOGA("Error: Fence Device not found.");
        return false;
    }

    // Create Event
    m_handle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    if (m_handle == nullptr)
    {
        ELOGA("Error: CreateEventW() Failed.");
        return false;
    }

    // Create Fence
    HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));
    if (FAILED(hr))
    {
        ELOGA("Error: ID3D12Device::CreateFence() Failed. errcode = 0x%x", hr);
        return false;
    }

    m_fence->SetName(L"MyFence");
    return true;
}

void Fence::Terminate()
{
    if (m_handle != nullptr)
    {
        CloseHandle(m_handle);
        m_handle = nullptr;
    }

    m_fence.Reset();
}

void Fence::Wait(UINT64 fenceValue, uint32_t msec)
{
    if (m_fence->GetCompletedValue() < fenceValue)
    {
        HRESULT hr = m_fence->SetEventOnCompletion(fenceValue, m_handle);
        if (FAILED(hr))
        {
            ELOGA("Error: ID3D12Fence::SetEventOnCompletion() Failed. errcode = 0x%x", hr);
            return;
        }

        WaitForSingleObject(m_handle, msec);
    }
}

ID3D12Fence* Fence::GetRowFence() const
{
    return m_fence.Get();
}
