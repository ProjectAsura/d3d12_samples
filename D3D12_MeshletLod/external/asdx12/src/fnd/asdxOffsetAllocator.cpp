//-----------------------------------------------------------------------------
// File : asdxOffsetAllocator.cpp
// Desc : Offset Allcoator.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

// Original Code Written by Sebastian Aaltonen.
// https://github.com/sebbbi/OffsetAllocator
// MIT License: https://github.com/sebbbi/OffsetAllocator/blob/main/LICENSE

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cassert>
#include <fnd/asdxOffsetAllocator.h>
#include <fnd/asdxBit.h>
#include <fnd/asdxLogger.h>


namespace {

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
static constexpr uint32_t TOP_BINS_INDEX_SHIFT  = 3;
static constexpr uint32_t LEAF_BINS_INDEX_MASK  = 0x7;
static constexpr uint32_t MANTISSA_BITS         = 3;
static constexpr uint32_t MANTISSA_VALUE        = 1 << MANTISSA_BITS;
static constexpr uint32_t MANTISSA_MASK         = MANTISSA_VALUE - 1;
static constexpr uint32_t NO_SPACE              = asdx::OffsetHandle::INVALID_OFFSET;

//-----------------------------------------------------------------------------
//      浮動小数への丸め上げします.
//-----------------------------------------------------------------------------
static uint32_t FloatRoundUp(uint32_t size)
{
    // ビンのサイズは、浮動小数点（指数＋仮数）分布（区分線形対数近似）に従います.
    // これにより、各サイズクラスで、平均オーバーヘッドのパーセンテージが同じになります。

    uint32_t exp      = 0;
    uint32_t mantissa = 0;

    if (size < MANTISSA_VALUE)
    {
        // Denorm: 0..(MANTISSA_VALUE-1)
        mantissa = size;
    }
    else
    {
        // 正規化済み： 隠れ上位ビットは常に1. 保存されない. floatと同じ.
        uint32_t highestSetBit    = 31 - asdx::CountZeroL(size);
        uint32_t mantissaStartBit = highestSetBit - MANTISSA_BITS;
        exp = mantissaStartBit + 1;
        mantissa = (size >> mantissaStartBit) & MANTISSA_MASK;

        uint32_t lowBitsMask = (1 << mantissaStartBit) - 1;

        // Round up!
        if ((size & lowBitsMask) != 0)
            mantissa++;
    }

    return (exp << MANTISSA_BITS) + mantissa; // + 丸め込みのための仮数->仮数のオーバーフローを許可.
}

//-----------------------------------------------------------------------------
//      浮動小数への丸め下げします.
//-----------------------------------------------------------------------------
static uint32_t FloatRoundDown(uint32_t size)
{
    uint32_t exp      = 0;
    uint32_t mantissa = 0;

    if (size < MANTISSA_VALUE)
    {
        // Denorm: 0..(MANTISSA_VALUE-1)
        mantissa = size;
    }
    else
    {
        // 正規化済み： 隠れ上位ビットは常に1. 保存されない. floatと同じ.
        uint32_t highestSetBit    = 31 - asdx::CountZeroL(size);
        uint32_t mantissaStartBit = highestSetBit - MANTISSA_BITS;
        exp = mantissaStartBit + 1;
        mantissa = (size >> mantissaStartBit) & MANTISSA_MASK;
    }

    return (exp << MANTISSA_BITS) | mantissa;
}

//-----------------------------------------------------------------------------
//      最下位ビットを検索します.
//-----------------------------------------------------------------------------
static uint32_t FindLowestSetBitAfter(uint32_t bitMask, uint32_t startBitIndex)
{
    uint32_t beforeIndex = (1 << startBitIndex) - 1;
    uint32_t afterIndex  = ~beforeIndex;
    uint32_t bitsAfter   = bitMask & afterIndex;

    if (bitsAfter == 0)
        return UINT32_MAX;

    return asdx::CountZeroR(bitsAfter);
}

} // namespace

namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// OffsetHandle class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
OffsetHandle::OffsetHandle(uint32_t offset, uint32_t size, uint32_t metaData)
: m_Offset  (offset)
, m_Size    (size)
, m_MetaData(metaData)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      コピーコンストラクタです.
//-----------------------------------------------------------------------------
OffsetHandle::OffsetHandle(const OffsetHandle& handle)
: m_Offset  (handle.m_Offset)
, m_Size    (handle.m_Size)
, m_MetaData(handle.m_MetaData)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      オフセットを取得します.
//-----------------------------------------------------------------------------
uint32_t OffsetHandle::GetOffset() const
{ return m_Offset; }

//-----------------------------------------------------------------------------
//      サイズを取得します.
//-----------------------------------------------------------------------------
uint32_t OffsetHandle::GetSize() const
{ return m_Size; }

//-----------------------------------------------------------------------------
//      無効かどうかチェックします.
//-----------------------------------------------------------------------------
bool OffsetHandle::IsValid() const
{ return m_Offset != NO_SPACE && m_MetaData != NO_SPACE; }

//-----------------------------------------------------------------------------
//      リセットします.
//-----------------------------------------------------------------------------
void OffsetHandle::Reset()
{
    m_Offset   = NO_SPACE;
    m_Size     = 0;
    m_MetaData = NO_SPACE;
}


///////////////////////////////////////////////////////////////////////////////
// OffsetAllocator class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      ムーブコンストラクタです.
//-----------------------------------------------------------------------------
OffsetAllocator::OffsetAllocator(OffsetAllocator&& other) noexcept
: m_Size                (other.m_Size)
, m_MaxAllocatableCount (other.m_MaxAllocatableCount)
, m_FreeStorage         (other.m_FreeStorage)
, m_UsedBinsTop         (other.m_UsedBinsTop)
, m_Nodes               (other.m_Nodes)
, m_FreeNodes           (other.m_FreeNodes)
, m_FreeOffset          (other.m_FreeOffset)
, m_UsedBins            (other.m_UsedBins)
, m_BinIndices          (other.m_BinIndices)
{
    other.m_Nodes               = nullptr;
    other.m_FreeNodes           = nullptr;
    other.m_FreeOffset          = -1;
    other.m_MaxAllocatableCount = 0;
    other.m_UsedBinsTop         = 0;
}

//-----------------------------------------------------------------------------
//      初期化します.
//-----------------------------------------------------------------------------
void OffsetAllocator::Init(uint32_t size, uint32_t maxAllocatableCount)
{
    m_Size                  = size;
    m_MaxAllocatableCount   = maxAllocatableCount;
    Reset();
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void OffsetAllocator::Term()
{
    if (m_Nodes)
    {
        delete [] m_Nodes;
        m_Nodes = nullptr;
    }

    if (m_FreeNodes)
    {
        delete [] m_FreeNodes;
        m_FreeNodes = nullptr;
    }

    for(auto i=0u; i<TOP_BINS_COUNT; ++i)
        m_UsedBins[i] = 0;

    for(auto i=0u; i<LEAF_BINS_COUNT; ++i)
        m_BinIndices[i] = Node::UNUSED;

    m_Size                  = 0;
    m_MaxAllocatableCount   = 0;
    m_FreeStorage           = 0;
    m_UsedBinsTop           = 0;
    m_FreeOffset            = -1;
}

//-----------------------------------------------------------------------------
//      リセットします.
//-----------------------------------------------------------------------------
void OffsetAllocator::Reset()
{
    m_FreeStorage           = 0;
    m_UsedBinsTop           = 0;
    m_FreeOffset            = int64_t(m_MaxAllocatableCount);

    for(auto i=0u; i<TOP_BINS_COUNT; ++i)
        m_UsedBins[i] = 0;

    for(auto i=0u; i<LEAF_BINS_COUNT; ++i)
        m_BinIndices[i] = Node::UNUSED;

    if (m_Nodes)
    {
        delete [] m_Nodes;
        m_Nodes = nullptr;
    }
    if (m_FreeNodes)
    {
        delete [] m_FreeNodes;
        m_FreeNodes = nullptr;
    }

    m_Nodes     = new Node    [m_MaxAllocatableCount + 1];
    m_FreeNodes = new uint32_t[m_MaxAllocatableCount + 1];

    for(auto i=0u; i<=m_MaxAllocatableCount; ++i)
    {
        m_FreeNodes[i] = m_MaxAllocatableCount - i;
    }

    InsertNode(m_Size, 0);
}

//-----------------------------------------------------------------------------
//      メモリを確保します.
//-----------------------------------------------------------------------------
OffsetHandle OffsetAllocator::Alloc(uint32_t size, uint32_t alignment)
{
    uint32_t alignSize = (size + (alignment - 1)) & ~(alignment - 1);
    return Alloc(alignSize);
}

//-----------------------------------------------------------------------------
//      メモリを確保します.
//-----------------------------------------------------------------------------
OffsetHandle OffsetAllocator::Alloc(uint32_t size)
{
    if (m_FreeOffset < 0 || size == 0)
    {
        ELOG("Error : Out of Memory.");
        return OffsetHandle(NO_SPACE, 0, NO_SPACE);
    }

    // alloc >= binとなるようにbinインデックスを切り上げる.
    // サイズに合う最小の bin インデックスを与える
    auto minBinIndex = FloatRoundUp(size);

    uint32_t minTopBinIndex  = minBinIndex >> TOP_BINS_INDEX_SHIFT;
    uint32_t minLeafBinIndex = minBinIndex & LEAF_BINS_INDEX_MASK;
        
    uint32_t topBinIndex  = minTopBinIndex;
    uint32_t leafBinIndex = NO_SPACE;

    // トップビンが存在する場合、そのリーフビンをスキャンする. これは失敗することがある(NO_SPACE)。
    if (m_UsedBinsTop & (1 << topBinIndex))
    {
        leafBinIndex = FindLowestSetBitAfter(m_UsedBins[topBinIndex], minLeafBinIndex);
    }

    // トップビンにスペースがなければ、+1からトップビンを探す.
    if (leafBinIndex == NO_SPACE)
    {
        topBinIndex = FindLowestSetBitAfter(m_UsedBinsTop, minTopBinIndex + 1);

        // スペース外？
        if (topBinIndex == NO_SPACE)
        {
            return OffsetHandle(NO_SPACE, 0, NO_SPACE);
        }

        // 一番上のビンは切り上げられたので、ここでのリーフビンはすべてallocに適合する. ビット0からリーフサーチを開始する。
        // 注：トップビットがセットされているため、少なくとも1つのリーフビットがセットされているので、この検索が失敗することはない.
        leafBinIndex = CountZeroR(uint32_t(m_UsedBins[topBinIndex]));
    }

    uint32_t binIndex = (topBinIndex << TOP_BINS_INDEX_SHIFT) | leafBinIndex;

    // ビンの先頭ノードをポップする. Bin top = node.next.
    auto  nodeIndex      = m_BinIndices[binIndex];
    auto& node           = m_Nodes[nodeIndex];
    auto  nodeTotalSize  = node.DataSize;

    node.DataSize = size;
    node.Used     = true;
    m_BinIndices[binIndex] = node.BinListNext;

    if (node.BinListNext != Node::UNUSED)
        m_Nodes[node.BinListNext].BinListPrev = Node::UNUSED;

    m_FreeStorage -= nodeTotalSize;

    // ビンが空か?
    if (m_BinIndices[binIndex] == Node::UNUSED)
    {
        // リーフビンマスクビットを削除.
        m_UsedBins[topBinIndex] &= ~(1 << leafBinIndex);

        // リーフビンが全て空か?
        if (m_UsedBins[topBinIndex] == 0)
        {
            // トップビンマスクビットを削除.
            m_UsedBinsTop &= ~(1 << topBinIndex);
        }
    }

    auto reminderSize = nodeTotalSize - size;
    if (reminderSize > 0)
    {
        auto newNodeIndex = InsertNode(reminderSize, node.DataOffset + size);

        // 隣り合うノードをリンクし、両方が空いていれば後でマージできるようにする.
        // そして、古い隣ノードを更新して、新しいノードを指すようにする.
        if (node.NeighborNext != Node::UNUSED)
            m_Nodes[node.NeighborNext].NeighborPrev = newNodeIndex;

        m_Nodes[newNodeIndex].NeighborPrev = nodeIndex;
        m_Nodes[newNodeIndex].NeighborNext = node.NeighborNext;
        node.NeighborNext = newNodeIndex;
    }

    return OffsetHandle(node.DataOffset, node.DataSize, nodeIndex);
}

//-----------------------------------------------------------------------------
//      メモリを解放します.
//-----------------------------------------------------------------------------
void OffsetAllocator::Free(OffsetHandle& handle)
{
    if (!handle.IsValid())
    {
        return;
    }

    if (!m_Nodes)
    {
        handle.Reset();
        return;
    }

    auto  nodeIndex = handle.m_MetaData;
    auto& node      = m_Nodes[nodeIndex];

    // 解放済み.
    if (!node.Used)
    {
        handle.Reset();
        return;
    }

    // 隣接とマージ
    auto offset = node.DataOffset;
    auto size   = node.DataSize;

    if ((node.NeighborPrev != Node::UNUSED) && (m_Nodes[node.NeighborPrev].Used == false))
    {
        // 前の（連続）空きノード： オフセットを前のノードのオフセットに変更.
        auto& prevNode = m_Nodes[node.NeighborPrev];
        offset = prevNode.DataOffset;
        size  += prevNode.DataSize;

        // ビンのリンクリストからノードを削除し、フリーリストに挿入.
        RemoveNode(node.NeighborPrev);

        assert(prevNode.NeighborNext == nodeIndex);
        node.NeighborPrev = prevNode.NeighborPrev;
    }

    if ((node.NeighborNext != Node::UNUSED) && (m_Nodes[node.NeighborNext].Used == false))
    {
        // 次の（連続）空きノード： オフセットは変わらない.
        auto& nextNode = m_Nodes[node.NeighborNext];
        size += nextNode.DataSize;

        // ビンのリンクリストからノードを削除し、フリーリストに挿入.
        RemoveNode(node.NeighborNext);

        assert(nextNode.NeighborPrev == nodeIndex);
        node.NeighborNext = nextNode.NeighborNext;
    }

    auto neighborNext = node.NeighborNext;
    auto neighborPrev = node.NeighborPrev;

    // フリーリストへと削除されたノードを挿入する.
    m_FreeNodes[++m_FreeOffset] = nodeIndex;

    // ビンに(結合された)フリーノードを挿入する.
    auto combinedNodeIndex = InsertNode(size, offset);

    // 新しい結合ノードと近隣ノードを接続する.
    if (neighborNext != Node::UNUSED)
    {
        m_Nodes[combinedNodeIndex].NeighborNext = neighborNext;
        m_Nodes[neighborNext].NeighborPrev      = combinedNodeIndex;
    }
    if (neighborPrev != Node::UNUSED)
    {
        m_Nodes[combinedNodeIndex].NeighborPrev = neighborPrev;
        m_Nodes[neighborPrev].NeighborNext      = combinedNodeIndex;
    }
}

//-----------------------------------------------------------------------------
//      使用サイズを取得します.
//-----------------------------------------------------------------------------
uint32_t OffsetAllocator::GetUsedSize() const
{ return m_Size - GetFreeSize(); }

//-----------------------------------------------------------------------------
//      未使用サイズを取得します.
//-----------------------------------------------------------------------------
uint32_t OffsetAllocator::GetFreeSize() const
{ return (m_FreeOffset >= 0) ? m_FreeStorage : 0; }

//-----------------------------------------------------------------------------
//      ビンにノードを挿入します.
//-----------------------------------------------------------------------------
uint32_t OffsetAllocator::InsertNode(uint32_t size, uint32_t offset)
{
    // bin >= allocとなるようにbinインデックスを切り捨てる.
    auto binIndex = FloatRoundDown(size);

    auto topBinIndex  = binIndex >> TOP_BINS_INDEX_SHIFT;
    auto leafBinIndex = binIndex & LEAF_BINS_INDEX_MASK;

    // ビンは以前は空だったか？
    if (m_BinIndices[binIndex] == Node::UNUSED)
    {
        // ビンマスクビットを設定.
        m_UsedBins[topBinIndex] |= 1 << leafBinIndex;
        m_UsedBinsTop           |= 1 << topBinIndex;
    }

    // フリーリストのノードを取り出し、ビンリンクリストの先頭に挿入 (next = old top).
    auto topNodeIndex = m_BinIndices[binIndex];
    auto nodeIndex    = m_FreeNodes[m_FreeOffset--];

    m_Nodes[nodeIndex] = GenNode(offset, size, topNodeIndex);

    if (topNodeIndex != Node::UNUSED)
        m_Nodes[topNodeIndex].BinListPrev = nodeIndex;
    m_BinIndices[binIndex] = nodeIndex;

    m_FreeStorage += size;

    return nodeIndex;
}

//-----------------------------------------------------------------------------
//      ビンからノードを削除します.
//-----------------------------------------------------------------------------
void OffsetAllocator::RemoveNode(uint32_t nodeIndex)
{
    auto& node = m_Nodes[nodeIndex];

    if (node.BinListPrev != Node::UNUSED)
    {
        // 単純なケース： 前のノードがある場合は，このノードをリストの中から削除するだけ.
        m_Nodes[node.BinListPrev].BinListNext = node.BinListNext;
        if (node.BinListNext != Node::UNUSED)
            m_Nodes[node.BinListNext].BinListPrev = node.BinListPrev;
    }
    else
    {
        // ハードケース： ビンの最初のノードであるビンを見つける.

        // bin >= allocとなるようにbinインデックスを切り捨てる.
        auto binIndex = FloatRoundDown(node.DataSize);

        auto topBinIndex  = binIndex >> TOP_BINS_INDEX_SHIFT;
        auto leafBinIndex = binIndex & LEAF_BINS_INDEX_MASK;

        m_BinIndices[binIndex] = node.BinListNext;
        if (node.BinListNext != Node::UNUSED)
            m_Nodes[node.BinListNext].BinListPrev = Node::UNUSED;

        // ビンが空か?
        if (m_BinIndices[binIndex] == Node::UNUSED)
        {
            // リーフビンのマスクビットを削除.
            m_UsedBins[topBinIndex] &= ~(1 << leafBinIndex);

            // 全てのリーフビンが空か?
            if (m_UsedBins[topBinIndex] == 0)
            {
                // トップビンマスクビットを削除.
                m_UsedBinsTop &= ~(1 << topBinIndex);
            }
        }
    }

    // フリーリストへノードを挿入.
    m_FreeNodes[++m_FreeOffset] = nodeIndex;
    m_FreeStorage -= node.DataSize;
}

//-----------------------------------------------------------------------------
//      ノードを生成します.
//-----------------------------------------------------------------------------
OffsetAllocator::Node OffsetAllocator::GenNode(uint32_t offset, uint32_t size, uint32_t binListNext)
{
    OffsetAllocator::Node node = {};
    node.DataOffset  = offset;
    node.DataSize    = size;
    node.BinListNext = binListNext;
    return node;
}

///////////////////////////////////////////////////////////////////////////////
// ThreadSafeOffsetAllocator class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      初期化処理です.
//-----------------------------------------------------------------------------
void ThreadSafeOffsetAllocator::Init(uint32_t size, uint32_t maxAllocatableCount)
{
    ScopedLock locker(&m_Lock);
    m_Allocator.Init(size, maxAllocatableCount);
}

//-----------------------------------------------------------------------------
//      終了処理です.
//-----------------------------------------------------------------------------
void ThreadSafeOffsetAllocator::Term()
{
    ScopedLock locker(&m_Lock);
    m_Allocator.Term();
}

//-----------------------------------------------------------------------------
//      リセットします.
//-----------------------------------------------------------------------------
void ThreadSafeOffsetAllocator::Reset()
{
    ScopedLock locker(&m_Lock);
    m_Allocator.Reset();
}

//-----------------------------------------------------------------------------
//      メモリを確保します.
//-----------------------------------------------------------------------------
OffsetHandle ThreadSafeOffsetAllocator::Alloc(uint32_t size)
{
    ScopedLock locker(&m_Lock);
    return m_Allocator.Alloc(size);
}

//-----------------------------------------------------------------------------
//      アライメントを指定してメモリを確保します.
//-----------------------------------------------------------------------------
OffsetHandle ThreadSafeOffsetAllocator::Alloc(uint32_t size, uint32_t alignment)
{
    uint32_t alignSize = (size + (alignment - 1)) & ~(alignment - 1);
    return Alloc(alignSize);
}

//-----------------------------------------------------------------------------
//      メモリを解放します.
//-----------------------------------------------------------------------------
void ThreadSafeOffsetAllocator::Free(OffsetHandle& handle)
{
    ScopedLock locker(&m_Lock);
    m_Allocator.Free(handle);
}

//-----------------------------------------------------------------------------
//      使用サイズを取得します.
//-----------------------------------------------------------------------------
uint32_t ThreadSafeOffsetAllocator::GetUsedSize() const
{ return m_Allocator.GetUsedSize(); }

//-----------------------------------------------------------------------------
//      未使用サイズを取得します.
//-----------------------------------------------------------------------------
uint32_t ThreadSafeOffsetAllocator::GetFreeSize() const
{ return m_Allocator.GetFreeSize(); }


} // namespace asdx
