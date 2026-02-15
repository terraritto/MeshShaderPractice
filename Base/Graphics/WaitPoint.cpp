#include "WaitPoint.h"

WaitPoint::WaitPoint()
	: m_fenceValue(0)
	, m_fence(nullptr)
{
}

WaitPoint::~WaitPoint()
{
	m_fenceValue = 0;
	m_fence = nullptr;
}

WaitPoint& WaitPoint::operator=(const WaitPoint& value)
{
	m_fenceValue = value.m_fenceValue;
	m_fence = value.m_fence;
	return *this;
}

bool WaitPoint::IsValid() const
{
	return m_fenceValue >= 1 && m_fence != nullptr;
}
