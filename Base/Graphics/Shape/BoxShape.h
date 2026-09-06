#pragma once
#include "MeshShaderPractice/Base/Graphics/Shape/ShapeBase.h"

class BoxShape : public ShapeBase
{
public:
	BoxShape();
	~BoxShape();

	bool Initialize(float size);
	void Terminate();

private:
	void CreateBoxShape(float size, std::vector<XMFLOAT3>& outPositions, std::vector<uint32_t>& outIndices);
};