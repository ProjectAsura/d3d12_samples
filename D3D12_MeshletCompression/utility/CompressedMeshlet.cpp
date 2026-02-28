//-----------------------------------------------------------------------------
// File : CompressedMeshlet.cpp
// Desc : Compressed Meshlet.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <array>
#include <CompressedMeshlet.h>
#include <fnd/asdxLogger.h>
#include <fnd/asdxMisc.h>


namespace {

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
const uint32_t kResCompressedMeshletsHeaderVersion = 1u;


///////////////////////////////////////////////////////////////////////////////
// ResCompressedMeshletsHeader structure
///////////////////////////////////////////////////////////////////////////////
struct ResCompressedMeshletsHeader
{
    char                Magic[4];
    uint32_t            Version;
    uint64_t            PositionCount;
    uint64_t            NormalCount;
    uint64_t            TangentCount;
    uint64_t            TexCoordCount;
    uint64_t            PrimitiveCount;
    uint64_t            VertexIndexCount;
    uint64_t            MeshletCount;
    uint64_t            SubsetCount;
    asdx::Vector4       BoundingSphere;
    QuantizationInfo3   PositionInfo;
    QuantizationInfo2   NormalInfo;
    QuantizationInfo1   TangentInfo;
    QuantizationInfo2   TexCoordInfo;
};

//-----------------------------------------------------------------------------
//      量子化を行います.
//-----------------------------------------------------------------------------
void Quantization1
(
    const std::vector<float>&          values,
    const std::vector<uint32_t>        indices,
    const std::vector<MeshletInfo>&    meshlets,
    QuantizationInfo1&                 outInfo,
    std::vector<uint16_t>&             outValues,
    std::vector<uint32_t>&             outQuantizationMeshletOffsets
)
{
    struct BoundingBox
    {
        float   Min;
        float   Max;
    };

    // AABBを求める.
    BoundingBox globalBox;
    std::vector<BoundingBox> meshletBox(meshlets.size());

    globalBox.Max = values[0];
    globalBox.Min = values[0];

    for(size_t i=0; i<meshlets.size(); ++i)
    {
        auto& offsets = meshlets[i].VertexOffset;
        auto  vertId  = indices[offsets];

        BoundingBox box;
        box.Max = values[vertId];
        box.Min = values[vertId];

        globalBox.Max = asdx::Max(globalBox.Max, values[vertId]);
        globalBox.Min = asdx::Min(globalBox.Min, values[vertId]);

        for(size_t j=1; j<meshlets[i].VertexCount; ++j)
        {
            vertId = indices[offsets + j];
            box.Max = asdx::Max(box.Max, values[vertId]);
            box.Min = asdx::Min(box.Min, values[vertId]);

            globalBox.Max = asdx::Max(globalBox.Max, values[vertId]);
            globalBox.Min = asdx::Min(globalBox.Min, values[vertId]);
        }

        meshletBox[i] = box;
    }

    const float kGlobalDelta = globalBox.Max - globalBox.Min;
    assert(kGlobalDelta > 0.0f);

    float largestDelta(0.0f);

    const auto kTargetBits = 16;

    for(size_t i=0; i<meshletBox.size(); ++i)
    {
        auto delta = meshletBox[i].Max - meshletBox[i].Min;
        largestDelta = asdx::Max(largestDelta, delta);
    }
    assert(largestDelta > 0.0f);

    const float    kMeshletsQuantizationStep = largestDelta / ((1u << kTargetBits) - 1);
    const uint32_t kGlobalQuantizationStates = uint32_t(kGlobalDelta / kMeshletsQuantizationStep);
    const float    kQuantizationFactor       = (kGlobalQuantizationStates - 1) / kGlobalDelta;
    const float    kDequantizationFactor     = kGlobalDelta / (kGlobalQuantizationStates - 1);

    outQuantizationMeshletOffsets.resize(meshlets.size());
    for(size_t i=0; i<meshlets.size(); ++i)
    {
        auto diffX = meshletBox[i].Min - globalBox.Min;
        assert(diffX >= 0.0f);

        const uint32_t kQuantizationMeshletOffset = uint32_t(diffX * kQuantizationFactor + 0.5f);

        for(auto j=0u; j<meshlets[i].VertexCount; ++j)
        {
            auto  vertId  = indices[meshlets[i].VertexOffset + j];
            const auto& v = values[vertId];

            const uint32_t kGlobalQuantizedValue = uint32_t((v - globalBox.Min) * kQuantizationFactor + 0.5f);
            const uint32_t kLocalQuantizedValue  = kGlobalQuantizedValue - kQuantizationMeshletOffset;
            assert(kLocalQuantizedValue <= UINT16_MAX);

            uint16_t localVal = {};
            localVal = uint16_t(kLocalQuantizedValue);
            outValues.emplace_back(localVal);   // メッシュレット単位で量子化するため，新しく生成する.

        #if 0
            // 復元チェック.
            {
                float x = (localVal.x + kQuantizationMeshletOffset[0]) * kDequantizationFactor[0] + globalBox.Min;
                assert((v.x - x) <= 1e-3f);
            }
        #endif
        }

        outQuantizationMeshletOffsets[i] = kQuantizationMeshletOffset;
    }

    // 逆量子化用パラメータを格納.
    outInfo.Base   = globalBox.Min;
    outInfo.Factor = kDequantizationFactor;
}

//-----------------------------------------------------------------------------
//      量子化を行います.
//-----------------------------------------------------------------------------
void Quantization2
(
    const std::vector<asdx::Vector2>&   values,
    const std::vector<uint32_t>&        indices,
    const std::vector<MeshletInfo>&     meshlets,
    QuantizationInfo2&                  outInfo,
    std::vector<uint16_t2>&             outValues,
    std::vector<uint32_t2>&             outQuantizationMeshletOffsets
)
{
    struct BoundingBox
    {
        asdx::Vector2   Min;
        asdx::Vector2   Max;
    };

    // AABBを求める.
    BoundingBox globalBox;
    std::vector<BoundingBox> meshletBox(meshlets.size());

    globalBox.Max = values[0];
    globalBox.Min = values[0];

    for(size_t i=0; i<meshlets.size(); ++i)
    {
        auto& offsets = meshlets[i].VertexOffset;
        auto  vertId  = indices[offsets];
        auto  value   = values[vertId];
        BoundingBox box;
        box.Max = value;
        box.Min = value;

        globalBox.Max = asdx::Vector2::Max(globalBox.Max, value);
        globalBox.Min = asdx::Vector2::Min(globalBox.Min, value);

        for(size_t j=1; j<meshlets[i].VertexCount; ++j)
        {
            vertId = indices[offsets + j];
            value  = values[vertId];

            box.Max = asdx::Vector2::Max(box.Max, value);
            box.Min = asdx::Vector2::Min(box.Min, value);

            globalBox.Max = asdx::Vector2::Max(globalBox.Max, value);
            globalBox.Min = asdx::Vector2::Min(globalBox.Min, value);
        }

        meshletBox[i] = box;
    }

    asdx::Vector2 kGlobalDelta = globalBox.Max - globalBox.Min;
    if (kGlobalDelta.x <= 0.0f) { kGlobalDelta.x = 1e-6f; }
    if (kGlobalDelta.y <= 0.0f) { kGlobalDelta.y = 1e-6f; }

    asdx::Vector2 largestDelta(0.0f, 0.0f);

    const auto kTargetBits = 16;

    for(size_t i=0; i<meshletBox.size(); ++i)
    {
        auto delta = meshletBox[i].Max - meshletBox[i].Min;
        largestDelta = asdx::Vector2::Max(largestDelta, delta);
    }
    if (largestDelta.x <= 0.0f) { largestDelta.x = 1e-6f; }
    if (largestDelta.y <= 0.0f) { largestDelta.y = 1e-6f; }

    const float kMeshletsQuantizationStep[2] = {
        largestDelta.x / ((1u << kTargetBits) - 1),
        largestDelta.y / ((1u << kTargetBits) - 1),
    };
    const uint32_t kGlobalQuantizationStates[2] = {
        uint32_t(kGlobalDelta.x / kMeshletsQuantizationStep[0]),
        uint32_t(kGlobalDelta.y / kMeshletsQuantizationStep[1]),
    };
    const float kQuantizationFactor[2] = {
        (kGlobalQuantizationStates[0] - 1) / kGlobalDelta.x,
        (kGlobalQuantizationStates[1] - 1) / kGlobalDelta.y,
    };
    const float kDequantizationFactor[2] = {
        kGlobalDelta.x / (kGlobalQuantizationStates[0] - 1),
        kGlobalDelta.y / (kGlobalQuantizationStates[1] - 1),
    };

    outQuantizationMeshletOffsets.resize(meshlets.size());

    for(size_t i=0; i<meshlets.size(); ++i)
    {
        auto diffX = meshletBox[i].Min.x - globalBox.Min.x;
        auto diffY = meshletBox[i].Min.y - globalBox.Min.y;
        assert(diffX >= 0.0f);
        assert(diffY >= 0.0f);

        const uint32_t kQuantizationMeshletOffset[2] = {
            uint32_t(diffX * kQuantizationFactor[0] + 0.5f),
            uint32_t(diffY * kQuantizationFactor[1] + 0.5f),
        };

        for(auto j=0u; j<meshlets[i].VertexCount; ++j)
        {
            auto  vertId  = indices[meshlets[i].VertexOffset + j];
            const auto& v = values[vertId];

            const uint32_t kGlobalQuantizedValue[2] = {
                uint32_t((v.x - globalBox.Min.x) * kQuantizationFactor[0] + 0.5f),
                uint32_t((v.y - globalBox.Min.y) * kQuantizationFactor[1] + 0.5f),
            };

            const uint32_t kLocalQuantizedValue[2] = {
                kGlobalQuantizedValue[0] - kQuantizationMeshletOffset[0],
                kGlobalQuantizedValue[1] - kQuantizationMeshletOffset[1],
            };
            assert(kLocalQuantizedValue[0] <= UINT16_MAX);
            assert(kLocalQuantizedValue[1] <= UINT16_MAX);

            uint16_t2 localVal = {};
            localVal.x = uint16_t(kLocalQuantizedValue[0]);
            localVal.y = uint16_t(kLocalQuantizedValue[1]);
            outValues.emplace_back(localVal);   // メッシュレット単位で量子化するため，新しく生成する.

        #if 0
            // 復元チェック.
            {
                float x = (localVal.x + kQuantizationMeshletOffset[0]) * kDequantizationFactor[0] + globalBox.Min.x;
                float y = (localVal.y + kQuantizationMeshletOffset[1]) * kDequantizationFactor[1] + globalBox.Min.y;
                assert((v.x - x) <= 1e-3f);
                assert((v.y - y) <= 1e-3f);
            }
        #endif
        }

        outQuantizationMeshletOffsets[i].x = kQuantizationMeshletOffset[0];
        outQuantizationMeshletOffsets[i].y = kQuantizationMeshletOffset[1];
    }

    // 逆量子化用パラメータを格納.
    outInfo.Base     = globalBox.Min;
    outInfo.Factor.x = kDequantizationFactor[0];
    outInfo.Factor.y = kDequantizationFactor[1];
}


//-----------------------------------------------------------------------------
//      量子化を行います.
//-----------------------------------------------------------------------------
void Quantization3
(
    const std::vector<asdx::Vector3>&   values,
    const std::vector<uint32_t>&        indices,
    const std::vector<MeshletInfo>&     meshlets,
    QuantizationInfo3&                  outInfo,
    std::vector<uint16_t3>&             outValues,
    std::vector<uint32_t3>&             outQuantizationMeshletOffsets
)
{
    struct BoundingBox
    {
        asdx::Vector3   Min;
        asdx::Vector3   Max;
    };

    // AABBを求める.
    BoundingBox globalBox;
    std::vector<BoundingBox> meshletBox(meshlets.size());

    globalBox.Max = values[0];
    globalBox.Min = values[0];

    for(size_t i=0; i<meshlets.size(); ++i)
    {
        const auto& offsets = meshlets[i].VertexOffset;
        auto vertId  = indices[offsets];
        auto value   = values[vertId];

        BoundingBox box;
        box.Max = value;
        box.Min = value;

        globalBox.Max = asdx::Vector3::Max(globalBox.Max, value);
        globalBox.Min = asdx::Vector3::Min(globalBox.Min, value);

        for(size_t j=1; j<meshlets[i].VertexCount; ++j)
        {
            vertId = indices[offsets + j];
            value  = values[vertId];
            box.Max = asdx::Vector3::Max(box.Max, value);
            box.Min = asdx::Vector3::Min(box.Min, value);

            globalBox.Max = asdx::Vector3::Max(globalBox.Max, value);
            globalBox.Min = asdx::Vector3::Min(globalBox.Min, value);
        }

        meshletBox[i] = box;
    }

    const asdx::Vector3 kGlobalDelta = globalBox.Max - globalBox.Min;
    assert(kGlobalDelta.x > 0.0f);
    assert(kGlobalDelta.y > 0.0f);
    assert(kGlobalDelta.z > 0.0f);

    asdx::Vector3 largestDelta(0.0f, 0.0f, 0.0f);

    const auto kTargetBits = 16;

    for(size_t i=0; i<meshletBox.size(); ++i)
    {
        auto delta = meshletBox[i].Max - meshletBox[i].Min;
        largestDelta = asdx::Vector3::Max(largestDelta, delta);
    }
    assert(largestDelta.x > 0.0f);
    assert(largestDelta.y > 0.0f);
    assert(largestDelta.y > 0.0f);

    const float kMeshletsQuantizationStep[3] = {
        largestDelta.x / ((1u << kTargetBits) - 1),
        largestDelta.y / ((1u << kTargetBits) - 1),
        largestDelta.z / ((1u << kTargetBits) - 1)
    };
    const uint32_t kGlobalQuantizationStates[3] = {
        uint32_t(kGlobalDelta.x / kMeshletsQuantizationStep[0]),
        uint32_t(kGlobalDelta.y / kMeshletsQuantizationStep[1]),
        uint32_t(kGlobalDelta.z / kMeshletsQuantizationStep[2])
    };
    const float kQuantizationFactor[3] = {
        (kGlobalQuantizationStates[0] - 1) / kGlobalDelta.x,
        (kGlobalQuantizationStates[1] - 1) / kGlobalDelta.y,
        (kGlobalQuantizationStates[2] - 1) / kGlobalDelta.z
    };
    const float kDequantizationFactor[3] = {
        kGlobalDelta.x / (kGlobalQuantizationStates[0] - 1),
        kGlobalDelta.y / (kGlobalQuantizationStates[1] - 1),
        kGlobalDelta.z / (kGlobalQuantizationStates[2] - 1)
    };

    outQuantizationMeshletOffsets.resize(meshlets.size());

    for(size_t i=0; i<meshlets.size(); ++i)
    {
        auto diffX = meshletBox[i].Min.x - globalBox.Min.x;
        auto diffY = meshletBox[i].Min.y - globalBox.Min.y;
        auto diffZ = meshletBox[i].Min.z - globalBox.Min.z;
        assert(diffX >= 0.0f);
        assert(diffY >= 0.0f);
        assert(diffZ >= 0.0f);

        const uint32_t kQuantizationMeshletOffset[3] = {
            uint32_t(diffX * kQuantizationFactor[0] + 0.5f),
            uint32_t(diffY * kQuantizationFactor[1] + 0.5f),
            uint32_t(diffZ * kQuantizationFactor[2] + 0.5f),
        };

        for(auto j=0u; j<meshlets[i].VertexCount; ++j)
        {
            auto  vertId  = indices[meshlets[i].VertexOffset + j];
            const auto& v = values[vertId];

            const uint32_t kGlobalQuantizedValue[3] = {
                uint32_t((v.x - globalBox.Min.x) * kQuantizationFactor[0] + 0.5f),
                uint32_t((v.y - globalBox.Min.y) * kQuantizationFactor[1] + 0.5f),
                uint32_t((v.z - globalBox.Min.z) * kQuantizationFactor[2] + 0.5f)
            };

            const int32_t kLocalQuantizedValue[3] = {
                int32_t(kGlobalQuantizedValue[0] - kQuantizationMeshletOffset[0]),
                int32_t(kGlobalQuantizedValue[1] - kQuantizationMeshletOffset[1]),
                int32_t(kGlobalQuantizedValue[2] - kQuantizationMeshletOffset[2])
            };
            assert(kLocalQuantizedValue[0] >= 0);
            assert(kLocalQuantizedValue[1] >= 0);
            assert(kLocalQuantizedValue[2] >= 0);

            assert(kLocalQuantizedValue[0] <= UINT16_MAX);
            assert(kLocalQuantizedValue[1] <= UINT16_MAX);
            assert(kLocalQuantizedValue[2] <= UINT16_MAX);

            uint16_t3 localVal = {};
            localVal.x = uint16_t(kLocalQuantizedValue[0]);
            localVal.y = uint16_t(kLocalQuantizedValue[1]);
            localVal.z = uint16_t(kLocalQuantizedValue[2]);
            outValues.emplace_back(localVal);   // メッシュレット単位で量子化するため，新しく生成する.

        #if 0
            // 復元チェック.
            {
                float x = (localVal.x + kQuantizationMeshletOffset[0]) * kDequantizationFactor[0] + globalBox.Min.x;
                float y = (localVal.y + kQuantizationMeshletOffset[1]) * kDequantizationFactor[1] + globalBox.Min.y;
                float z = (localVal.z + kQuantizationMeshletOffset[2]) * kDequantizationFactor[2] + globalBox.Min.z;
                assert((v.x - x) <= 1e-3f);
                assert((v.y - y) <= 1e-3f);
                assert((v.z - z) <= 1e-3f);
            }
        #endif
        }

        outQuantizationMeshletOffsets[i].x = kQuantizationMeshletOffset[0];
        outQuantizationMeshletOffsets[i].y = kQuantizationMeshletOffset[1];
        outQuantizationMeshletOffsets[i].z = kQuantizationMeshletOffset[2];
    }

    // 逆量子化用パラメータを格納.
    outInfo.Base     = globalBox.Min;
    outInfo.Factor.x = kDequantizationFactor[0];
    outInfo.Factor.y = kDequantizationFactor[1];
    outInfo.Factor.z = kDequantizationFactor[2];
}

//-----------------------------------------------------------------------------
//      量子化を行います.
//-----------------------------------------------------------------------------
void Quantization4
(
    const std::vector<asdx::Vector4>&   values,
    const std::vector<uint32_t>&        indices,
    const std::vector<MeshletInfo>&     meshlets,
    QuantizationInfo4&                  outInfo,
    std::vector<uint16_t4>&             outValues,
    std::vector<uint32_t4>&             outQuantizationMeshletOffsets
)
{
    struct BoundingBox
    {
        asdx::Vector4   Min;
        asdx::Vector4   Max;
    };

    // AABBを求める.
    BoundingBox globalBox;
    std::vector<BoundingBox> meshletBox(meshlets.size());

    globalBox.Max = values[0];
    globalBox.Min = values[0];

    for(size_t i=0; i<meshlets.size(); ++i)
    {
        auto& offsets = meshlets[i].VertexOffset;
        auto  vertId  = indices[offsets];
        BoundingBox box;
        box.Max = values[vertId];
        box.Min = values[vertId];

        globalBox.Max = asdx::Vector4::Max(globalBox.Max, values[vertId]);
        globalBox.Min = asdx::Vector4::Min(globalBox.Min, values[vertId]);

        for(size_t j=1; j<meshlets[i].VertexCount; ++j)
        {
            vertId = indices[offsets + j];
            box.Max = asdx::Vector4::Max(box.Max, values[vertId]);
            box.Min = asdx::Vector4::Min(box.Min, values[vertId]);

            globalBox.Max = asdx::Vector4::Max(globalBox.Max, values[vertId]);
            globalBox.Min = asdx::Vector4::Min(globalBox.Min, values[vertId]);
        }

        meshletBox[i] = box;
    }

    const asdx::Vector4 kGlobalDelta = globalBox.Max - globalBox.Min;
    assert(kGlobalDelta.x > 0.0f);
    assert(kGlobalDelta.y > 0.0f);
    assert(kGlobalDelta.z > 0.0f);
    assert(kGlobalDelta.w > 0.0f);

    asdx::Vector4 largestDelta(0.0f, 0.0f, 0.0f, 0.0f);

    const auto kTargetBits = 16;

    for(size_t i=0; i<meshletBox.size(); ++i)
    {
        auto delta = meshletBox[i].Max - meshletBox[i].Min;
        largestDelta = asdx::Vector4::Max(largestDelta, delta);
    }
    assert(largestDelta.x > 0.0f);
    assert(largestDelta.y > 0.0f);
    assert(largestDelta.z > 0.0f);
    assert(largestDelta.w > 0.0f);

    const float kMeshletsQuantizationStep[4] = {
        largestDelta.x / ((1u << kTargetBits) - 1),
        largestDelta.y / ((1u << kTargetBits) - 1),
        largestDelta.z / ((1u << kTargetBits) - 1),
        largestDelta.w / ((1u << kTargetBits) - 1),
    };
    const uint32_t kGlobalQuantizationStates[4] = {
        uint32_t(kGlobalDelta.x / kMeshletsQuantizationStep[0]),
        uint32_t(kGlobalDelta.y / kMeshletsQuantizationStep[1]),
        uint32_t(kGlobalDelta.z / kMeshletsQuantizationStep[2]),
        uint32_t(kGlobalDelta.w / kMeshletsQuantizationStep[3]),
    };
    const float kQuantizationFactor[4] = {
        (kGlobalQuantizationStates[0] - 1) / kGlobalDelta.x,
        (kGlobalQuantizationStates[1] - 1) / kGlobalDelta.y,
        (kGlobalQuantizationStates[2] - 1) / kGlobalDelta.z,
        (kGlobalQuantizationStates[3] - 1) / kGlobalDelta.w,
    };
    const float kDequantizationFactor[4] = {
        kGlobalDelta.x / (kGlobalQuantizationStates[0] - 1),
        kGlobalDelta.y / (kGlobalQuantizationStates[1] - 1),
        kGlobalDelta.z / (kGlobalQuantizationStates[2] - 1),
        kGlobalDelta.w / (kGlobalQuantizationStates[3] - 1),
    };

    outQuantizationMeshletOffsets.resize(meshlets.size());

    for(size_t i=0; i<meshlets.size(); ++i)
    {
        auto diffX = meshletBox[i].Min.x - globalBox.Min.x;
        auto diffY = meshletBox[i].Min.y - globalBox.Min.y;
        auto diffZ = meshletBox[i].Min.z - globalBox.Min.z;
        auto diffW = meshletBox[i].Min.w - globalBox.Min.w;
        assert(diffX >= 0.0f);
        assert(diffY >= 0.0f);
        assert(diffZ >= 0.0f);
        assert(diffW >= 0.0f);

        const uint32_t kQuantizationMeshletOffset[4] = {
            uint32_t(diffX * kQuantizationFactor[0] + 0.5f),
            uint32_t(diffY * kQuantizationFactor[1] + 0.5f),
            uint32_t(diffZ * kQuantizationFactor[2] + 0.5f),
            uint32_t(diffW * kQuantizationFactor[3] + 0.5f),
        };

        for(auto j=0u; j<meshlets[i].VertexCount; ++j)
        {
            auto  vertId  = indices[meshlets[i].VertexOffset + j];
            const auto& v = values[vertId];

            const uint32_t kGlobalQuantizedValue[4] = {
                uint32_t((v.x - globalBox.Min.x) * kQuantizationFactor[0] + 0.5f),
                uint32_t((v.y - globalBox.Min.y) * kQuantizationFactor[1] + 0.5f),
                uint32_t((v.z - globalBox.Min.z) * kQuantizationFactor[2] + 0.5f),
                uint32_t((v.w - globalBox.Min.w) * kQuantizationFactor[3] + 0.5f),
            };

            const uint32_t kLocalQuantizedValue[4] = {
                kGlobalQuantizedValue[0] - kQuantizationMeshletOffset[0],
                kGlobalQuantizedValue[1] - kQuantizationMeshletOffset[1],
                kGlobalQuantizedValue[2] - kQuantizationMeshletOffset[2],
                kGlobalQuantizedValue[3] - kQuantizationMeshletOffset[3],
            };
            assert(kLocalQuantizedValue[0] <= UINT16_MAX);
            assert(kLocalQuantizedValue[1] <= UINT16_MAX);
            assert(kLocalQuantizedValue[2] <= UINT16_MAX);
            assert(kLocalQuantizedValue[3] <= UINT16_MAX);

            uint16_t4 localVal = {};
            localVal.x = uint16_t(kLocalQuantizedValue[0]);
            localVal.y = uint16_t(kLocalQuantizedValue[1]);
            localVal.z = uint16_t(kLocalQuantizedValue[2]);
            localVal.w = uint16_t(kLocalQuantizedValue[3]);
            outValues.emplace_back(localVal);   // メッシュレット単位で量子化するため，新しく生成する.

        #if 0
            // 復元チェック.
            {
                float x = (localVal.x + kQuantizationMeshletOffset[0]) * kDequantizationFactor[0] + globalBox.Min.x;
                float y = (localVal.y + kQuantizationMeshletOffset[1]) * kDequantizationFactor[1] + globalBox.Min.y;
                float z = (localVal.z + kQuantizationMeshletOffset[2]) * kDequantizationFactor[2] + globalBox.Min.z;
                float w = (localVal.w + kQuantizationMeshletOffset[3]) * kDequantizationFactor[3] + globalBox.Min.w;
                assert((v.x - x) <= 1e-3f);
                assert((v.y - y) <= 1e-3f);
                assert((v.z - z) <= 1e-3f);
                assert((v.z - w) <= 1e-3f);
            }
        #endif
        }

        outQuantizationMeshletOffsets[i].x = kQuantizationMeshletOffset[0];
        outQuantizationMeshletOffsets[i].y = kQuantizationMeshletOffset[1];
        outQuantizationMeshletOffsets[i].z = kQuantizationMeshletOffset[2];
        outQuantizationMeshletOffsets[i].w = kQuantizationMeshletOffset[3];
    }

    // 逆量子化用パラメータを格納.
    outInfo.Base     = globalBox.Min;
    outInfo.Factor.x = kDequantizationFactor[0];
    outInfo.Factor.y = kDequantizationFactor[1];
    outInfo.Factor.z = kDequantizationFactor[2];
    outInfo.Factor.w = kDequantizationFactor[3];
}

//-----------------------------------------------------------------------------
//      八面体用の繰り返し処理.
//-----------------------------------------------------------------------------
asdx::Vector2 OctWrap(const asdx::Vector2& v)
{
    asdx::Vector2 result;
    result.x = (1.0f - abs(v.y)) * (v.x >= 0.0f ? 1.0f : -1.0f);
    result.y = (1.0f - abs(v.x)) * (v.y >= 0.0f ? 1.0f : -1.0f);
    return result;
}

//-----------------------------------------------------------------------------
//      八面体エンコーディングします.
//-----------------------------------------------------------------------------
asdx::Vector2 PackNormal(const asdx::Vector3& value)
{
    float mag = (abs(value.x) + abs(value.y) + abs(value.z));
    float invMag = (mag > 0) ? 1.0f / mag : 1.0f;
    asdx::Vector3 n = value * invMag;
    auto t = asdx::Vector2(n.x, n.y);
    t = (n.z >= 0.0f) ? t : OctWrap(t);
    return t * 0.5f + asdx::Vector2(0.5f, 0.5f);
}

//-----------------------------------------------------------------------------
//      八面体エンコーディングします.
//-----------------------------------------------------------------------------
asdx::Vector3 UnpackNormal(const asdx::Vector2& value)
{
    asdx::Vector2 encoded = value * 2.0f - asdx::Vector2(1.0f, 1.0f);
    asdx::Vector3 n = asdx::Vector3(encoded.x, encoded.y, 1.0f - abs(encoded.x) - abs(encoded.y));
    float t = asdx::Saturate(-n.z);
    n.x += (n.x >= 0.0f) ? -t : t;
    n.y += (n.y >= 0.0f) ? -t : t;
    return asdx::Vector3::Normalize(n);
}

float EncodeDiamond(const asdx::Vector2& v)
{
     float m = abs(v.x) + abs(v.y);
     if (m == 0.0f)
         return 0.0f;
     float x = v.x / m;
     float s = asdx::Sign(v.x);
     return -s * 0.25f * x + 0.5f + s * 0.25f;
}

asdx::Vector2 DecodeDiamond(float v)
{
     if (v == 0.0f)
         return asdx::Vector2(0.0f, 0.0f);
     asdx::Vector2 result;
     float s = asdx::Sign(v - 0.5f);
     result.x = -s * 4.0f * v + 1.0f + s * 2.0f;
     result.y =  s * (1.0f - abs(result.x));
     return asdx::Vector2::Normalize(result);
}

float EncodeTangent(const asdx::Vector3& normal, const asdx::Vector3& tangent)
{
     asdx::Vector3 t1;
     if (abs(normal.y) > abs(normal.z))
         t1 = asdx::Vector3(normal.y, -normal.x, 0.0f);
     else
         t1 = asdx::Vector3(normal.z, 0.0f, -normal.x);
     t1 = asdx::Vector3::Normalize(t1);
     auto t2 = asdx::Vector3::Cross(t1, normal);
     auto packedTangent = asdx::Vector2(asdx::Vector3::Dot(tangent, t1), asdx::Vector3::Dot(tangent, t2));
     return EncodeDiamond(packedTangent);
}

asdx::Vector3 DecodeTangent(const asdx::Vector3& normal, float diamondTangent)
{
     asdx::Vector3 t1;
     if (abs(normal.y) > abs(normal.z))
         t1 = asdx::Vector3(normal.y, -normal.x, 0.0f);
     else
         t1 = asdx::Vector3(normal.z, 0.0f, -normal.x);
     t1 = asdx::Vector3::Normalize(t1);
     auto t2 = asdx::Vector3::Cross(t1, normal);
     auto packedTangent = DecodeDiamond(diamondTangent);
     return packedTangent.x * t1 + packedTangent.y * t2;
}


} // namespace

//-----------------------------------------------------------------------------
//      圧縮メッシュレットを生成します.
//-----------------------------------------------------------------------------
bool CreateCompressedMeshlets(const ResMeshlets& input, ResCompressedMeshlets& output)
{
    // 位置座標の量子化.
    Quantization3(input.Positions, input.VertexIndices, input.Meshlets, output.PositionInfo, output.Positions, output.OffsetPosition);

    // 法線ベクトルのエンコーディング.
    {
        std::vector<asdx::Vector2> octahedronNormals(input.Normals.size());
        std::vector<float> encodeTangents(input.Tangents.size());
        for(size_t i=0; i<input.Normals.size(); ++i)
        {
            octahedronNormals[i] = PackNormal(input.Normals[i]);
        //#if 1
        //    auto decode = UnpackNormal(octahedronNormals[i]);
        //    assert(decode.x - input.Normals[i].x <= 1e-6f);
        //    assert(decode.y - input.Normals[i].y <= 1e-6f);
        //    assert(decode.z - input.Normals[i].z <= 1e-6f);
        //#endif
            if (!input.Tangents.empty())
                encodeTangents[i] = EncodeTangent(input.Normals[i], input.Tangents[i]);
        }

        Quantization2(octahedronNormals, input.VertexIndices, input.Meshlets, output.NormalInfo, output.Normals, output.OffsetNormal);

        if (!input.Tangents.empty())
            Quantization1(encodeTangents, input.VertexIndices, input.Meshlets, output.TangentInfo, output.Tangents, output.OffsetTangent);
    }

    // テクスチャ座標の量子化.
    if (!input.TexCoords.empty())
        Quantization2(input.TexCoords, input.VertexIndices, input.Meshlets, output.TexCoordInfo, output.TexCoords, output.OffsetTexCoord);

    output.VertexIndices  = input.VertexIndices;
    output.Primitives     = input.Primitives;
    output.Meshlets       = input.Meshlets;
    output.Subsets        = input.Subsets;
    output.BoundingSphere = input.BoundingSphere;

    return true;
}

//-----------------------------------------------------------------------------
//      圧縮メッシュレットを保存します.
//-----------------------------------------------------------------------------
bool SaveCompressedMeshlets(const char* path, const ResCompressedMeshlets& value)
{
    ResCompressedMeshletsHeader header = {};
    strcpy_s(header.Magic, "CMS");
    header.Version                  = kResCompressedMeshletsHeaderVersion;
    header.PositionCount            = value.Positions    .size();
    header.NormalCount              = value.Normals      .size();
    header.TangentCount             = value.Tangents     .size();
    header.TexCoordCount            = value.TexCoords    .size();
    header.PrimitiveCount           = value.Primitives   .size();
    header.VertexIndexCount         = value.VertexIndices.size();
    header.MeshletCount             = value.Meshlets     .size();
    header.SubsetCount              = value.Subsets      .size();
    header.BoundingSphere           = value.BoundingSphere;
    header.PositionInfo             = value.PositionInfo;
    header.NormalInfo               = value.NormalInfo;
    header.TangentInfo              = value.TangentInfo;
    header.TexCoordInfo             = value.TexCoordInfo;

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
    if (!value.Primitives   .empty()) { fwrite(value.Primitives   .data(), sizeof(value.Primitives   [0]), value.Primitives   .size(), fp); }
    if (!value.VertexIndices.empty()) { fwrite(value.VertexIndices.data(), sizeof(value.VertexIndices[0]), value.VertexIndices.size(), fp); }
    if (!value.Meshlets     .empty()) { fwrite(value.Meshlets     .data(), sizeof(value.Meshlets     [0]), value.Meshlets     .size(), fp); }
    if (!value.Subsets      .empty()) { fwrite(value.Subsets      .data(), sizeof(value.Subsets      [0]), value.Subsets      .size(), fp); }

    if (!value.OffsetPosition.empty()) { fwrite(value.OffsetPosition.data(), sizeof(value.OffsetPosition[0]), value.OffsetPosition.size(), fp); }
    if (!value.OffsetNormal  .empty()) { fwrite(value.OffsetNormal  .data(), sizeof(value.OffsetNormal  [0]), value.OffsetNormal  .size(), fp); }
    if (!value.OffsetTangent .empty()) { fwrite(value.OffsetTangent .data(), sizeof(value.OffsetTangent [0]), value.OffsetTangent .size(), fp); }
    if (!value.OffsetTexCoord.empty()) { fwrite(value.OffsetTexCoord.data(), sizeof(value.OffsetTexCoord[0]), value.OffsetTexCoord.size(), fp); }

    fclose(fp);

    return true;
}

//-----------------------------------------------------------------------------
//      圧縮メッシュレットを読み込みます.
//-----------------------------------------------------------------------------
bool LoadCompressedMeshlets(const char* path, ResCompressedMeshlets& result)
{
    FILE* fp = nullptr;
    auto err = fopen_s(&fp, path, "rb");
    if (err != 0)
    {
        ELOG("Error : File Open Failed. path = %s", path);
        return false;
    }

    ResCompressedMeshletsHeader header = {};
    fread(&header, sizeof(header), 1, fp);
    if (strcmp(header.Magic, "CMS") != 0)
    {
        fclose(fp);
        ELOG("Error : Invalid File.");
        return false;
    }

    if (header.Version != kResCompressedMeshletsHeaderVersion)
    {
        fclose(fp);
        ELOG("Error : Invalid Version. File Version = %u, Current Version = %u", header.Version, kResCompressedMeshletsHeaderVersion);
        return false;
    }

    result.Positions    .resize(header.PositionCount);
    result.Normals      .resize(header.NormalCount);
    result.Tangents     .resize(header.TangentCount);
    result.TexCoords    .resize(header.TexCoordCount);
    result.Primitives   .resize(header.PrimitiveCount);
    result.VertexIndices.resize(header.VertexIndexCount);
    result.Meshlets     .resize(header.MeshletCount);
    result.Subsets      .resize(header.SubsetCount);

    result.OffsetPosition.resize(header.MeshletCount);
    if (!result.Normals  .empty()) { result.OffsetNormal  .resize(header.MeshletCount); }
    if (!result.Tangents .empty()) { result.OffsetTangent .resize(header.MeshletCount); }
    if (!result.TexCoords.empty()) { result.OffsetTexCoord.resize(header.MeshletCount); }

    result.BoundingSphere   = header.BoundingSphere;
    result.PositionInfo     = header.PositionInfo;
    result.NormalInfo       = header.NormalInfo;
    result.TangentInfo      = header.TangentInfo;
    result.TexCoordInfo     = header.TexCoordInfo;

    if (!result.Positions    .empty()) { fread(result.Positions    .data(), sizeof(result.Positions    [0]), result.Positions    .size(), fp); }
    if (!result.Normals      .empty()) { fread(result.Normals      .data(), sizeof(result.Normals      [0]), result.Normals      .size(), fp); }
    if (!result.Tangents     .empty()) { fread(result.Tangents     .data(), sizeof(result.Tangents     [0]), result.Tangents     .size(), fp); }
    if (!result.TexCoords    .empty()) { fread(result.TexCoords    .data(), sizeof(result.TexCoords    [0]), result.TexCoords    .size(), fp); }
    if (!result.Primitives   .empty()) { fread(result.Primitives   .data(), sizeof(result.Primitives   [0]), result.Primitives   .size(), fp); }
    if (!result.VertexIndices.empty()) { fread(result.VertexIndices.data(), sizeof(result.VertexIndices[0]), result.VertexIndices.size(), fp); }
    if (!result.Meshlets     .empty()) { fread(result.Meshlets     .data(), sizeof(result.Meshlets     [0]), result.Meshlets     .size(), fp); }
    if (!result.Subsets      .empty()) { fread(result.Subsets      .data(), sizeof(result.Subsets      [0]), result.Subsets      .size(), fp); }

    if (!result.OffsetPosition.empty()) { fread(result.OffsetPosition.data(), sizeof(result.OffsetPosition[0]), result.OffsetPosition.size(), fp); }
    if (!result.OffsetNormal  .empty()) { fread(result.OffsetNormal  .data(), sizeof(result.OffsetNormal  [0]), result.OffsetNormal  .size(), fp); }
    if (!result.OffsetTangent .empty()) { fread(result.OffsetTangent .data(), sizeof(result.OffsetTangent [0]), result.OffsetTangent .size(), fp); }
    if (!result.OffsetTexCoord.empty()) { fread(result.OffsetTexCoord.data(), sizeof(result.OffsetTexCoord[0]), result.OffsetTexCoord.size(), fp); }

    fclose(fp);

    return true;
}