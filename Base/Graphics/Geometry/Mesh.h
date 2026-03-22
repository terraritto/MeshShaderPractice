#pragma once
#include "MeshShaderPractice/Base/Graphics/Resource/ResourceMesh.h"
#include "MeshShaderPractice/Base/Graphics/Buffer/VertexBuffer.h"
#include "MeshShaderPractice/Base/Graphics/Buffer/IndexBuffer.h"
#include "MeshShaderPractice/Base/Graphics/Buffer/StructuredBuffer.h"

class Mesh
{
public:
	Mesh();
	~Mesh();

	bool Initialize(ID3D12GraphicsCommandList* command, const ResourceMesh& mesh);
	void Terminate();

	const std::string& GetName() const;
	const VertexBuffer& GetPositions() const;
	const IndexBuffer& GetIndices() const;

	const uint32_t GetVertexCount() const;
	const uint32_t GetIndexCount() const;

	const StructuredBuffer& GetMeshlets() const;
	const StructuredBuffer& GetUniqueVertexIndices() const;
	const StructuredBuffer& GetPrimitiveIndices() const;

	const uint32_t GetMeshletCount() const;

private:
	std::string m_name;
	VertexBuffer m_positionsBuffer;
	IndexBuffer m_indicesBuffer;

	uint32_t m_vertexCount = 0;
	uint32_t m_indexCount = 0;
	
	// for meshlet
	StructuredBuffer m_meshletsBuffer;
	StructuredBuffer m_uniqueVertexIndicesBuffer;
	StructuredBuffer m_primitiveIndicesBuffer;

	uint32_t m_meshletCount = 0;
};