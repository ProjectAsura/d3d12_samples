//-----------------------------------------------------------------------------
// File : CompressedMeshletAS.hlsl
// Desc : Meshlet Culling Amplification Shader.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// MeshletInfoEx structure
///////////////////////////////////////////////////////////////////////////////
struct MeshletInfoEx
{
    uint        VertexOffset;
    uint        PrimitiveOffset;
    uint        VertexCount;
    uint        PrimitiveCount;
    uint        NormalCone;
    float4      BoundingSphere;
    uint3       OffsetPosition;
    uint2       OffsetNormal;
    uint        OffsetTangent;
    uint2       OffsetTexCoord;
};

///////////////////////////////////////////////////////////////////////////////
// Constants structure
///////////////////////////////////////////////////////////////////////////////
struct Constants
{
    uint    MeshletCount;
    uint    InstanceId;
    float   MinContribution;
    uint    Flags;
};


///////////////////////////////////////////////////////////////////////////////
// TransParam structure
///////////////////////////////////////////////////////////////////////////////
struct TransParam
{
    float4x4 View;
    float4x4 Proj;
    float4x4 ViewProj;
    float3   CameraPos;
    float    Padding;
    float4   Planes[6];

    float4x4 DebugView;
    float4x4 DebugProj;
    float4x4 DebugViewProj;
    float3   DebugCameraPos;
    float    Padding2;
    float4   DebugPlanes[6];
};

struct QuantizationInfo1
{
    float1 Base;
    float1 Factor;
};

struct QuantizationInfo2
{
    float2 Base;
    float2 Factor;
};

struct QuantizationInfo3
{
    float3 Base;
    float3 Factor;
};


///////////////////////////////////////////////////////////////////////////////
// MeshInstanceParam structure
///////////////////////////////////////////////////////////////////////////////
struct MeshInstanceParam
{
    float4x4            CurrWorld;
    float4x4            PrevWorld;
    QuantizationInfo3   PositionInfo;
    QuantizationInfo2   NormalInfo;
    QuantizationInfo1   TangentInfo;
    QuantizationInfo2   TexCoordInfo;
};

///////////////////////////////////////////////////////////////////////////////
// Payload structure
///////////////////////////////////////////////////////////////////////////////
struct Payload
{
    uint MeshletIndices[32];
};

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
groupshared Payload                 g_Payload;
ConstantBuffer<Constants>           g_Constants     : register(b0);
ConstantBuffer<TransParam>          g_TransParam    : register(b1);
StructuredBuffer<MeshletInfoEx>     g_Meshlets      : register(t6);
StructuredBuffer<MeshInstanceParam> g_MeshInstances : register(t7);


//-----------------------------------------------------------------------------
//      Unormをfloatに変換します.
//-----------------------------------------------------------------------------
float4 UnpackUnorm(uint value)
{
    float4 v;
    v.x = float((value >> 0) & 0xFF);
    v.y = float((value >> 8) & 0xFF);
    v.z = float((value >> 16) & 0xFF);
    v.w = float((value >> 24) & 0xFF);
    v = v / 255.0;
    return v;
}

//-----------------------------------------------------------------------------
//      バウンディングスフィアを変換します.
//-----------------------------------------------------------------------------
float4 TransformSphere(float4 sphere, float4x4 world)
{
    // 中心をワールド変換.
    float3 center = mul((float3x3)world, sphere.xyz);

    // 各軸の長さの2乗値を求める.
    float sx = dot(world._11_12_13, world._11_12_13);
    float sy = dot(world._21_22_23, world._21_22_23);
    float sz = dot(world._31_32_33, world._31_32_33);
 
    // 最も大きいものをスケールとして採用.
    float scale = sqrt(max(sx, max(sy, sz)));
    return float4(center, sphere.w * scale);
}

//-----------------------------------------------------------------------------
//      スクリーン上の矩形を求めます.
//-----------------------------------------------------------------------------
float4 SphereScreenExtents(float4 sphere, float4x4 viewProj)
{
    // https://gist.github.com/JarkkoPFC/1186bc8a861dae3c8339b0cda4e6cdb3
    float4 result;
    float r2 = sphere.w * sphere.w;
    float d  = sphere.z * sphere.w;

    float hv = sqrt(sphere.x * sphere.x + sphere.z * sphere.z - r2);
    float ha = sphere.x * hv;
    float hb = sphere.x * sphere.w;
    float hc = sphere.z * hv;
    result.x = (ha - d) * viewProj._11 / (hc + hb); // left
    result.z = (ha + d) * viewProj._11 / (hc - hb); // right

    float vv = sqrt(sphere.y * sphere.y + sphere.z * sphere.z - r2);
    float va = sphere.y * vv;
    float vb = sphere.y * sphere.w;
    float vc = sphere.z * vv;
    result.y = (va - d) * viewProj._22 / (vc + vb); // bottom
    result.w = (va + d) * viewProj._22 / (vc - vb); // top.

    return result;
}

//-----------------------------------------------------------------------------
//      視錐台の中にスフィアが含まれるかどうかチェックします.
//-----------------------------------------------------------------------------
bool Contains(float4 planes[6], float4 sphere)
{
    // sphereは事前に位置座標がワールド変換済み，半径もスケール適用済みとします.
    float4 center = float4(sphere.xyz, 1.0f);

    for(int i=0; i<6; ++i)
    {
        if (dot(center, planes[i]) < -sphere.w)
            return false; // カリングする.
    }

    // カリングしない.
    return true;
}

//-----------------------------------------------------------------------------
//      寄与カリングを行います.
//-----------------------------------------------------------------------------
bool ContributionCulling(float4 sphere, float4x4 viewProj)
{
    float4 LBRT = SphereScreenExtents(sphere, viewProj);

    float w = abs(LBRT.z - LBRT.x); // (left - right).
    float h = abs(LBRT.w - LBRT.y); // (top - bottom).
    
    return max(w, h) < g_Constants.MinContribution; // カリングする.
}

//-----------------------------------------------------------------------------
//      法錐カリングを行います.
//-----------------------------------------------------------------------------
bool NormalConeCulling(float4 normalCone, float3 viewDir)
{
    // normalConeはワールド変換済みとします.
    return dot(normalCone.xyz, -viewDir) > normalCone.w; // カリングする.
}

//-----------------------------------------------------------------------------
//      視認できるかどうかチェックします.
//-----------------------------------------------------------------------------
bool IsVisible(MeshletInfoEx info, float3 cameraPos, float4 planes[6], float4x4 world, float4x4 viewProj)
{
    // [-1, 1] に展開.
    float4 normalCone = UnpackUnorm(info.NormalCone) * 2.0f - 1.0f;

    // 角度がゼロかどうか判定.
    if (normalCone.w <= 1e-6f)
        return false;

    // ワールド空間に変換.
    float4 sphere = TransformSphere(info.BoundingSphere, world);
    float3 axis   = normalize(mul((float3x3)world, normalCone.xyz));
 
    // 視錐台カリング.
    if (!Contains(planes, sphere))
        return false;

    // 寄与カリング.
    if (ContributionCulling(sphere, viewProj))
        return false;

    // 視線ベクトルを求める.
    float3 viewDir = normalize(sphere.xyz - cameraPos);

    // 法錐カリング.
    if (NormalConeCulling(float4(axis, normalCone.w), viewDir))
        return false;

    // カリングしない.
    return true;
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
[numthreads(32, 1, 1)]
void main(uint dispatchId : SV_DispatchThreadID)
{
    bool visible = false;

    if (dispatchId < g_Constants.MeshletCount)
    {
        MeshletInfoEx info = g_Meshlets[dispatchId];
        MeshInstanceParam instance = g_MeshInstances[g_Constants.InstanceId];

        // 常にメインカメラを使用する.
        visible = IsVisible(info, g_TransParam.CameraPos, g_TransParam.Planes, instance.CurrWorld, g_TransParam.ViewProj);
    }

    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        g_Payload.MeshletIndices[index] = dispatchId;
    }

    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, g_Payload);
}