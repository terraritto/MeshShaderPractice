#pragma once
#include <cstdint>

struct ResourceMeshlet
{
	// Offset within meshlet vertices and meshlet triangles.
	// Count means number of vertices and triangles used in the meshlet.
	uint32_t m_vertexOffset;
	uint32_t m_vertexCount;
	uint32_t m_primitiveOffset;
	uint32_t m_primitiveCount;
};
