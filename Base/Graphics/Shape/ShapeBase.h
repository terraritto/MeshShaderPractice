#pragma once
#include "MeshShaderPractice/Base/Graphics/Buffer/ConstantBuffer.h"
#include "MeshShaderPractice/Base/Graphics/Buffer/IndexBuffer.h"
#include "MeshShaderPractice/Base/Graphics/Buffer/VertexBuffer.h"
#include "MeshShaderPractice/Base/Util.h"

class ShapeBase
{
public:
	ShapeBase();
	~ShapeBase();

	void Draw(ID3D12GraphicsCommandList* command);

	// setter
	void SetWorld(const XMMATRIX& value);
	void SetColor(const XMVECTOR& value);

	// getter
	const XMMATRIX& GetWorld() const;
	const XMVECTOR& GetColor() const;

protected:
	bool InitBuffer(std::vector<XMFLOAT3>& positions, std::vector<uint32_t>& indices);
	void Reset();
	D3D12_GPU_VIRTUAL_ADDRESS UpdateParameter();

protected:
	struct alignas(256) ShapeParam
	{
		XMMATRIX m_world;
		XMVECTOR m_color;
	};

protected:
	VertexBuffer m_vertexBuffer;
	IndexBuffer m_indexBuffer;
	ConstantBuffer m_constantBuffer;
	ShapeParam m_parameter;
	uint8_t m_bufferIndex = 0;
	uint32_t m_indexCount = 0;
};