//-----------------------------------------------------------------------------
// File : asdxPool.h
// Desc : Pool Container.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cassert>
#include <new>
#include <fnd/asdxList.h>


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// Pool class
///////////////////////////////////////////////////////////////////////////////
template<typename T>
class Pool
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    Pool() = default;

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~Pool()
    { Term(); }

    //-------------------------------------------------------------------------
    //! @brief      初期化処理です.
    //! 
    //! @param[in]      size        プールのサイズ.
    //! @retval true    初期化に成功.
    //! @retval false   初期化に失敗.
    //-------------------------------------------------------------------------
    bool Init(size_t size)
    {
        if (m_Init)
        { return false; }

        m_pEnetries = new(std::nothrow) Holder [size];
        if (m_pEntries == nullptr)
        { return false; }

        for(size_t i=0; i<size; ++i)
        { m_FreeList.push_back(&m_pEntries[i]); }

        m_Init = true;
        return true;
    }

    //-------------------------------------------------------------------------
    //! @brief      終了処理です.
    //-------------------------------------------------------------------------
    void Term()
    {
        if (!m_Init)
        { return; }

        m_FreeList.clear();
        m_UsedList.clear();

        if (m_pEntries)
        {
            delete[] m_pEntries;
            m_pEntries = nullptr;
        }

        m_Init = false;
    }

    //-------------------------------------------------------------------------
    //! @brief      プールに登録します.
    //! 
    //! @param[in]      item        登録するアイテム.
    //! @retval true    登録に成功.
    //! @retval false   登録に失敗.
    //-------------------------------------------------------------------------
    bool Add(T item)
    {
        if (m_FreeList.empty())
        { return false; }

        auto node = &m_FreeList.front();
        m_FreeList.pop_front();

        node->m_Item = item;

        m_UsedList.push_back(node);
        m_Count++;
        return true;
    }

    //-------------------------------------------------------------------------
    //! @brief      プールから削除します.
    //! 
    //! @param[in]      item        削除するアイテム.
    //! @retval true    登録に成功.
    //! @retval false   登録に失敗.
    //-------------------------------------------------------------------------
    bool Remove(T item)
    {
        for(auto& itr : m_UsedList)
        {
            if (itr.m_Item == item)
            {
                auto node = &itr;
                m_UsedList.erase(itr);

                node.m_Item = T();

                m_FreeList.push_back(node);
                m_Count--;
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------
    //! @brief      登録済みかどうかチェックします.
    //! 
    //! @param[in]      item        チェックするアイテム.
    //! @retval true    登録済みです.
    //! @retval false   未登録です.
    //-------------------------------------------------------------------------
    bool Contains(T item)
    {
        for(auto& itr : m_UsedList)
        {
            if (itr.m_Item == item)
            { return true; }
        }

        return false;
    }

    //-------------------------------------------------------------------------
    //! @brief      登録数を取得します.
    //! 
    //! @return     登録数を返却します.
    //-------------------------------------------------------------------------
    size_t GetCount() const { return m_Count; }

    //-------------------------------------------------------------------------
    //! @brief      プールから削除します.
    //! 
    //! @param[in]      checker     チェック関数.
    //! @param[in]      key         チェックキー.
    //! @retval true    登録に成功.
    //! @retval false   登録に失敗.
    //-------------------------------------------------------------------------
    template<typename Checker, typename Key>
    bool Remove(Checker checker, Key key)
    {
        for(auto& itr : m_UsedList)
        {
            if (checker(itr, key))
            {
                auto node = &itr;
                m_UsedList.erase(itr);

                node.m_Item = T();

                m_FreeList.push_back(node);
                m_Count--;
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------
    //! @brief      登録済みかどうかチェックします.
    //! 
    //! @param[in]      checker     チェック関数.
    //! @param[in]      key         チェックキー.
    //! @retval true    登録済みです.
    //! @retval false   未登録です.
    //-------------------------------------------------------------------------
    template<typename Checker, typename Key>
    bool Contains(Checker checker, Key key)
    {
        for(auto& itr : m_UsedList)
        {
            if (checker(itr, key))
            { return true; }
        }

        return false;
    }

    //-------------------------------------------------------------------------
    //! @brief      指定キーに応じたアイテム取得を試みます.
    //! 
    //! @param[in]      checker     チェック関数.
    //! @param[in]      key         チェックキー.
    //! @retval true    取得に成功.
    //! @retval false   取得に失敗.
    //-------------------------------------------------------------------------
    template<typename Checker, typename Key>
    bool TryGet(Checker checker, Key key, T* pResult)
    {
        assert(pResult != nullptr);

        for(auto& itr : m_UsedList)
        {
            if (checker(itr, key))
            {
                *pResult = item.m_Item;
                return true;
            }
        }

        return false;
    }


private:
    ///////////////////////////////////////////////////////////////////////////
    // Holder structure
    ///////////////////////////////////////////////////////////////////////////
    struct Holder : public List<Holder>::Node
    {
        T m_Item = {};
    };

    //=========================================================================
    // private variables.
    //=========================================================================
    List<Holder> m_FreeList;
    List<Holder> m_UsedList;
    Holder*      m_pEntries = nullptr;
    bool         m_Init     = false;
    size_t       m_Count    = 0;

    //=========================================================================
    // private methods.
    //=========================================================================
    /* NOTHING */
};

} // namespace asdx
