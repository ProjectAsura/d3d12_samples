//-----------------------------------------------------------------------------
// File : SimpleMS.hlsl
// Desc : Simple Mesh Shader.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// MSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct MSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
    float2 TexCoord : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////
// PrimOutput structure
///////////////////////////////////////////////////////////////////////////////
struct PrimOutput
{
    bool   Culling     : SV_CullPrimitive;
    uint   MeshletId   : MESHLET_ID;
    uint   PrimitiveId : PRIMITIVE_ID;
};

///////////////////////////////////////////////////////////////////////////////
// MeshletInfo structure
///////////////////////////////////////////////////////////////////////////////
struct MeshletInfoEx
{
    uint        VertexOffset;
    uint        VertexCount;
    uint        PrimitiveOffset;
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
    float4   RenderTargetSize;  // xy: size, zw: invSize.

    float4x4 DebugView;
    float4x4 DebugProj;
    float4x4 DebugViewProj;
    float3   DebugCameraPos;
    float    Padding2;
    float4   DebugPlanes[6];
};

///////////////////////////////////////////////////////////////////////////////
// Pyaload structure
///////////////////////////////////////////////////////////////////////////////
struct Payload
{
    uint MeshletIndices[32];
};

struct QuantizationInfo1
{
    float Base;
    float Factor;
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


//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
ConstantBuffer<Constants>           g_Constants     : register(b0);
ConstantBuffer<TransParam>          g_TransParam    : register(b1);
StructuredBuffer<uint16_t3>         g_Positions     : register(t0);
StructuredBuffer<uint16_t2>         g_Normals       : register(t1);
StructuredBuffer<uint16_t>          g_Tangents      : register(t2);
StructuredBuffer<uint16_t2>         g_TexCoords     : register(t3);
//StructuredBuffer<uint>              g_VertexIndices : register(t4);
ByteAddressBuffer                   g_Primitives    : register(t5);
StructuredBuffer<MeshletInfoEx>     g_Meshlets      : register(t6);
StructuredBuffer<MeshInstanceParam> g_MeshInstances : register(t7);
RWStructuredBuffer<uint3>           g_DebugWrite    : register(u0);

//-----------------------------------------------------------------------------
//      色相からRGB値を求めます.
//-----------------------------------------------------------------------------
float3 HueToRGB(float hue) 
{
    // https://www.ronja-tutorials.com/post/041-hsv-colorspace/
    hue = frac(hue); //only use fractional part of hue, making it loop
    float r = -1.0f + abs(hue * 6.0f - 3.0f); //red
    float g =  2.0f - abs(hue * 6.0f - 2.0f); //green
    float b =  2.0f - abs(hue * 6.0f - 4.0f); //blue
    return saturate(float3(r, g, b)); //clamp between 0 and 1
}

//-----------------------------------------------------------------------------
//      リニアからSRGBへの変換.
//-----------------------------------------------------------------------------
float3 ToSRGB(float3 color)
{
    float3 result;
    result.x  = (color.x < 0.0031308) ? 12.92 * color.x : 1.055 * pow(abs(color.x), 1.0f / 2.4) - 0.05f;
    result.y  = (color.y < 0.0031308) ? 12.92 * color.y : 1.055 * pow(abs(color.y), 1.0f / 2.4) - 0.05f;
    result.z  = (color.z < 0.0031308) ? 12.92 * color.z : 1.055 * pow(abs(color.z), 1.0f / 2.4) - 0.05f;

    return result;
}

//-----------------------------------------------------------------------------
//      背面カリングとゼロ面積カリング.
//-----------------------------------------------------------------------------
bool IsBackFaceOrZeroArea(float3 worldPos[3], float3 cameraPos)
{
    float3 viewDir = normalize(cameraPos - worldPos[0].xyz);

    float3 a = worldPos[1].xyz - worldPos[0].xyz;
    float3 b = worldPos[2].xyz - worldPos[0].xyz;
    float3 n = cross(a, b);
    return dot(n, viewDir) >= 0.0f; // カリングする.
}

//-----------------------------------------------------------------------------
//      視錐台カリングと微小プリミティブカリング.
//-----------------------------------------------------------------------------
bool PrimitiveCulling(float2 posSS[3])
{
    bool culled = false;
    
    float2 mini = 1.0f.xx;
    float2 maxi = 0.0f.xx;

    // 視錐台カリング.
    for (uint i = 0; i < 3; ++i)
    {
        mini = min(mini, posSS[i]);
        maxi = max(maxi, posSS[i]);
    }
    culled |= (any(mini > 1.0f) || any(maxi < 0.0f)); // カリングする.

    // 微小プリミティブカリング.
    maxi *= g_TransParam.RenderTargetSize.xy;
    mini *= g_TransParam.RenderTargetSize.xy;
    culled |= any(round(mini) == round(maxi));  // カリングする.

    return culled;
}

//-----------------------------------------------------------------------------
//      プリミティブインデックスを取得します.
//-----------------------------------------------------------------------------
uint3 GetPrimitiveIndex(uint triangleIndex)
{
    // 3バイト単位で三角形のインデックスが格納されている
    uint baseByteOffset = triangleIndex * 3;

    // 各インデックスのバイトオフセット
    uint3 byteOffset = uint3(baseByteOffset + 0, baseByteOffset + 1, baseByteOffset + 2);

    // 各インデックスが含まれる4バイト境界を計算
    uint3 alignedOffset = byteOffset & (~3u).xxx;

    // バイトをまたぐ場合を考慮して，8バイトロード.
    uint2 raw = g_Primitives.Load2(alignedOffset.x);

    // dataX/Y/Z を切り出す（同じ位置なら再利用）
    uint3 data;
    data.x = raw.x;
    data.y = (alignedOffset.y != alignedOffset.x) ? raw.y : data.x;
    data.z = (alignedOffset.z != alignedOffset.y) ? raw.y : data.y;

    // シフト量
    uint3 shift = (byteOffset & (3u).xxx) * 8;

    // 抽出（それぞれの正しいロード結果から抽出）
    return (data >> shift) & 0xFF.xxx;
}

//-----------------------------------------------------------------------------
//      法線ベクトルをアンパッキングします.
//-----------------------------------------------------------------------------
float3 UnpackNormal(float2 packed)
{
    // Octahedron normal vector encoding.
    // https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
    float2 encoded = packed * 2.0f - 1.0f;
    float3 n = float3(encoded.x, encoded.y, 1.0f - abs(encoded.x) - abs(encoded.y));
    float  t = saturate(-n.z);
#if __HLSL_VERSION >= 2021
    n.xy += select(n.xy >= 0.0f, -t, t);
#else
    n.xy += (n.xy >= 0.0f) ? -t : t;
#endif
    return normalize(n);
}

//-----------------------------------------------------------------------------
//      ダイアモンドエンコードをデコードします.
//-----------------------------------------------------------------------------
float2 DecodeDiamond(float v)
{
     if (v == 0.0f)
         return float2(0.0f, 0.0f);
     float2 result;
     float s = sign(v - 0.5f);
     result.x = -s * 4.0f * v + 1.0f + s * 2.0f;
     result.y =  s * (1.0f - abs(result.x));
     return normalize(result);
}

//-----------------------------------------------------------------------------
//      接線データをデコードします.
//-----------------------------------------------------------------------------
float3 DecodeTangent(float3 normal, float diamondTangent)
{
     float3 t1;
     if (abs(normal.y) > abs(normal.z))
         t1 = float3(normal.y, -normal.x, 0.0f);
     else
         t1 = float3(normal.z, 0.0f, -normal.x);
     t1 = normalize(t1);
     float3 t2 = cross(t1, normal);
     float2 packedTangent = DecodeDiamond(diamondTangent);
     return packedTangent.x * t1 + packedTangent.y * t2;
}

//-----------------------------------------------------------------------------
//      量子化された位置座標を取得します.
//-----------------------------------------------------------------------------
uint3 GetQuantizedPosition(uint vertexIndex)
{ return g_Positions[vertexIndex]; }

//-----------------------------------------------------------------------------
//      量子化された法線ベクトルを取得します.
//-----------------------------------------------------------------------------
uint2 GetQuantizedNormal(uint vertexIndex)
{ return g_Normals[vertexIndex]; }

//-----------------------------------------------------------------------------
//      量子化された接線ベクトルを取得します.
//-----------------------------------------------------------------------------
uint GetQunatizedTangent(uint vertexIndex)
{ return g_Tangents[vertexIndex]; }

//-----------------------------------------------------------------------------
//      量子化されたテクスチャ座標を取得します.
//-----------------------------------------------------------------------------
uint2 GetQuantizedTexCoord(uint vertexIndex)
{ return g_TexCoords[vertexIndex]; }

//-----------------------------------------------------------------------------
//      位置座標を逆量子化します.
//-----------------------------------------------------------------------------
float3 DequantizePosition(uint3 value, MeshletInfoEx info, MeshInstanceParam param)
{ return (value + info.OffsetPosition) * param.PositionInfo.Factor + param.PositionInfo.Base; }

//-----------------------------------------------------------------------------
//      法線ベクトルを逆量子化します.
//-----------------------------------------------------------------------------
float3 DequantizeNormal(uint2 value, MeshletInfoEx info, MeshInstanceParam param)
{
    float2 packedNormal = (value + info.OffsetNormal) * param.NormalInfo.Factor + param.NormalInfo.Base;
    return UnpackNormal(packedNormal);
}

//-----------------------------------------------------------------------------
//      接線ベクトルを逆量子化します.
//-----------------------------------------------------------------------------
float3 DequantizeTangent(uint value, float3 normal, MeshletInfoEx info, MeshInstanceParam param)
{
    float angle = (value + info.OffsetTangent) * param.TangentInfo.Factor + param.TangentInfo.Base;
    return DecodeTangent(normal, angle);
}

//-----------------------------------------------------------------------------
//      テクスチャ座標を逆量子化します.
//-----------------------------------------------------------------------------
float2 DequantizeTexCoord(uint2 value, MeshletInfoEx info, MeshInstanceParam param)
{ return (value + info.OffsetTexCoord) * param.TexCoordInfo.Factor + param.TexCoordInfo.Base; }


//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void main
(
    uint                      threadId : SV_GroupThreadID,
    uint                      groupId  : SV_GroupID,
    in  payload  Payload      payload,
    out vertices MSOutput     verts[256],
    out indices  uint3        tris [256],
    out primitives PrimOutput prims[256]
)
{
    // メッシュレット情報を取得.
    uint meshletIndex = payload.MeshletIndices[groupId];
    MeshletInfoEx info = g_Meshlets[meshletIndex];

    // 出力頂点数とインデックス数を設定.
    SetMeshOutputCounts(info.VertexCount, info.PrimitiveCount);

    // メッシュインスタンスパラメータを取得.
    MeshInstanceParam instance = g_MeshInstances[g_Constants.InstanceId];

    float4x4 view;
    float4x4 proj;

    if (!!(g_Constants.Flags & 0x1))
    {
        view = g_TransParam.DebugView;
        proj = g_TransParam.DebugProj;
    }
    else
    {
        view = g_TransParam.View;
        proj = g_TransParam.Proj;
    }

    // インデックス数以内の場合.
    for(uint i=0; i<2; ++i)
    {
        uint id = threadId + i * 128;

        if (id < info.PrimitiveCount)
        {
            // プリミティブインデックスを設定.
            uint3 tri = GetPrimitiveIndex(id + info.PrimitiveOffset);
            tris[id] = tri;

            // カリング用.
            float3 posW [3];
            float2 posSS[3];
            
            for (uint j = 0; j < 3; ++j)
            {
                uint idx = tri[j];

                // 頂点数を超える場合は処理しない.
                if (idx >= info.VertexCount)
                    continue;

                uint vertId = idx + info.VertexOffset;

                // 位置座標を計算します.
                uint3  quantizedPos = GetQuantizedPosition(vertId);
                float4 localPos     = float4(DequantizePosition(quantizedPos, info, instance), 1.0f); 
                float4 worldPos     = mul(instance.CurrWorld, localPos);
                float4 viewPos      = mul(view, worldPos);
                float4 projPos      = mul(proj, viewPos);
 
                // 法線ベクトルを計算します.
                uint2  quantizedNormal = GetQuantizedNormal(vertId);
                float3 localNormal     = DequantizeNormal(quantizedNormal, info, instance);
                float3 worldNormal     = normalize(mul((float3x3)instance.CurrWorld, localNormal));

                // 接線ベクトルを計算します.
                uint   quantizedTangent = GetQunatizedTangent(vertId);
                float3 localTangent     = DequantizeTangent(quantizedTangent, localNormal, info, instance);
                float3 worldTangent     = normalize(mul((float3x3)instance.CurrWorld, localTangent));

                // テクスチャ座標を計算します.
                uint2  quantizedTexCoord = GetQuantizedTexCoord(vertId);
                float2 texcoord          = DequantizeTexCoord(quantizedTexCoord, info, instance);

                // 出力データを設定.
                MSOutput output;
                output.Position = projPos;
                output.Normal   = worldNormal;
                output.Tangent  = worldTangent;
                output.TexCoord = texcoord;
                verts[idx]      = output;

                // カリング用データを保存.
                posW [j] = worldPos.xyz;
                posSS[j] = (projPos.xy / projPos.w) * 0.5f + 0.5f;
            }

            // カリング
            bool culled = false;
            culled |= IsBackFaceOrZeroArea(posW, g_TransParam.CameraPos);
            culled |= PrimitiveCulling(posSS);

            // プリミティブアトリビュートを出力.
            PrimOutput output  = (PrimOutput) 0;
            output.Culling     = culled;
            output.MeshletId   = meshletIndex;
            output.PrimitiveId = id + info.PrimitiveOffset;
            prims[id] = output;
        }
    }
}

