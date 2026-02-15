// Original Code Written by Sebastian Aaltonen.
// https://github.com/sebbbi/OffsetAllocator
// MIT License: https://github.com/sebbbi/OffsetAllocator/blob/main/LICENSE

#include "OffsetAllocator.h"
#include <bit>
#include <cassert>
#include "MeshShaderPractice/Base/Util/Logger.h"

OffsetAllocator::OffsetAllocator(OffsetAllocator&& other) noexcept
	: m_size(other.m_size)
	, m_maxAllocatableCount(other.m_maxAllocatableCount)
	, m_freeStorage(other.m_freeStorage)
	, m_usedBinsTop(other.m_usedBinsTop)
	, m_nodes(other.m_nodes)
	, m_freeNodes(other.m_freeNodes)
	, m_usedBins(other.m_usedBins)
	, m_binIndices(other.m_binIndices)
{
	other.m_nodes.clear();
	other.m_freeNodes.clear();
	other.m_freeOffset = -1;
	other.m_maxAllocatableCount = 0;
	other.m_usedBinsTop = 0;
}

void OffsetAllocator::Initialize(uint32_t size, uint32_t maxAllocatableCount)
{
	m_size = size;
	m_maxAllocatableCount = maxAllocatableCount;
	Reset();
}

void OffsetAllocator::Terminate()
{
	m_nodes.clear();
	m_freeNodes.clear();

	for (auto i = 0u; i < TOP_BINS_COUNT; ++i)
	{
		m_usedBins[i] = 0;
	}

	for (auto i = 0u; i < LEAF_BINS_COUNT; ++i)
	{
		m_binIndices[i] = Node::UNUSED;
	}

	m_size = 0;
	m_maxAllocatableCount = 0;
	m_freeStorage = 0;
	m_usedBinsTop = 0;
	m_freeOffset = -1;
}

void OffsetAllocator::Reset()
{
	m_freeStorage = 0;
	m_usedBinsTop = 0;
	m_freeOffset = int64_t(m_maxAllocatableCount);

	for (auto i = 0u; i < TOP_BINS_COUNT; ++i)
	{
		m_usedBins[i] = 0;
	}

	for (auto i = 0u; i < LEAF_BINS_COUNT; ++i)
	{
		m_binIndices[i] = Node::UNUSED;
	}

	m_nodes.clear(); m_nodes.resize(m_maxAllocatableCount + 1);
	m_freeNodes.clear(); m_freeNodes.resize(m_maxAllocatableCount + 1);

	for (auto i = 0u; i <= m_maxAllocatableCount; ++i)
	{
		// set index for free nodes.
		m_freeNodes[i] = m_maxAllocatableCount - i;
	}

	// Root
	InsertNode(m_size, 0);
}

OffsetHandle OffsetAllocator::Allocate(uint32_t size, uint32_t alignment)
{
	// size adjust for alignment, alignment must be 2^n.
	uint32_t alignmentSize = (size + (alignment - 1)) & ~(alignment - 1);
	return Allocate(alignmentSize);
}

OffsetHandle OffsetAllocator::Allocate(uint32_t size)
{
	if (m_freeOffset < 0 || size == 0)
	{
		ELOGA("Error: Out of memory.");
		return OffsetHandle(OffsetHandle::NO_SPACE, 0, OffsetHandle::NO_SPACE);
	}

	// round up bin index to satisfy alloc >= bin.
	// it is smallest bin index appropriate size.
	uint32_t minBinIndex = FloatRoundUp(size);

	uint32_t minTopBinIndex = minBinIndex >> TOP_BINS_INDEX_SHIFT;
	uint32_t minLeafBinIndex = minBinIndex & LEAF_BINS_INDEX_MASK;

	uint32_t topBinIndex = minTopBinIndex;
	uint32_t leafBinIndex = OffsetHandle::NO_SPACE;

	// if top bin exist, scan this leaf bin.
	// this process may be failed.(NO_SPACE)
	if (m_usedBinsTop & (1 << topBinIndex))
	{
		leafBinIndex = FindLowestSetBitAfter(m_usedBins[topBinIndex], minLeafBinIndex);
	}

	if (leafBinIndex == OffsetHandle::NO_SPACE)
	{
		// if top bin is NO_SPACE, Search from (BinIndex + 1).
		topBinIndex = FindLowestSetBitAfter(m_usedBinsTop, minTopBinIndex + 1);

		if (topBinIndex == OffsetHandle::NO_SPACE)
		{
			return OffsetHandle(OffsetHandle::NO_SPACE, 0, OffsetHandle::NO_SPACE);
		}

		// top bin can be rounded up, so this leaf bin fits alloc operation.
		// leaf searching start from bit 0.
		leafBinIndex = std::countr_zero(uint32_t(m_usedBins[topBinIndex]));
	}

	uint32_t binIndex = (topBinIndex << TOP_BINS_INDEX_SHIFT) | leafBinIndex;

	// pop head node of bin.
	uint32_t nodeIndex = m_binIndices[binIndex];
	Node& node = m_nodes[nodeIndex];
	uint32_t nodeTotalSize = node.m_dataSize;

	// check used
	node.m_dataSize = size;
	node.m_isUsed = true;
	m_binIndices[binIndex] = node.m_binListNext;

	if (node.m_binListNext != Node::UNUSED)
	{
		m_nodes[node.m_binListNext].m_binListPrev = Node::UNUSED;
	}

	m_freeStorage -= nodeTotalSize;

	// bin is empty?
	if (m_binIndices[binIndex] == Node::UNUSED)
	{
		// delete leaf-bin mask bit.
		m_usedBins[topBinIndex] &= ~(1 << leafBinIndex);

		// leaf bin is empty all?
		if (m_usedBins[topBinIndex] == 0)
		{
			// delete top-bin mask bit.
			m_usedBinsTop &= ~(1 << topBinIndex);
		}
	}

	uint32_t reminderSize = nodeTotalSize - size;
	if (reminderSize > 0)
	{
		uint32_t newNodeIndex = InsertNode(reminderSize, node.m_dataOffset + size);

		// link neighbor node because if Prev and Next is empty, you can merge later.
		if (node.m_neighborNext != Node::UNUSED)
		{
			m_nodes[node.m_neighborNext].m_neighborPrev = newNodeIndex;
		}

		m_nodes[newNodeIndex].m_neighborPrev = nodeIndex;
		m_nodes[newNodeIndex].m_neighborNext = node.m_neighborNext;
		node.m_neighborNext = newNodeIndex;
	}

	return OffsetHandle(node.m_dataOffset, node.m_dataSize, nodeIndex);
}

void OffsetAllocator::Free(OffsetHandle& handle)
{
	if (!handle.IsValid()) { return; }
	if (m_nodes.empty()) { handle.Reset(); return; }

	uint32_t nodeIndex = handle.m_metaData;
	Node& node = m_nodes[nodeIndex];

	// already node is freed.
	if (!node.m_isUsed)
	{
		handle.Reset(); return;
	}

	// merge neighbor.
	uint32_t offset = node.m_dataOffset;
	uint32_t size = node.m_dataSize;

	if ((node.m_neighborPrev != Node::UNUSED) && (m_nodes[node.m_neighborPrev].m_isUsed == false))
	{
		// prev empty node: current offset change to prev node offset.
		Node& prevNode = m_nodes[node.m_neighborPrev];
		offset = prevNode.m_dataOffset;
		size += prevNode.m_dataSize;

		// remove node from link-list of bin and insert to free-list.
		RemoveNode(node.m_neighborPrev);

		assert(prevNode.m_neighborNext == nodeIndex);
		node.m_neighborPrev = prevNode.m_neighborPrev;
	}

	if ((node.m_neighborNext != Node::UNUSED) && (m_nodes[node.m_neighborNext].m_isUsed == false))
	{
		// next empty node: offset isn't changed.
		Node& nextNode = m_nodes[node.m_neighborNext];
		size += nextNode.m_dataSize;

		// remove node from link-list of bin and insert to free-list.
		RemoveNode(node.m_neighborNext);

		assert(nextNode.m_neighborPrev == nodeIndex);
		node.m_neighborNext = nextNode.m_neighborNext;
	}

	uint32_t neighborNext = node.m_neighborNext;
	uint32_t neighborPrev = node.m_neighborPrev;

	// deleted node is inserted to free node.
	m_freeNodes[++m_freeOffset] = nodeIndex;

	// insert free node combined bin.
	uint32_t combinedNodeIndex = InsertNode(size, offset);

	// connect new combined node and neighbor node.
	if (neighborNext != Node::UNUSED)
	{
		m_nodes[combinedNodeIndex].m_neighborNext = neighborNext;
		m_nodes[neighborNext].m_neighborPrev = combinedNodeIndex;
	}
	if (neighborPrev != Node::UNUSED)
	{
		m_nodes[combinedNodeIndex].m_neighborPrev = neighborPrev;
		m_nodes[neighborPrev].m_neighborNext = combinedNodeIndex;
	}
}

uint32_t OffsetAllocator::InsertNode(uint32_t size, uint32_t offset)
{
	// round down bin index to satisfy alloc >= bin.
	uint32_t binIndex = FloatRoundDown(size);

	uint32_t topBinIndex = binIndex >> TOP_BINS_INDEX_SHIFT;
	uint32_t leafBinIndex = binIndex & LEAF_BINS_INDEX_MASK;

	// Prebiously, bin is empty?
	if (m_binIndices[binIndex] == Node::UNUSED)
	{
		// set bin mask bit.
		m_usedBins[topBinIndex] |= 1 << leafBinIndex;
		m_usedBinsTop |= 1 << topBinIndex;
	}

	// pop node from free list, and insert bin link list.
	uint32_t topNodeIndex = m_binIndices[binIndex];
	uint32_t nodeIndex = m_freeNodes[m_freeOffset--];

	m_nodes[nodeIndex] = GenerateNode(offset, size, topNodeIndex);

	if (topNodeIndex != Node::UNUSED)
	{
		m_nodes[topNodeIndex].m_binListPrev = nodeIndex;
	}
	m_binIndices[binIndex] = nodeIndex;

	m_freeStorage += size;

	return nodeIndex;
}

void OffsetAllocator::RemoveNode(uint32_t index)
{
	Node& node = m_nodes[index];

	if (node.m_binListPrev != Node::UNUSED)
	{
		// simple case
		// if prev node exist, this node delete from list.
		m_nodes[node.m_binListPrev].m_binListNext = node.m_binListNext;
		if (node.m_binListNext != Node::UNUSED)
		{
			m_nodes[node.m_binListNext].m_binListPrev = node.m_binListPrev;
		}
	}
	else
	{
		// hard case
		// find bin of first node
		uint32_t binIndex = FloatRoundDown(node.m_dataSize);

		uint32_t topBinIndex = binIndex >> TOP_BINS_INDEX_SHIFT;
		uint32_t leafBinIndex = binIndex & LEAF_BINS_INDEX_MASK;

		m_binIndices[binIndex] = node.m_binListNext;
		if (node.m_binListNext != Node::UNUSED)
		{
			m_nodes[node.m_binListNext].m_binListPrev = Node::UNUSED;
		}

		// is bin empty?
		if (m_binIndices[binIndex] == Node::UNUSED)
		{
			// delete mask bit of leaf bin.
			m_usedBins[topBinIndex] &= ~(1 << leafBinIndex);

			// is all leaf bin empty?
			if (m_usedBins[topBinIndex] == 0)
			{
				// delete mask bit of top bin
				m_usedBinsTop &= ~(1 << topBinIndex);
			}
		}
	}

	// insert node into free list.
	m_freeNodes[++m_freeOffset] = index;
	m_freeStorage -= node.m_dataSize;
}

OffsetAllocator::Node OffsetAllocator::GenerateNode(uint32_t offset, uint32_t size, uint32_t binListNext)
{
	Node node = {};
	node.m_dataOffset = offset;
	node.m_dataSize = size;
	node.m_binListNext = binListNext;
	return node;
}

uint32_t OffsetAllocator::GetUsedSize() const
{
	return m_size - GetFreeSize();
}

uint32_t OffsetAllocator::GetFreeSize() const
{
	return (m_freeOffset >= 0) ? m_freeStorage : 0;
}

uint32_t OffsetAllocator::FloatRoundUp(uint32_t size)
{
	uint32_t exp = 0;
	uint32_t mantissa = 0;
	
	if (size < MANTISSA_VALUE)
	{
		mantissa = size;
	}
	else
	{
		uint32_t highestSetBit = 31 - std::countl_zero(size);
		uint32_t mantissaStartBit = highestSetBit - MANTISSA_BITS;
		exp = mantissaStartBit + 1;
		mantissa = (size >> mantissaStartBit) & MANTISSA_MASK;

		uint32_t lowBitsMask = (1 << mantissaStartBit) - 1;

		// Round up
		if ((size & lowBitsMask) != 0) { mantissa++; }
	}

	return (exp << MANTISSA_BITS) + mantissa;
}

uint32_t OffsetAllocator::FloatRoundDown(uint32_t size)
{
	uint32_t exp = 0;
	uint32_t mantissa = 0;

	if (size < MANTISSA_VALUE)
	{
		mantissa = size;
	}
	else
	{
		uint32_t highestSetBit = 31 - std::countl_zero(size);
		uint32_t mantissaStartBit = highestSetBit - MANTISSA_BITS;
		exp = mantissaStartBit + 1;
		mantissa = (size >> mantissaStartBit) & MANTISSA_MASK;
	}

	return (exp << MANTISSA_BITS) | mantissa;
}

uint32_t OffsetAllocator::FindLowestSetBitAfter(uint32_t bitMask, uint32_t startBitIndex)
{
	uint32_t beforeIndex = (1 << startBitIndex) - 1;
	uint32_t afterIndex = ~beforeIndex;
	uint32_t bitsAfter = bitMask & afterIndex;

	if (bitsAfter == 0) { return UINT32_MAX; }

	return std::countr_zero(bitsAfter);
}
