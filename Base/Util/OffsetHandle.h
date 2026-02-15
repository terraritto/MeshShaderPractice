#pragma once
#include <cstdint>

class OffsetAllocator;

class OffsetHandle
{
public:
	OffsetHandle() = default;
	OffsetHandle(const OffsetHandle& handle);

	// Getter
	uint32_t GetOffset() const;
	uint32_t GetSize() const;
	bool IsValid() const;

private:
	OffsetHandle(uint32_t offset, uint32_t size, uint32_t metaData);

	void Reset();

public:
	static constexpr uint32_t INVALID_OFFSET = UINT32_MAX;

private:
	static constexpr uint32_t NO_SPACE = UINT32_MAX;

public:
	// Hold Data
	uint32_t m_offset = INVALID_OFFSET;
	uint32_t m_size = 0;
	uint32_t m_metaData = INVALID_OFFSET;

private:
	friend class OffsetAllocator;
};