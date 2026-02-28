//-----------------------------------------------------------------------------
// File : CompressedMeshlet.h
// Desc : Compressed Meshlet.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <Meshlet.h>


///////////////////////////////////////////////////////////////////////////////
// uint16_t2 union
///////////////////////////////////////////////////////////////////////////////
union uint16_t2
{
    struct {
        uint16_t    x;
        uint16_t    y;
    };
    uint16_t    v[2];
};

///////////////////////////////////////////////////////////////////////////////
// uint16_t3 union
///////////////////////////////////////////////////////////////////////////////
union uint16_t3
{
    struct {
        uint16_t    x;
        uint16_t    y;
        uint16_t    z;
    };
    uint16_t v[3];
};

///////////////////////////////////////////////////////////////////////////////
// uint16_t4 union
///////////////////////////////////////////////////////////////////////////////
union uint16_t4
{
    struct {
        uint16_t    x;
        uint16_t    y;
        uint16_t    z;
        uint16_t    w;
    };
    uint16_t    v[4];
};

///////////////////////////////////////////////////////////////////////////////
// uint32_t2 union
///////////////////////////////////////////////////////////////////////////////
union uint32_t2
{
    struct {
        uint32_t    x;
        uint32_t    y;
    };
    uint32_t v[2];
};

///////////////////////////////////////////////////////////////////////////////
// uint32_t3 union
///////////////////////////////////////////////////////////////////////////////
union uint32_t3
{
    struct {
        uint32_t    x;
        uint32_t    y;
        uint32_t    z;
    };
    uint32_t v[3];
};

///////////////////////////////////////////////////////////////////////////////
// uint32_t4 union
///////////////////////////////////////////////////////////////////////////////
union uint32_t4
{
    struct {
        uint32_t    x;
        uint32_t    y;
        uint32_t    z;
        uint32_t    w;
    };
    uint32_t v[4];
};

///////////////////////////////////////////////////////////////////////////////
// QuantizationInfo1
///////////////////////////////////////////////////////////////////////////////
struct QuantizationInfo1
{
    float   Base;
    float   Factor;
};

///////////////////////////////////////////////////////////////////////////////
// QuantizationInfo2
///////////////////////////////////////////////////////////////////////////////
struct QuantizationInfo2
{
    asdx::Vector2   Base;
    asdx::Vector2   Factor;
};

///////////////////////////////////////////////////////////////////////////////
// QuantizationInfo3
///////////////////////////////////////////////////////////////////////////////
struct QuantizationInfo3
{
    asdx::Vector3   Base;
    asdx::Vector3   Factor;
};

///////////////////////////////////////////////////////////////////////////////
// QuantizationInfo4
///////////////////////////////////////////////////////////////////////////////
struct QuantizationInfo4
{
    asdx::Vector4   Base;
    asdx::Vector4   Factor;
};


///////////////////////////////////////////////////////////////////////////////
// ResCompressedMeshlets structure
///////////////////////////////////////////////////////////////////////////////
struct ResCompressedMeshlets
{
    std::vector<uint16_t3>                  Positions;
    std::vector<uint16_t2>                  Normals;
    std::vector<uint16_t>                   Tangents;       // 接線復元用角度.
    std::vector<uint16_t2>                  TexCoords;
    std::vector<uint8_t3>                   Primitives;
    std::vector<uint32_t>                   VertexIndices;
    std::vector<MeshletInfo>                Meshlets;
    std::vector<ResSubset>                  Subsets;
    asdx::Vector4                           BoundingSphere;
    QuantizationInfo3                       PositionInfo;
    QuantizationInfo2                       NormalInfo;
    QuantizationInfo1                       TangentInfo;
    QuantizationInfo2                       TexCoordInfo;
    std::vector<uint32_t3>                  OffsetPosition; // 量子化用オフセット.
    std::vector<uint32_t2>                  OffsetNormal;   // 量子化用オフセット.
    std::vector<uint32_t>                   OffsetTangent;  // 量子化用オフセット.
    std::vector<uint32_t2>                  OffsetTexCoord; // 量子化用オフセット.
};

//-----------------------------------------------------------------------------
//! @brief      圧縮メッシュレットを生成します.
//! 
//! @param[in]      input       入力メッシュレット.
//! @param[out]     output      出力圧縮メッシュレット.
//! @retval true    生成に成功.
//! @retval false   生成に失敗.
//-----------------------------------------------------------------------------
bool CreateCompressedMeshlets(const ResMeshlets& input, ResCompressedMeshlets& output);

//-----------------------------------------------------------------------------
//! @brief      圧縮メッシュレットを保存します.
//! 
//! @param[in]      path        ファイルパス.
//! @param[in]      value       保存する圧縮メッシュレット.
//! @retval true    保存に成功.
//! @retval false   保存に失敗.
//-----------------------------------------------------------------------------
bool SaveCompressedMeshlets(const char* path, const ResCompressedMeshlets& value);

//-----------------------------------------------------------------------------
//! @brief      圧縮メッシュレットを読み込みます.
//! 
//! @param[in]      path        ファイルパス.
//! @param[out]     result      読み込み先メッシュレット
//! @retval true    読み込みに成功.
//! @retval false   読み込みに失敗.
//-----------------------------------------------------------------------------
bool LoadCompressedMeshlets(const char* path, ResCompressedMeshlets& result);