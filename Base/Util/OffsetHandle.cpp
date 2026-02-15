#include "OffsetHandle.h"

OffsetHandle::OffsetHandle(const OffsetHandle& handle)
	: m_offset(handle.m_offset)
	, m_size(handle.m_size)
	, m_metaData(handle.m_metaData)
{}

OffsetHandle::OffsetHandle(uint32_t offset, uint32_t size, uint32_t metaData)
	: m_offset(offset)
	, m_size(size)
	, m_metaData(metaData)
{}

uint32_t OffsetHandle::GetOffset() const
{
	return m_offset;
}

uint32_t OffsetHandle::GetSize() const
{
	return m_size;
}

bool OffsetHandle::IsValid() const
{
	return m_offset != NO_SPACE && m_metaData != NO_SPACE;
}

void OffsetHandle::Reset()
{
	m_offset = NO_SPACE;
	m_size = 0;
	m_metaData = NO_SPACE;
}
