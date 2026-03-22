#include "Bounds.h"

Bounds::Bounds()
{
}

Bounds::Bounds(const XMFLOAT3& minValue, const XMFLOAT3& maxValue)
    : m_minValue(minValue)
    , m_maxValue(maxValue)
{
}

XMFLOAT3 Bounds::GetSpan() const
{
    XMFLOAT3 result;
    result.x = std::abs(m_maxValue.x - m_minValue.x);
    result.y = std::abs(m_maxValue.y - m_minValue.y);
    result.z = std::abs(m_maxValue.z - m_minValue.z);
    return result;
}

void Bounds::SetValue(const XMFLOAT3& minValue, const XMFLOAT3& maxValue)
{
    m_minValue = minValue; m_maxValue = maxValue;
}
