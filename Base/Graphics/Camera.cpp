#include "Camera.h"
#include <DirectXMath.h>
#include <cmath>

Camera::Camera()
{
    m_current.m_position = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);
    m_current.m_target = VecZero;
    m_current.m_upward = VecUnitY;
    m_current.m_angle = Vec2Zero;
    m_current.m_forward = VecUnitZ;
    m_current.m_distance = 1.0f;
    m_dirtyFlag = static_cast<uint32_t>(DirtyFlag::Position);

    m_preserve = m_current;
}

Camera::~Camera()
{
}

void Camera::UpdateByEvent(const Event& value)
{
    if (value.m_type & static_cast<uint32_t>(EventType::Rotate))
    {
        Rotate(value.m_rotateH, value.m_rotateV);
    }

    if (value.m_type & static_cast<uint32_t>(EventType::PanTilt))
    {
        Pantilt(value.m_rotateH, value.m_rotateV);
    }

    if (value.m_type & static_cast<uint32_t>(EventType::Dolly))
    {
        Dolly(value.m_dolly);
    }

    if (value.m_type & static_cast<uint32_t>(EventType::Move))
    {
        Move(value.m_moveX, value.m_moveY, value.m_moveZ);
    }

    if (value.m_type & static_cast<uint32_t>(EventType::Reset))
    {
        Reset();
    }

    Update();
}

void Camera::Update()
{
    if (m_dirtyFlag == static_cast<uint32_t>(DirtyFlag::None)) { return; }

    if (m_dirtyFlag & static_cast<uint32_t>(DirtyFlag::Position))
    {
        ComputePosition();
    }

    if (m_dirtyFlag & static_cast<uint32_t>(DirtyFlag::Target))
    {
        ComputeTarget();
    }

    if (m_dirtyFlag & static_cast<uint32_t>(DirtyFlag::Angle))
    {
        ComputeAngle();
    }

    m_view = DirectX::XMMatrixLookAtRH
    (
        m_current.m_position,
        m_current.m_target,
        m_current.m_upward
    );

    m_dirtyFlag = static_cast<uint32_t>(DirtyFlag::None);
}

void Camera::Preserve()
{
    m_preserve = m_current;
}

void Camera::Reset()
{
    m_current = m_preserve;
    m_dirtyFlag = static_cast<uint32_t>(DirtyFlag::Matrix);
    Update();
}

void Camera::SetPosition(const XMVECTOR& value)
{
    m_current.m_position = value;
    ComputeTarget();
    m_dirtyFlag = static_cast<uint32_t>(DirtyFlag::Angle);
    Update();
}

void Camera::SetTarget(const XMVECTOR& value)
{
    m_current.m_target = value;
    ComputePosition();
    m_dirtyFlag = static_cast<uint32_t>(DirtyFlag::Angle);
    Update();
}

const float& Camera::GetAngleV() const
{
    return m_current.m_angle.x;
}

const float& Camera::GetAngleH() const
{
    return m_current.m_angle.y;
}

const float& Camera::GetDistance() const
{
    return m_current.m_distance;
}

const XMVECTOR& Camera::GetPosition() const
{
    return m_current.m_position;
}

const XMVECTOR& Camera::GetTarget() const
{
    return m_current.m_target;
}

const XMVECTOR& Camera::GetUpward() const
{
    return m_current.m_upward;
}

const XMMATRIX& Camera::GetView() const
{
    return m_view;
}

void Camera::Rotate(float angleH, float angleV)
{
    ComputeAngle();
    ComputeTarget();

    m_current.m_angle.x += angleH;
    m_current.m_angle.y += angleV;
    
    // resolve gimbal lock. 
    if (m_current.m_angle.y > DirectX::XM_PIDIV2 - FLT_EPSILON)
    {
        m_current.m_angle.y = DirectX::XM_PIDIV2 - FLT_EPSILON;
    }
    if (m_current.m_angle.y < -DirectX::XM_PIDIV2 + FLT_EPSILON)
    {
        m_current.m_angle.y = -DirectX::XM_PIDIV2 + FLT_EPSILON;
    }

    m_dirtyFlag |= static_cast<uint32_t>(DirtyFlag::Position);
}

void Camera::Pantilt(float angleH, float angleV)
{
    ComputeAngle();
    ComputePosition();

    m_current.m_angle.x += angleH;
    m_current.m_angle.y += angleV;

    m_dirtyFlag |= static_cast<uint32_t>(DirtyFlag::Target);
}

void Camera::Move(float moveX, float moveY, float moveZ)
{
    XMVECTOR translate = DirectX::XMVectorScale(m_view.r[0], moveX);
    translate = DirectX::XMVectorAdd(DirectX::XMVectorScale(m_view.r[1], moveY), translate);
    translate = DirectX::XMVectorAdd(DirectX::XMVectorScale(m_view.r[2], moveZ), translate);

    m_current.m_position = DirectX::XMVectorAdd(translate, m_current.m_position);
    m_current.m_target = DirectX::XMVectorAdd(translate, m_current.m_target);

    m_dirtyFlag |= static_cast<uint32_t>(DirtyFlag::Matrix);
}

void Camera::Dolly(float value)
{
    ComputeAngle();
    ComputeTarget();

    m_current.m_distance += value;
    if (m_current.m_distance < 0.001f)
    {
        m_current.m_distance = 0.001f;
    }

    m_dirtyFlag |= static_cast<uint32_t>(DirtyFlag::Position);
}

void Camera::ComputePosition()
{
    ToVector(m_current.m_angle.x, m_current.m_angle.y, &m_current.m_forward, &m_current.m_upward);
    m_current.m_position = DirectX::XMVectorSubtract(m_current.m_target ,DirectX::XMVectorScale(m_current.m_forward, m_current.m_distance));
}

void Camera::ComputeTarget()
{
    ToVector(m_current.m_angle.x, m_current.m_angle.y, &m_current.m_forward, &m_current.m_upward);
    m_current.m_target = DirectX::XMVectorAdd(m_current.m_position, DirectX::XMVectorScale(m_current.m_forward, m_current.m_distance));
}

void Camera::ComputeAngle()
{
    m_current.m_forward = DirectX::XMVectorSubtract(m_current.m_target, m_current.m_position);
    ToAngle(m_current.m_forward, &m_current.m_angle.x, &m_current.m_angle.y, &m_current.m_distance);
    ToVector(m_current.m_angle.x, m_current.m_angle.y, nullptr, &m_current.m_upward);
}

float Camera::Cosine(float rad)
{
    if (abs(rad) < FLT_EPSILON) { return 1.0f; }
    return cosf(rad);
}

float Camera::Sine(float rad)
{
    if (abs(rad) < FLT_EPSILON) { return 0.0f; }
    return sinf(rad);
}

float Camera::CalculateAngle(float sine, float cosine)
{
    float result = asinf(sine);
    if (cosine < 0.0f)
    {
        result = DirectX::XM_PI - result;
    }
    return result;
}

void Camera::ToAngle(const XMVECTOR& v, float* angleH, float* angleV, float* dist)
{
    if (dist != nullptr)
    {
        *dist = DirectX::XMVector3Length(v).m128_f32[0];
    }

    XMVECTOR src = DirectX::XMVectorSet(-v.m128_f32[0], 0.0f, -v.m128_f32[2], 0.0f);
    XMVECTOR dest = src;

    if (angleH != nullptr)
    {
        if (fabs(src.m128_f32[0]) > FLT_EPSILON || fabs(src.m128_f32[2]) > FLT_EPSILON)
        {
            dest = DirectX::XMVector3Normalize(src);
        }

        *angleH = CalculateAngle(dest.m128_f32[0], dest.m128_f32[2]);
    }

    if (angleV != nullptr)
    {
        float d = DirectX::XMVector3Length(src).m128_f32[0];
        src.m128_f32[0] = d;
        src.m128_f32[1] = -v.m128_f32[2];
        src.m128_f32[2] = 0.0f;

        dest = src;

        if (fabs(src.m128_f32[0]) > FLT_EPSILON || fabs(src.m128_f32[2]) > FLT_EPSILON)
        {
            dest = DirectX::XMVector3Normalize(src);
        }

        *angleH = CalculateAngle(dest.m128_f32[1], dest.m128_f32[0]);
    }
}

void Camera::ToVector(float angleH, float angleV, XMVECTOR* forward, XMVECTOR* upward)
{
    float sx = Sine(angleH), sy = Sine(angleV);
    float cx = Cosine(angleH), cy = Cosine(angleV);

    if (forward != nullptr)
    {
        forward->m128_f32[0] = -cy * sx;
        forward->m128_f32[1] = -sy;
        forward->m128_f32[2] = -cy * cx;
    }

    if (upward != nullptr)
    {
        upward->m128_f32[0] = -sy * sx;
        upward->m128_f32[1] = cy;
        upward->m128_f32[2] = -sy * cx;
    }
}

