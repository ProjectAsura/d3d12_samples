//-----------------------------------------------------------------------------
// File : TaaCS.hlsl
// Desc : Temporal Anti-Alias.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "Math.hlsli"
#include "TextureUtil.hlsli"

#ifndef ENABLE_REVERSE_Z
#define ENABLE_REVERSE_Z    (0)
#endif//ENABLE_REVERSE_Z

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
static const float kVarianceIntersectionMaxT  = 100.0f;
static const float kFrameVelocityInPixelsDiff = 256.0f; // 1920 x 1080.


///////////////////////////////////////////////////////////////////////////////
// CbTemporalAA constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer CbTemporalAA : register(b0)
{
    float   Gamma;          // 1.0でうまく動くとのこと. [0.75, 1.25]で設定するのがGoodとのこと.
    float   BlendFactor;
    float2  MapSize;
    float2  InvMapSize;
    float2  Jitter;
    uint    Flags;
    uint3   Reserved;
};


//-----------------------------------------------------------------------------
// Textures and Samplers.
//-----------------------------------------------------------------------------
Texture2D           ColorMap        : register(t0);
Texture2D           HistoryMap      : register(t1);
Texture2D<float2>   VelocityMap     : register(t2);
Texture2D<float>    DepthMap        : register(t3);
RWTexture2D<float4> OutColorMap     : register(u0);
RWTexture2D<float4> OutHistoryMap   : register(u1);
SamplerState        PointClamp      : register(s1); // Samplers.hlsliと合わせている.
SamplerState        LinearClamp     : register(s4); // Samplers.hlsliと合わせている.

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
static const int2 kOffsets[8] = {
    int2(-1, -1),
    int2(-1,  1),
    int2( 1, -1),
    int2( 1,  1),
    int2( 1,  0),
    int2( 0, -1),
    int2( 0,  1),
    int2(-1,  0)
};


//-----------------------------------------------------------------------------
//      現在フレームのカラーを取得します.
//-----------------------------------------------------------------------------
float4 GetCurrentColor(float2 uv)
{ return ColorMap.SampleLevel(LinearClamp, uv, 0.0f); }

//-----------------------------------------------------------------------------
//      ヒストリーカラーを取得します.
//-----------------------------------------------------------------------------
float4 GetHistoryColor(float2 uv)
{ return BicubicSampleCatmullRom(HistoryMap, LinearClamp, uv, MapSize); }

//-----------------------------------------------------------------------------
//      速度ベクトルを取得します.
//-----------------------------------------------------------------------------
float2 GetVelocity(float2 uv)
{
    // https://www.gdcvault.com/play/1022970/Temporal-Reprojection-Anti-Aliasing-in

    float2 result = VelocityMap.SampleLevel(LinearClamp, uv, 0.0f);
    float  currLengthSq = dot(result, result);

    // 最も長い速度ベクトルを取得.
    [unroll] for(uint i=0; i<8; ++i)
    {
        float2 velocity = VelocityMap.SampleLevel(LinearClamp, uv, 0.0f, kOffsets[i]);
        float  lengthSq = dot(velocity, velocity);
        if (lengthSq > currLengthSq)
        {
            result       = velocity;
            currLengthSq = lengthSq;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
//      隣接ピクセルを考慮して深度を取得します.
//-----------------------------------------------------------------------------
float GetCurrentDepth(float2 uv)
{
    // 隣接4x4ピクセルをチェックし，最も手前のものを返却.
    float4 depths = DepthMap.GatherRed(PointClamp, uv);
    #if ENABLE_REVERSE_Z
        return max( max(depths.x, depths.y), max(depths.z, depths.w) );
    #else
        return min( min(depths.x, depths.y), min(depths.z, depths.w) );
    #endif
}

//-----------------------------------------------------------------------------
//      隣接ピクセルを考慮して深度を取得します.
//-----------------------------------------------------------------------------
float GetPreviousDepth(float2 uv)
{
    // CurrentDepthと逆演算.
    float4 depths = DepthMap.GatherRed(PointClamp, uv);
    #if ENABLE_REVERSE_Z
        return min( max(depths.x, depths.y), min(depths.z, depths.w) ) + 0.001f;
    #else
        return max( min(depths.x, depths.y), max(depths.z, depths.w) ) + 0.001f;
    #endif
}

//-----------------------------------------------------------------------------
//      ヒストリーカラーをクリップするためのバウンディングボックスを求めます.
//-----------------------------------------------------------------------------
void CalcColorBoundingBox
(
    float2      uv,
    float3      curColor,
    float       gamma,
    out float3  minColor,
    out float3  maxColor
)
{
    // 平均と分散を求める.
    float3 ave = curColor;
    float3 var = curColor * curColor;

    [unroll] for(uint i=0; i<8; ++i)
    {
        float3 newColor = RGBToYCoCg(ColorMap.SampleLevel(LinearClamp, uv, 0.0f, kOffsets[i])).rgb;
        ave += newColor;
        var += newColor * newColor;
    }

    const float invSamples = 1.0f / 9.0f;
    ave *= invSamples;
    var *= invSamples;
    var = sqrt(var - ave * ave) * gamma;

    // 分散クリッピング.
    minColor = float3(ave - var);
    maxColor = float3(ave + var);
}

//-----------------------------------------------------------------------------
//      最終カラーを求めます.
//-----------------------------------------------------------------------------
float4 GetFinalColor(float2 uv, float3 currentColor, float3 historyColor, float weight)
{
    float newWeight = saturate(0.5f - weight);
    return float4(lerp(currentColor, historyColor, saturate(BlendFactor)), newWeight);
}

//-----------------------------------------------------------------------------
//      隣接ピクセルを考慮した現在カラーを取得します.
//-----------------------------------------------------------------------------
float3 GetCurrentNeighborColor(float2 uv, float3 currentColor)
{
    const float centerWeight = 4.0f;
    float3 accColor = currentColor * centerWeight;
    [unroll] 
    for(uint i=0; i<4; ++i)
    {
        accColor += ColorMap.SampleLevel(LinearClamp, uv, 0.0f, kOffsets[i]).rgb;
    }
    const float invWeight = 1.0f / (4.0f + centerWeight);
    accColor *= invWeight;
    return accColor;
}

//-----------------------------------------------------------------------------
//      重みを求めます.
//-----------------------------------------------------------------------------
float CalcHdrWeightY(float3 ycocg, float exposure = 1.0f)
{ return rcp(ycocg.x * exposure + 4.0f); }

//-----------------------------------------------------------------------------
//      ブレンドウェイトを求めます.
//-----------------------------------------------------------------------------
float2 CalcBlendWeight(float historyWeight, float currentWeight, float blend)
{
    float blendH = (1.0f - blend) * historyWeight;
    float blendC = blend * currentWeight;
    return float2(blendH, blendC) * rcp(blendH + blendC);
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
[numthreads(8, 8, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    uint2 remappedId = RemapLane8x8(dispatchId.xy, groupIndex);

    if (any(remappedId >= (uint2)MapSize))
    { return; }

    const float2 currUV = ((float2)remappedId + 0.5f.xx) * InvMapSize;

    // 現在フレームのカラー.
    float4 currColor = GetCurrentColor(currUV + Jitter * InvMapSize);
    
    // 1920x1080をベースとしたスケール.
    const float sizeScale = MapSize.x / 1920.0f;

    // 速度ベクトルを取得.
    float2 velocity = GetVelocity(currUV);
    float  velocityDelta = saturate(1.0f - length(velocity) / (kFrameVelocityInPixelsDiff * sizeScale));

    // 前フレームのテクスチャ座標を計算.
    float2 prevUV = currUV + (velocity * InvMapSize);

    // 深度値を取得.
    float currDepth  = GetCurrentDepth(currUV);
    float prevDepth  = GetPreviousDepth(prevUV);
    float depthDelta = step(currDepth, prevDepth);

    // 画面内かどうか?
    float inScreen = (all(0.0f.xx <= prevUV) && all(prevUV < 1.0f.xx)) ? 1.0f : 0.0f;
    float resetHistory = (Flags & 0x1) ? 0.0f : 1.0f;

    // ヒストリーが有効かどうかチェックする.
    // 速度の差分，深度の差分，画面範囲であるか，いずれかすくなくとも１つがゼロなら無効と判断.
    bool isValidHistory = (velocityDelta * depthDelta * inScreen * resetHistory) > 0.0f;

    // ヒストリーバッファが無効な場合は隣接ピクセルを考慮して最終カラーを求める.
    if (!isValidHistory) {
        float3 neighborColor = GetCurrentNeighborColor(currUV, currColor.rgb);
        float4 finalColor    = SaturateFloat(float4(neighborColor, 0.5f));

        // 出力.
        OutColorMap  [remappedId] = float4(finalColor.rgb, 1.0f);
        OutHistoryMap[remappedId] = finalColor;
        return;
    }

    // ヒストリーカラーを取得.
    float4 prevColor = GetHistoryColor(prevUV);

    // YCoCgに変換.
    currColor.rgb = RGBToYCoCg(currColor).rgb;
    prevColor.rgb = RGBToYCoCg(prevColor).rgb;

    // クリップ済みヒストリーカラーを取得する.
    float3 minColor, maxColor;
    CalcColorBoundingBox(prevUV, currColor.rgb, Gamma, minColor, maxColor);

    // AABBでクリップ.
    float t = IntersectAABB(currColor.rgb - prevColor.rgb, prevColor.rgb, minColor, maxColor);
    prevColor.rgb = lerp (prevColor.rgb, currColor.rgb, saturate(t));
    prevColor.rgb = clamp(prevColor.rgb, minColor, maxColor);

    // 重みを求める.
    float  blend      = max(1.0f - BlendFactor, saturate(0.01f * prevColor.x / abs(currColor.x - prevColor.x)));
    float  currWeight = CalcHdrWeightY(currColor.rgb);
    float  prevWeight = CalcHdrWeightY(prevColor.rgb);
    float2 weights    = CalcBlendWeight(prevWeight, currWeight, saturate(blend));

    // 補間処理.
    float4 finalColor = prevColor * weights.x + currColor * weights.y;

    // RGBに戻す.
    finalColor.rgb = YCoCgToRGB(finalColor).rgb;

    // NaNを潰しておく.
    finalColor = SaturateFloat(finalColor);

    // 出力.
    OutColorMap  [remappedId] = float4(finalColor.rgb, 1.0f);
    OutHistoryMap[remappedId] = finalColor;
}