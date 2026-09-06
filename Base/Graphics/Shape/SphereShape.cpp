#include "SphereShape.h"

SphereShape::SphereShape()
	: ShapeBase()
{
}

SphereShape::~SphereShape()
{
	Terminate();
}

bool SphereShape::Initialize(float radius, uint32_t slice)
{
	std::vector<XMFLOAT3> positions;
	std::vector<uint32_t> indices;

	// Create
	CreateSphereShape(radius, slice, slice, positions, indices);

	if (!InitBuffer(positions, indices))
	{
		return false;
	}

	return true;
}

void SphereShape::Terminate()
{
	Reset();
}

void SphereShape::CreateSphereShape(float radius, uint32_t slice, uint32_t stack, std::vector<XMFLOAT3>& outPositions, std::vector<uint32_t>& outIndices)
{
	outPositions.clear();
	outIndices.clear();

	// Create Vertices
	for (auto i = 0u; i <= stack; ++i)
	{
		float phi = DirectX::XM_PI * static_cast<float>(i) / static_cast<float>(stack); // [0, pi]
		for (auto j = 0u; j <= slice; ++j)
		{
			float theta = DirectX::XM_2PI * static_cast<float>(j) / static_cast<float>(slice); // [0, 2pi]

			float x = radius * sinf(phi) * cosf(theta);
			float y = radius * cosf(phi);
			float z = radius * sinf(phi) * sinf(theta);
			outPositions.push_back({ x,y,z });
		}
	}

	// Create Indices
	uint32_t ringVertexCount = slice + 1;
	for (auto i = 0u; i < stack; ++i)
	{
		for (auto j = 0u; j < slice; ++j)
		{
			uint32_t i0 = i * ringVertexCount + j;
			uint32_t i1 = i0 + 1;
			uint32_t i2 = i0 + ringVertexCount;
			uint32_t i3 = i2 + 1;

			outIndices.push_back(i0);
			outIndices.push_back(i2);
			outIndices.push_back(i1);

			outIndices.push_back(i1);
			outIndices.push_back(i2);
			outIndices.push_back(i3);
		}
	}

	outPositions.shrink_to_fit();
	outIndices.shrink_to_fit();
}
