#pragma once
#include "MeshShaderPractice/Base/Graphics/Shape/ShapeBase.h"

class SphereShape : public ShapeBase
{
public:
	SphereShape();
	~SphereShape();

	bool Initialize(float radius, uint32_t slice);
	void Terminate();

private:
	void CreateSphereShape(float radius, uint32_t slice, uint32_t stack, std::vector<XMFLOAT3>& outPositions, std::vector<uint32_t>& outIndices);
};