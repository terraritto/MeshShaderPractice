#include "Mesh.h"
#include "MeshShaderPractice/Base/Graphics/GraphicsProxy.h"
#include "MeshShaderPractice/Base/Util/Logger.h"

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
    Terminate();
}

bool Mesh::Initialize(ID3D12GraphicsCommandList* command, const ResourceMesh& mesh)
{
    bool isUseMeshlet = GraphicsProxy::IsUseMeshlet();
    m_name = mesh.m_name;
    
    // position
    {
        const std::vector<XMFLOAT3>& positions = mesh.m_positions;
        if (!m_positionsBuffer.Initialize(sizeof(XMFLOAT3) * positions.size(), sizeof(XMFLOAT3), isUseMeshlet))
        {
            ELOGA("Error: Positions Initialize Failed.");
            return false;
        }

        // Mapping
        void* data = m_positionsBuffer.Map();
        memcpy(data, positions.data(), sizeof(XMFLOAT3) * positions.size());
        m_positionsBuffer.UnMap();

        m_vertexCount = static_cast<uint32_t>(mesh.m_positions.size());
    }

    // index
    {
        const std::vector<uint32_t>& indices = mesh.m_indices;
        if (!m_indicesBuffer.Initialize(sizeof(uint32_t) * indices.size(), isUseMeshlet))
        {
            ELOGA("Error: Indices Initialize Failed.");
            return false;
        }

        // Mapping
        void* data = m_indicesBuffer.Map();
        memcpy(data, indices.data(), sizeof(uint32_t) * indices.size());
        m_indicesBuffer.UnMap();

        m_indexCount = static_cast<uint32_t>(mesh.m_indices.size());
    }


    // if isn't use meshlet, meshlet buffer don't create meshlet buffer.
    if (!isUseMeshlet) { return true; }

    // meshlet
    {
        const std::vector<ResourceMeshlet>& meshlets = mesh.m_meshlets;
        if (!m_meshletsBuffer.Initialize(command, meshlets.size(), sizeof(ResourceMeshlet), meshlets.data()))
        {
            ELOGA("Error: Meshlets Initialize Failed.");
            return false;
        }

        m_meshletCount = static_cast<uint32_t>(meshlets.size());
    }

    // unique vertex indices
    {
        const std::vector<uint32_t>& uniqueVertexIndices = mesh.m_uniqueVertexIndices;
        if (!m_uniqueVertexIndicesBuffer.Initialize(command, uniqueVertexIndices.size(), sizeof(uint32_t), uniqueVertexIndices.data()))
        {
            ELOGA("Error: UniqueVertexIndices Initialize Failed.");
            return false;
        }
    }

    // primitive indices
    {
        const std::vector<uint32_t>& primitiveIndices = mesh.m_primitiveIndices;
        if (!m_primitiveIndicesBuffer.Initialize(command, primitiveIndices.size(), sizeof(uint32_t), primitiveIndices.data()))
        {
            ELOGA("Error: PrimitiveIndices Initialize Failed.");
            return false;
        }
    }

    return true;
}

void Mesh::Terminate()
{
    m_name.clear();
    m_vertexCount = 0;
    m_indexCount = 0;

    m_positionsBuffer.Terminate();
    m_indicesBuffer.Terminate();

    if (GraphicsProxy::IsUseMeshlet())
    {
        m_meshletsBuffer.Terminate();
        m_uniqueVertexIndicesBuffer.Terminate();
        m_primitiveIndicesBuffer.Terminate();
    }
}

const std::string& Mesh::GetName() const
{
    return m_name;
}

const VertexBuffer& Mesh::GetPositions() const
{
    return m_positionsBuffer;
}

const IndexBuffer& Mesh::GetIndices() const
{
    return m_indicesBuffer;
}

const uint32_t Mesh::GetVertexCount() const
{
    return m_vertexCount;
}

const uint32_t Mesh::GetIndexCount() const
{
    return m_indexCount;
}

const StructuredBuffer& Mesh::GetMeshlets() const
{
    return m_meshletsBuffer;
}

const StructuredBuffer& Mesh::GetUniqueVertexIndices() const
{
    return m_uniqueVertexIndicesBuffer;
}

const StructuredBuffer& Mesh::GetPrimitiveIndices() const
{
    return m_primitiveIndicesBuffer;
}

const uint32_t Mesh::GetMeshletCount() const
{
    return m_meshletCount;
}
