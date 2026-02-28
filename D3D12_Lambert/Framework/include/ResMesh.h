//-----------------------------------------------------------------------------
// File : ResMesh.h
// Desc : Mesh Resource Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>


///////////////////////////////////////////////////////////////////////////////
// ResMaterial structure
///////////////////////////////////////////////////////////////////////////////
struct ResMaterial
{
    DirectX::XMFLOAT3   Diffuse;        //!< 拡散反射成分です.
    DirectX::XMFLOAT3   Specular;       //!< 鏡面反射成分です.
    float               Alpha;          //!< 透過成分です.
    float               Shininess;      //!< 鏡面反射強度です.
    std::wstring        DiffuseMap;     //!< ディフューズマップファイルパスです.
    std::wstring        SpecularMap;    //!< スペキュラーマップファイルパスです.
    std::wstring        ShininessMap;   //!< シャイネスマップファイルパスです.
    std::wstring        NormalMap;      //!< 法線マップファイルパスです.
};

///////////////////////////////////////////////////////////////////////////////
// MeshVertex structure
///////////////////////////////////////////////////////////////////////////////
class MeshVertex
{
public:
    DirectX::XMFLOAT3   Position;
    DirectX::XMFLOAT3   Normal;
    DirectX::XMFLOAT2   TexCoord;
    DirectX::XMFLOAT3   Tangent;

    MeshVertex() = default;

    MeshVertex(
        DirectX::XMFLOAT3 const& position,
        DirectX::XMFLOAT3 const& normal,
        DirectX::XMFLOAT2 const& texcoord,
        DirectX::XMFLOAT3 const& tangent)
    : Position  (position)
    , Normal    (normal)
    , TexCoord  (texcoord)
    , Tangent   (tangent)
    { /* DO_NOTHING */ }

    static const D3D12_INPUT_LAYOUT_DESC InputLayout;

private:
    static const int InputElementCount = 4;
    static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

///////////////////////////////////////////////////////////////////////////////
// ResMesh structure
///////////////////////////////////////////////////////////////////////////////
struct ResMesh
{
    std::vector<MeshVertex>     Vertices;     //!< 頂点データです.
    std::vector<uint32_t>       Indices;      //!< 頂点インデックスです.
    uint32_t                    MaterialId;   //!< マテリアル番号です.
};

//-----------------------------------------------------------------------------
//! @brief      メッシュをロードします.
//!
//! @param[in]      filename        ファイルパス.
//! @param[out]     meshes          メッシュの格納先です.
//! @param[out]     materials       マテリアルの格納先です.
//! @retval true    ロードに成功.
//! @retval false   ロードに失敗.
//-----------------------------------------------------------------------------
bool LoadMesh(
    const wchar_t*             filename,
    std::vector<ResMesh>&      meshes,
    std::vector<ResMaterial>&  materials);
