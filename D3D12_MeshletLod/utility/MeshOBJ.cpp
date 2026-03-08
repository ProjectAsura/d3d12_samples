//------------------------------------------------------------------------------------------
// File : MeshOBJ.cpp
// Desc : Wavefront OBJ File Loader Module.
// Copyright(c) Project Asura. All right reserved.
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <tuple>
#include "MeshOBJ.h"
#include <fnd/asdxMisc.h>
#include <fnd/asdxLogger.h>
#include <meshoptimizer.h>


namespace {

//-----------------------------------------------------------------------------------------
// Constant Values
//-----------------------------------------------------------------------------------------
const static unsigned int OBJ_BUFFER_LENGTH = 1024;
const static unsigned int OBJ_NAME_LENGTH   = 256;


//-----------------------------------------------------------------------------
//      マテリアルを初期化します.
//-----------------------------------------------------------------------------
void InitMaterial(MeshOBJ::Material& mat)
{
    mat.Name.clear();
    mat.Ambient   = asdx::Vector3(0.2f, 0.2f, 0.2f);
    mat.Diffuse   = asdx::Vector3(0.8f, 0.8f, 0.8f);
    mat.Specular  = asdx::Vector3(1.0f, 1.0f, 1.0f);
    mat.Shininess = 0.0f;
    mat.Alpha     = 1.0f;

    mat.MapKa  .clear();
    mat.MapKd  .clear();
    mat.MapKs  .clear();
    mat.MapBump.clear();
}

//-----------------------------------------------------------------------------
//      ベクターコンテナをクリアします.
//-----------------------------------------------------------------------------
template<typename T>
void Clear(std::vector<T>& val)
{
    val.clear();
    val.shrink_to_fit();
}

} // namespace


///////////////////////////////////////////////////////////////////////////////
// MeshOBJ class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      メモリを解放します.
//-----------------------------------------------------------------------------
void MeshOBJ::Reset()
{
    Clear(m_Positions);
    Clear(m_Normals);
    Clear(m_TexCoords);
    Clear(m_Indices);
    Clear(m_Subsets);
    Clear(m_Materials);
    m_Directory.clear();
}

//-----------------------------------------------------------------------------
//      ファイルをロードします.
//-----------------------------------------------------------------------------
bool MeshOBJ::Load(const char* path)
{
    //　OBJ, MTLファイルを読み込み
    if (!LoadOBJFile(path))
    {
        ELOGA("Error : MeshOBJ::LoadOBJFile() Failed. path = %s", path);
        return false;
    }

    //　正常終了
    return true;
}

//-----------------------------------------------------------------------------
//      指定されたマテリアル名を検索します.
//-----------------------------------------------------------------------------
bool MeshOBJ::FindMaterial(const char* name, uint32_t& result) const
{
    auto itr = std::find_if(
        m_Materials.begin(),
        m_Materials.end(),
        [name](const Material& val)
        { return strcmp(name, val.Name.c_str()) == 0; }
    );

    if (itr != m_Materials.end())
    {
        result = uint32_t(std::distance(m_Materials.begin(), itr));
        return true;
    }

    result = UINT32_MAX;
    return false;
}

//-----------------------------------------------------------------------------
//      OBJファイルをロードします.
//-----------------------------------------------------------------------------
bool MeshOBJ::LoadOBJFile(const char* path)
{
    std::ifstream file;

    char buf[OBJ_BUFFER_LENGTH] = {0};

    uint32_t faceCount   = 0;   // こちらはサブセットごとにリセットされる.
    uint32_t faceIndex   = 0;   // こちらはずっとカウントアップ.

    // 一時保存用.
    std::vector<asdx::Vector3> positions;
    std::vector<asdx::Vector3> normals;
    std::vector<asdx::Vector2> texcoords;

    // ロードディレクトリを取得.
    m_Directory = asdx::GetDirectoryPathA(path);

    //　ファイルを開く
    file.open(path, std::ios::in);

    //　チェック
    if ( !file.is_open() )
    {
        ELOGA("Error : File Open Failed. path = %s", path);
        return false;
    }

    //　ループ
    for( ;; )
    {
        file >> buf;
        if ( !file )
            break;

        //　コメント
        if ( 0 == strcmp( buf, "#" ) )
        {
            // DO_NOTHING.
        }
        //　頂点座標
        else if ( 0 == strcmp( buf, "v" ) )
        {
            float x, y, z;
            file >> x >> y >> z;
            positions.emplace_back(x, y, z);
        }
        //　テクスチャ座標
        else if ( 0 == strcmp( buf, "vt" ) )
        {
            float u, v;
            file >> u >> v;
            texcoords.emplace_back(u, v);
        }
        //　法線ベクトル
        else if ( 0 == strcmp( buf, "vn" ) )
        {
            float x, y, z;
            file >> x >> y >> z;
            normals.emplace_back(x, y, z);
        }
        // グループ.
        else if ( 0 == strcmp( buf, "g") )
        {
        }
        // 
        else if ( 0 == strcmp( buf, "s") )
        {
        }
        //　面
        else if ( 0 == strcmp( buf, "f" ) )
        {
            uint32_t indexP[4] = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };
            uint32_t indexU[4] = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };
            uint32_t indexN[4] = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };

            // カウントアップ.
            faceCount++;
            faceIndex++;

            uint32_t count = 0;

            //　三角形・四角形のみ対応
            for (auto face = 0u; face < 4; face++)
            {
                count++;    //　頂点数を数える
                uint32_t index = 0;

                file >> index;
                index--; // 0始まりに補正.
                indexP[face] = index;
                if (face < 3)
                {
                    m_Positions.emplace_back(positions[index]);
                }

                if ( '/' == file.peek() )
                {
                    file.ignore();

                    //　テクスチャ座標インデックス
                    if ( '/' != file.peek() )
                    {
                        file >> index;
                        index--;    // 0始まりに補正.
                        indexU[face] = index;

                        if (face < 3)
                        {
                            m_TexCoords.emplace_back(texcoords[index]);
                        }
                    }

                    //　法線ベクトルインデックス
                    if ( '/' == file.peek() )
                    {
                        file.ignore();

                        file >> index;
                        index--;    // 0始まりに補正.
                        indexN[face] = index;

                        if (face < 3)
                        {
                            m_Normals.emplace_back(normals[index]);
                        }
                    }
                }

                //　カウントが3未満
                if (face < 3)
                {
                    auto idx = uint32_t(m_Positions.size() - 1);
                    m_Indices.emplace_back(idx);
                }

                //　次が改行だったら終了
                if ( ( '\n' == file.peek() )
                  || ( '\r' == file.peek() ) )
                {
                    break;
                }

            }

            // 四角形ポリゴンの場合，三角形を追加する
            if (count > 3)
            {
                // カウントアップ.
                faceCount++;
                faceIndex++;

                // 頂点とインデックスを追加
                for (auto face = 1u; face < 4; face++)
                {
                    auto next = (face + 1) % 4;

                    if ( indexP[next] != UINT32_MAX )
                    { m_Positions.emplace_back(positions[indexP[next]]); }
                    if ( indexU[next] != UINT32_MAX )
                    { m_TexCoords.emplace_back(texcoords[indexU[next]]); }
                    if ( indexN[next] != UINT32_MAX )
                    { m_Normals.emplace_back(normals[indexN[next]]); }

                    auto index = uint32_t(m_Positions.size() - 1);
                    m_Indices.emplace_back(index);
                }
            }
        }

        //　マテリアルファイル
        else if ( 0 == strcmp( buf, "mtllib" ) )
        {
            char filePath[OBJ_NAME_LENGTH] = {};
            file >> filePath;
            //　マテリアルファイルの読み込み
            if ( filePath[0] )
            {
                auto mtlPath = m_Directory + "/" + filePath;
                if ( !LoadMTLFile(mtlPath.c_str()) )
                {
                    ELOGA("Error : LoadMTLFile() Failed. path = %s", mtlPath.c_str());
                    return false;
                }
            }
        }

        //　マテリアル
        else if ( 0 == strcmp( buf, "usemtl" ) )
        {
            if (m_Subsets.size() > 0)
            {
                auto prevIndex = m_Subsets.size() - 1;
                m_Subsets[prevIndex].Count = faceCount * 3;
                faceCount = 0;
            }

            char name[OBJ_NAME_LENGTH] = {0};
            file >> name;

            uint32_t materialId = 0;
            FindMaterial(name, materialId);

            Subset subset = {};
            subset.MaterialId = materialId;
            subset.Count      = 0;
            subset.Offset     = faceIndex * 3;
            m_Subsets.emplace_back(subset);
        }

        file.ignore( OBJ_BUFFER_LENGTH, '\n' );
    }

    //　サブセット最後のカウント数を設定.
    if (m_Subsets.size() > 0 )
    {
        auto prevIndex = m_Subsets.size() - 1;
        m_Subsets[prevIndex].Count = faceCount * 3;
    }
    else
    {
        Subset subset = {};
        subset.Offset     = 0;
        subset.MaterialId = 0;
        subset.Count      = uint32_t(m_Indices.size());
        m_Subsets.emplace_back(subset);
    }

    //　ファイルを閉じる
    file.close();

    // 反時計周りから時計回りにインデックスを並び替える(= DirectX12で表面になるようにする).
    for(size_t i=0; i<m_Indices.size(); i+=3)
    {
        //auto i0 = m_Indices[i + 0];
        auto i1 = m_Indices[i + 1];
        auto i2 = m_Indices[i + 2];

        //m_Indices[i + 0] = i0;
        m_Indices[i + 1] = i2;
        m_Indices[i + 2] = i1;
    }

    // 法線ベクトルが無ければ算出する.
    if (!HasNormals())
    {
        if (!ComputeNormal())
        { ELOG( "Error : ComputeNormal() Failed."); }
    }

    // サブセット最適化.
    {
        // マテリアルIDとOffsetでサブセットをソート.
        std::sort(m_Subsets.begin(), m_Subsets.end(),
        [](const Subset& lhs, const Subset& rhs)
        {
            return std::tie(lhs.MaterialId, lhs.Offset) < std::tie(rhs.MaterialId, rhs.Offset);
        });

        // データをサブセットで並び替える.
        {
            std::vector<Subset>     mergedSubset;
            std::vector<uint32_t>   newIndices;

            size_t dstOffset = 0;

            for(size_t i=0; i<m_Subsets.size(); )
            {
                auto materialId = m_Subsets[i].MaterialId;

                Subset merged = {};
                merged.MaterialId = materialId;
                merged.Offset     = uint32_t(dstOffset);
                merged.Count      = 0;

                while(i < m_Subsets.size() && m_Subsets[i].MaterialId == materialId)
                {
                    const auto& s = m_Subsets[i];
                    newIndices.insert(
                        newIndices.end(),
                        m_Indices.begin() + s.Offset,
                        m_Indices.begin() + s.Offset + s.Count);

                    merged.Count += s.Count;
                    dstOffset    += s.Count;
                    ++i;
                }

                mergedSubset.emplace_back(merged);
            }

            m_Subsets   = std::move(mergedSubset);
            m_Indices   = std::move(newIndices);
        }
    }

    meshopt_Stream streams[4];
    streams[0].data   = m_Positions.data();
    streams[0].size   = sizeof(asdx::Vector3);
    streams[0].stride = sizeof(asdx::Vector3);

    streams[1].data   = m_Normals.data();
    streams[1].size   = sizeof(asdx::Vector3);
    streams[1].stride = sizeof(asdx::Vector3);

    size_t streamCount = 2;
    if (HasTexCoords())
    {
        streams[2].data   = m_TexCoords.data();
        streams[2].size   = sizeof(asdx::Vector2);
        streams[2].stride = sizeof(asdx::Vector2);
        streamCount++;
    }

    std::vector<uint32_t> remap(m_Indices.size());
    auto vertexCount = meshopt_generateVertexRemapMulti(
        remap.data(),
        m_Indices.data(),
        m_Indices.size(),
        m_Positions.size(),
        streams,
        streamCount);

    {
        std::vector<uint32_t> indices(m_Indices.size());
        meshopt_remapIndexBuffer(indices.data(), m_Indices.data(), m_Indices.size(), remap.data());
        m_Indices = std::move(indices);
    }

    {
        std::vector<asdx::Vector3> positions(vertexCount);
        meshopt_remapVertexBuffer(positions.data(), m_Positions.data(), m_Positions.size(), sizeof(asdx::Vector3), remap.data());
        m_Positions = std::move(positions);
    }

    {
        std::vector<asdx::Vector3> normals(vertexCount);
        meshopt_remapVertexBuffer(normals.data(), m_Normals.data(), m_Normals.size(), sizeof(asdx::Vector3), remap.data());
        m_Normals = std::move(normals);
    }

    if (HasTexCoords())
    {
        std::vector<asdx::Vector2> texcoords(vertexCount);
        meshopt_remapVertexBuffer(texcoords.data(), m_TexCoords.data(), m_TexCoords.size(), sizeof(asdx::Vector2), remap.data());
        m_TexCoords = std::move(texcoords);
    }

    // 接線ベクトルを算出する.
    if (HasTexCoords())
    {
        if (!ComputeTangent())
        { ELOG("Error : ComputeTangent() Failed."); }
    }


    //　正常終了
    return true;
}

//-----------------------------------------------------------------------------
//      MTLファイルをロードします.
//-----------------------------------------------------------------------------
bool MeshOBJ::LoadMTLFile(const char* path)
{
    std::ifstream file;

    char buf[OBJ_BUFFER_LENGTH] = {0};

    int curIndex = -1;

    //　ファイルを開く
    file.open(path, std::ios::in );

    //　チェック
    if ( !file.is_open() )
    {
        ELOGA("Error : File Open Failed. path = %s", path);
        return false;
    }

    //　ループ
    for( ;; )
    {
        file >> buf;
        if ( !file || file.eof() )
            break;

        // New Material
        if ( 0 == strcmp( buf, "newmtl" ) )
        {
            curIndex++;
            Material mat;
            InitMaterial(mat);
            m_Materials.emplace_back(mat);

            char name[OBJ_NAME_LENGTH] = {0};
            file >> name;
            m_Materials[curIndex].Name = name;
        }
        // Ambient Color
        else if ( 0 == strcmp( buf, "Ka" ) )
        {
            auto& m = m_Materials[curIndex];
            file >> m.Ambient.x >> m.Ambient.y >> m.Ambient.z;
        }
        // Diffuse Color
        else if ( 0 == strcmp( buf, "Kd" ) )
        {
            auto& m = m_Materials[curIndex];
            file >> m.Diffuse.x >> m.Diffuse.y >> m.Diffuse.z;
        }
        // Specular Color
        else if ( 0 == strcmp( buf, "Ks" ) )
        {
            auto& m = m_Materials[curIndex];
            file >> m.Specular.x >> m.Specular.y >> m.Specular.z;
        }
        else if ( 0 == strcmp( buf, "Ke" ) )
        {
            auto& m = m_Materials[curIndex];
            file >> m.Emissive.x >> m.Emissive.y >> m.Emissive.z;
        }
        // Alpha
        else if ( 0 == strcmp( buf, "d" ) ||
                  0 == strcmp( buf, "Tr" ) )
        {
            auto& m = m_Materials[curIndex];
            file >> m.Alpha;
        }
        // Shininess
        else if ( 0 == strcmp( buf, "Ns" ) )
        {
            auto& m = m_Materials[curIndex];
            file >> m.Shininess;
        }
        // Ambient Map
        else if ( 0 == strcmp( buf, "map_Ka" ) )
        {
            char mapKaName[OBJ_NAME_LENGTH];
            file >> mapKaName;

            auto& m = m_Materials[curIndex];
            m.MapKa = mapKaName;
        }
        // Diffuse Map
        else if ( 0 == strcmp( buf, "map_Kd" ) )
        {
            char mapKdName[OBJ_NAME_LENGTH];
            file >> mapKdName;

            auto& m = m_Materials[curIndex];
            m.MapKd = mapKdName;
        }
        // Specular Map
        else if ( 0 == strcmp( buf, "map_Ks" ) )
        {
            char mapKsName[OBJ_NAME_LENGTH];
            file >> mapKsName;

            auto& m = m_Materials[curIndex];
            m.MapKs = mapKsName;
        }
        // Bump Map
        else if ( ( 0 == strcmp( buf, "map_Bump" ) )
               || ( 0 == strcmp( buf, "map_bump" ) ) 
               || ( 0 == strcmp( buf, "bump"     ) ) )
        {
            char mapBumpName[OBJ_NAME_LENGTH];
            file >> mapBumpName;

            auto& m = m_Materials[curIndex];
            m.MapBump = mapBumpName;
        }
        //else if ( 0 == strcmp( buf, "disp" ) )
        //{
        //    char displacementMapName[OBJ_NAME_LENGTH];
        //    file >> displacementMapName;

        //    auto& m = m_Materials[curIndex];
        //    m.MapDisplacement = displacementMapName;
        //}

        file.ignore( OBJ_BUFFER_LENGTH, '\n' );
    }

    //　ファイルを閉じる
    file.close();

    // メモリ最適化
    m_Materials.shrink_to_fit();

    //　正常終了
    return true;
}

//-----------------------------------------------------------------------------
//      法線ベクトルを計算します.
//-----------------------------------------------------------------------------
bool MeshOBJ::ComputeNormal()
{
    if (!HasPositions())
    { return false; }

    if (!HasIndices())
    { return false; }

    if (HasNormals())
    { return true; }

    m_Normals.resize(m_Positions.size());
    if (!HasNormals())
    {
        ELOG("Error : Out of Memory.");
        return false;
    }

    auto vertexCount = m_Positions.size();
    auto indexCount  = m_Indices.size();

    // 法線データを初期化.
    for(size_t i=0; i<vertexCount; ++i)
    {
        m_Normals[i].x = 0.0f;
        m_Normals[i].y = 0.0f;
        m_Normals[i].z = 0.0f;
    }

    for(size_t i=0; i<indexCount; i+=3)
    {
        const auto& i0 = m_Indices[i + 0];
        const auto& i1 = m_Indices[i + 1];
        const auto& i2 = m_Indices[i + 2];

        const auto& p0 = m_Positions[i0];
        const auto& p1 = m_Positions[i1];
        const auto& p2 = m_Positions[i2];

        // 面法線を算出.
        auto n = asdx::Vector3::ComputeNormal(p0, p1, p2);

        // 面法線を加算.
        m_Normals[i0] += n;
        m_Normals[i1] += n;
        m_Normals[i2] += n;
    }

    // 加算した法線を正規化し，頂点法線を求める.
    for(size_t i=0; i<vertexCount; ++i)
    {
        m_Normals[i] = asdx::Vector3::SafeNormalize(m_Normals[i], m_Normals[i]);
    }

    const float SMOOTHING_ANGLE = 59.7f;
    float cosSmooth = cosf(asdx::ToRadian(SMOOTHING_ANGLE));

    // スムージング処理
    for(size_t i=0; i<indexCount; i+=3 )
    {
        const auto& i0 = m_Indices[i + 0];
        const auto& i1 = m_Indices[i + 1];
        const auto& i2 = m_Indices[i + 2];

        const auto& p0 = m_Positions[i0];
        const auto& p1 = m_Positions[i1];
        const auto& p2 = m_Positions[i2];

        auto fn = asdx::Vector3::ComputeNormal(p0, p1, p2);

        // 頂点法線と面法線のなす角度を算出.
        auto cosAngle0 = asdx::Vector3::Dot(m_Normals[i0], fn);
        auto cosAngle1 = asdx::Vector3::Dot(m_Normals[i1], fn);
        auto cosAngle2 = asdx::Vector3::Dot(m_Normals[i2], fn);

        m_Normals[i0] = (cosAngle0 >= cosSmooth) ? m_Normals[i0] : fn;
        m_Normals[i1] = (cosAngle1 >= cosSmooth) ? m_Normals[i1] : fn;
        m_Normals[i2] = (cosAngle2 >= cosSmooth) ? m_Normals[i2] : fn;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      接線ベクトルを計算します.
//-----------------------------------------------------------------------------
bool MeshOBJ::ComputeTangent()
{
    if (!HasPositions())
    { return false; }

    if (!HasIndices())
    { return false; }

    if (!HasNormals())
    { return false; }

    if (!HasTexCoords())
    { return false; }

    m_Tangents.resize(m_Positions.size());

    // 接線ベクトルを初期化.
    for(size_t i=0; i<m_Tangents.size(); ++i )
    {
        m_Tangents[i] = asdx::Vector3(0.0f, 0.0f, 0.0f);
    }

    for(size_t i=0; i<m_Indices.size(); i+=3 )
    {
        const auto& i0 = m_Indices[ i + 0 ];
        const auto& i1 = m_Indices[ i + 1 ];
        const auto& i2 = m_Indices[ i + 2 ];

        const auto& p0 = m_Positions[i0];
        const auto& p1 = m_Positions[i1];
        const auto& p2 = m_Positions[i2];

        const auto& t0 = m_TexCoords[i0];
        const auto& t1 = m_TexCoords[i1];
        const auto& t2 = m_TexCoords[i2];

        asdx::Vector3 e0;
        asdx::Vector3 e1;

        e0.x = p1.x - p0.x;
        e0.y = t1.x - t0.x;
        e0.z = t1.y - t0.y;

        e1.x = p2.x - p0.x;
        e1.y = t2.x - t0.x;
        e1.z = t2.y - t0.y;

        asdx::Vector3 crs;
        crs = asdx::Vector3::Cross(e0, e1);
        crs = asdx::Vector3::SafeNormalize(crs, asdx::Vector3(1.0f, 0.0f, 0.0f));

        asdx::Vector3 tan0;
        asdx::Vector3 tan1;
        asdx::Vector3 tan2;
        float tanX = -crs.y / crs.x;

        //asdx::Vector3 bin0;
        //asdx::Vector3 bin1;
        //asdx::Vector3 bin2;
        //float binX = -crs.z / crs.x;

        tan0.x = tanX;
        tan1.x = tanX;
        tan2.x = tanX;

        //bin0.x = binX;
        //bin1.x = binX;
        //bin2.x = binX;

        e0.x = p1.y - p0.y;
        e1.x = p2.y - p0.y;
        crs = asdx::Vector3::Cross(e0, e1);
        crs = asdx::Vector3::SafeNormalize(crs, asdx::Vector3(1.0f, 0.0f, 0.0f));

        float tanY = -crs.y / crs.x;
        tan0.y = tanY;
        tan1.y = tanY;
        tan2.y = tanY;

        //float binY = -crs.z / crs.x;
        //bin0.y = binY;
        //bin1.y = binY;
        //bin2.y = binY;

        e0.x = p1.z - p0.z;
        e1.x = p2.z - p0.z;
        crs = asdx::Vector3::Cross(e0, e1);
        crs = asdx::Vector3::SafeNormalize(crs, asdx::Vector3(1.0f, 0.0f, 0.0f));

        const auto& n0 = m_Normals[i0];
        const auto& n1 = m_Normals[i1];
        const auto& n2 = m_Normals[i2];

        {
            float tanZ = -crs.y / crs.x;

            tan0.z = tanZ;
            tan1.z = tanZ;
            tan2.z = tanZ;


            float dp0 = asdx::Vector3::Dot(tan0, n0);
            float dp1 = asdx::Vector3::Dot(tan1, n1);
            float dp2 = asdx::Vector3::Dot(tan2, n2);

            tan0 -= (n0 * dp0);
            tan1 -= (n1 * dp1);
            tan2 -= (n2 * dp2);

            tan0 = asdx::Vector3::SafeNormalize(tan0, asdx::Vector3(1.0f, 0.0f, 0.0f));
            tan1 = asdx::Vector3::SafeNormalize(tan1, asdx::Vector3(1.0f, 0.0f, 0.0f));
            tan2 = asdx::Vector3::SafeNormalize(tan2, asdx::Vector3(1.0f, 0.0f, 0.0f));

            m_Tangents[i0] = tan0;
            m_Tangents[i1] = tan1;
            m_Tangents[i2] = tan2;
        }

        {
            //float binZ = -crs.z / crs.x;
            //bin0.z = binZ;
            //bin1.z = binZ;
            //bin2.z = binZ;

            //dp0 = asdx::Vector3::Dot(bin0, n0);
            //dp1 = asdx::Vector3::Dot(bin1, n1);
            //dp2 = asdx::Vector3::Dot(bin2, n2);

            //bin0 -= (n0 * dp0);
            //bin1 -= (n1 * dp1);
            //bin2 -= (n2 * dp2);

            //bin0 = asdx::Vector3::SafeNormalize(bin0, asdx::Vector3(0.0f, 1.0f, 0.0f));
            //bin1 = asdx::Vector3::SafeNormalize(bin1, asdx::Vector3(0.0f, 1.0f, 0.0f));
            //bin2 = asdx::Vector3::SafeNormalize(bin2, asdx::Vector3(0.0f, 1.0f, 0.0f));

            //m_Binormals[i0] = bin0;
            //m_Binormals[i1] = bin1;
            //m_Binormals[i2] = bin2;
        }
    }

    return true;
}