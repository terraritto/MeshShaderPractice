#pragma once
#include <cstdint>
#include "MeshShaderPractice/Base/Util.h"

class Camera
{
public:
	enum class EventType
	{
		Rotate		= 0x1 << 0,
		Dolly		= 0x1 << 1,
		Move		= 0x1 << 2,
		PanTilt		= 0x1 << 3,
		Reset		= 0x1 << 4
	};

	struct Event
	{
		uint32_t m_type = 0;
		float m_rotateH = 0.0f;
		float m_rotateV = 0.0f;
		float m_pan = 0.0f;
		float m_tilt = 0.0f;
		float m_dolly = 0.0f;
		float m_moveX = 0.0f;
		float m_moveY = 0.0f;
		float m_moveZ = 0.0f;
	};

	Camera();
	~Camera();

	// Operate
	void UpdateByEvent(const Event& value);
	void Update();
	void Preserve();
	void Reset();

	// setter
	void SetPosition(const XMVECTOR& value);
	void SetTarget(const XMVECTOR& value);

	// Getter
	const float& GetAngleV() const;
	const float& GetAngleH() const;
	const float& GetDistance() const;

	const XMVECTOR& GetPosition() const;
	const XMVECTOR& GetTarget() const;
	const XMVECTOR& GetUpward() const;
	const XMMATRIX& GetView() const;

private:
	void Rotate(float angleH, float angleV);
	void Pantilt(float angleH, float angleV);
	void Move(float moveX, float moveY, float moveZ);
	void Dolly(float value);

	void ComputePosition();
	void ComputeTarget();
	void ComputeAngle();

	float Cosine(float rad);
	float Sine(float rad);
	float CalculateAngle(float sine, float cosine);
	void ToAngle(const XMVECTOR& v, float* angleH, float* angleV, float* dist);
	void ToVector(float angleH, float angleV, XMVECTOR* forward, XMVECTOR* upward);

private:
	enum class DirtyFlag
	{
		None			= 0x0,
		Position		= 0x1 << 0,
		Target			= 0x1 << 1,
		Angle			= 0x1 << 2,
		Matrix			= 0x1 << 3
	};

	struct Param
	{
		XMVECTOR m_position;
		XMVECTOR m_target;
		XMVECTOR m_upward;
		XMVECTOR m_forward;
		XMFLOAT2 m_angle;
		float m_distance;
	};

private:
	Param m_current;
	Param m_preserve;
	XMMATRIX m_view = MatIdentity;
	uint32_t m_dirtyFlag = static_cast<uint32_t>(DirtyFlag::None);
};