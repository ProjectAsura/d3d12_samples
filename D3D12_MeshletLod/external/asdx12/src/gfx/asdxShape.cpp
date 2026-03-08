//-----------------------------------------------------------------------------
// File : asdxShape.cpp
// Desc : Debug Shape.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <vector>
#include <fnd/asdxLogger.h>
#include <fnd/asdxMisc.h>
#include <gfx/asdxShape.h>
#include <gfx/asdxPipelineState.h>


namespace {

#include "../res/shaders/Compiled/ShapeVS.inc"
#include "../res/shaders/Compiled/ShapePS.inc"

///////////////////////////////////////////////////////////////////////////////
// CameraParam structure
///////////////////////////////////////////////////////////////////////////////
struct CameraParam
{
    asdx::Matrix View;
    asdx::Matrix Proj;
};

//-----------------------------------------------------------------------------
//      ボックス形状を生成します.
//-----------------------------------------------------------------------------
void CreateBoxShape
(
    float width,
    float height,
    float depth,
    std::vector<asdx::Vector3>& outPositions,
    std::vector<uint32_t>&      outIndices,
    std::vector<asdx::Vector3>* outNormals   = nullptr,
    std::vector<asdx::Vector2>* outTexcoords = nullptr
)
{
    outPositions.clear();
    outIndices  .clear();

    if (outNormals != nullptr)
        outNormals->clear();
    if (outTexcoords != nullptr)
        outTexcoords->clear();

    float w2 = width  * 0.5f;
    float h2 = height * 0.5f;
    float d2 = depth  * 0.5f;

    // 頂点定義（各面ごとに4頂点、法線・UV固定）
    struct Face
    {
        asdx::Vector3 normal;
        asdx::Vector3 corners[4];
    };

    const Face faces[] = {
        // +Z (Front)
        {
            asdx::Vector3(0, 0, 1),
            {
                asdx::Vector3(-w2, -h2, +d2),
                asdx::Vector3(-w2, +h2, +d2),
                asdx::Vector3(+w2, +h2, +d2),
                asdx::Vector3(+w2, -h2, +d2),
            }
        },
        // -Z (Back)
        {
            asdx::Vector3(0, 0, -1),
            {
                asdx::Vector3(+w2, -h2, -d2),
                asdx::Vector3(+w2, +h2, -d2),
                asdx::Vector3(-w2, +h2, -d2),
                asdx::Vector3(-w2, -h2, -d2),
            }
        },
        // +Y (Top)
        {
            asdx::Vector3(0, 1, 0),
            {
                asdx::Vector3(-w2, +h2, +d2),
                asdx::Vector3(-w2, +h2, -d2),
                asdx::Vector3(+w2, +h2, -d2),
                asdx::Vector3(+w2, +h2, +d2),
            }
        },
        // -Y (Bottom)
        {
            asdx::Vector3(0, -1, 0), 
            {
                asdx::Vector3(-w2, -h2, -d2),
                asdx::Vector3(-w2, -h2, +d2),
                asdx::Vector3(+w2, -h2, +d2),
                asdx::Vector3(+w2, -h2, -d2),
            }
        },
        // -X (Left)
        {
            asdx::Vector3(-1, 0, 0),
            {
                asdx::Vector3(-w2, -h2, -d2),
                asdx::Vector3(-w2, +h2, -d2),
                asdx::Vector3(-w2, +h2, +d2),
                asdx::Vector3(-w2, -h2, +d2),
            }
        },
        // +X (Right)
        {
            asdx::Vector3(1, 0, 0),
            {
                asdx::Vector3(+w2, -h2, +d2),
                asdx::Vector3(+w2, +h2, +d2),
                asdx::Vector3(+w2, +h2, -d2),
                asdx::Vector3(+w2, -h2, -d2),
            }
        },
    };

    const asdx::Vector2 uvs[4] = {
        asdx::Vector2(0, 1),
        asdx::Vector2(0, 0),
        asdx::Vector2(1, 0),
        asdx::Vector2(1, 1),
    };

    for (auto face = 0; face < 6; ++face)
    {
        const auto&  f = faces[face];
        auto baseIndex = uint32_t(outPositions.size());

        for (auto i = 0; i < 4; ++i)
        {
            outPositions.emplace_back(f.corners[i]);
            if (outNormals != nullptr)
                outNormals->emplace_back(f.normal);
            if (outTexcoords != nullptr)
                outTexcoords->emplace_back(uvs[i]);
        }

        // 三角形2枚
        outIndices.push_back(baseIndex + 0);
        outIndices.push_back(baseIndex + 1);
        outIndices.push_back(baseIndex + 2);

        outIndices.push_back(baseIndex + 0);
        outIndices.push_back(baseIndex + 2);
        outIndices.push_back(baseIndex + 3);
    }

    outPositions.shrink_to_fit();
    outIndices  .shrink_to_fit();

    if (outNormals != nullptr)
        outNormals->shrink_to_fit();
    if (outTexcoords != nullptr)
        outTexcoords->shrink_to_fit();
}

//-----------------------------------------------------------------------------
//      ピラミッド形状を生成します.
//-----------------------------------------------------------------------------
void CreatePyramidShape
(
    float baseWidth,
    float baseDepth,
    float height,
    std::vector<asdx::Vector3>& outPositions,
    std::vector<uint32_t>&      outIndices,
    std::vector<asdx::Vector3>* outNormals   = nullptr,
    std::vector<asdx::Vector2>* outTexcoords = nullptr
)
{
    outPositions.clear();
    outIndices.clear();

    if (outNormals != nullptr)
        outNormals->clear();
    if (outTexcoords != nullptr)
        outTexcoords->clear();

    float w2 = baseWidth * 0.5f;
    float d2 = baseDepth * 0.5f;

    asdx::Vector3 baseCorners[4] = {
        asdx::Vector3(-w2, 0.0f, -d2), // 0: back-left
        asdx::Vector3(+w2, 0.0f, -d2), // 1: back-right
        asdx::Vector3(+w2, 0.0f, +d2), // 2: front-right
        asdx::Vector3(-w2, 0.0f, +d2), // 3: front-left
    };

    asdx::Vector3 apex = asdx::Vector3(0.0f, height, 0.0f);

    // UVs: base (0,1)-(1,0) | sides (0,1)-(1,1)-(0.5,0)
    asdx::Vector2 baseUVs[4] = {
        asdx::Vector2(0, 1),
        asdx::Vector2(1, 1),
        asdx::Vector2(1, 0),
        asdx::Vector2(0, 0),
    };

    asdx::Vector2 sideUVs[3] = {
        asdx::Vector2(0, 1),
        asdx::Vector2(1, 1),
        asdx::Vector2(0.5f, 0),
    };

    uint32_t baseIndex;

    // Base face (bottom quad)
    baseIndex = uint32_t(outPositions.size());
    for (int i = 0; i < 4; ++i)
    {
        outPositions.emplace_back(baseCorners[i]);

        if (outNormals != nullptr)
            outNormals->emplace_back(0.0f, -1.0f, 0.0f);

        if (outTexcoords != nullptr)
            outTexcoords->emplace_back(baseUVs[i]);
    }

    outIndices.push_back(baseIndex + 0);
    outIndices.push_back(baseIndex + 1);
    outIndices.push_back(baseIndex + 2);
    outIndices.push_back(baseIndex + 0);
    outIndices.push_back(baseIndex + 2);
    outIndices.push_back(baseIndex + 3);

    // Side faces (4 triangles)
    const int faceOrder[4][2] = {
        {0, 1}, // back
        {1, 2}, // right
        {2, 3}, // front
        {3, 0}, // left
    };

    for (int i = 0; i < 4; ++i)
    {
        const auto& p0 = baseCorners[faceOrder[i][0]];
        const auto& p1 = baseCorners[faceOrder[i][1]];

        baseIndex = uint32_t(outPositions.size());

        outPositions.emplace_back(p0);      // base corner 0
        outPositions.emplace_back(p1);      // base corner 1
        outPositions.emplace_back(apex);    // apex

        if (outNormals != nullptr)
        {
            auto normal = asdx::Vector3::ComputeNormal(p0, p1, apex);

            outNormals->emplace_back(normal);
            outNormals->emplace_back(normal);
            outNormals->emplace_back(normal);
        }

        if (outTexcoords != nullptr)
        {
            outTexcoords->emplace_back(sideUVs[0]);
            outTexcoords->emplace_back(sideUVs[1]);
            outTexcoords->emplace_back(sideUVs[2]);
        }

        outIndices.push_back(baseIndex + 0);
        outIndices.push_back(baseIndex + 1);
        outIndices.push_back(baseIndex + 2);
    }
}

//-----------------------------------------------------------------------------
//      円柱形状を生成します.
//-----------------------------------------------------------------------------
void CreateCylinderShape
(
    float       radius,
    float       height,
    uint32_t    sliceCount,
    std::vector<asdx::Vector3>& outPositions,
    std::vector<uint32_t>&      outIndices,
    std::vector<asdx::Vector3>* outNormals      = nullptr,
    std::vector<asdx::Vector2>* outTexcoords    = nullptr
)
{
    outPositions.clear();
    outIndices  .clear();

    if (outNormals != nullptr)
        outNormals->clear();
    if (outTexcoords != nullptr)
        outTexcoords->clear();

    float halfHeight = height * 0.5f;
    float dTheta = asdx::F_2PI / sliceCount;

    // 側面
    for (uint32_t i = 0; i <= sliceCount; ++i)
    {
        float theta = i * dTheta;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);
        float u = 1.0f - (float)i / sliceCount;

        auto n = asdx::Vector3::Normalize(asdx::Vector3(x, 0.0f, z));

        // 下部頂点
        outPositions.emplace_back(x, -halfHeight, z);
        if (outNormals != nullptr)
            outNormals->emplace_back(n);
        if (outTexcoords != nullptr)
            outTexcoords->emplace_back(u, 1.0f);

        // 上部頂点
        outPositions.emplace_back(x, +halfHeight, z);
        if (outNormals != nullptr)
            outNormals->emplace_back(n);
        if (outTexcoords != nullptr)
            outTexcoords->emplace_back(u, 0.0f);
    }

    // インデックス（側面）
    uint32_t sideVertexCount = uint32_t(outPositions.size());
    for (auto i = 0u; i < sliceCount; ++i)
    {
        uint32_t base = i * 2;
        outIndices.push_back(base);
        outIndices.push_back(base + 1);
        outIndices.push_back(base + 3);

        outIndices.push_back(base);
        outIndices.push_back(base + 3);
        outIndices.push_back(base + 2);
    }

    // 上面
    uint32_t topCenterIndex = uint32_t(outPositions.size());
    outPositions.emplace_back(0.0f, +halfHeight, 0.0f);
    if (outNormals != nullptr)
        outNormals->emplace_back(0.0f, 1.0f, 0.0f);
    if (outTexcoords != nullptr)
        outTexcoords->emplace_back(0.5f, 0.5f);

    for (auto i = 0u; i <= sliceCount; ++i)
    {
        float theta = i * dTheta;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);

        outPositions.emplace_back(x, +halfHeight, z);
        if (outNormals != nullptr)
            outNormals->emplace_back(0.0f, 1.0f, 0.0f);
        if (outTexcoords != nullptr)
        {
            float u =  x / radius / 2.0f + 0.5f;
            float v = -z / radius / 2.0f + 0.5f;
            outTexcoords->emplace_back(u, v);
        }
    }

    for (auto i = 0u; i < sliceCount; ++i)
    {
        outIndices.push_back(topCenterIndex);
        outIndices.push_back(topCenterIndex + i + 1);
        outIndices.push_back(topCenterIndex + i + 2);
    }

    // 底面
    uint32_t bottomCenterIndex = uint32_t(outPositions.size());
    outPositions.emplace_back(0.0f, -halfHeight, 0.0f);
    if (outNormals != nullptr)
        outNormals->emplace_back(0.0f, -1.0f, 0.0f);
    if (outTexcoords != nullptr)
        outTexcoords->emplace_back(0.5f, 0.5f);

    for (auto i = 0u; i <= sliceCount; ++i)
    {
        float theta = i * dTheta;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);

        outPositions.emplace_back(x, -halfHeight, z);
        if (outNormals != nullptr)
            outNormals->emplace_back(0.0f, -1.0f, 0.0f);
        if (outTexcoords != nullptr)
        {
            float u = x / radius / 2.0f + 0.5f;
            float v = z / radius / 2.0f + 0.5f;
            outTexcoords->emplace_back(u, v);
        }
    }

    for (auto i = 0u; i < sliceCount; ++i) 
    {
        outIndices.push_back(bottomCenterIndex);
        outIndices.push_back(bottomCenterIndex + i + 2);
        outIndices.push_back(bottomCenterIndex + i + 1);
    }

    outPositions.shrink_to_fit();
    outIndices  .shrink_to_fit();

    if (outNormals != nullptr)
        outNormals->shrink_to_fit();
    if (outTexcoords != nullptr)
        outTexcoords->shrink_to_fit();
}

//-----------------------------------------------------------------------------
//      円錐形状を生成します.
//-----------------------------------------------------------------------------
void CreateConeShape
(
    float       radius,
    float       height,
    uint32_t    sliceCount,
    std::vector<asdx::Vector3>& outPositions,
    std::vector<uint32_t>&      outIndices,
    std::vector<asdx::Vector3>* outNormals   = nullptr,
    std::vector<asdx::Vector2>* outTexcoords = nullptr
)
{
    outPositions.clear();
    outIndices  .clear();

    if (outNormals != nullptr)
        outNormals->clear();
    if (outTexcoords != nullptr)
        outTexcoords->clear();

    float halfHeight = height * 0.5f;
    float dTheta = asdx::F_2PI / sliceCount;

    // 側面の頂点（各スライスの底面＋頂点）
    auto apexPos = asdx::Vector3(0.0f, +halfHeight, 0.0f);

    for (auto i = 0u; i <= sliceCount; ++i)
    {
        float theta = i * dTheta;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);

        // 側面の位置（底辺点）
        outPositions.emplace_back(x, -halfHeight, z);

        // 法線（底辺点 → 頂点の中間法線）
        if (outNormals != nullptr)
        {
            auto p = asdx::Vector3(x, 0.0f, z);
            auto h = asdx::Vector3(0.0f, radius, 0.0f); // height と radius に応じて傾き調整
            auto n = asdx::Vector3::Normalize(p + h);
            outNormals->emplace_back(n);
        }

        // UV（横方向展開）
        if (outTexcoords != nullptr)
        {
            float u = 1.0f - (float)i / sliceCount;
            outTexcoords->emplace_back(u, 1.0f);
        }
    }

    // 頂点（円錐の頂点）
    uint32_t apexIndex = uint32_t(outPositions.size());
    outPositions.emplace_back(apexPos);
    if (outNormals != nullptr)
        outNormals->emplace_back(0.0f, 1.0f, 0.0f); // 一般的にはダミーでも良い
    if (outTexcoords != nullptr)
        outTexcoords->emplace_back(0.5f, 0.0f);     // 任意

    // 側面インデックス
    for (auto i = 0u; i < sliceCount; ++i) 
    {
        outIndices.push_back(apexIndex);
        outIndices.push_back(i);
        outIndices.push_back(i + 1);
    }

    // 底面中心
    uint32_t bottomCenterIndex = uint32_t(outPositions.size());
    outPositions.emplace_back(0.0f, -halfHeight, 0.0f);
    if (outNormals != nullptr)
        outNormals->emplace_back(0.0f, -1.0f, 0.0f);
    if (outTexcoords != nullptr)
        outTexcoords->emplace_back(0.5f, 0.5f);

    // 底面の円周頂点
    for (auto i = 0u; i <= sliceCount; ++i)
    {
        float theta = i * dTheta;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);

        outPositions.emplace_back(x, -halfHeight, z);
        if (outNormals != nullptr)
            outNormals->emplace_back(0.0f, -1.0f, 0.0f);
        if (outTexcoords != nullptr)
        {
            float u = x / radius / 2.0f + 0.5f;
            float v = z / radius / 2.0f + 0.5f;
            outTexcoords->emplace_back(u, v);
        }
    }

    // 底面インデックス（三角形ファン）
    for (auto i = 0u; i < sliceCount; ++i)
    {
        outIndices.push_back(bottomCenterIndex);
        outIndices.push_back(bottomCenterIndex + i + 2);
        outIndices.push_back(bottomCenterIndex + i + 1);
    }
}

//-----------------------------------------------------------------------------
//      球形状を生成します.
//-----------------------------------------------------------------------------
void CreateSphereShape
(
    float       radius,
    uint32_t    sliceCount,
    uint32_t    stackCount,
    std::vector<asdx::Vector3>& outPositions,
    std::vector<uint32_t>&      outIndices,
    std::vector<asdx::Vector3>* outNormals   = nullptr,
    std::vector<asdx::Vector2>* outTexcoords = nullptr
)
{
    outPositions.clear();
    outIndices  .clear();

    if (outNormals != nullptr)
        outNormals->clear();
    if (outTexcoords != nullptr)
        outTexcoords->clear();

    // 頂点生成
    for (auto i = 0u; i <= stackCount; ++i)
    {
        float phi = asdx::F_PI * (float)i / (float)stackCount; // [0, π]

        for (auto j = 0u; j <= sliceCount; ++j)
        {
            float theta = asdx::F_2PI * (float)j / (float)sliceCount; // [0, 2π]

            float x = radius * sinf(phi) * cosf(theta);
            float y = radius * cosf(phi);
            float z = radius * sinf(phi) * sinf(theta);

            asdx::Vector3 pos(x, y, z);
            outPositions.emplace_back(pos);
            if (outNormals != nullptr)
            {
                auto normal = asdx::Vector3::Normalize(pos);
                outNormals->emplace_back(normal);
            }
            if (outTexcoords != nullptr)
            {
                float u = (float)j / sliceCount;
                float v = 1.0f - (float)i / stackCount;
                outTexcoords->emplace_back(u, v);
            }
        }
    }

    // インデックス生成
    uint32_t ringVertexCount = sliceCount + 1;
    for (auto i = 0u; i < stackCount; ++i)
    {
        for (auto j = 0u; j < sliceCount; ++j)
        {
            uint32_t i0 = i * ringVertexCount + j;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + ringVertexCount;
            uint32_t i3 = i2 + 1;

            outIndices.push_back(i0);
            outIndices.push_back(i2);
            outIndices.push_back(i1);

            outIndices.push_back(i1);
            outIndices.push_back(i2);
            outIndices.push_back(i3);
        }
    }

    outPositions.shrink_to_fit();
    outIndices  .shrink_to_fit();

    if (outNormals != nullptr)
        outNormals->shrink_to_fit();
    if (outTexcoords != nullptr)
        outTexcoords->shrink_to_fit();
}

//-----------------------------------------------------------------------------
//      半球形状を生成します.
//-----------------------------------------------------------------------------
void CreateHemisphereShape
(
    float       radius,
    uint32_t    sliceCount,
    uint32_t    stackCount,
    std::vector<asdx::Vector3>& outPositions,
    std::vector<uint32_t>&      outIndices,
    std::vector<asdx::Vector3>* outNormals   = nullptr,
    std::vector<asdx::Vector2>* outTexcoords = nullptr
)
{
    outPositions.clear();
    outIndices.clear();

    if (outNormals != nullptr)
        outNormals->clear();
    if (outTexcoords != nullptr)
        outTexcoords->clear();

    float phiStep   = asdx::F_PIDIV2 / stackCount; // 半球なので π/2 まで
    float thetaStep = asdx::F_2PI / sliceCount;

    // 頂点生成
    for (auto i = 0u; i <= stackCount; ++i)
    {
        float phi = i * phiStep;

        for (auto j = 0u; j <= sliceCount; ++j)
        {
            float theta = j * thetaStep;

            // 球面座標系 → デカルト座標系
            float x = radius * sinf(phi) * cosf(theta);
            float y = radius * cosf(phi);
            float z = radius * sinf(phi) * sinf(theta);

            asdx::Vector3 position(x, y, z);
            outPositions.emplace_back(position);
            if (outNormals != nullptr)
            {
                auto normal = asdx::Vector3::Normalize(position);
                outNormals->emplace_back(normal);
            }
            if (outTexcoords != nullptr)
            {
                float u = theta / asdx::F_2PI;
                float v = phi / asdx::F_PIDIV2;
                outTexcoords->emplace_back(u, v);
            }
        }
    }

    // インデックス生成
    uint32_t ringVertexCount = sliceCount + 1;
    for (auto i = 0u; i < stackCount; ++i)
    {
        for (auto j = 0u; j < sliceCount; ++j)
        {
            uint32_t i0 = (i + 0) * ringVertexCount + (j + 0);
            uint32_t i1 = (i + 1) * ringVertexCount + (j + 0);
            uint32_t i2 = (i + 1) * ringVertexCount + (j + 1);
            uint32_t i3 = (i + 0) * ringVertexCount + (j + 1);

            outIndices.push_back(i0);
            outIndices.push_back(i1);
            outIndices.push_back(i2);

            outIndices.push_back(i0);
            outIndices.push_back(i2);
            outIndices.push_back(i3);
        }
    }

    outPositions.shrink_to_fit();
    outIndices  .shrink_to_fit();

    if (outNormals != nullptr)
        outNormals->shrink_to_fit();
    if (outTexcoords != nullptr)
        outTexcoords->shrink_to_fit();
}

//-----------------------------------------------------------------------------
//      円盤形状を生成します.
//-----------------------------------------------------------------------------
void CreateDiskShape
(
    float       radius,
    uint32_t    sliceCount,
    std::vector<asdx::Vector3>& outPositions,
    std::vector<uint32_t>&      outIndices,
    std::vector<asdx::Vector3>* outNormals   = nullptr,
    std::vector<asdx::Vector2>* outTexcoords = nullptr
)
{
    outPositions.clear();
    outIndices.clear();
    if (outNormals != nullptr)
        outNormals->clear();
    if (outTexcoords != nullptr)
        outTexcoords->clear();

    float dTheta = asdx::F_2PI / sliceCount;

    // 中心点
    outPositions.emplace_back(0.0f, 0.0f, 0.0f);
    if (outNormals != nullptr)
        outNormals->emplace_back(0.0f, 1.0f, 0.0f); // 上向き
    if (outTexcoords != nullptr)
        outTexcoords->emplace_back(0.5f, 0.5f);

    // 円周上の点
    for (auto i = 0u; i <= sliceCount; ++i)
    {
        auto theta = i * dTheta;
        auto x = radius * cosf(theta);
        auto z = radius * sinf(theta);

        outPositions.emplace_back(x, 0.0f, z);
        if (outNormals != nullptr)
            outNormals->emplace_back(0.0f, 1.0f, 0.0f); // 一律上向き
        if (outTexcoords != nullptr)
        {
            auto u = 0.5f + 0.5f * cosf(theta);
            auto v = 0.5f + 0.5f * sinf(theta);
            outTexcoords->emplace_back(u, v);
        }
    }

    // インデックス（三角形ファン）
    for (auto i = 1u; i <= sliceCount; ++i)
    {
        outIndices.push_back(0);        // 中心
        outIndices.push_back(i + 0);
        outIndices.push_back(i + 1);
    }

    outPositions.shrink_to_fit();
    outIndices  .shrink_to_fit();
    if (outNormals != nullptr)
        outNormals->shrink_to_fit();
    if (outTexcoords != nullptr)
        outTexcoords->shrink_to_fit();
}

//-----------------------------------------------------------------------------
//      扇形形状を生成します.
//-----------------------------------------------------------------------------
void CreateFanShape
(
    float       radius,
    float       startAngle,
    float       sweepAngle,
    uint32_t    sliceCount,
    std::vector<asdx::Vector3>& outPositions,
    std::vector<uint32_t>&      outIndices,
    std::vector<asdx::Vector3>* outNormals = nullptr,
    std::vector<asdx::Vector2>* outTexcoords = nullptr
)
{
    outPositions.clear();
    outIndices  .clear();
    if (outNormals != nullptr)
        outNormals->clear();
    if (outTexcoords != nullptr)
        outTexcoords->clear();

    float dTheta = sweepAngle / sliceCount;

    // 中心点
    outPositions.emplace_back(0.0f, 0.0f, 0.0f);
    if (outNormals != nullptr)
        outNormals->emplace_back(0.0f, 1.0f, 0.0f); // 上向き
    if (outTexcoords != nullptr)
        outTexcoords->emplace_back(0.5f, 0.5f);

    // 円弧上の頂点を生成
    for (uint32_t i = 0; i <= sliceCount; ++i)
    {
        float theta = startAngle + i * dTheta;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);

        outPositions.emplace_back(x, 0.0f, z);
        if (outNormals != nullptr)
            outNormals->emplace_back(0.0f, 1.0f, 0.0f);
        if (outTexcoords != nullptr)
        {
            float u = 0.5f + 0.5f * cosf(theta);
            float v = 0.5f + 0.5f * sinf(theta);
            outTexcoords->emplace_back(u, v);
        }
    }

    // インデックス（三角形ファン）
    for (uint32_t i = 1; i <= sliceCount; ++i) 
    {
        outIndices.push_back(0);       // 中心点
        outIndices.push_back(i);
        outIndices.push_back(i + 1);
    }

    outPositions.shrink_to_fit();
    outIndices  .shrink_to_fit();
    if (outNormals != nullptr)
        outNormals->shrink_to_fit();
    if (outTexcoords != nullptr)
        outTexcoords->shrink_to_fit();
}

//-----------------------------------------------------------------------------
//      平面形状を生成します.
//-----------------------------------------------------------------------------
void CreatePlaneShape
(
    float    width,
    float    depth,
    uint32_t widthSegments,
    uint32_t depthSegments,
    std::vector<asdx::Vector3>& outPositions,
    std::vector<uint32_t>&      outIndices,
    std::vector<asdx::Vector3>* outNormals   = nullptr,
    std::vector<asdx::Vector2>* outTexcoords = nullptr
)
{
    outPositions.clear();
    outIndices.clear();
    if (outNormals != nullptr)
       outNormals->clear();
    if (outTexcoords != nullptr)
        outTexcoords->clear();

    uint32_t vertexCountX = widthSegments + 1;
    uint32_t vertexCountZ = depthSegments + 1;

    float halfWidth = width * 0.5f;
    float halfDepth = depth * 0.5f;

    float dx = width / widthSegments;
    float dz = depth / depthSegments;

    float du = 1.0f / widthSegments;
    float dv = 1.0f / depthSegments;

    // 頂点生成
    for (uint32_t z = 0; z < vertexCountZ; ++z) 
    {
        float posZ = -halfDepth + z * dz;
        float v = z * dv;

        for (uint32_t x = 0; x < vertexCountX; ++x)
        {
            float posX = -halfWidth + x * dx;
            float u = x * du;

            outPositions.emplace_back(posX, 0.0f, posZ);
            if (outNormals != nullptr)
                outNormals->emplace_back(0.0f, 1.0f, 0.0f);
            if (outTexcoords != nullptr)
                outTexcoords->emplace_back(u, v);
        }
    }

    // インデックス（三角形分割）
    for (uint32_t z = 0; z < depthSegments; ++z)
    {
        for (uint32_t x = 0; x < widthSegments; ++x)
        {
            uint32_t i0 = z * vertexCountX + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + vertexCountX;
            uint32_t i3 = i2 + 1;

            // 三角形1
            outIndices.push_back(i0);
            outIndices.push_back(i2);
            outIndices.push_back(i1);

            // 三角形2
            outIndices.push_back(i1);
            outIndices.push_back(i2);
            outIndices.push_back(i3);
        }
    }

    outPositions.shrink_to_fit();
    outIndices  .shrink_to_fit();
    if (outNormals != nullptr)
        outNormals->shrink_to_fit();
    if (outTexcoords != nullptr)
        outTexcoords->shrink_to_fit();
}

//-----------------------------------------------------------------------------
//      カプセル形状を生成します.
//-----------------------------------------------------------------------------
void CreateCapsuleShape
(
    float       radius,
    float       height,
    uint32_t    sliceCount,
    uint32_t    stackCount,
    std::vector<asdx::Vector3>& outPositions,
    std::vector<uint32_t>&      outIndices,
    std::vector<asdx::Vector3>* outNormals   = nullptr,
    std::vector<asdx::Vector2>* outTexcoords = nullptr
)
{
    outPositions.clear();
    outIndices  .clear();
    if (outNormals != nullptr)
        outNormals->clear();
    if (outTexcoords != nullptr)
        outTexcoords->clear();

    const float    cylinderHeight   = height;
    const float    halfHeight       = cylinderHeight * 0.5f;
    const uint32_t hemisphereStacks = stackCount / 2;

    // 上半球
    for (auto i = 0u; i <= hemisphereStacks; ++i) 
    {
        float phi = asdx::F_PIDIV2 * ((float)i / hemisphereStacks); // 0 to PI/2
        for (auto j = 0u; j <= sliceCount; ++j) 
        {
            float theta = asdx::F_2PI * ((float)j / sliceCount);
            float x = radius * sinf(phi) * cosf(theta);
            float y = radius * sinf(phi) * sinf(theta);
            float z = radius * cosf(phi) + halfHeight;
            outPositions.emplace_back(x, y, z);
            if (outNormals != nullptr)
            {
                auto n = asdx::Vector3::Normalize(asdx::Vector3(x, y, z));
                outNormals->emplace_back(n);
            }
            if (outTexcoords != nullptr)
            {
                float u = (float)j / sliceCount;
                float v = 1.0f - ((float)i / stackCount);
                outTexcoords->emplace_back(u, v);
            }
        }
    }

    // 円柱（側面）
    for (auto i = 1u; i < stackCount; ++i)
    {
        float t = (float)i / stackCount;
        float z = asdx::Lerp(-halfHeight, +halfHeight, t);
        for (auto j = 0u; j <= sliceCount; ++j)
        {
            float theta = asdx::F_2PI * ((float)j / sliceCount);
            float x = radius * cosf(theta);
            float y = radius * sinf(theta);
            outPositions.emplace_back(x, y, z);
            if (outNormals != nullptr)
            {
                auto n = asdx::Vector3::Normalize(asdx::Vector3(x, y, z));
                outNormals->emplace_back(n);
            }
            if (outTexcoords != nullptr)
            {
                float u = (float)j / sliceCount;
                float v = 1.0f - (float)(i + hemisphereStacks) / (stackCount + hemisphereStacks * 2);
                outTexcoords->emplace_back(u, v);
            }
        }
    }

    // 下半球
    for (auto i = 1u; i <= hemisphereStacks; ++i)
    {
        float phi = asdx::F_PIDIV2 * ((float)i / hemisphereStacks);
        for (auto j = 0u; j <= sliceCount; ++j)
        {
            float theta = asdx::F_2PI * ((float)j / sliceCount);
            float x =  radius * sinf(phi) * cosf(theta);
            float y =  radius * sinf(phi) * sinf(theta);
            float z = -radius * cosf(phi) - halfHeight;
            outPositions.emplace_back(x, y, z);
            if (outNormals != nullptr)
            {
                auto n = asdx::Vector3::Normalize(asdx::Vector3(x, y, z));
                outNormals->emplace_back(n);
            }
            if (outTexcoords != nullptr)
            {
                float u = (float)j / sliceCount;
                float v = 1.0f - ((float)(i + stackCount) / (stackCount + hemisphereStacks * 2));
                outTexcoords->emplace_back(u, v);
            }
        }
    }

    // インデックス生成
    auto ringVertexCount  = sliceCount + 1;
    auto stackVertexCount = stackCount + hemisphereStacks * 2;
    for (auto i = 0u; i < stackVertexCount; ++i)
    {
        for (auto j = 0u; j < sliceCount; ++j)
        {
            uint32_t i0 = i * ringVertexCount + j;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + ringVertexCount;
            uint32_t i3 = i2 + 1;

            outIndices.push_back(i0);
            outIndices.push_back(i2);
            outIndices.push_back(i1);

            outIndices.push_back(i1);
            outIndices.push_back(i2);
            outIndices.push_back(i3);
        }
    }

    outPositions.shrink_to_fit();
    outIndices  .shrink_to_fit();
    if (outNormals != nullptr)
        outNormals->shrink_to_fit();
    if (outTexcoords != nullptr)
        outTexcoords->shrink_to_fit();
}

} // namespace

namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// ShapeStates class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ShapeStates::ShapeStates()
: m_BufferIndex(0)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ShapeStates::~ShapeStates()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool ShapeStates::Init(ID3D12Device* pDevice, DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat)
{
    if (pDevice == nullptr)
    { return false; }

    // ルートシグニチャ生成.
    {
        D3D12_ROOT_PARAMETER params[2] = {};
        asdx::InitAsCBV(params[0], 0, D3D12_SHADER_VISIBILITY_ALL);
        asdx::InitAsCBV(params[1], 1, D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.pParameters        = params;
        desc.NumParameters      = _countof(params);
        desc.pStaticSamplers    = nullptr;
        desc.NumStaticSamplers  = 0;
        desc.Flags              = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        asdx::RefPtr<ID3DBlob> pBlob;
        asdx::RefPtr<ID3DBlob> pErrorBlob;

        auto hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, pBlob.GetAddress(), pErrorBlob.GetAddress());
        if (FAILED(hr))
        {
            ELOG("Error : D3D12SerializeRootSignature() Failed. errcode = 0x%x, msg = %s", hr, reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
            return false;
        }

        hr = pDevice->CreateRootSignature(0, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.GetAddress()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateRootSignature() Failed. errcode = 0x%x", hr);
            return false;
        }
    }

    D3D12_INPUT_ELEMENT_DESC elements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // 不透明パイプラインステート生成.
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature         = m_RootSignature.GetPtr();
        desc.VS                     = { ShapeVS, sizeof(ShapeVS) };
        desc.PS                     = { ShapePS, sizeof(ShapePS) };
        desc.BlendState             = asdx::Preset::Opaque;
        desc.SampleMask             = D3D12_DEFAULT_SAMPLE_MASK;
        desc.RasterizerState        = asdx::Preset::CullNone;
        desc.DepthStencilState      = asdx::Preset::DepthDefault;
        desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.InputLayout            = { elements, 1 };
        desc.NumRenderTargets       = 1;
        desc.RTVFormats[0]          = colorFormat;
        desc.DSVFormat              = depthFormat;
        desc.SampleDesc.Count       = 1;
        desc.SampleDesc.Quality     = 0;

        auto hr = pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_OpaqueState.GetAddress()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. errcode = 0x%x", hr);
            return false;
        }
    }

    // 半透明パイプラインステート生成.
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature         = m_RootSignature.GetPtr();
        desc.VS                     = { ShapeVS, sizeof(ShapeVS) };
        desc.PS                     = { ShapePS, sizeof(ShapePS) };
        desc.BlendState             = asdx::Preset::AlphaBlend;
        desc.SampleMask             = D3D12_DEFAULT_SAMPLE_MASK;
        desc.RasterizerState        = asdx::Preset::CullNone;
        desc.DepthStencilState      = asdx::Preset::DepthReadOnly;
        desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.InputLayout            = { elements, 1 };
        desc.NumRenderTargets       = 1;
        desc.RTVFormats[0]          = colorFormat;
        desc.DSVFormat              = depthFormat;
        desc.SampleDesc.Count       = 1;
        desc.SampleDesc.Quality     = 0;

        auto hr = pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_TranslucentState.GetAddress()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. errcode = 0x%x", hr);
            return false;
        }
    }

    // ワイヤーフレームパイプラインステート生成.
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature         = m_RootSignature.GetPtr();
        desc.VS                     = { ShapeVS, sizeof(ShapeVS) };
        desc.PS                     = { ShapePS, sizeof(ShapePS) };
        desc.BlendState             = asdx::Preset::Opaque;
        desc.SampleMask             = D3D12_DEFAULT_SAMPLE_MASK;
        desc.RasterizerState        = asdx::Preset::Wireframe;
        desc.DepthStencilState      = asdx::Preset::DepthDefault;
        desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.InputLayout            = { elements, 1 };
        desc.NumRenderTargets       = 1;
        desc.RTVFormats[0]          = colorFormat;
        desc.DSVFormat              = depthFormat;
        desc.SampleDesc.Count       = 1;
        desc.SampleDesc.Quality     = 0;

        auto hr = pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_WireframeState.GetAddress()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateGraphicsPipelineState() Failed. errcode = 0x%x", hr);
            return false;
        }
    }

    // カメラ定数バッファ生成.
    {
        auto size = asdx::RoundUp(sizeof(ShapeParam), 256) * 2;

        D3D12_HEAP_PROPERTIES props = {
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            1,
            1
        };

        D3D12_RESOURCE_DESC desc = {
            D3D12_RESOURCE_DIMENSION_BUFFER,
            0,
            size,
            1,
            1,
            1,
            DXGI_FORMAT_UNKNOWN,
            { 1, 0 },
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            D3D12_RESOURCE_FLAG_NONE
        };

        auto hr = pDevice->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_CameraBuffer.GetAddress()));
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr );
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void ShapeStates::Term()
{
    m_CameraBuffer    .Reset();
    m_WireframeState  .Reset();
    m_TranslucentState.Reset();
    m_OpaqueState     .Reset();
    m_RootSignature   .Reset();
}

//-----------------------------------------------------------------------------
//      ビュー行列と射影行列を設定します.
//-----------------------------------------------------------------------------
void ShapeStates::SetMatrix(const asdx::Matrix& view, const asdx::Matrix& proj)
{
    m_View = view;
    m_Proj = proj;

    CameraParam param;
    param.View = m_View;
    param.Proj = m_Proj;

    auto size   = asdx::RoundUp(sizeof(CameraParam), 256);
    auto offset = m_BufferIndex * size;

    uint8_t* pData = nullptr;
    auto hr = m_CameraBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    if (SUCCEEDED(hr))
    { memcpy(pData + offset, &param, sizeof(param)); }
    m_CameraBuffer->Unmap(0, nullptr);

    m_BufferIndex = (m_BufferIndex + 1) & 0x1;
}

//-----------------------------------------------------------------------------
//      不透明ステートを適用します.
//-----------------------------------------------------------------------------
void ShapeStates::ApplyOpaqueState(ID3D12GraphicsCommandList* pCmd)
{
    auto size    = asdx::RoundUp(sizeof(CameraParam), 256);
    auto offset  = m_BufferIndex * size;
    auto address = m_CameraBuffer->GetGPUVirtualAddress() + offset;

    pCmd->SetGraphicsRootSignature(m_RootSignature.GetPtr());
    pCmd->SetPipelineState(m_OpaqueState.GetPtr());
    pCmd->SetGraphicsRootConstantBufferView(0, address);
    pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//-----------------------------------------------------------------------------
//      半透明ステートを適用します.
//-----------------------------------------------------------------------------
void ShapeStates::ApplyTranslucentState(ID3D12GraphicsCommandList* pCmd)
{
    auto size    = asdx::RoundUp(sizeof(CameraParam), 256);
    auto offset  = m_BufferIndex * size;
    auto address = m_CameraBuffer->GetGPUVirtualAddress() + offset;

    pCmd->SetGraphicsRootSignature(m_RootSignature.GetPtr());
    pCmd->SetPipelineState(m_TranslucentState.GetPtr());
    pCmd->SetGraphicsRootConstantBufferView(0, address);
    pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//-----------------------------------------------------------------------------
//      ワイヤーフレームステートを適用します.
//-----------------------------------------------------------------------------
void ShapeStates::ApplyWireframeState(ID3D12GraphicsCommandList* pCmd)
{
    auto size    = asdx::RoundUp(sizeof(CameraParam), 256);
    auto offset  = m_BufferIndex * size;
    auto address = m_CameraBuffer->GetGPUVirtualAddress() + offset;

    pCmd->SetGraphicsRootSignature(m_RootSignature.GetPtr());
    pCmd->SetPipelineState(m_WireframeState.GetPtr());
    pCmd->SetGraphicsRootConstantBufferView(0, address);
    pCmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

///////////////////////////////////////////////////////////////////////////////
// ShapeBase class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ShapeBase::ShapeBase()
{
    m_Param.World = asdx::Matrix::CreateIdentity();
    m_Param.Color = asdx::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    m_BufferIndex = 0;
}

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ShapeBase::~ShapeBase()
{ Reset(); }

//-----------------------------------------------------------------------------
//      ワールド行列を設定します.
//-----------------------------------------------------------------------------
void ShapeBase::SetWorld(const asdx::Matrix& value)
{ m_Param.World = value; }

//-----------------------------------------------------------------------------
//      カラーを設定します.
//-----------------------------------------------------------------------------
void ShapeBase::SetColor(const asdx::Vector4& value)
{ m_Param.Color = value; }

//-----------------------------------------------------------------------------
//      ワールド行列を取得します.
//-----------------------------------------------------------------------------
const asdx::Matrix& ShapeBase::GetWorld() const
{ return m_Param.World; }

//-----------------------------------------------------------------------------
//      カラーを取得します.
//-----------------------------------------------------------------------------
const asdx::Vector4& ShapeBase::GetColor() const
{ return m_Param.Color; }

//-----------------------------------------------------------------------------
//      バッファを初期化します.
//-----------------------------------------------------------------------------
bool ShapeBase::InitBuffer
(
    ID3D12Device*           pDevice,
    const asdx::Vector3*    positions,
    size_t                  positionCount,
    const uint32_t*         indices,
    size_t                  indexCount
)
{
    // 頂点バッファ生成.
    {
        D3D12_HEAP_PROPERTIES prop = {};
        prop.Type                   = D3D12_HEAP_TYPE_UPLOAD;
        prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
        prop.VisibleNodeMask        = 1;
        prop.CreationNodeMask       = 1;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width              = sizeof(asdx::Vector3) * positionCount;
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.MipLevels          = 1;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        auto state = D3D12_RESOURCE_STATE_GENERIC_READ;
        auto flags = D3D12_HEAP_FLAG_NONE;

        auto hr = pDevice->CreateCommittedResource(
            &prop,
            flags,
            &desc,
            state,
            nullptr,
            IID_PPV_ARGS(m_VB.GetAddress()));
        if ( FAILED(hr) )
        {
            ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
            return false;
        }

        m_VBV.BufferLocation = m_VB->GetGPUVirtualAddress();
        m_VBV.SizeInBytes    = uint32_t(sizeof(asdx::Vector3) * positionCount);
        m_VBV.StrideInBytes  = uint32_t(sizeof(asdx::Vector3));

        if (positions != nullptr)
        {
            uint8_t* pDst = nullptr;
            auto hr = m_VB->Map(0, nullptr, reinterpret_cast<void**>(&pDst));
            if (FAILED(hr))
            {
                ELOG("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
                return false;
            }

            memcpy(pDst, positions, sizeof(asdx::Vector3) * positionCount);

            m_VB->Unmap(0, nullptr);
        }
    }

    // インデックスバッファ生成.
    if (indexCount > 0)
    {
        D3D12_HEAP_PROPERTIES prop = {};
        prop.Type                   = D3D12_HEAP_TYPE_UPLOAD;
        prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
        prop.VisibleNodeMask        = 1;
        prop.CreationNodeMask       = 1;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width              = sizeof(uint32_t) * indexCount;
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.MipLevels          = 1;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        auto state = D3D12_RESOURCE_STATE_GENERIC_READ;
        auto flags = D3D12_HEAP_FLAG_NONE;

        auto hr = pDevice->CreateCommittedResource(
            &prop,
            flags,
            &desc,
            state,
            nullptr,
            IID_PPV_ARGS(m_IB.GetAddress()));
        if ( FAILED(hr) )
        {
            ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. errode = 0x%x", hr);
            return false;
        }

        m_IndexCount = uint32_t(indexCount);

        m_IBV.BufferLocation = m_IB->GetGPUVirtualAddress();
        m_IBV.Format         = DXGI_FORMAT_R32_UINT;
        m_IBV.SizeInBytes    = uint32_t(sizeof(uint32_t) * indexCount);

        if (indices != nullptr)
        {
            uint8_t* pDst = nullptr;
            auto hr = m_IB->Map(0, nullptr, reinterpret_cast<void**>(&pDst));
            if (FAILED(hr))
            {
                ELOG("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
                return false;
            }

            memcpy(pDst, indices, sizeof(uint32_t) * indexCount);
            m_IB->Unmap(0, nullptr);
        }
    }

    // 定数バッファ生成.
    {
        auto size = asdx::RoundUp(sizeof(ShapeParam), 256) * 2;

        D3D12_HEAP_PROPERTIES props = {
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            1,
            1
        };

        D3D12_RESOURCE_DESC desc = {
            D3D12_RESOURCE_DIMENSION_BUFFER,
            0,
            size,
            1,
            1,
            1,
            DXGI_FORMAT_UNKNOWN,
            { 1, 0 },
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            D3D12_RESOURCE_FLAG_NONE
        };

        auto hr = pDevice->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_CB.GetAddress()));
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr );
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
//      リセット処理を行います.
//-----------------------------------------------------------------------------
void ShapeBase::Reset()
{
    m_VB.Reset();
    m_IB.Reset();
    m_CB.Reset();
    m_BufferIndex = 0;
    m_Param.World = asdx::Matrix::CreateIdentity();
    m_Param.Color = asdx::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
}

//-----------------------------------------------------------------------------
//      定数バッファを更新します.
//-----------------------------------------------------------------------------
D3D12_GPU_VIRTUAL_ADDRESS ShapeBase::UpdateParam()
{
    auto size   = asdx::RoundUp(sizeof(ShapeParam), 256);
    auto offset = m_BufferIndex * size;

    uint8_t* pData = nullptr;
    auto hr = m_CB->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    if (SUCCEEDED(hr))
    { memcpy(pData + offset, &m_Param, sizeof(m_Param)); }
    m_CB->Unmap(0, nullptr);

    m_BufferIndex = (m_BufferIndex + 1) & 0x1;

    return m_CB->GetGPUVirtualAddress() + offset;
}

//-----------------------------------------------------------------------------
//      描画処理を行います.
//-----------------------------------------------------------------------------
void ShapeBase::Draw(ID3D12GraphicsCommandList* pCmd)
{
    auto address = UpdateParam();

    pCmd->IASetVertexBuffers(0, 1, &m_VBV);
    pCmd->IASetIndexBuffer(&m_IBV);
    pCmd->SetGraphicsRootConstantBufferView(1, address);
    pCmd->DrawIndexedInstanced(m_IndexCount, 1, 0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
// BoxShape class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
BoxShape::BoxShape()
: ShapeBase()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
BoxShape::~BoxShape()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool BoxShape::Init(ID3D12Device* pDevice, float size)
{
    std::vector<asdx::Vector3> positions;
    std::vector<uint32_t>      indices;

    CreateBoxShape(size, size, size, positions, indices);

    if (!InitBuffer(pDevice,
        positions.data(), positions.size(),
        indices  .data(), indices  .size()))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void BoxShape::Term()
{ Reset(); }


///////////////////////////////////////////////////////////////////////////////
// SphereShape class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
SphereShape::SphereShape()
: ShapeBase()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
SphereShape::~SphereShape()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool SphereShape::Init(ID3D12Device* pDevice, float radius, uint32_t sliceCount)
{
    std::vector<asdx::Vector3> positions;
    std::vector<uint32_t>      indices;

    CreateSphereShape(radius, sliceCount, sliceCount, positions, indices);

    if (!InitBuffer(pDevice,
        positions.data(), positions.size(),
        indices  .data(), indices  .size()))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void SphereShape::Term()
{ Reset(); }


///////////////////////////////////////////////////////////////////////////////
// HemisphereShape class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
HemisphereShape::HemisphereShape()
: ShapeBase()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
HemisphereShape::~HemisphereShape()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool HemisphereShape::Init(ID3D12Device* pDevice, float radius, uint32_t sliceCount)
{
    std::vector<asdx::Vector3> positions;
    std::vector<uint32_t>      indices;

    CreateHemisphereShape(radius, sliceCount, sliceCount, positions, indices);

    if (!InitBuffer(pDevice,
        positions.data(), positions.size(),
        indices  .data(), indices  .size()))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void HemisphereShape::Term()
{ Reset(); }


///////////////////////////////////////////////////////////////////////////////
// ConeShape class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ConeShape::ConeShape()
: ShapeBase()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ConeShape::~ConeShape()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool ConeShape::Init(ID3D12Device* pDevice, float height, float radius, uint32_t sliceCount)
{
    std::vector<asdx::Vector3> positions;
    std::vector<uint32_t>      indices;

    CreateConeShape(radius, height, sliceCount, positions, indices);

    if (!InitBuffer(pDevice,
        positions.data(), positions.size(),
        indices  .data(), indices  .size()))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void ConeShape::Term()
{ Reset(); }


///////////////////////////////////////////////////////////////////////////////
// PyramidShape class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
PyramidShape::PyramidShape()
: ShapeBase()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
PyramidShape::~PyramidShape()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool PyramidShape::Init(ID3D12Device* pDevice, float length, float width)
{
    std::vector<asdx::Vector3> positions;
    std::vector<uint32_t>      indices;

    CreatePyramidShape(width, width, length, positions, indices);

    if (!InitBuffer(pDevice,
        positions.data(), positions.size(),
        indices  .data(), indices  .size()))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void PyramidShape::Term()
{ Reset(); }


///////////////////////////////////////////////////////////////////////////////
// CylinderShape class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
CylinderShape::CylinderShape()
: ShapeBase()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
CylinderShape::~CylinderShape()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool CylinderShape::Init(ID3D12Device* pDevice, float radius, float height, uint32_t sliceCount)
{
    std::vector<asdx::Vector3> positions;
    std::vector<uint32_t>      indices;

    CreateCylinderShape(radius, height, sliceCount, positions, indices);

    if (!InitBuffer(pDevice,
        positions.data(), positions.size(),
        indices  .data(), indices  .size()))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void CylinderShape::Term()
{ Reset(); }


///////////////////////////////////////////////////////////////////////////////
// PlaneShape class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
PlaneShape::PlaneShape()
: ShapeBase()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
PlaneShape::~PlaneShape()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool PlaneShape::Init(ID3D12Device* pDevice, float width, float height)
{
    std::vector<asdx::Vector3> positions;
    std::vector<uint32_t>      indices;

    CreatePlaneShape(width, height, 1, 1, positions, indices);

    if (!InitBuffer(pDevice,
        positions.data(), positions.size(),
        indices  .data(), indices  .size()))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void PlaneShape::Term()
{ Reset(); }


///////////////////////////////////////////////////////////////////////////////
// CapsuleShape class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
CapsuleShape::CapsuleShape()
: ShapeBase()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
CapsuleShape::~CapsuleShape()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool CapsuleShape::Init(ID3D12Device* pDevice, float length, float radius, uint32_t sliceCount)
{
    std::vector<asdx::Vector3> positions;
    std::vector<uint32_t>      indices;

    CreateCapsuleShape(radius, length, sliceCount, sliceCount, positions, indices);

    if (!InitBuffer(pDevice,
        positions.data(), positions.size(),
        indices  .data(), indices  .size()))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void CapsuleShape::Term()
{ Reset(); }


///////////////////////////////////////////////////////////////////////////////
// DiskShape class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
DiskShape::DiskShape()
: ShapeBase()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
DiskShape::~DiskShape()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool DiskShape::Init(ID3D12Device* pDevice, float radius, uint32_t sliceCount)
{
    std::vector<asdx::Vector3> positions;
    std::vector<uint32_t>      indices;

    CreateDiskShape(radius, sliceCount, positions, indices);

    if (!InitBuffer(pDevice,
        positions.data(), positions.size(),
        indices  .data(), indices  .size()))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void DiskShape::Term()
{ Reset(); }


///////////////////////////////////////////////////////////////////////////////
// FanShape class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
FanShape::FanShape()
: ShapeBase()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
FanShape::~FanShape()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool FanShape::Init(ID3D12Device* pDevice, float radius, float startAngleRad, float sweepAngleRad, uint32_t sliceCount)
{
    std::vector<asdx::Vector3> positions;
    std::vector<uint32_t>      indices;

    CreateFanShape(radius, startAngleRad, sweepAngleRad, sliceCount, positions, indices);

    if (!InitBuffer(pDevice,
        positions.data(), positions.size(),
        indices.data(), indices.size()))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void FanShape::Term()
{ Reset(); }


///////////////////////////////////////////////////////////////////////////////
// BoneShape class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
BoneShape::BoneShape()
: ShapeBase()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
BoneShape::~BoneShape()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool BoneShape::Init(ID3D12Device* pDevice, float length, float width)
{
    auto s = width * 0.5f;
    asdx::Vector3 vertices[6] = {
        asdx::Vector3(0.0f, length, 0.0f),

        asdx::Vector3(-s, length * 0.1f, -s),
        asdx::Vector3( s, length * 0.1f, -s),
        asdx::Vector3( s, length * 0.1f,  s),
        asdx::Vector3(-s, length * 0.1f,  s),

        asdx::Vector3(0.0f, 0.0f, 0.0f),
    };

    // 基底変換行列
    auto basis = asdx::Matrix(
        0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    for(auto i=0; i<6; ++i)
    { vertices[i] = asdx::Vector3::Transform(vertices[i], basis); }

    uint32_t indices[24] = {
        0, 2, 1,
        0, 3, 2,
        0, 4, 3,
        0, 1, 4,

        1, 2, 5,
        2, 3, 5,
        4, 3, 5,
        4, 1, 5,
    };

    if (!InitBuffer(pDevice, vertices, 6, indices, 24))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void BoneShape::Term()
{ Reset(); }


} // namespace asdx
