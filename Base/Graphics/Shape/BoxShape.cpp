#include "BoxShape.h"

BoxShape::BoxShape()
	: ShapeBase()
{
}

BoxShape::~BoxShape()
{
	Terminate();
}

bool BoxShape::Initialize(float size)
{
	std::vector<XMFLOAT3> positions;
	std::vector<uint32_t> indices;

	// Create
	CreateBoxShape(size, positions, indices);

	if (!InitBuffer(positions, indices))
	{
		return false;
	}

	return true;
}

void BoxShape::Terminate()
{
	Reset();
}

void BoxShape::CreateBoxShape(float size, std::vector<XMFLOAT3>& outPositions, std::vector<uint32_t>& outIndices)
{
	outPositions.clear();
	outIndices.clear();

	float width = size * 0.5f;
	float height = width;
	float depth = width;

	struct Face { XMFLOAT3 corners[4]; };
	std::vector<Face> faces; faces.resize(6); // Cube Face is 6.
	// +Z(Front)
	faces[0] =
	{
		XMFLOAT3(-width, -height,   depth),
		XMFLOAT3(-width,  height,   depth),
		XMFLOAT3( width,  height,   depth),
		XMFLOAT3( width, -height,   depth)
	};
	// -Z(Back)
	faces[1] =
	{
		XMFLOAT3( width, -height,  -depth),
		XMFLOAT3( width,  height,  -depth),
		XMFLOAT3(-width,  height,  -depth),
		XMFLOAT3(-width, -height,  -depth)
	};
	// +Y(Top)
	faces[2] =
	{
		XMFLOAT3(-width,  height,   depth),
		XMFLOAT3(-width,  height,  -depth),
		XMFLOAT3( width,  height,  -depth),
		XMFLOAT3( width,  height,   depth)
	};
	// -Y(Bottom)
	faces[3] =
	{
		XMFLOAT3(-width, -height,  -depth),
		XMFLOAT3(-width, -height,   depth),
		XMFLOAT3( width, -height,   depth),
		XMFLOAT3( width, -height,  -depth)
	};
	// +X(Right)
	faces[4] =
	{
		XMFLOAT3( width, -height,   depth),
		XMFLOAT3( width,  height,   depth),
		XMFLOAT3( width,  height,  -depth),
		XMFLOAT3( width, -height,  -depth)
	};
	// -X(Left)
	faces[5] =
	{
		XMFLOAT3(-width, -height,  -depth),
		XMFLOAT3(-width,  height,  -depth),
		XMFLOAT3(-width,  height,   depth),
		XMFLOAT3(-width, -height,   depth)
	};

	for (int i = 0; i < 6; ++i)
	{
		const auto& face = faces[i];
		auto baseIndex = uint32_t(outPositions.size());

		// add position
		for (auto& corner : face.corners)
		{
			outPositions.push_back(corner);
		}

		// plane constructs two triangles.
		outIndices.push_back(baseIndex);
		outIndices.push_back(baseIndex+1);
		outIndices.push_back(baseIndex+2);
		outIndices.push_back(baseIndex);
		outIndices.push_back(baseIndex+2);
		outIndices.push_back(baseIndex+3);
	}

	outPositions.shrink_to_fit();
	outIndices.shrink_to_fit();
}
