#include "CommandList.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

CommandList::CommandList()
	: m_allocator()
	, m_commandList()
	, m_index(0)
{
}

CommandList::~CommandList()
{
	Terminate();
}

bool CommandList::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
	if (device == nullptr)
	{
		ELOGA("Error: CommandList Invalid Argument.");
		return false;
	}

	HRESULT hr = S_OK;

	// create CommandAllocator
	for (int i = 0; i < 2; ++i)
	{
		hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(m_allocator[i].GetAddressOf()));
		if (FAILED(hr))
		{
			ELOGA("Error: ID3D12Device::CreateCommandAllocator failed. errcode = 0x%x", hr);
			return false;
		}
	}

	hr = device->CreateCommandList
	(
		0,
		type,
		m_allocator[0].Get(),
		nullptr,
		IID_PPV_ARGS(m_commandList.GetAddressOf())
	);
	if (FAILED(hr))
	{
		ELOGA("Error: ID3D12Device::CreateCommandList() Failed. errcode = 0x%x", hr);
		return false;
	}

	m_commandList->Close();
	m_index = 0;

	return true;
}

void CommandList::Terminate()
{
	m_commandList.Reset();

	for (int i = 0; i < 2; ++i)
	{
		m_allocator[i].Reset();
	}
}

ID3D12GraphicsCommandList6* CommandList::Reset()
{
	// doudle buffering
	m_index = (m_index + 1) & 0x1;

	// reset command allocator
	m_allocator[m_index]->Reset();

	// reset command list
	m_commandList->Reset(m_allocator[m_index].Get(), nullptr);

	// set descriptor heaps
	GraphicsProxy::SetDescriptorHeaps(m_commandList.Get());

	return m_commandList.Get();
}

ID3D12CommandAllocator* CommandList::GetD3D12CommandAllocator(uint8_t index) const
{
	assert(index < 2);
	return m_allocator[index].Get();
}

ID3D12GraphicsCommandList6* CommandList::GetD3D12CommandList() const
{
	return m_commandList.Get();
}

uint8_t CommandList::GetIndex() const
{
	return m_index;
}

void CommandList::SetName(LPCWSTR name)
{
	m_commandList->SetName(name);
}
