#include "ShapeBase.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

ShapeBase::ShapeBase()
{
	m_parameter.m_world = DirectX::XMMatrixIdentity();
	m_parameter.m_color = DirectX::g_XMOne;
}

ShapeBase::~ShapeBase()
{
	Reset();
}

void ShapeBase::Draw(ID3D12GraphicsCommandList* command)
{
	auto address = UpdateParameter();
	auto vb = m_vertexBuffer.GetView();
	auto ib = m_indexBuffer.GetView();
	command->IASetVertexBuffers(0, 1, &vb);
	command->IASetIndexBuffer(&ib);
	command->SetGraphicsRootConstantBufferView(1, address);
	command->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
}

void ShapeBase::SetWorld(const XMMATRIX& value)
{
	m_parameter.m_world = value;
}

void ShapeBase::SetColor(const XMVECTOR& value)
{
	m_parameter.m_color = value;
}

const XMMATRIX& ShapeBase::GetWorld() const
{
	return m_parameter.m_world;
}

const XMVECTOR& ShapeBase::GetColor() const
{
	return m_parameter.m_color;
}

bool ShapeBase::InitBuffer(std::vector<XMFLOAT3>& positions, std::vector<uint32_t>& indices)
{
	auto device = GraphicsProxy::GetD3D12Device();

	// Vertex
	{
		if (!m_vertexBuffer.Initialize(sizeof(XMFLOAT3) * positions.size(), sizeof(XMFLOAT3)))
		{
			ELOGA("Error: Positions Initialize Failed.");
			return false;
		}

		// Mapping
		void* data = m_vertexBuffer.Map();
		memcpy(data, positions.data(), sizeof(XMFLOAT3) * positions.size());
		m_vertexBuffer.UnMap();
	}

	// Index
	{
		if (!m_indexBuffer.Initialize(sizeof(uint32_t) * indices.size()))
		{
			ELOGA("Error: Indices Initialize Failed.");
			return false;
		}

		// Mapping
		void* data = m_indexBuffer.Map();
		memcpy(data, indices.data(), sizeof(uint32_t) * indices.size());
		m_indexBuffer.UnMap();

		m_indexCount = static_cast<uint32_t>(indices.size());
	}
	
	// Constant Buffer
	{
		if (!m_constantBuffer.Initialize(sizeof(ShapeParam) * 2))
		{
			return false;
		}
	}

	return true;
}

void ShapeBase::Reset()
{
	// term
	m_vertexBuffer.Terminate();
	m_indexBuffer.Terminate();
	m_constantBuffer.Terminate();
	
	// init
	m_bufferIndex = 0;
	m_parameter.m_world = DirectX::XMMatrixIdentity();
	m_parameter.m_color = DirectX::g_XMOne;
}

D3D12_GPU_VIRTUAL_ADDRESS ShapeBase::UpdateParameter()
{
	auto size = sizeof(ShapeParam);
	auto offset = m_bufferIndex * size;

	// write
	uint8_t* data = nullptr;
	data = m_constantBuffer.MapAs<uint8_t>();
	if (data == nullptr) { return D3D12_GPU_VIRTUAL_ADDRESS(); }
	memcpy(data + offset, &m_parameter, sizeof(ShapeParam));

	// swap
	m_bufferIndex = (m_bufferIndex + 1) & 0x1;

	return m_constantBuffer.GetGpuAddress() + offset;
}
