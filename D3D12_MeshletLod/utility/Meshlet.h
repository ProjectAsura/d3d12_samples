//-----------------------------------------------------------------------------
// File : Meshlet.h
// Desc : Meshlets.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <fnd/asdxMath.h>
#include <vector>


///////////////////////////////////////////////////////////////////////////////
// uint8_t union
///////////////////////////////////////////////////////////////////////////////
union uint8_t2
{
    struct {
        uint8_t x;
        uint8_t y;
    };
    uint8_t v[2];
};

///////////////////////////////////////////////////////////////////////////////
// uint8_t3 union
///////////////////////////////////////////////////////////////////////////////
union uint8_t3
{
    struct {
        uint8_t     x;
        uint8_t     y;
        uint8_t     z;
    };
    uint8_t v[3];
};

///////////////////////////////////////////////////////////////////////////////
// uint8_t4 union
///////////////////////////////////////////////////////////////////////////////
union uint8_t4
{
    struct {
        uint8_t     x;
        uint8_t     y;
        uint8_t     z;
        uint8_t     w;
    };
    uint8_t v[4];
};

///////////////////////////////////////////////////////////////////////////////
// PrimitiveIndex structure
///////////////////////////////////////////////////////////////////////////////
struct PrimitiveIndex
{
    uint32_t  x        : 10;
    uint32_t  y        : 10;
    uint32_t  z        : 10;
    uint32_t  reserved : 2;
};
static_assert(sizeof (PrimitiveIndex) == sizeof (uint32_t), "Size Not Match");
static_assert(alignof(PrimitiveIndex) == alignof(uint32_t), "Alignment Not Match");

///////////////////////////////////////////////////////////////////////////////
// MeshletInfo structure
//////////////////////////////////////////////////////////////////////////////
struct MeshletInfo
{
    uint32_t        VertexOffset;
    uint32_t        VertexCount;
    uint32_t        PrimitiveOffset;
    uint32_t        PrimitiveCount;
    uint8_t4        NormalCone;
    asdx::Vector4   BoundingSphere;
};

///////////////////////////////////////////////////////////////////////////////
// ResSubset structure
///////////////////////////////////////////////////////////////////////////////
struct ResSubset
{
    uint64_t        MeshletOffset;
    uint64_t        MeshletCount;
    uint32_t        MaterialId;
};

///////////////////////////////////////////////////////////////////////////////
// ResMeshlets structure
///////////////////////////////////////////////////////////////////////////////
struct ResMeshlets
{
    std::vector<asdx::Vector3>      Positions;
    std::vector<asdx::Vector3>      Normals;
    std::vector<asdx::Vector3>      Tangents;
    std::vector<asdx::Vector2>      TexCoords;
    std::vector<uint8_t3>           Primitives;
    std::vector<uint32_t>           VertexIndices;
    std::vector<MeshletInfo>        Meshlets;
    std::vector<ResSubset>          Subsets;
    asdx::Vector4                   BoundingSphere;
};

//-----------------------------------------------------------------------------
//! @brief      プリミティブインデックスに変換します.
//-----------------------------------------------------------------------------
inline PrimitiveIndex ToPrimitiveIndex(const uint8_t3& value)
{
    PrimitiveIndex result = {};
    result.x = value.x;
    result.y = value.y;
    result.z = value.z;
    return result;
}

//-----------------------------------------------------------------------------
//! @brief      プリミティブインデックスからの変換を行います.
//-----------------------------------------------------------------------------
inline uint8_t3 FromPrimitiveIndex(const PrimitiveIndex& value)
{
    uint8_t3 result = {};
    result.x = uint8_t(value.x);
    result.y = uint8_t(value.y);
    result.z = uint8_t(value.z);
    return result;
}

//-----------------------------------------------------------------------------
//! @brief      メッシュレット生成を行います.
//! 
//! @param[in]      path
//! @param[out]     result
//! @retval true    生成に成功.
//! @retval false   生成に失敗.
//-----------------------------------------------------------------------------
bool CreateMeshlets(const char* path, ResMeshlets& result);

//-----------------------------------------------------------------------------
//! @brief      頂点シェーダ用の頂点インデックスを生成します.
//! 
//! @param[in]      meshlets
//! @param[out]     indices
//! @retval true    生成に成功.
//! @retval false   生成に失敗.
//-----------------------------------------------------------------------------
bool CreateVertexIndices(const ResMeshlets& meshlets, std::vector<uint32_t>& indices);

//-----------------------------------------------------------------------------
//! @brief      メッシュレットを保存します.
//! 
//! @param[in]      path        ファイルパス.
//! @param[in]      value       保存するメッシュレット.
//! @retval true    保存に成功.
//! @retval false   保存に失敗.
//-----------------------------------------------------------------------------
bool SaveResMeshlets(const char* path, const ResMeshlets& value);

//-----------------------------------------------------------------------------
//! @brief      メッシュレットを読み込みします.
//! 
//! @param[in]      path        ファイルパス.
//! @param[out]     result      読み込み先メッシュレット.
//! @retval true    読み込みに成功.
//! @retval false   読み込みに失敗.
//-----------------------------------------------------------------------------
bool LoadResMeshlets(const char* path, ResMeshlets& result);