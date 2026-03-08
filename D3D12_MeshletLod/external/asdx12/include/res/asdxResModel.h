//-----------------------------------------------------------------------------
// File : asdxResModel.h
// Desc : Model Resource.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>
#include <string>
#include <vector>
#include <fnd/asdxMath.h>


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// ResMesh structure
///////////////////////////////////////////////////////////////////////////////
struct ResMesh
{
    static const uint32_t MAX_TEXCOORD_LAYERS = 4;   //!< 最大テクスチャ座標レイヤー数.

    std::string                 Name;                           //!< メッシュ名.
    uint32_t                    MaterialId;                     //!< マテリアルID.
    uint32_t                    BoneInfluenceCount;             //!< 影響するボーン数.
    std::vector<asdx::Vector3>  Positions;                      //!< 位置座標.
    std::vector<asdx::Vector3>  Normals;                        //!< 法線ベクトル.
    std::vector<asdx::Vector3>  Tangents;                       //!< 接線ベクトル.
    std::vector<asdx::Vector2>  TexCoords[MAX_TEXCOORD_LAYERS]; //!< テクスチャ座標.
    std::vector<asdx::Vector4>  Colors;                         //!< 頂点カラー.
    std::vector<uint16_t>       BoneIndices;                    //!< ボーン番号.
    std::vector<float>          BoneWeights;                    //!< ボーン重み.
    std::vector<uint32_t>       VertexIndices;                  //!< 頂点番号.

    //-------------------------------------------------------------------------
    //! @brief      解放処理を行います.
    //-------------------------------------------------------------------------
    void Dispose()
    {
        Name          .clear();
        Positions     .clear();
        Normals       .clear();
        for(auto i=0u; i<MAX_TEXCOORD_LAYERS; ++i)
        { TexCoords[i].clear(); }
        Colors        .clear();
        BoneIndices   .clear();
        BoneWeights   .clear();
        VertexIndices .clear();

        Name          .shrink_to_fit();
        Positions     .shrink_to_fit();
        Normals       .shrink_to_fit();
        for(auto i=0u; i<MAX_TEXCOORD_LAYERS; ++i)
        { TexCoords[i].shrink_to_fit(); }
        Colors        .shrink_to_fit();
        BoneIndices   .shrink_to_fit();
        BoneWeights   .shrink_to_fit();
        VertexIndices .shrink_to_fit();

        MaterialId = 0;
    }
};

///////////////////////////////////////////////////////////////////////////////
// ResLodRange structure
///////////////////////////////////////////////////////////////////////////////
struct ResLodRange
{
    uint32_t    Offset;     //!< メッシュオフセット番号.
    uint32_t    Count;      //!< メッシュ数.
};

///////////////////////////////////////////////////////////////////////////////
// ResModel structure
///////////////////////////////////////////////////////////////////////////////
struct ResModel
{
    std::vector<ResMesh>        Meshes;     //!< メッシュです.
    std::vector<ResLodRange>    LodRanges;  //!< LOD範囲です.
    std::vector<std::string>    Materials;  //!< マテリアル名です.

    //-------------------------------------------------------------------------
    //! @brief      解放処理を行います.
    //-------------------------------------------------------------------------
    void Dispose()
    {
        for(size_t i=0; i<Meshes.size(); ++i)
        { Meshes[i].Dispose(); }

        for(size_t i=0; i<Materials.size(); ++i)
        {
            Materials[i].clear();
            Materials[i].shrink_to_fit();
        }

        Meshes   .clear();
        LodRanges.clear();
        Materials.clear();

        Meshes   .shrink_to_fit();
        LodRanges.shrink_to_fit();
        Materials.shrink_to_fit();
    }

    //-------------------------------------------------------------------------
    //! @brief      ファイルからモデルリソースを生成します
    //!             読み込み可能なファイルは OBJ です.
    //! 
    //! @param[in]      filename        ファイル名です.
    //! @retval true    リソース生成に成功.
    //! @retval false   リソース生成に失敗.
    //-------------------------------------------------------------------------
    bool LoadFromFileA(const char* filename);

    //-------------------------------------------------------------------------
    //! @brief      ファイルからモデルリソースを生成します.
    //!             読み込み可能なファイルは OBJ です.
    //! 
    //! @param[in]      filename        ファイル名です.
    //! @retval true    リソース生成に成功.
    //! @retval false   リソース生成に失敗.
    //-------------------------------------------------------------------------
    bool LoadFromFileW(const wchar_t* filename);
};

} // namespace asdx

