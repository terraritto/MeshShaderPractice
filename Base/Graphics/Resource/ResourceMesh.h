#pragma once
#include "MeshShaderPractice/Base/Util.h"
#include "MeshShaderPractice/Base/Graphics/Resource/ResourceMeshlet.h"

// Hold pure Mesh resource
class ResourceMesh
{
public:
	size_t GetVerticesNum() const;
	size_t GetIndicesNum() const;

protected:
	std::string m_name;
	std::vector<XMFLOAT3> m_positions;
	std::vector<uint32_t> m_indices;

	// for meshlet
	std::vector<ResourceMeshlet> m_meshlets;
	std::vector<uint32_t> m_uniqueVertexIndices;
	std::vector<uint32_t> m_primitiveIndices;

private:
	friend class ResourceModel;
	friend class Mesh;
};