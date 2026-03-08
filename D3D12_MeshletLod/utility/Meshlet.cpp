//-----------------------------------------------------------------------------
// File : Meshlet.cpp
// Desc : Meshlets.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdio>
#include "Meshlet.h"
#include "MeshOBJ.h"
#include <meshoptimizer.h>
#include <fnd/asdxLogger.h>


namespace {

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
const uint32_t kResMeshletsHeaderVersion = 1u;


///////////////////////////////////////////////////////////////////////////////
// ResMeshletsHeader structure
///////////////////////////////////////////////////////////////////////////////
struct ResMeshletsHeader
{
    char            Magic[4];
    uint32_t        Version;
    uint64_t        PositionCount;
    uint64_t        NormalCount;
    uint64_t        TangentCount;
    uint64_t        TexCoordCount;
    uint64_t        VertexIndexCount;
    uint64_t        PrimitiveCount;
    uint64_t        MeshletCount;
    uint64_t        SubsetCount;
    asdx::Vector4   BoundingSphere;
};

} // namespace


//-----------------------------------------------------------------------------
//      メッシュレット生成を行います.
//-----------------------------------------------------------------------------
bool CreateMeshlets(const char* path, ResMeshlets& result)
{
    MeshOBJ mesh;
    if (!mesh.Load(path))
    {
        ELOGA("Error : MeshOBJ::Load() Filed. path = %s", path);
        return false;
    }

    auto& indices   = mesh.GetIndices  ();
    auto& positions = mesh.GetPositions();
    auto& normals   = mesh.GetNormals  ();
    auto& texcoords = mesh.GetTexCoords();
    auto& tangents  = mesh.GetTangents ();

    const size_t kMaxVertices  = 256;
    const size_t kMaxTriangles = 256;
    const float  kConeWeight   = 0.0f;

    uint64_t meshletOffset = 0;

    for(const auto& subset : mesh.GetSubsets())
    {
        auto maxMeshlets = meshopt_buildMeshletsBound(subset.Count, kMaxVertices, kMaxTriangles);

        std::vector<meshopt_Meshlet> meshlets        (maxMeshlets);
        std::vector<uint32_t>        meshletVertices (maxMeshlets * kMaxVertices);
        std::vector<uint8_t>         meshletTriangles(maxMeshlets * kMaxTriangles * 3);

        auto meshletCount = meshopt_buildMeshlets(
            meshlets.data(),
            meshletVertices.data(),
            meshletTriangles.data(),
            &indices[subset.Offset],
            subset.Count,
            &positions[0].x,
            positions.size(),
            sizeof(asdx::Vector3),
            kMaxVertices,
            kMaxTriangles,
            kConeWeight
        );

        meshlets.resize(meshletCount);

        for(auto& meshlet : meshlets)
        {
            meshopt_optimizeMeshlet(
                &meshletVertices [meshlet.vertex_offset],
                &meshletTriangles[meshlet.triangle_offset],
                meshlet.triangle_count,
                meshlet.vertex_count);

            auto primOffset = uint32_t(result.Primitives.size());
            auto vertOffset = uint32_t(result.VertexIndices.size());

            for(auto i=0u; i<meshlet.vertex_count; ++i)
            {
                auto vertIndex = meshletVertices[i + meshlet.vertex_offset];
                result.VertexIndices.emplace_back(vertIndex);
            }

            for(auto i=0u; i<meshlet.triangle_count * 3; i+= 3)
            {
                uint8_t3 tris = {};
                tris.x = meshletTriangles[i + 0 + meshlet.triangle_offset];
                tris.y = meshletTriangles[i + 1 + meshlet.triangle_offset];
                tris.z = meshletTriangles[i + 2 + meshlet.triangle_offset];

                result.Primitives.emplace_back(tris);
            }

            MeshletInfo m = {};
            m.VertexOffset     = vertOffset;//meshlet.vertex_offset;
            m.VertexCount      = meshlet.vertex_count;
            m.PrimitiveOffset  = primOffset;//meshlet.triangle_offset;
            m.PrimitiveCount   = meshlet.triangle_count;

            auto bounds = meshopt_computeMeshletBounds(
                &meshletVertices[meshlet.vertex_offset],
                &meshletTriangles[meshlet.triangle_offset],
                meshlet.triangle_count,
                &positions[0].x,
                positions.size(),
                sizeof(asdx::Vector3));

            auto normalCone = asdx::Vector4(
                asdx::Saturate(bounds.cone_axis[0] * 0.5f + 0.5f),
                asdx::Saturate(bounds.cone_axis[1] * 0.5f + 0.5f),
                asdx::Saturate(bounds.cone_axis[2] * 0.5f + 0.5f),
                asdx::Saturate(bounds.cone_cutoff  * 0.5f + 0.5f));

            m.NormalCone.x = uint8_t(normalCone.x * 255.0f);
            m.NormalCone.y = uint8_t(normalCone.y * 255.0f);
            m.NormalCone.z = uint8_t(normalCone.z * 255.0f);
            m.NormalCone.w = uint8_t(normalCone.w * 255.0f);

            m.BoundingSphere.x = bounds.center[0];
            m.BoundingSphere.y = bounds.center[1];
            m.BoundingSphere.z = bounds.center[2];
            m.BoundingSphere.w = bounds.radius;

            result.Meshlets.emplace_back(m);
        }

        ResSubset subsets = {};
        subsets.MaterialId    = subset.MaterialId;
        subsets.MeshletOffset = meshletOffset;
        subsets.MeshletCount  = meshletCount;

        result.Subsets.emplace_back(subsets);

        meshletOffset += meshletCount;
    }

    result.Positions = positions;
    result.Normals   = normals;
    result.Tangents  = tangents;
    result.TexCoords = texcoords;

    result.VertexIndices.shrink_to_fit();
    result.Primitives   .shrink_to_fit();
    result.Meshlets     .shrink_to_fit();
    result.Subsets      .shrink_to_fit();

    {
        auto bounds = meshopt_computeSphereBounds(
            &result.Positions[0].x,
            result.Positions.size(),
            sizeof(asdx::Vector3),
            nullptr,
            0);

        result.BoundingSphere = asdx::Vector4(
            bounds.center[0],
            bounds.center[1],
            bounds.center[2],
            bounds.radius);
    }

    return true;
}

//-----------------------------------------------------------------------------
//      頂点シェーダ用の頂点インデックスを求めます.
//-----------------------------------------------------------------------------
bool CreateVertexIndices(const ResMeshlets& meshlets, std::vector<uint32_t>& dstIndices)
{
    dstIndices.reserve(meshlets.Positions.size());
    for(auto& meshlet : meshlets.Meshlets)
    {
        for(size_t i=0u; i<meshlet.VertexCount; ++i)
        {
            auto vertId = meshlets.VertexIndices[i + meshlet.VertexOffset];
            dstIndices.emplace_back(vertId);
        }
    }

    dstIndices.shrink_to_fit();

    return true;
}

//-----------------------------------------------------------------------------
//      メッシュレットを保存します.
//-----------------------------------------------------------------------------
bool SaveResMeshlets(const char* path, const ResMeshlets& value)
{
    ResMeshletsHeader header = {};
    strcpy_s(header.Magic, "MSH");
    header.Version          = kResMeshletsHeaderVersion;
    header.PositionCount    = value.Positions    .size();
    header.NormalCount      = value.Normals      .size();
    header.TangentCount     = value.Tangents     .size();
    header.TexCoordCount    = value.TexCoords    .size();
    header.VertexIndexCount = value.VertexIndices.size();
    header.PrimitiveCount   = value.Primitives   .size();
    header.MeshletCount     = value.Meshlets     .size();
    header.SubsetCount      = value.Subsets      .size();
    header.BoundingSphere   = value.BoundingSphere;

    FILE* fp = nullptr;
    auto err = fopen_s(&fp, path, "wb");
    if (err != 0)
    {
        ELOG("Error : File Open Failed. path = %s", path);
        return false;
    }

    fwrite(&header, sizeof(header), 1, fp);

    if (!value.Positions    .empty()) { fwrite(value.Positions    .data(), sizeof(value.Positions    [0]), value.Positions    .size(), fp); }
    if (!value.Normals      .empty()) { fwrite(value.Normals      .data(), sizeof(value.Normals      [0]), value.Normals      .size(), fp); }
    if (!value.Tangents     .empty()) { fwrite(value.Tangents     .data(), sizeof(value.Tangents     [0]), value.Tangents     .size(), fp); }
    if (!value.TexCoords    .empty()) { fwrite(value.TexCoords    .data(), sizeof(value.TexCoords    [0]), value.TexCoords    .size(), fp); }
    if (!value.VertexIndices.empty()) { fwrite(value.VertexIndices.data(), sizeof(value.VertexIndices[0]), value.VertexIndices.size(), fp); }
    if (!value.Primitives   .empty()) { fwrite(value.Primitives   .data(), sizeof(value.Primitives   [0]), value.Primitives   .size(), fp); }
    if (!value.Meshlets     .empty()) { fwrite(value.Meshlets     .data(), sizeof(value.Meshlets     [0]), value.Meshlets     .size(), fp); }
    if (!value.Subsets      .empty()) { fwrite(value.Subsets      .data(), sizeof(value.Subsets      [0]), value.Subsets      .size(), fp); }

    fclose(fp);

    return true;
}

//-----------------------------------------------------------------------------
//      メッシュレットを読み込みします.
//-----------------------------------------------------------------------------
bool LoadResMeshlets(const char* path, ResMeshlets& result)
{
    FILE* fp = nullptr;
    auto err = fopen_s(&fp, path, "rb");
    if (err != 0)
    {
        ELOG("Error : File Open Failed. path = %s", path);
        return false;
    }

    ResMeshletsHeader header = {};
    fread(&header, sizeof(header), 1, fp);
    if (strcmp(header.Magic, "MSH") != 0)
    {
        fclose(fp);
        ELOG("Error : Invalid File.");
        return false;
    }

    if (header.Version != kResMeshletsHeaderVersion)
    {
        fclose(fp);
        ELOG("Error : Invalid Version. File Version = %u, Current Version = %u", header.Version, kResMeshletsHeaderVersion);
        return false;
    }
 
    result.Positions    .resize(header.PositionCount);
    result.Normals      .resize(header.NormalCount);
    result.Tangents     .resize(header.TangentCount);
    result.TexCoords    .resize(header.TexCoordCount);
    result.VertexIndices.resize(header.VertexIndexCount);
    result.Primitives   .resize(header.PrimitiveCount);
    result.Meshlets     .resize(header.MeshletCount);
    result.Subsets      .resize(header.SubsetCount);

    result.BoundingSphere = header.BoundingSphere;

    if (!result.Positions    .empty()) { fread(result.Positions    .data(), sizeof(result.Positions    [0]), result.Positions    .size(), fp); }
    if (!result.Normals      .empty()) { fread(result.Normals      .data(), sizeof(result.Normals      [0]), result.Normals      .size(), fp); }
    if (!result.Tangents     .empty()) { fread(result.Tangents     .data(), sizeof(result.Tangents     [0]), result.Tangents     .size(), fp); }
    if (!result.TexCoords    .empty()) { fread(result.TexCoords    .data(), sizeof(result.TexCoords    [0]), result.TexCoords    .size(), fp); }
    if (!result.VertexIndices.empty()) { fread(result.VertexIndices.data(), sizeof(result.VertexIndices[0]), result.VertexIndices.size(), fp); }
    if (!result.Primitives   .empty()) { fread(result.Primitives   .data(), sizeof(result.Primitives   [0]), result.Primitives   .size(), fp); }
    if (!result.Meshlets     .empty()) { fread(result.Meshlets     .data(), sizeof(result.Meshlets     [0]), result.Meshlets     .size(), fp); }
    if (!result.Subsets      .empty()) { fread(result.Subsets      .data(), sizeof(result.Subsets      [0]), result.Subsets      .size(), fp); }

    fclose(fp);

    return true;
}