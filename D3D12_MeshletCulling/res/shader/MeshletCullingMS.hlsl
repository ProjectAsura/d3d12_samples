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
    float2 TexCoord : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////
// PrimOutput structure
///////////////////////////////////////////////////////////////////////////////
struct PrimOutput
{
    float4 Color   : COLOR0;
    bool   Culling : SV_CullPrimitive;
};

///////////////////////////////////////////////////////////////////////////////
// MeshletInfo structure
///////////////////////////////////////////////////////////////////////////////
struct MeshletInfo
{
    uint        VertexOffset;
    uint        VertexCount;
    uint        PrimitiveOffset;    
    uint        PrimitiveCount;
    uint        NormalCone;
    float4      BoundingSphere;
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

///////////////////////////////////////////////////////////////////////////////
// MeshInstanceParam structure
///////////////////////////////////////////////////////////////////////////////
struct MeshInstanceParam
{
    float4x4 CurrWorld;
    float4x4 PrevWorld;
};

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
ConstantBuffer<Constants>           g_Constants     : register(b0);
ConstantBuffer<TransParam>          g_TransParam    : register(b1);
StructuredBuffer<float3>            g_Positions     : register(t0);
StructuredBuffer<float3>            g_Normals       : register(t1);
StructuredBuffer<float2>            g_TexCoords     : register(t2);
ByteAddressBuffer                   g_Primitives    : register(t3);
StructuredBuffer<uint>              g_VertexIndices : register(t4);
StructuredBuffer<MeshletInfo>       g_Meshlets      : register(t5);
StructuredBuffer<MeshInstanceParam> g_MeshInstances : register(t6);

//-----------------------------------------------------------------------------
//      パッキングされたプリミティブ番号を展開します.
//-----------------------------------------------------------------------------
uint3 UnpackPrimitiveIndex(uint packed)
{
    return uint3(
        packed & 0x3FF,
        (packed >> 10) & 0x3FF,
        (packed >> 20) & 0x3FF);
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

    // 必要なデータを決定.
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
    float3 viewDir = normalize(cameraPos - worldPos[0]);

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
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void main
(
    uint                      threadId : SV_GroupThreadID,
    uint                      groupId  : SV_GroupID,
    in  payload  Payload      payload,
    out vertices MSOutput     vertices[256],
    out indices  uint3        indices [256],
    out primitives PrimOutput prims   [256]
)
{
    // メッシュレット情報を取得.
    uint meshletIndex = payload.MeshletIndices[groupId];
    MeshletInfo info = g_Meshlets[meshletIndex];

    // 出力頂点数とインデックス数を設定.
    SetMeshOutputCounts(info.VertexCount, info.PrimitiveCount);

    // メッシュインスタンスパラメータを取得.
    MeshInstanceParam instanceParam = g_MeshInstances[g_Constants.InstanceId];

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
        #if 1
            uint3 tris = GetPrimitiveIndex(id + info.PrimitiveOffset);
        #else
            uint3 tris = UnpackPrimitiveIndex(g_Primitives2[id + info.PrimitiveOffset]);
        #endif
            indices[id] = tris;

            // カリング用.
            float3 posW [3];
            float2 posSS[3];

            for (uint j = 0; j < 3; ++j)
            {
                uint idx = tris[j];
                // 頂点数を超える場合は処理しない.
                if (idx >= info.VertexCount)
                    continue;

                uint vertId = g_VertexIndices[idx + info.VertexOffset];

                float4 localPos = float4(g_Positions[vertId], 1.0f);
                float4 worldPos = mul(instanceParam.CurrWorld, localPos);
                float4 viewPos  = mul(view, worldPos);
                float4 projPos  = mul(proj, viewPos);
 
                float3 localNormal = g_Normals[vertId];
                float3 worldNormal = normalize(mul((float3x3) instanceParam.CurrWorld, localNormal));
            
                MSOutput output;
                output.Position = projPos;
                output.Normal   = worldNormal;
                output.TexCoord = g_TexCoords[vertId];

                vertices[idx] = output;

                posW [j] = worldPos.xyz;
                posSS[j] = (projPos.xy / projPos.w) * 0.5f + 0.5f;
            }

            // カリング
            bool culled = false;
            culled |= IsBackFaceOrZeroArea(posW, g_TransParam.CameraPos);
            culled |= PrimitiveCulling(posSS);

            // プリミティブアトリビュートを出力.
            PrimOutput output = (PrimOutput) 0;
            output.Color.rgb = ToSRGB(HueToRGB(meshletIndex * 1.71f));
            output.Color.a   = 1.0f;
            output.Culling   = culled;
            prims[id] = output;
        }
    }
}

