#pragma once
#include "MeshShaderPractice/Base/Util.h"

class Bounds
{
public:
	Bounds();
	Bounds(const XMFLOAT3& minValue, const XMFLOAT3& maxValue);

	// Getter
	XMFLOAT3 GetSpan() const;

	// Setter
	void SetValue(const XMFLOAT3& minValue, const XMFLOAT3& maxValue);

protected:
	XMFLOAT3 m_minValue;
	XMFLOAT3 m_maxValue;
};