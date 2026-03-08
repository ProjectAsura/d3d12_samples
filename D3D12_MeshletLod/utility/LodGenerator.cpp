//-----------------------------------------------------------------------------
// File : LodGenerator.cpp
// Desc : LOD Generator.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

#define NOMINMAX

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <functional>
#include <algorithm>
#include <unordered_map>

#include <LodGenerator.h>
#include <meshoptimizer.h>

#include <fnd/asdxLogger.h>

// 関数名が被って，ビルドエラーになるため名前空間に入れる.
namespace metis {
#include <GKlib.h>
#include <metis.h>
} // namespace metis


namespace {

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
static const int      kMinGroups    = 64;    // 最小グループ数.
static const uint32_t kMaxLodLevels = 256;   // 最大LOD数.


using Edge = std::pair<uint32_t, uint32_t>; // エッジデータ - (頂点0 - 頂点1).

///////////////////////////////////////////////////////////////////////////////
// EdgeHash structure
///////////////////////////////////////////////////////////////////////////////
struct EdgeHash
{
    size_t operator() (const Edge& value) const
    {
        const auto hasher = std::hash<uint32_t>{};
        return hasher(value.first) ^ hasher(value.second);
    }
};

///////////////////////////////////////////////////////////////////////////////
// MeshletGroup structure
///////////////////////////////////////////////////////////////////////////////
struct MeshletGroup
{
    std::vector<size_t>   MeshletIds;
};

///////////////////////////////////////////////////////////////////////////////
// MergeInfo structure
///////////////////////////////////////////////////////////////////////////////
struct MergeInfo
{
    std::vector<uint32_t>   Indices;
    float                   Error;
    bool                    IsMerged;
    asdx::Vector4           BoundingSphere;
};

///////////////////////////////////////////////////////////////////////////////
// MergeVertex structure
///////////////////////////////////////////////////////////////////////////////
struct MergeVertex
{
    asdx::Vector3 Pos;
    asdx::Vector3 Normal;
    asdx::Vector2 TexCoord;
};

///////////////////////////////////////////////////////////////////////////////
// SubsetMeshlets structure.
///////////////////////////////////////////////////////////////////////////////
struct SubsetMeshlets
{
    uint32_t                    MaterialId;
    std::vector<LodMeshletInfo> Meshlets;
};

using EdgeToMeshletMap  = std::unordered_map<Edge,   std::vector<size_t>, EdgeHash>; // エッジからメッシュレットへの辞書.
using MeshletToEdgeMap  = std::unordered_map<size_t, std::vector<Edge>>;             // メッシュレットからエッジへの辞書.
using VertexAdjacentMap = std::unordered_map<uint32_t, std::vector<uint32_t>>;       // 頂点番号から隣接頂点番号への辞書.

//-----------------------------------------------------------------------------
//      末尾に連結します.
//-----------------------------------------------------------------------------
template<typename T>
void add_range(std::vector<T>& dst, std::vector<T>& src)
{
    if (src.empty()) { return; }
    dst.insert(dst.end(), src.begin(), src.end());
}

//-----------------------------------------------------------------------------
//      条件に合致するものを削除します.
//-----------------------------------------------------------------------------
template<typename L, typename R, typename H, typename P, typename A, typename Predicate>
void remove_if(std::unordered_map<L, R, H, P, A>& val, Predicate pred)
{
    auto itr = std::begin(val);
    while(itr != std::end(val))
    {
        if (pred(*itr))
            itr = val.erase(itr);
        else
            itr++;
    }
}

//-----------------------------------------------------------------------------
//      重複データを削除します.
//-----------------------------------------------------------------------------
template<typename T>
void unique_erase(std::vector<T>& val)
{
    auto itr = std::unique(std::begin(val), std::end(val));
    val.erase(itr, std::end(val));
}

//-----------------------------------------------------------------------------
//      バウンディングスフィアを計算します.
//-----------------------------------------------------------------------------
asdx::Vector4 ComputeBoundingSphere(const std::vector<uint32_t>& indices, const std::vector<MergeVertex>& vertices)
{
    // MEMO : meshoptimizer内の実装から拝借.

    size_t pmin[3] = {0, 0, 0};
    size_t pmax[3] = {0, 0, 0};

    for (auto index : indices)
    {
        auto& p = vertices[index].Pos;

        // x
        pmin[0] = (p.x < vertices[pmin[0]].Pos.x) ? index : pmin[0];
        pmax[0] = (p.x > vertices[pmax[0]].Pos.x) ? index : pmax[0];
        // y
        pmin[1] = (p.y < vertices[pmin[1]].Pos.y) ? index : pmin[1];
        pmax[1] = (p.y > vertices[pmax[1]].Pos.y) ? index : pmax[1];
        // z
        pmin[2] = (p.z < vertices[pmin[2]].Pos.z) ? index : pmin[2];
        pmax[2] = (p.z > vertices[pmax[2]].Pos.z) ? index : pmax[2];
    }

    float paxisd2 = 0;
    int paxis = 0;
    for (int axis = 0; axis < 3; ++axis)
    {
        auto& p1 = vertices[pmin[axis]].Pos;
        auto& p2 = vertices[pmax[axis]].Pos;

        asdx::Vector3 d(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
        float d2 = d.x * d.x + d.y * d.y + d.z * d.z;

        if (d2 > paxisd2)
        {
            paxisd2 = d2;
            paxis = axis;
        }
    }

    auto& p1 = vertices[pmin[paxis]].Pos;
    auto& p2 = vertices[pmax[paxis]].Pos;

    asdx::Vector3 center((p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f, (p1.z + p2.z) * 0.5f);
    float radius = sqrtf(paxisd2) * 0.5f;

    for (auto index : indices)
    {
        auto& p = vertices[index].Pos;
        float d2 = (p.x - center.x) * (p.x - center.x) + (p.y - center.y) * (p.y - center.y) + (p.z - center.z) * (p.z - center.z);

        if (d2 > radius * radius)
        {
            float d = sqrtf(d2);
            assert(d > 0);

            float k = 0.5f + (radius / d) / 2;

            center.x = center.x * k + p.x * (1 - k);
            center.y = center.y * k + p.y * (1 - k);
            center.z = center.z * k + p.z * (1 - k);
            radius = (radius + d) / 2;
        }
    }

    return asdx::Vector4(center, radius);
}

//-----------------------------------------------------------------------------
//      マテリアルごとにメッシュを分割します.
//-----------------------------------------------------------------------------
void Conversion(const ResMeshlets& input, std::vector<SubsetMeshlets>& output)
{
    output.resize(input.Subsets.size());
    for(size_t i=0; i<input.Subsets.size(); ++i)
    {
        const auto& subset = input.Subsets[i];
        output[i].MaterialId = subset.MaterialId;
        output[i].Meshlets.resize(subset.MeshletCount);

        for(size_t j=0; j<subset.MeshletCount; ++j)
        {
            auto idx = subset.MeshletOffset + j;

            const auto& src = input.Meshlets[idx];
            auto& dst = output[i].Meshlets[j];

            dst.NormalCone      = src.NormalCone;
            dst.BoundingSphere  = src.BoundingSphere;
            dst.MaterialId      = subset.MaterialId;
            dst.Lod             = 0;
            dst.ParentError     = std::numeric_limits<float>::infinity();
            dst.GroupError      = 0.0f;
            dst.ParentBounds    = input.BoundingSphere;
            dst.GroupBounds     = input.BoundingSphere;

            dst.VertIndices.resize(src.VertexCount);
            dst.Primitives .resize(src.PrimitiveCount);
            for(auto k=0u; k<src.PrimitiveCount; ++k)
            {
                const auto& prims = input.Primitives[src.PrimitiveOffset + k];
                dst.Primitives[k] = prims;

                for(auto v=0; v<3; ++v)
                {
                    auto idx = prims.v[v];
                    dst.VertIndices[idx] = input.VertexIndices[idx + src.VertexOffset];
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//      ResLodMeshletに変換します.
//-----------------------------------------------------------------------------
void Conversion(const ResMeshlets& input, std::vector<LodMeshletInfo>& output)
{
    output.resize(input.Meshlets.size());
    for(size_t i=0; i<input.Subsets.size(); ++i)
    {
        const auto& subset = input.Subsets[i];
        for(size_t j=0; j<subset.MeshletCount; ++j)
        {
            auto idx = subset.MeshletOffset + j;

            const auto& src = input.Meshlets[idx];
            auto& dst = output[idx];

            dst.NormalCone      = src.NormalCone;
            dst.BoundingSphere  = src.BoundingSphere;
            dst.MaterialId      = subset.MaterialId;
            dst.Lod             = 0;
            dst.ParentError     = std::numeric_limits<float>::infinity();
            dst.GroupError      = 0.0f;
            dst.ParentBounds    = input.BoundingSphere;
            dst.GroupBounds     = input.BoundingSphere;

            dst.VertIndices.resize(src.VertexCount);
            dst.Primitives .resize(src.PrimitiveCount);
            for(auto k=0u; k<src.PrimitiveCount; ++k)
            {
                const auto& prims = input.Primitives[src.PrimitiveOffset + k];
                dst.Primitives[k] = prims;

                for(auto v=0; v<3; ++v)
                {
                    auto idx = prims.v[v];
                    dst.VertIndices[idx] = input.VertexIndices[idx + src.VertexOffset];
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//      ボーダーエッジマップを構築します.
//-----------------------------------------------------------------------------
void BuildMeshletConectivity
(
    const std::vector<LodMeshletInfo>&  meshlets,
    EdgeToMeshletMap&                   edge2Meshlet,
    MeshletToEdgeMap&                   meshlet2Edge
)
{
    // エッジリストを構築.
    for(size_t mId = 0; mId < meshlets.size(); ++mId)
    {
        const auto& meshlet = meshlets[mId];
        for(size_t pId = 0; pId < meshlet.Primitives.size(); ++pId)
        {
            const auto& prim = meshlet.Primitives[pId];
            for(size_t j=0; j<3; ++j)
            {
                const auto& e0 = prim.v[j];
                const auto& e1 = prim.v[(j + 1) % 3];
                if (e0 == e1)
                    continue;

                Edge edge = { std::min(e0, e1), std::max(e0, e1) };

                edge2Meshlet[edge].push_back(mId);
                meshlet2Edge[mId].emplace_back(edge);
            }
        }
    }

    // メッシュレットが1つでないものは複数接続されていてボーダーではないので，削除する.
    {
        remove_if(edge2Meshlet, [&](const auto& pair) {
            return pair.second.size() != 1;
        });
    }
}

//-----------------------------------------------------------------------------
//      境界上の頂点を求めます.
//-----------------------------------------------------------------------------
void BuildVertexConectivity
(
    const std::vector<LodMeshletInfo>&  meshlets,
    size_t                              vertexCount,
    std::vector<bool>&                  boundaryVertices,
    VertexAdjacentMap&                  adjacentVertices
)
{
    EdgeToMeshletMap edge2Meshlet;
    adjacentVertices.clear();

    // エッジリストを構築.
    for(size_t mId = 0; mId < meshlets.size(); ++mId)
    {
        const auto& meshlet = meshlets[mId];
        for(size_t pId = 0; pId < meshlet.Primitives.size(); ++pId)
        {
            const auto& prim = meshlet.Primitives[pId];
            for(size_t j=0; j<3; ++j)
            {
                const auto& e0 = prim.v[j];
                const auto& e1 = prim.v[(j + 1) % 3];
                if (e0 == e1)
                    continue;

                auto v0 = meshlet.VertIndices[e0];
                auto v1 = meshlet.VertIndices[e1];

                Edge edge = { std::min(v0, v1), std::max(v0, v1) };

                edge2Meshlet[edge].push_back(mId);

                adjacentVertices[v0].push_back(v1);
                adjacentVertices[v1].push_back(v0);
            }
        }
    }

    // メッシュレットが1つでないものは複数接続されているのでボーダーではないので，削除する.
    {
        remove_if(edge2Meshlet, [&](const auto& pair) {
            return pair.second.size() != 1;
        });
    }

    // 重複削除
    for(auto& pair : adjacentVertices)
    { unique_erase(pair.second); }

    for(size_t i=0; i<boundaryVertices.size(); ++i)
    { boundaryVertices[i] = false; }

    for(const auto& pair : edge2Meshlet)
    {
        if (pair.second.size() != 1)
            continue;

        auto e0 = pair.first.first;
        auto e1 = pair.first.second;
        boundaryVertices[e0] = true;
        boundaryVertices[e1] = true;
    }
}

//-----------------------------------------------------------------------------
//      接続性に基づいて，メッシュレットをグループ化します.
//-----------------------------------------------------------------------------
std::vector<MeshletGroup> GroupMeshlets(const std::vector<LodMeshletInfo>& meshlets)
{
    using namespace metis;

    EdgeToMeshletMap e2m;
    MeshletToEdgeMap m2e;

    auto meshletCount = uint32_t(meshlets.size());
    // あまりにも小さいやつは1個にまとめる.
    if (meshletCount <= (kMinGroups * 2))
    {
        MeshletGroup group;
        group.MeshletIds.resize(meshletCount);
        for(auto i=0u; i<meshletCount; ++i)
        { group.MeshletIds[i] = i; }

        return std::vector<MeshletGroup>{ group };
    }

    // 接続性を構築.
    BuildMeshletConectivity(meshlets, e2m, m2e);

    // 接続性が無い場合は，1つのグループとして返す.
    if (e2m.empty())
    {
        MeshletGroup group;
        group.MeshletIds.resize(meshletCount);
        for(auto i=0u; i<meshletCount; ++i)
        { group.MeshletIds[i] = i; }

        return std::vector<MeshletGroup>{ group };
    }

    idx_t count = idx_t(meshletCount);

    // idx_t はMETISで定義されている.
    std::vector<idx_t>   partition;
    std::vector<idx_t>   xAdjacency;
    std::vector<idx_t>   edgeAdjacency;
    std::vector<idx_t>   edgeWeights;

    partition .resize (count);
    xAdjacency.reserve(count + 1);

    for(size_t i=0; i<meshletCount; ++i)
    {
        auto offsetEdgeAdjacency = idx_t(edgeAdjacency.size());
        for(const auto& edge : m2e[i])
        {
            // 境界エッジからメッシュレットを探す.
            auto itr = e2m.find(edge);
            if (itr == e2m.end())
                continue;

            // 接続されているメッシュレットについて処理.
            const auto& connections = itr->second;
            for(const auto& connectedMeshlet : connections)
            {
                // 自分自身ならスキップ.
                if (connectedMeshlet == i)
                    continue;

                // 隣接エッジリストに登録されているかチェック.
                auto edgeItr = std::find(
                    edgeAdjacency.begin() + offsetEdgeAdjacency,
                    edgeAdjacency.end(),
                    connectedMeshlet);

                // 未登録.
                if (edgeItr == edgeAdjacency.end())
                {
                    edgeAdjacency.emplace_back(idx_t(connectedMeshlet));
                    edgeWeights  .emplace_back(1);
                }
                // 登録済み.
                else
                {
                    auto d = std::distance(edgeAdjacency.begin(), edgeItr);
                    assert(d >= 0);
                    edgeWeights[d]++;
                }
            }
        }

        // メッシュレット開始番号を登録.
        xAdjacency.push_back(offsetEdgeAdjacency);
    }
    xAdjacency.push_back(idx_t(edgeAdjacency.size()));

    idx_t constrainCount = 1;
    idx_t partsCount     = count / kMinGroups;

    idx_t options[METIS_NOPTIONS] = {};
    METIS_SetDefaultOptions(options);

    options[METIS_OPTION_OBJTYPE]   = METIS_OBJTYPE_CUT;
    options[METIS_OPTION_CCORDER]   = 1; // Identifies the connected components.
    options[METIS_OPTION_NUMBERING] = 0; // C-Style.

    idx_t edgeCut; // final cost of the cut found by METIS.

    // METISを使ってグループ分けする.
    auto ret = METIS_PartGraphKway(
        &count,
        &constrainCount,
        xAdjacency.data(),
        edgeAdjacency.data(),
        nullptr, // vertex weights.
        nullptr, // vertex size
        edgeWeights.data(),
        &partsCount,
        nullptr,
        nullptr,
        options,
        &edgeCut,
        partition.data());

    // グループ番号を記録する.
    std::vector<MeshletGroup> groups(partsCount);
    for(auto i=0u; i<meshletCount; ++i)
    {
        auto groupId = partition[i];
        groups[groupId].MeshletIds.push_back(i);
    }

    // グループ分けした番号を返却.
    return groups;
}

//-----------------------------------------------------------------------------
//      グループ化されたメッシュレットをマージし，ポリゴン削減を行います.
//-----------------------------------------------------------------------------
MergeInfo SimplifyGroup
(
    const MeshletGroup&                 group,
    const std::vector<LodMeshletInfo>&  meshlets,
    const std::vector<asdx::Vector3>&   positions,
    const std::vector<asdx::Vector3>&   normals,
    const std::vector<asdx::Vector2>&   texcoords,
    const std::vector<uint32_t>&        vertIndices,
    uint32_t                            lodIndex
)
{
    MergeInfo result;

    std::unordered_map<uint32_t, uint32_t> dict; // mergeIndex <---> verteIndex の辞書.

    // グループごとにマージする.
    std::vector<uint32_t>    mergedIdx;
    std::vector<MergeVertex> mergedPos;

    // 最大数でメモリ確保.
    mergedIdx.reserve(group.MeshletIds.size() * 256 * 3);
    mergedPos.reserve(group.MeshletIds.size() * 256);

    for(size_t i=0; i<group.MeshletIds.size(); ++i)
    {
        const auto& meshletId = group.MeshletIds[i];
        const auto& meshlet   = meshlets[meshletId];

        for(size_t j=0u; j<meshlet.Primitives.size(); ++j)
        {
            const auto& tris = meshlet.Primitives[j];

            // マージで壊れたやつは取り除く.
            if (tris.x == tris.y && tris.x == tris.z)
                continue;

            for(auto k=0; k<3; ++k)
            {
                auto vertId = meshlet.VertIndices[tris.v[k]];
                auto index  = uint32_t(mergedIdx.size());

                MergeVertex vtx = {};
                vtx.Pos      = positions[vertId];
                vtx.Normal   = normals[vertId];
                vtx.TexCoord = texcoords[vertId];

                mergedIdx.emplace_back(index);
                mergedPos.emplace_back(vtx);

                dict.try_emplace(index, vertId);
            }
        }
    }
    mergedIdx.shrink_to_fit();
    mergedPos.shrink_to_fit();

    // 重複データを削る.
    {
        std::vector<uint32_t> remap(mergedIdx.size());
        auto vertexCount = meshopt_generateVertexRemap(
            remap.data(), mergedIdx.data(), mergedIdx.size(), mergedPos.data(), mergedPos.size(), sizeof(mergedPos[0]));

        // 位置座標をリマップ.
        {
            std::vector<MergeVertex> temp(vertexCount);
            meshopt_remapVertexBuffer(temp.data(), &mergedPos[0], mergedPos.size(), sizeof(mergedPos[0]), remap.data());
            mergedPos = std::move(temp);
        }

        // 辞書をリマップ.
        {
            std::unordered_map<uint32_t, uint32_t> remapDict;
            for(const auto& pair : dict)
            {
                auto newIdx = pair.first;
                auto vertId = pair.second;

                newIdx = remap[newIdx];
                remapDict.try_emplace(newIdx, vertId);
            }

            dict = std::move(remapDict);
        }

        // 頂点インデックスをリマップ.
        mergedIdx = std::move(remap);
    }

    // ポリゴンを半分ずつ減らす.
    size_t targetIndexCount = mergedIdx.size() / 2;

    // ポリゴン削減を行う.
    if (targetIndexCount > 0)
    {
        uint32_t options = meshopt_SimplifyLockBorder | meshopt_SimplifyErrorAbsolute;

        float clusterError    = 0.0f;   // QEM (Quadratic Error Metrics) の格納先.
        float permissiveError = 0.03f;  // 許容誤差 (3%未満とする).

        assert(targetIndexCount > 0);

        std::vector<uint32_t> indices(mergedIdx.size());

        const float kWeight = 1.0f;
        const float kAttrWeights[] = {
            kWeight,    // Normal.x
            kWeight,    // Normal.y
            kWeight,    // Normal.z
            kWeight,    // TexCoord.x
            kWeight     // TexCoord.y
        };

        // 法線ベクトルとテクスチャ座標を考慮して簡略化.
        auto indexCount = meshopt_simplifyWithAttributes(
            indices.data(),
            mergedIdx.data(),
            mergedIdx.size(),
            &mergedPos[0].Pos.x,
            mergedPos.size(),
            sizeof(MergeVertex),
            &mergedPos[0].Normal.x,
            sizeof(MergeVertex),
            kAttrWeights,
            _countof(kAttrWeights),
            nullptr,
            targetIndexCount,
            permissiveError,
            options,
            &clusterError
        );
        indices.resize(indexCount);

        // 簡略化後は不要なので解放.
        mergedIdx.clear();
        mergedIdx.shrink_to_fit();

        // バウンディングスフィアを求める.
        result.BoundingSphere = ComputeBoundingSphere(indices, mergedPos);

        mergedPos.clear();
        mergedPos.shrink_to_fit();

        // エラー値を設定.
        result.Error = clusterError;

        // 元の頂点インデックス番号を復元する.
        result.Indices.reserve(indices.size());
        for(const auto& index : indices)
        {
            auto vertId = dict[index];
            result.Indices.emplace_back(vertId);
        }

        // エラーが 0 なければ，マージされて変形した.
        result.IsMerged = (clusterError > 0.0f);
    }
    // ポリゴン削減を行わない.
    else
    {
        result.Error = 0.0f;    // 頂点をそのまま使うので...

        // 元の頂点インデックス番号を復元する.
        result.Indices.reserve(mergedIdx.size());
        for(const auto& index : mergedIdx)
        {
            auto vertId = dict[index];
            result.Indices.emplace_back(vertId);
        }

        // マージせず.
        result.IsMerged = false;

        auto bounds = meshopt_computeSphereBounds(&mergedPos[0].Pos.x, mergedPos.size(), sizeof(mergedPos[0]), nullptr, 0);
        result.BoundingSphere.x = bounds.center[0];
        result.BoundingSphere.y = bounds.center[1];
        result.BoundingSphere.z = bounds.center[2];
        result.BoundingSphere.w = bounds.radius;
    }

    return result;
}

//-----------------------------------------------------------------------------
//      ポリゴン削減されたメッシュから，新しくメッシュレットを生成します.
//-----------------------------------------------------------------------------
std::vector<LodMeshletInfo> BuildMeshlets
(
    const MergeInfo&                    mergeInfo,
    const std::vector<asdx::Vector3>&   positions,
    uint32_t                            lodIndex,
    uint32_t                            materialId,
    float                               parentError
)
{
    const size_t kMaxVertices  = 256;
    const size_t kMaxTriangles = 256;
    const float  kConeWeight   = 0.0f;

    // メッシュレットサイズを算出.
    auto maxMeshlets = meshopt_buildMeshletsBound(mergeInfo.Indices.size(), kMaxVertices, kMaxTriangles);

    std::vector<meshopt_Meshlet> meshlets        (maxMeshlets);
    std::vector<uint32_t>        meshletVertices (maxMeshlets * kMaxVertices);
    std::vector<uint8_t>         meshletTriangles(maxMeshlets * kMaxTriangles * 3);

    // メッシュレットを生成.
    auto meshletCount = meshopt_buildMeshlets(
        meshlets.data(),
        meshletVertices.data(),
        meshletTriangles.data(),
        mergeInfo.Indices.data(),
        mergeInfo.Indices.size(),
        &positions[0].x,
        positions.size(),
        sizeof(asdx::Vector3),
        kMaxVertices,
        kMaxTriangles,
        kConeWeight
    );

    std::vector<LodMeshletInfo> result(meshletCount);

    for(auto i=0; i<meshletCount; ++i)
    {
        const auto& src = meshlets[i];
        auto& dst = result[i];

        meshopt_optimizeMeshlet(
            &meshletVertices [src.vertex_offset],
            &meshletTriangles[src.triangle_offset],
            src.triangle_count,
            src.vertex_count);

        dst.VertIndices.resize(src.vertex_count);
        for(auto i=0u; i<src.vertex_count; ++i)
        {
            dst.VertIndices[i] = meshletVertices[src.vertex_offset + i];
        }

        dst.Primitives.resize(src.triangle_count);
        for(auto i=0u, j=0u; i<src.triangle_count * 3; i+= 3, ++j)
        {
            uint8_t3 tris = {};
            tris.x = meshletTriangles[i + 0 + src.triangle_offset];
            tris.y = meshletTriangles[i + 1 + src.triangle_offset];
            tris.z = meshletTriangles[i + 2 + src.triangle_offset];
            dst.Primitives[j] = tris;
        }

        // カリング用情報を生成.
        auto bounds = meshopt_computeMeshletBounds(
            &meshletVertices[src.vertex_offset],
            &meshletTriangles[src.triangle_offset],
            src.triangle_count,
            &positions[0].x,
            positions.size(),
            sizeof(asdx::Vector3));

        auto normalCone = asdx::Vector4(
            asdx::Saturate(bounds.cone_axis[0] * 0.5f + 0.5f),
            asdx::Saturate(bounds.cone_axis[1] * 0.5f + 0.5f),
            asdx::Saturate(bounds.cone_axis[2] * 0.5f + 0.5f),
            asdx::Saturate(bounds.cone_cutoff  * 0.5f + 0.5f));

        dst.NormalCone.x = uint8_t(normalCone.x * 255.0f);
        dst.NormalCone.y = uint8_t(normalCone.y * 255.0f);
        dst.NormalCone.z = uint8_t(normalCone.z * 255.0f);
        dst.NormalCone.w = uint8_t(normalCone.w * 255.0f);

        dst.BoundingSphere.x = bounds.center[0];
        dst.BoundingSphere.y = bounds.center[1];
        dst.BoundingSphere.z = bounds.center[2];
        dst.BoundingSphere.w = bounds.radius;

        dst.Lod        = lodIndex;
        dst.MaterialId = materialId;

        dst.GroupError  = mergeInfo.Error + parentError;
        dst.GroupBounds = mergeInfo.BoundingSphere;

        dst.ParentError  = std::numeric_limits<float>::infinity();
        dst.ParentBounds = asdx::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    return result;
}

} // namespace


//-----------------------------------------------------------------------------
//      LODメッシュレットを生成します.
//-----------------------------------------------------------------------------
bool CreateLodMeshlets(const ResMeshlets& meshlets, ResLodMeshlets& lodMesh)
{
    // サブセットごとにLODメッシュレットに変換.
    std::vector<SubsetMeshlets> subsets;
    Conversion(meshlets, subsets);

    // サブセットのメモリを確保.
    lodMesh.Subsets.resize(subsets.size());

    uint32_t maxLodLevel = 0;

    // マテリアルごとに処理.
    for(size_t i=0; i<subsets.size(); ++i)
    {
        // 入力データ.
        std::vector<LodMeshletInfo> input = std::move(subsets[i].Meshlets);

        // LOD範囲.
        ResLodRange range = {};
        range.Offset = uint32_t(lodMesh.Meshlets.size());
        range.Count  = uint32_t(input.size());

        // マテリアルごとの全LODを含むメッシュレット総数のカウンター.
        uint32_t totalMeshletCount = uint32_t(input.size());
        
        lodMesh.Subsets[i].MaterialId       = subsets[i].MaterialId;
        lodMesh.Subsets[i].MeshletOffset    = range.Offset;
        lodMesh.Subsets[i].LodRangeOffset   = uint32_t(lodMesh.LodRanges.size());
        lodMesh.LodRanges.emplace_back(range);

        // メッシュレットを追加.
        add_range(lodMesh.Meshlets, input);

        // LODレベル.
        uint32_t lodIndex = 1;

        // 指定数に達するまでループ.
        while(input.size() > 1 && lodIndex < (kMaxLodLevels - 1))
        {
            // 接続性に基づいてメッシュレットをグループ化.
            auto groups = GroupMeshlets(input);

            bool isMerged = false;
            std::vector<LodMeshletInfo> simplifies;
            for(const auto& group : groups)
            {
                // グループ化したものを1つのメッシュにマージして，ポリゴン削減する.
                auto mergedInfo = SimplifyGroup(group, input, meshlets.Positions, meshlets.Normals, meshlets.TexCoords, meshlets.VertexIndices, lodIndex);

                // マージされていなければ以降の処理はスキップ.
                if (!mergedInfo.IsMerged)
                    continue;

                float parentError = 0;
                for(auto id : group.MeshletIds)
                {
                    const auto& meshlet = input[id];
                    parentError = asdx::Max(parentError, meshlet.GroupError);
                }

                // ポリゴン削減されたメッシュを，新しくメッシュレットに分割.
                auto newOnes = BuildMeshlets(mergedInfo, meshlets.Positions, lodIndex, subsets[i].MaterialId, parentError);

                const auto groupError = mergedInfo.Error + parentError;
                for(auto& id : group.MeshletIds)
                {
                    // 1つ前のLOD(=入力データinput)が今新しく作ったメッシュレットの親になる.
                    const auto offset = lodMesh.LodRanges.back().Offset;
                    auto& parent = lodMesh.Meshlets[offset + id];
                    parent.ParentError  = groupError;
                    parent.ParentBounds = mergedInfo.BoundingSphere;
                }

                // 新しいメッシュレットを追加.
                add_range(simplifies, newOnes);

                // マージした.
                isMerged = true;
            }

            // 1回もマージされなければおしまい.
            if (!isMerged)
                break;

            // 新しいメッシュレットに差し替える.
            input = std::move(simplifies);

            // LOD範囲を設定.
            range.Offset = uint32_t(lodMesh.Meshlets.size());
            range.Count  = uint32_t(input.size());
            lodMesh.LodRanges.emplace_back(range);

            totalMeshletCount += range.Count;

            // LODレベルをカウントアップ.
            lodIndex++;

            add_range(lodMesh.Meshlets, input);
        }

        // 総数を記録.
        lodMesh.Subsets[i].MeshletCount  = totalMeshletCount;
        lodMesh.Subsets[i].LodRangeCount = lodIndex;

        maxLodLevel = asdx::Max(maxLodLevel, lodIndex);
    }

    lodMesh.Positions       = meshlets.Positions;
    lodMesh.Normals         = meshlets.Normals;
    lodMesh.Tangents        = meshlets.Tangents;
    lodMesh.TexCoords       = meshlets.TexCoords;
    lodMesh.BoundingSphere  = meshlets.BoundingSphere;
    lodMesh.MaxLodLevel     = maxLodLevel;

    lodMesh.LodRanges.shrink_to_fit();

    // 正常終了.
    return true;
}
