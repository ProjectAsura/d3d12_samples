//-----------------------------------------------------------------------------
// File : asdxOctree.h
// Desc : Octree
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <fnd/asdxBit.h>
#include <fnd/asdxList.h>
#include <fnd/asdxMath.h>
#include <unordered_map>


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// Octree class
///////////////////////////////////////////////////////////////////////////////
template<typename T>
class Octree
{
public:
    struct Node
    {
        List<T>     Objects;
    };

    void Init(uint8_t maxLevels, const Vector3& rootMin, const Vector3& rootMax)
    {
        if (!m_Nodes.empty())
        { Term(); }

        m_RootMin   = rootMin;
        m_RootMax   = rootMax;
        m_MaxLevels = maxLevels;

        auto size = Vector3::Abs(rootMax - rootMin);
        m_CellSize = size / ((float)(1 << maxLevels));
    }

    void Term()
    {
        if (!m_Nodes.empty())
        {
            for(auto& itr : m_Nodes)
            { itr.second.Objects.clear(); }
            m_Nodes.clear();
        }
        m_CellSize  = {};
        m_RootMin   = {};
        m_RootMax   = {};
        m_MaxLevels = 0;
    }

    void Add(uint32_t hash, T* object)
    {
        assert(object != nullptr);
        auto itr = m_Nodes.find(hash);
        if (itr != m_Nodes.end())
        {
            itr->second.Objects.push_back(object);
            return;
        }

        Node node = {};
        m_Nodes[hash] = node;
        m_Nodes[hash].Objects.push_back(object);
    }

    void Remove(uint32_t hash, T* object)
    {
        assert(object != nullptr);
        auto itr = m_Nodes.find(hash);
        if (itr == m_Nodes.end())
            return;

        itr.Objects.erase(object);
    }

    Node* Find(uint32_t hash)
    {
        auto itr = m_Nodes.find(hash);
        return (itr == m_Nodes.end()) ? nullptr : &(itr->second);
    }

    const Node* Find(uint32_t hash) const
    {
        auto itr = m_Nodes.find(hash);
        return (itr == m_Nodes.end()) ? nullptr : &(itr->second);
    }

    uint32_t CalcHash(const Vector3& mini, const Vector3& maxi)
    {
        auto lhs   = CalcPointCode(mini);
        auto rhs   = CalcPointCode(maxi);
        auto xor   = lhs ^ rhs;
        auto shift = 32 - CountZeroL(xor);
        return lhs >> shift;
    }

    static uint32_t ToIndex(uint32_t hash, uint32_t level)
    {
        auto offset = ((1 << (level * 3)) - 1) / 7;
        return hash + offset;
    }

    static uint32_t CalcParentCode(uint32_t childCode)
    { return childCode >> 3; }

    static uint32_t CalcChildCode(uint32_t parentCode, uint8_t childIndex)
    { return (parentCode << 3) | childIndex; }

    uint8_t GetMaxLevels() const
    { return m_MaxLevels; }

    size_t GetNodeCount() const
    { return m_Nodes.size(); }

    const Vector3& GetCellSize() const
    { return m_CellSize; }

    const Vector3& GetRootMax() const 
    { return m_RootMax; }

    const Vector3& GetRootMin() const
    { return m_RootMin; }

private:
    std::unordered_map<uint32_t, Node> m_Nodes; // ノード.

    uint8_t     m_MaxLevels = 0;        // レベル数.
    Vector3     m_RootMin   = {};       // ルートレベルの最小値.
    Vector3     m_RootMax   = {};       // ルートレベルの最大値.
    Vector3     m_CellSize  = {};       // 末端のセルサイズ.

    uint32_t CalcPointCode(const Vector3& p)
    {
        return EncodeMorton3(
            uint32_t((p.x - m_RootMin.x) / m_CellSize.x),
            uint32_t((p.y - m_RootMin.y) / m_CellSize.y),
            uint32_t((p.z - m_RootMin.z) / m_CellSize.z));
    }
};

} // namespace asdx
