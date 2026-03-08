//-----------------------------------------------------------------------------
// File : LodGenerator.h
// Desc : Meshlet LOD Generator.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>
#include <Meshlet.h>


///////////////////////////////////////////////////////////////////////////////
// LodMeshletInfo structure
///////////////////////////////////////////////////////////////////////////////
struct LodMeshletInfo
{
    uint8_t4        NormalCone;     //!< 法錐.
    asdx::Vector4   BoundingSphere; //!< カリング用バウンディングスフィア.

    uint32_t        MaterialId;     //!< マテリアルID.
    uint32_t        Lod;            //!< LOD番号.
    float           ParentError;    //!< 親の誤差尺度(LOD判定用).
    float           GroupError;     //!< メッシュレットが所属するグループの誤差尺度(LOD判定用).
    asdx::Vector4   ParentBounds;   //!< 親のバウンディングスフィア(LOD判定用).
    asdx::Vector4   GroupBounds;    //!< メッシュレットが所属するグループのバウンディングス(LOD判定用).

    std::vector<uint8_t3>   Primitives;     //!< プリミティブ番号.
    std::vector<uint32_t>   VertIndices;    //!< 頂点番号.
};

///////////////////////////////////////////////////////////////////////////////
// ResLodRange structure
///////////////////////////////////////////////////////////////////////////////
struct ResLodRange
{
    uint32_t  Offset;     //!< メッシュレットオフセット.
    uint32_t  Count;      //!< メッシュレット数.
};

///////////////////////////////////////////////////////////////////////////////
// ResLodSubset structure.
///////////////////////////////////////////////////////////////////////////////
struct ResLodSubset
{
    uint32_t    MaterialId;     //!< マテリアルID.
    uint32_t    MeshletOffset;  //!< 最初のLODへオフセット.
    uint32_t    MeshletCount;   //!< 全LODを含むメッシュレット数.
    uint32_t    LodRangeOffset; //!< LOD範囲へのオフセット.
    uint32_t    LodRangeCount;  //!< LOD範囲の数(=LOD数).
};

///////////////////////////////////////////////////////////////////////////////
// ResLodMeshlet structure
///////////////////////////////////////////////////////////////////////////////
struct ResLodMeshlets
{
    std::vector<asdx::Vector3>      Positions;          //!< 位置座標.
    std::vector<asdx::Vector3>      Normals;            //!< 法線ベクトル.
    std::vector<asdx::Vector3>      Tangents;           //!< 接線ベクトル.
    std::vector<asdx::Vector2>      TexCoords;          //!< テクスチャ座標.
    std::vector<LodMeshletInfo>     Meshlets;           //!< メッシュレット.
    std::vector<ResLodSubset>       Subsets;            //!< サブセット.
    std::vector<ResLodRange>        LodRanges;          //!< LOD範囲.
    asdx::Vector4                   BoundingSphere;     //!< バウンディングスフィア.
    uint32_t                        MaxLodLevel;        //!< 最大LODレベル.
};


//-----------------------------------------------------------------------------
//! @brief      LODメッシュレットを生成します.
//-----------------------------------------------------------------------------
bool CreateLodMeshlets(const ResMeshlets& meshlets, ResLodMeshlets& lodMeshlets);

//-----------------------------------------------------------------------------
//! @brief      LODメッシュレットを保存します.
//-----------------------------------------------------------------------------
//bool SaveLodMeshlets(const char* path, const ResLodMeshlets& lodMeshlets);



