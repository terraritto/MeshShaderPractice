#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "MeshShaderPractice/Base/Util/OffsetHandle.h"


class OffsetAllocator
{
	struct Node;
public:
	OffsetAllocator() = default;
	OffsetAllocator(OffsetAllocator&& other) noexcept;

	void Initialize(uint32_t size, uint32_t maxAllocatableCount = 128 * 1024);
	void Terminate();
	void Reset();

	// Alloc/Free
	OffsetHandle Allocate(uint32_t size, uint32_t alignment);
	OffsetHandle Allocate(uint32_t size);
	void Free(OffsetHandle& handle);

	// Node Operation
	uint32_t InsertNode(uint32_t size, uint32_t offset);
	void RemoveNode(uint32_t index);
	Node GenerateNode(uint32_t offset, uint32_t size, uint32_t binListNext);

	// Getter
	uint32_t GetUsedSize() const;
	uint32_t GetFreeSize() const;

private:
	uint32_t FloatRoundUp(uint32_t size);
	uint32_t FloatRoundDown(uint32_t size);
	uint32_t FindLowestSetBitAfter(uint32_t bitMask, uint32_t startBitIndex);

private:
	struct Node
	{
		static constexpr uint32_t UNUSED = UINT32_MAX;

		uint32_t m_dataOffset = 0;
		uint32_t m_dataSize = 0;
		uint32_t m_binListPrev = UNUSED;
		uint32_t m_binListNext = UNUSED;
		uint32_t m_neighborPrev = UNUSED;
		uint32_t m_neighborNext = UNUSED;
		bool m_isUsed = false;
	};

public:
	// Bin
	static constexpr uint32_t TOP_BINS_COUNT = 32;
	static constexpr uint32_t BINS_PER_LEAF = 8;
	static constexpr uint32_t LEAF_BINS_COUNT = TOP_BINS_COUNT * BINS_PER_LEAF;
	static constexpr uint32_t TOP_BINS_INDEX_SHIFT = 3;
	static constexpr uint32_t LEAF_BINS_INDEX_MASK = 0x7;

	// Mantissa
	static constexpr uint32_t MANTISSA_BITS = 3;
	static constexpr uint32_t MANTISSA_VALUE = 1 << MANTISSA_BITS;
	static constexpr uint32_t MANTISSA_MASK = MANTISSA_VALUE - 1;

private:
	uint32_t m_size = 0;		// memory size
	uint32_t m_maxAllocatableCount = 0; // max available allocate count
	uint32_t m_freeStorage = 0;	// non-used storage
	uint32_t m_usedBinsTop = 0; // top of used Bin
	std::vector<Node> m_nodes; // nodes
	std::vector<uint32_t> m_freeNodes; // free nodes fpr index
	int64_t m_freeOffset = -1; // free offset

	std::array<uint8_t, TOP_BINS_COUNT> m_usedBins; // used Bin
	std::array<uint32_t, LEAF_BINS_COUNT> m_binIndices; // Bin index
};
