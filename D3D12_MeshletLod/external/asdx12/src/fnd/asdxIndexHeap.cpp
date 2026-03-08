//-----------------------------------------------------------------------------
// File : asdxIndexHeap.cpp
// Desc : Index Heap.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <new>
#include <cassert>
#include <fnd/asdxIndexHeap.h>
#include <fnd/asdxLogger.h>

namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// IndexHolder struct
///////////////////////////////////////////////////////////////////////////////
struct IndexHolder : public List<IndexHolder>::Node
{
    uint32_t    Offset = IndexHandle::INVALID_OFFSET;
    uint32_t    Count  = 0;
    bool        Valid  = false;
};


///////////////////////////////////////////////////////////////////////////////
// IndexHandle class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      ハンドルが有効かどうかチェックします.
//-----------------------------------------------------------------------------
bool IndexHandle::IsValid() const
{
    if (m_pHeap == nullptr || m_pHolder == nullptr)
    { return false; }

    return m_pHolder->Valid;
}

//-----------------------------------------------------------------------------
//      インデックスオフセットを取得します.
//-----------------------------------------------------------------------------
uint32_t IndexHandle::GetOffset() const
{
    if (!IsValid())
    { return INVALID_OFFSET; }

    return m_pHeap->GetOffset(m_pHolder);
}

//-----------------------------------------------------------------------------
//      インデックス数を取得します.
//-----------------------------------------------------------------------------
uint32_t IndexHandle::GetCount() const
{
    if (!IsValid())
    { return 0; }

    return m_pHeap->GetCount(m_pHolder);
}

///////////////////////////////////////////////////////////////////////////////
// IndexHeap class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
IndexHeap::~IndexHeap()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool IndexHeap::Init(uint32_t count)
{
    if (m_Init)
    { return true; }

    m_Holders = new (std::nothrow) IndexHolder[count];
    if (m_Holders == nullptr)
    { return false; }

    for(auto i=0u; i<count; ++i)
    {
        m_Holders[i].Valid  = false;
        m_Holders[i].Count  = 0;
        m_Holders[i].Offset = 0;
        m_FreeList.push_back(&m_Holders[i]);
    }

    m_Capacity  = count;
    m_UsedCount = 0;
    m_Init      = true;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void IndexHeap::Term()
{
    if (!m_Init)
    { return; }

    ScopedLock locker(&m_Lock);

    for(auto& itr : m_UsedList)
    {
        itr.Valid   = false;
        itr.Offset  = IndexHandle::INVALID_OFFSET;
        itr.Count   = 0;
    }

    m_UsedList.clear();
    m_FreeList.clear();

    if (m_Holders)
    {
        delete[] m_Holders;
        m_Holders = nullptr;
    }

    m_UsedCount = 0;
    m_Capacity  = 0;
    m_Init      = false;
}

//-----------------------------------------------------------------------------
//      初期化済みかどうかチェックします.
//-----------------------------------------------------------------------------
bool IndexHeap::IsInit() const
{ return m_Init; }

//-----------------------------------------------------------------------------
//      インデックスを確保します.
//-----------------------------------------------------------------------------
IndexHandle IndexHeap::Alloc(uint32_t count)
{
    assert(m_Init == true);
    ScopedLock locker(&m_Lock);

    if (m_Capacity < m_UsedCount + count)
    {
        ELOG("Error : Max Count Over.");
        return IndexHandle();
    }

    if (m_FreeList.empty())
    {
        ELOG("Error : Handle Count Over");
        return IndexHandle();
    }

    auto node = &m_FreeList.front();
    if (node->Offset + count > m_Capacity)
    {
        ELOG("Error : Out of Memory.");
        return IndexHandle();
    }

    m_FreeList.pop_front();
    node->Count = count;
    node->Valid = true;

    m_UsedList.push_back(node);
    auto& front  = m_FreeList.front();
    front.Offset = node->Offset + count;

    m_UsedCount += count;

    return IndexHandle(this, node);
}

//-----------------------------------------------------------------------------
//      インデックスを解放します.
//-----------------------------------------------------------------------------
void IndexHeap::Free(IndexHandle& handle)
{
    assert(m_Init == true);
    ScopedLock locker(&m_Lock);

    // 異なるヒープ
    if (handle.m_pHeap != this)
    {
        assert(handle.m_pHeap != this);
        return;
    }

    // 解放済み.
    if (!handle.m_pHolder->Valid)
    {
        handle.m_pHeap   = nullptr;
        handle.m_pHolder = nullptr;
        return;
    }

    for(auto& itr : m_UsedList)
    {
        auto node = &itr;

        // 自分を探す.
        if (node == handle.m_pHolder)
        {
            m_UsedCount -= node->Count;

            auto& front = m_FreeList.front();
            // マージ可能であれば連結.
            if (front.Offset == (node->Offset + node->Count))
            { front.Offset -= node->Count; }

            // フリーリストに戻す前に無効にしておく.
            node->Valid  = false;
            node->Count  = 0;
            if (!m_FreeList.empty())
            { node->Offset = IndexHandle::INVALID_OFFSET; }

            // 使用リストから外してフリーリストに入れる.
            m_UsedList.erase(node);
            m_FreeList.push_back(node);

            // 正常終了.
            return;
        }
    }
}

//-----------------------------------------------------------------------------
//      メモリコンパクションを実行します.
//-----------------------------------------------------------------------------
bool IndexHeap::Compact()
{
    assert(m_Init == true);
    ScopedLock locker(&m_Lock);

    bool dirty = false;
    uint32_t lastIndex = 0;
    uint32_t lastCount = 0;
    
    for(auto& itr : m_UsedList)
    {
        // 前詰めにする.
        if (lastIndex + lastCount != itr.Offset)
        {
            itr.Offset = lastIndex + lastCount;
            dirty = true;
        }
        lastIndex = itr.Offset;
        lastCount = itr.Count;
    }

    auto& front = m_FreeList.front();
    front.Offset = lastIndex + lastCount;

    return dirty;
}

//-----------------------------------------------------------------------------
//      インデックスオフセットを取得します.
//-----------------------------------------------------------------------------
uint32_t IndexHeap::GetOffset(const IndexHolder* pHolder)
{
    ScopedLock locker(&m_Lock);

    if (m_UsedList.empty())
    { return IndexHandle::INVALID_OFFSET; }

    if (!pHolder->Valid)
    { return IndexHandle::INVALID_OFFSET; }

    return pHolder->Offset;
}

//-----------------------------------------------------------------------------
//      インデックス数を取得します.
//-----------------------------------------------------------------------------
uint32_t IndexHeap::GetCount(const IndexHolder* pHolder)
{
    ScopedLock locker(&m_Lock);

    if (m_UsedList.empty())
    { return 0; }

    if (!pHolder->Valid)
    { return 0; }

    return pHolder->Count;
}

//-----------------------------------------------------------------------------
//      使用数を取得します.
//-----------------------------------------------------------------------------
uint32_t IndexHeap::GetUsedCount() const
{ return m_UsedCount; }

//-----------------------------------------------------------------------------
//      未使用数を取得します.
//-----------------------------------------------------------------------------
uint32_t IndexHeap::GetFreeCount() const
{ return m_Capacity - m_UsedCount; }

} // namespace asdx
