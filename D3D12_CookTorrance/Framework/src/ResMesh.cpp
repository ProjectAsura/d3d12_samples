//-----------------------------------------------------------------------------
// File : ResMesh.cpp
// Desc : Resource Mesh Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "ResMesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <codecvt>
#include <cassert>


namespace {

//-----------------------------------------------------------------------------
//      UTF-8文字列に変換します.
//-----------------------------------------------------------------------------
std::string ToUTF8(const std::wstring& value)
{
    auto length = WideCharToMultiByte(
        CP_UTF8, 0U, value.data(), -1, nullptr, 0, nullptr, nullptr);
    auto buffer = new char [length];

    WideCharToMultiByte(
        CP_UTF8, 0U, value.data(), -1, buffer, length, nullptr, nullptr);

    std::string result(buffer);
    delete[] buffer;
    buffer = nullptr;

    return result;
}

//-----------------------------------------------------------------------------
//      std::wstring型に変換します.
//-----------------------------------------------------------------------------
std::wstring Convert(const aiString& path)
{
    wchar_t temp[256] = {};
    size_t  size;
    mbstowcs_s(&size, temp, path.C_Str(), 256);
    return std::wstring(temp);
}

///////////////////////////////////////////////////////////////////////////////
// MeshLoader class
///////////////////////////////////////////////////////////////////////////////
class MeshLoader
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
    MeshLoader();
    ~MeshLoader();

    bool Load(
        const wchar_t*             filename,
        std::vector<ResMesh>&      meshes,
        std::vector<ResMaterial>&  materials);

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    const aiScene*      m_pScene = nullptr;   // シーンデータ.

    //=========================================================================
    // private methods.
    //=========================================================================
    void ParseMesh(ResMesh& dstMesh, const aiMesh* pSrcMesh);
    void ParseMaterial(ResMaterial& dstMaterial, const aiMaterial* pSrcMaterial);
};

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
MeshLoader::MeshLoader()
: m_pScene(nullptr)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
MeshLoader::~MeshLoader()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      メッシュをロードします.
//-----------------------------------------------------------------------------
bool MeshLoader::Load
(
    const wchar_t*             filename,
    std::vector<ResMesh>&      meshes,
    std::vector<ResMaterial>&  materials
)
{
    if (filename == nullptr)
    { return false; }

    // wchar_t から char型(UTF-8)に変換します.
	auto path = ToUTF8(filename);

    Assimp::Importer importer;
    unsigned int flag = 0;
    flag |= aiProcess_Triangulate;
    flag |= aiProcess_PreTransformVertices;
    flag |= aiProcess_CalcTangentSpace;
    flag |= aiProcess_GenSmoothNormals;
    flag |= aiProcess_GenUVCoords;
    flag |= aiProcess_RemoveRedundantMaterials;
    flag |= aiProcess_OptimizeMeshes;

    // ファイルを読み込み.
    m_pScene = importer.ReadFile(path, flag);

    // チェック.
    if (m_pScene == nullptr)
    { return false; }

    // メッシュのメモリを確保.
    meshes.clear();
    meshes.resize(m_pScene->mNumMeshes);

    // メッシュデータを変換.
    for(size_t i=0; i<meshes.size(); ++i)
    {
        const auto pMesh = m_pScene->mMeshes[i];
        ParseMesh(meshes[i], pMesh);
    }

    // マテリアルのメモリを確保.
    materials.clear();
    materials.resize(m_pScene->mNumMaterials);

    // マテリアルデータを変換.
    for(size_t i=0; i<materials.size(); ++i)
    {
        const auto pMaterial = m_pScene->mMaterials[i];
        ParseMaterial(materials[i], pMaterial);
    }

    // 不要になったのでクリア.
    importer.FreeScene();
    m_pScene = nullptr;

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      メッシュデータを解析します.
//-----------------------------------------------------------------------------
void MeshLoader::ParseMesh(ResMesh& dstMesh, const aiMesh* pSrcMesh)
{
    // マテリアル番号を設定.
    dstMesh.MaterialId = pSrcMesh->mMaterialIndex;

    aiVector3D zero3D(0.0f, 0.0f, 0.0f);

    // 頂点データのメモリを確保.
    dstMesh.Vertices.resize(pSrcMesh->mNumVertices);

    for(auto i=0u; i<pSrcMesh->mNumVertices; ++i)
    {
        auto pPosition = &(pSrcMesh->mVertices[i]);
        auto pNormal   = &(pSrcMesh->mNormals[i]);
        auto pTexCoord = (pSrcMesh->HasTextureCoords(0)) ? &(pSrcMesh->mTextureCoords[0][i]) : &zero3D;
        auto pTangent  = (pSrcMesh->HasTangentsAndBitangents()) ? &(pSrcMesh->mTangents[i]) : &zero3D;

        dstMesh.Vertices[i] = MeshVertex(
            DirectX::XMFLOAT3(pPosition->x, pPosition->y, pPosition->z),
            DirectX::XMFLOAT3(pNormal  ->x, pNormal  ->y, pNormal  ->z),
            DirectX::XMFLOAT2(pTexCoord->x, pTexCoord->y),
            DirectX::XMFLOAT3(pTangent ->x, pTangent ->y, pTangent ->z)
        );
    }

    // 頂点インデックスのメモリを確保.
    dstMesh.Indices.resize(pSrcMesh->mNumFaces * 3);

    for(auto i=0u; i<pSrcMesh->mNumFaces; ++i)
    {
        const auto& face = pSrcMesh->mFaces[i];
        assert(face.mNumIndices == 3);  // 三角形化しているので必ず3になっている.

        dstMesh.Indices[i * 3 + 0] = face.mIndices[0];
        dstMesh.Indices[i * 3 + 1] = face.mIndices[1];
        dstMesh.Indices[i * 3 + 2] = face.mIndices[2];
    }
}

//-----------------------------------------------------------------------------
//      マテリアルデータを解析します.
//-----------------------------------------------------------------------------
void MeshLoader::ParseMaterial(ResMaterial& dstMaterial, const aiMaterial* pSrcMaterial)
{
    // 拡散反射成分.
    {
        aiColor3D color(0.0f, 0.0f, 0.0f);

        if (pSrcMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
        {
            dstMaterial.Diffuse.x = color.r;
            dstMaterial.Diffuse.y = color.g;
            dstMaterial.Diffuse.z = color.b;
        }
        else
        {
            dstMaterial.Diffuse.x = 0.5f;
            dstMaterial.Diffuse.y = 0.5f;
            dstMaterial.Diffuse.z = 0.5f;
        }
    }

    // 鏡面反射成分.
    {
        aiColor3D color(0.0f, 0.0f, 0.0f);

        if (pSrcMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
        {
            dstMaterial.Specular.x = color.r;
            dstMaterial.Specular.y = color.g;
            dstMaterial.Specular.z = color.b;
        }
        else
        {
            dstMaterial.Specular.x = 0.0f;
            dstMaterial.Specular.y = 0.0f;
            dstMaterial.Specular.z = 0.0f;
        }
    }

    // 鏡面反射強度.
    {
        auto shininess = 0.0f;
        if (pSrcMaterial->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
        { dstMaterial.Shininess = shininess; }
        else
        { dstMaterial.Shininess = 0.0f; }
    }

    // ディフューズマップ.
    {
        aiString path;
        if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) == AI_SUCCESS)
        { dstMaterial.DiffuseMap = Convert(path); }
        else
        { dstMaterial.DiffuseMap.clear(); }
    }

    // スペキュラーマップ.
    {
        aiString path;
        if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_SPECULAR(0), path) == AI_SUCCESS)
        { dstMaterial.SpecularMap = Convert(path); }
        else
        { dstMaterial.SpecularMap.clear(); }
    }

    // シャイネスマップ.
    {
        aiString path;
        if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_SHININESS(0), path) == AI_SUCCESS)
        { dstMaterial.ShininessMap = Convert(path); }
        else
        { dstMaterial.ShininessMap.clear(); }
    }

    // 法線マップ
    {
        aiString path;
        if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_NORMALS(0), path) == AI_SUCCESS)
        { dstMaterial.NormalMap = Convert(path); }
        else
        {
            if (pSrcMaterial->Get(AI_MATKEY_TEXTURE_HEIGHT(0), path) == AI_SUCCESS)
            { dstMaterial.NormalMap = Convert(path); }
            else
            { dstMaterial.NormalMap.clear(); }
        }
    }
}

} // namespace

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
const D3D12_INPUT_ELEMENT_DESC MeshVertex::InputElements[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};
const D3D12_INPUT_LAYOUT_DESC MeshVertex::InputLayout = { MeshVertex::InputElements, MeshVertex::InputElementCount };
static_assert(sizeof(MeshVertex) == 44, "Vertex struct/layout mismatch");


//-----------------------------------------------------------------------------
//      メッシュをロードします.
//-----------------------------------------------------------------------------
bool LoadMesh
(
    const wchar_t*            filename,
    std::vector<ResMesh>&      meshes,
    std::vector<ResMaterial>&  materials
)
{
    MeshLoader loader;
    return loader.Load(filename, meshes, materials);
}
