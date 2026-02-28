//-----------------------------------------------------------------------------
// File : asdxResModel.cpp
// Desc : Model Resource.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <res/asdxResModel.h>
#include <fnd/asdxLogger.h>
#include <fnd/asdxMisc.h>
#include <fstream>
#include <algorithm>
#include <tuple>
#include <map>


namespace {

///////////////////////////////////////////////////////////////////////////////
// SubsetOBJ structure
///////////////////////////////////////////////////////////////////////////////
struct SubsetOBJ 
{
    std::string     MeshName;                   //!< メッシュ名.
    uint32_t        MaterialId  = UINT32_MAX;   //!< マテリアルID.
    uint32_t        IndexStart  = 0;            //!< 開始頂点インデックス.
    uint32_t        IndexCount  = 0;            //!< 頂点インデックス数.
};

///////////////////////////////////////////////////////////////////////////////
// IndexOBJ structure
///////////////////////////////////////////////////////////////////////////////
struct IndexOBJ
{
    uint32_t    P;      //!< 位置.
    uint32_t    N;      //!< 法線.
    uint32_t    U;      //!< テクスチャ座標.
};


//-----------------------------------------------------------------------------
//      法線ベクトルを計算します.
//-----------------------------------------------------------------------------
void CalcNormals(asdx::ResMesh& mesh)
{
    auto vertexCount = mesh.Positions.size();
    std::vector<asdx::Vector3> normals;
    normals.resize(vertexCount);

    // 法線データ初期化.
    for(size_t i=0; i<vertexCount; ++i)
    {
        normals[i] = asdx::Vector3(0.0f, 0.0f, 0.0f);
    }

    auto indexCount = mesh.VertexIndices.size();
    for(size_t i=0; i<indexCount; i+=3)
    {
        auto i0 = mesh.VertexIndices[i + 0];
        auto i1 = mesh.VertexIndices[i + 1];
        auto i2 = mesh.VertexIndices[i + 2];

        const auto& p0 = mesh.Positions[i0];
        const auto& p1 = mesh.Positions[i1];
        const auto& p2 = mesh.Positions[i2];

        // エッジ.
        auto e0 = p1 - p0;
        auto e1 = p2 - p0;

        // 面法線を算出.
        auto fn = asdx::Vector3::Cross(e0, e1);
        fn = asdx::Vector3::SafeNormalize(fn, fn);

        // 面法線を加算.
        normals[i0] += fn;
        normals[i1] += fn;
        normals[i2] += fn;
    }

    // 加算した法線を正規化し，頂点法線を求める.
    for(size_t i=0; i<vertexCount; ++i)
    {
        normals[i] = asdx::Vector3::SafeNormalize(normals[i], normals[i]);
    }

    const auto SMOOTHING_ANGLE = 59.7f;
    auto cosSmooth = cosf(asdx::ToDegree(SMOOTHING_ANGLE));

    // スムージング処理.
    for(size_t i=0; i<indexCount; i+=3)
    {
        auto i0 = mesh.VertexIndices[i + 0];
        auto i1 = mesh.VertexIndices[i + 1];
        auto i2 = mesh.VertexIndices[i + 2];

        const auto& p0 = mesh.Positions[i0];
        const auto& p1 = mesh.Positions[i1];
        const auto& p2 = mesh.Positions[i2];

        // エッジ.
        auto e0 = p1 - p0;
        auto e1 = p2 - p0;

        // 面法線を算出.
        auto fn = asdx::Vector3::Cross(e0, e1);
        fn = asdx::Vector3::SafeNormalize(fn, fn);

        // 頂点法線と面法線のなす角度を算出.
        auto c0 = asdx::Vector3::Dot(normals[i0], fn);
        auto c1 = asdx::Vector3::Dot(normals[i1], fn);
        auto c2 = asdx::Vector3::Dot(normals[i2], fn);

        // スムージング処理.
        mesh.Normals[i0] = (c0 >= cosSmooth) ? normals[i0] : fn;
        mesh.Normals[i1] = (c1 >= cosSmooth) ? normals[i1] : fn;
        mesh.Normals[i2] = (c2 >= cosSmooth) ? normals[i2] : fn;
    }

    normals.clear();
}

//-----------------------------------------------------------------------------
//      接線ベクトルを計算します.
//-----------------------------------------------------------------------------
void CalcTangentsRoughly(asdx::ResMesh& mesh)
{
    auto vertexCount = mesh.Positions.size();
    mesh.Tangents.resize(vertexCount);
    for(size_t i=0; i<vertexCount; ++i)
    {
        asdx::Vector3 T, B;
        asdx::CalcONB(mesh.Normals[i], T, B);
        mesh.Tangents[i] = T;
    }
}

//-----------------------------------------------------------------------------
//      接線ベクトルを計算します.
//-----------------------------------------------------------------------------
void CalcTangents(asdx::ResMesh& resource)
{
    // テクスチャ座標が無い場合は接線ベクトルをきちんと計算できないので，
    // 雑に計算する.
    if (resource.TexCoords[0].empty())
    {
        CalcTangentsRoughly(resource);
        return;
    }

    auto vertexCount = resource.Positions.size();
    resource.Tangents.resize(vertexCount);

    // 接線ベクトルを初期化.
    for(size_t i=0; i<vertexCount; ++i)
    {
        resource.Tangents[i] = asdx::Vector3(0.0f, 0.0f, 0.0f);
    }

    auto indexCount = resource.VertexIndices.size();
    for(size_t i=0; i<indexCount - 3; i+=3)
    {
        auto i0 = resource.VertexIndices[i + 0];
        auto i1 = resource.VertexIndices[i + 1];
        auto i2 = resource.VertexIndices[i + 2];

        const auto& p0 = resource.Positions[i0];
        const auto& p1 = resource.Positions[i1];
        const auto& p2 = resource.Positions[i2];

        const auto& t0 = resource.TexCoords[0][i0];
        const auto& t1 = resource.TexCoords[0][i1];
        const auto& t2 = resource.TexCoords[0][i2];

        auto e1 = p1 - p0;
        auto e2 = p2 - p0;

        float x1 = t1.x - t0.x;
        float x2 = t2.x - t0.x;

        float y1 = t1.y - t0.y;
        float y2 = t2.y - t0.y;

        float r = 1.0f / (x1 * y2 - x2 * y1);

        asdx::Vector3 T = (e1 * y2 - e2 * y1) * r;

        resource.Tangents[i0] += T;
        resource.Tangents[i1] += T;
        resource.Tangents[i2] += T;
    }

    for(size_t i=0; i<vertexCount; ++i)
    {
        // Reject = a - b * Dot(a, b);
        const auto& a = resource.Tangents[i];
        const auto& b = resource.Normals[i];
        auto T = a - b * asdx::Vector3::Dot(a, b);
        resource.Tangents[i] = asdx::Vector3::SafeNormalize(T, T);
    }
}

} // namespace

namespace asdx {

//-----------------------------------------------------------------------------
//      MTLファイルからマテリアルをロードします.
//-----------------------------------------------------------------------------
bool LoadFromMTL(const char* path, ResModel& model)
{
    std::ifstream stream;

    stream.open(path, std::ios::in);

    if (!stream.is_open())
    {
        ELOGA("Error : File Open Failed. path = %s", path);
        return false;
    }

    const uint32_t BUFFER_LENGTH = 2048;
    char buf[BUFFER_LENGTH] = {};
    //int32_t index = -1;

    for(;;)
    {
        stream >> buf;

        if (!stream || stream.eof())
            break;

        if (0 == strcmp(buf, "newmtl"))
        {
            std::string name;
            stream >> name;
            model.Materials.push_back(name);
            //index++;
        }
        else if (0 == strcmp(buf, "Ka"))
        {
            //stream >> model.Materials[index].Ka.x 
            //       >> model.Materials[index].Ka.y
            //       >> model.Materials[index].Ka.z;
        }
        else if (0 == strcmp(buf, "Kd"))
        {
            //stream >> model.Materials[index].Kd.x
            //       >> model.Materials[index].Kd.y
            //       >> model.Materials[index].Kd.z;
        }
        else if (0 == strcmp(buf, "Ks"))
        {
            //stream >> model.Materials[index].Ks.x
            //       >> model.Materials[index].Ks.y
            //       >> model.Materials[index].Ks.z;
        }
        else if (0 == strcmp(buf, "Ke"))
        {
            //stream >> model.Materials[index].Ke.x
            //       >> model.Materials[index].Ke.y
            //       >> model.Materials[index].Ke.z;
        }
        else if (0 == strcmp(buf, "d") || 0 == strcmp(buf, "Tr"))
        {
            //stream >> model.Materials[index].Tr;
        }
        else if (0 == strcmp(buf, "Ns"))
        {
            //stream >> model.Materials[index].Ns;
        }
        else if (0 == strcmp(buf, "map_Ka"))
        {
            //stream >> model.Materials[index].map_Ka;
        }
        else if (0 == strcmp(buf, "map_Kd"))
        {
            //stream >> model.Materials[index].map_Kd;
        }
        else if (0 == strcmp(buf, "map_Ks"))
        {
            //stream >> model.Materials[index].map_Ks;
        }
        else if (0 == strcmp(buf, "map_Ke"))
        {
            //stream >> model.Materials[index].map_Ke;
        }
        else if (0 == _stricmp(buf, "map_bump") || 0 == strcmp(buf, "bump"))
        {
            //stream >> model.Materials[index].map_bump;
        }
        else if (0 == strcmp(buf, "disp"))
        {
            //stream >> model.Materials[index].disp;
        }
        else if (0 == strcmp(buf, "norm"))
        {
            //stream >> model.Materials[index].norm;
        }

        stream.ignore(BUFFER_LENGTH, '\n');
    }

    // ファイルを閉じる.
    stream.close();

    // メモリ最適化.
    model.Materials.shrink_to_fit();

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      OBJファイルからモデルをロードします.
//-----------------------------------------------------------------------------
bool LoadFromOBJ(std::ifstream& stream, const std::string& directory, ResModel& model)
{
    if (!stream.is_open())
    {
        ELOGA("Error : File Open Failed.");
        return false;
    }

    const uint32_t BUFFER_LENGTH = 2048;
    char buf[BUFFER_LENGTH] = {};
    std::string group;

    uint32_t faceIndex = 0;
    uint32_t faceCount = 0;

    std::vector<asdx::Vector3>      positions;
    std::vector<asdx::Vector3>      normals;
    std::vector<asdx::Vector2>      texcoords;
    std::vector<IndexOBJ>           indices;
    std::vector<SubsetOBJ>          subsets;
    std::map<std::string, uint32_t> materials;

    for(;;)
    {
        stream >> buf;
        if (!stream || stream.eof())
            break;

        if (0 == strcmp(buf, "#"))
        {
            /* DO_NOTHING */
        }
        else if (0 == strcmp(buf, "v"))
        {
            asdx::Vector3 v;
            stream >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (0 == strcmp(buf, "vt"))
        {
            asdx::Vector2 vt;
            stream >> vt.x >> vt.y;
            texcoords.push_back(vt);
        }
        else if (0 == strcmp(buf, "vn"))
        {
            asdx::Vector3 vn;
            stream >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (0 == strcmp(buf, "g"))
        {
            stream >> group;
        }
        else if (0 == strcmp(buf, "f"))
        {
            uint32_t ip, it, in;
            uint32_t p[4] = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };
            uint32_t t[4] = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };
            uint32_t n[4] = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };

            uint32_t count = 0;
            uint32_t index = 0;

            faceIndex++;
            faceCount++;

            for(auto i=0; i<4; ++i)
            {
                count++;

                // 位置座標インデックス.
                stream >> ip;
                p[i] = ip - 1;

                if ('/' == stream.peek())
                {
                    stream.ignore();

                    // テクスチャ座標インデックス.
                    if ('/' != stream.peek())
                    {
                        stream >> it;
                        t[i] = it - 1;
                    }

                    // 法線インデックス.
                    if ('/' == stream.peek())
                    {
                        stream.ignore();

                        stream >> in;
                        n[i] = in - 1;
                    }
                }

                if (count <= 3)
                {
                    IndexOBJ f0 = { p[i], t[i], n[i] };
                    indices.push_back(f0);
                }

                if ('\n' == stream.peek() || '\r' == stream.peek())
                    break;
            }

            // 四角形.
            if (count > 3 && p[3] != UINT32_MAX)
            {
                assert(count == 4);

                faceIndex++;
                faceCount++;

                IndexOBJ f0 = { p[2], t[2], n[2] };
                IndexOBJ f1 = { p[3], t[3], n[3] };
                IndexOBJ f2 = { p[0], t[0], n[0] };

                indices.push_back(f0);
                indices.push_back(f1);
                indices.push_back(f2);
            }
        }
        else if (0 == strcmp(buf, "mtllib"))
        {
            std::string path;
            stream >> path;
            if (!path.empty())
            {
                path = directory + "/" + path;

                if (!LoadFromMTL(path.c_str(), model))
                {
                    ELOGA("Error : Material Load Failed.");
                    return false;
                }

                // MaterialId検索マップ構築.
                for(size_t i=0; i<model.Materials.size(); ++i)
                { materials[model.Materials[i]] = uint32_t(i); }
            }
        }
        else if (0 == strcmp(buf, "usemtl"))
        {
            SubsetOBJ subset = {};

            std::string materialName;
            stream >> materialName;

            auto itr = materials.find(materialName);
            if (itr != materials.end())
            { subset.MaterialId = itr->second; }

            if (group.empty())
            { group = "group" + std::to_string(subsets.size()); }

            subset.MeshName   = group;
            subset.IndexStart = faceIndex * 3;

            auto index = subsets.size() - 1;
            subsets.push_back(subset);

            group.clear();

            if (subsets.size() > 1)
            {
                subsets[index].IndexCount = faceCount * 3;
                faceCount = 0;
            }
        }

        stream.ignore(BUFFER_LENGTH, '\n');
    }

    if (subsets.size() > 0)
    {
        auto index = subsets.size();
        subsets[index - 1].IndexCount = faceCount * 3;
    }

    stream.close();

    std::stable_sort(subsets.begin(), subsets.end(),
        [](const SubsetOBJ& lhs, const SubsetOBJ& rhs)
        {
            return std::tie(lhs.MaterialId, lhs.IndexStart)
                 < std::tie(rhs.MaterialId, rhs.IndexStart);
        });

    uint32_t matId  = UINT32_MAX;
    uint32_t vertId = 0;
    uint32_t meshId = 0;

    asdx::ResMesh dstMesh;

    for(size_t i=0; i<subsets.size(); ++i)
    {
        auto& subset = subsets[i];

        if (matId != subset.MaterialId)
        {
            if (matId != UINT32_MAX)
            {
                if (!normals.empty())
                { CalcNormals(dstMesh); }

                dstMesh.Positions    .shrink_to_fit();
                dstMesh.Normals      .shrink_to_fit();
                dstMesh.TexCoords[0] .shrink_to_fit();
                dstMesh.VertexIndices.shrink_to_fit();

                model.Meshes.emplace_back(dstMesh);

                dstMesh = asdx::ResMesh();
                vertId = 0;
            }

            dstMesh.Name       = std::string("mesh") + std::to_string(meshId);
            dstMesh.MaterialId = subset.MaterialId;

            meshId++;
            matId = subset.MaterialId;
        }

        for(size_t j=0; j<subset.IndexCount; ++j)
        {
            auto id = subset.IndexStart + j;
            auto& index = indices[id];

            dstMesh.Positions[vertId] = positions[index.P];

            if (!normals.empty())
            { dstMesh.Normals[vertId] = normals[index.N]; }

            if (!texcoords.empty())
            { dstMesh.TexCoords[0][vertId] = texcoords[index.U]; }

            dstMesh.VertexIndices.push_back(vertId);

            vertId++;
        }
    }

    if (matId != UINT32_MAX)
    {
        if (normals.empty())
        { CalcNormals(dstMesh); }

        dstMesh.Positions    .shrink_to_fit();
        dstMesh.Normals      .shrink_to_fit();
        dstMesh.TexCoords[0] .shrink_to_fit();
        dstMesh.VertexIndices.shrink_to_fit();

        model.Meshes.emplace_back(dstMesh);
    }

    model.Meshes.shrink_to_fit();

    positions.clear();
    normals  .clear();
    texcoords.clear();
    indices  .clear();
    subsets  .clear();

    return true;
}

//-----------------------------------------------------------------------------
//      ファイルからリソースモデルを生成します.
//-----------------------------------------------------------------------------
bool ResModel::LoadFromFileA(const char* filename)
{
    std::ifstream stream;
    stream.open(filename, std::ios::in);

    auto directory = asdx::GetDirectoryPathA(filename);
    auto ext = asdx::GetExtA(filename);

    if (ext == "obj")
    {
        return LoadFromOBJ(stream, directory, *this);
    }

    return false;
}

//-----------------------------------------------------------------------------
//      ファイルからリソースモデルを生成します.
//-----------------------------------------------------------------------------
bool ResModel::LoadFromFileW(const wchar_t* filename)
{
    auto filenameA = asdx::ToStringA(filename);
    return LoadFromFileA(filenameA.c_str());
}

} // namespace asdx
