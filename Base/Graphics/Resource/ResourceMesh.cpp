#include "ResourceMesh.h"

size_t ResourceMesh::GetVerticesNum() const
{
    return m_positions.size();
}

size_t ResourceMesh::GetIndicesNum() const
{
    return m_indices.size();
}
