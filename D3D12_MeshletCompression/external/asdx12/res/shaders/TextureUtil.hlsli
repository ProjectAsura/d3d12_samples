//-----------------------------------------------------------------------------
// File : TextureUtil.hlsli
// Desc : Math Utility.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#ifndef ASDX_TEXTURE_UTIL_HLSLI
#define ASDX_TEXTURE_UTIL_HLSLI


///////////////////////////////////////////////////////////////////////////////
// BilinearData structure
///////////////////////////////////////////////////////////////////////////////
struct BilinearData
{
    float2 origin;
    float2 weight;
};

//-----------------------------------------------------------------------------
//      単なる視差マッピング.
//-----------------------------------------------------------------------------
float2 ParallaxMapping
(
    Texture2D       heightMap,      // 高さマップ.
    SamplerState    heightSmp,      // サンプラー.
    float2          texcoord,       // テクスチャ座標.
    float3          V,              // 視線ベクトル.
    float           heightScale     // 高さスケール.
)
{
    float  h = heightMap.Sample(heightSmp, texcoord).r;
    float2 p = (V.xy / V.z) * (h * heightScale);
    return texcoord - p;
}

//-----------------------------------------------------------------------------
//      視差遮断マッピング.
//-----------------------------------------------------------------------------
float2 ParallxOcclusionMapping
(
    Texture2D           heightMap,      // 高さマップ.
    SamplerState        heightSmp,      // 高さマップ用サンプラー.
    Texture2D<float>    depthMap,       // 深度マップ.
    SamplerState        depthSmp,       // 深度マップ用サンプラー.
    float2              texcoord,       // テクスチャ座標.
    float3              V,              // 視線ベクトル.
    float               heightScale,    // 高さスケール.
    const float         layerCount      // レイヤー数.
)
{
    float layerDepth     = 1.0f / layerCount; 
    float currLayerDepth = 0.0f;

    float2 p = V.xy * heightScale;
    float2 delta = p / layerCount;

    float2 currUV = texcoord;
    float currDepth = depthMap.Sample(depthSmp, currUV);

    [loop]
    while(currLayerDepth < currDepth)
    {
        currUV -= delta;
        currDepth = depthMap.Sample(depthSmp, currUV);
        currLayerDepth += layerDepth;
    }

    float2 prevUV = currUV + delta;
    float afterDepth = currDepth - currLayerDepth;
    float beforeDepth = depthMap.Sample(depthSmp, prevUV) - currLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);
    return lerp(currUV, prevUV, weight);
}

//-----------------------------------------------------------------------------
//      MinMipマップをサンプリングします.
//-----------------------------------------------------------------------------
float SampleMinLodFilter2D
(
    Texture2D       sampledTex,
    uint2           sampledTexDim,
    Texture2D       minLodTex,
    uint2           minLodTexDim,
    float2          uv,
    SamplerState    smp,
    float4          filterSlope
)
{
    uint2 minLodAspect = sampledTexDim / minLodTexDim;

    int slopeU = filterSlope.x;
    int slopeV = filterSlope.y;
    if (minLodAspect.x > minLodAspect.y)
    { slopeU *= (minLodAspect.x / minLodAspect.y); }
    else
    { slopeV *= (minLodAspect.y / minLodAspect.x); }

    float2 absoluteCoords = float2(minLodTexDim) * uv;
    float2 clampedCoords = round(absoluteCoords) / float2(minLodTexDim);
    float4 rawSamples = minLodTex.Gather(smp, clampedCoords).zwyx;

    float fracU = frac(absoluteCoords.x);
    float fracV = 1.0f - frac(absoluteCoords.y);

    float offsetU = filterSlope.z;
    float offsetV = filterSlope.w;

    int nearestTexel = ((fracU < 0.5f) ? 0 : 1) | ((fracV < 0.5f) ? 0 : 2);
    float gradientU = (fracU < 0.5f) ? fracU - offsetU : 1.0f - fracU - offsetU;
    float gradientV = (fracV < 0.5f) ? fracV - offsetV : 1.0f - fracV - offsetV;
    float weightU = saturate(gradientU * slopeU);
    float weightV = saturate(gradientV * slopeV);

    float4 newSamples;
    newSamples.x = rawSamples[nearestTexel];
    newSamples.y = max(rawSamples[nearestTexel & 2], rawSamples[(nearestTexel & 2) | 1]);
    newSamples.z = max(rawSamples[nearestTexel & 1], rawSamples[(nearestTexel & 1) | 2]);
    newSamples.w = max(rawSamples.x, max(rawSamples.y, max(rawSamples.z, rawSamples.w)));

    float revWeightU = 1.0f - weightU;
    float revWeightV = 1.0f - weightV;

    return newSamples.x *    weightU *    weightV
         + newSamples.y * revWeightU *    weightV
         + newSamples.z *    weightU * revWeightV
         + newSamples.w * revWeightU * revWeightV;
}

//-----------------------------------------------------------------------------
//      最小深度を保ちながら深度をダウンサンプルします.
//-----------------------------------------------------------------------------
float NearstDepthDownSample1(Texture2D<float> depthBuffer, SamplerState depthSampler, float2 uv)
{
    float4 depth = depthBuffer.GatherRed(depthSampler, uv);
    return min(depth.x, min(depth.y, min(depth.z, depth.w)));
}

//-----------------------------------------------------------------------------
//      最小深度を保ちながら深度をダウンサンプルします.
//-----------------------------------------------------------------------------
float NearstDepthDownSample4(Texture2D<float> depthBuffer, SamplerState depthSampler, float2 uv)
{
    float4 depth0 = depthBuffer.GatherRed(depthSampler, uv, int2(0, 0));
    float4 depth1 = depthBuffer.GatherRed(depthSampler, uv, int2(0, 2));
    float4 depth2 = depthBuffer.GatherRed(depthSampler, uv, int2(2, 0));
    float4 depth3 = depthBuffer.GatherRed(depthSampler, uv, int2(2, 2));

    float4 minDepth = min(depth0, min(depth1, min(depth2, depth3)));
    return min(minDepth.x, min(minDepth.y, min(minDepth.z, minDepth.w)));
}

//-----------------------------------------------------------------------------
//      最大深度を保ちながら深度をダウンサンプルします.
//-----------------------------------------------------------------------------
float FarestDepthDownSample1(Texture2D<float> depthBuffer, SamplerState depthSampler, float2 uv)
{
    float4 depth = depthBuffer.GatherRed(depthSampler, uv);
    return max(depth.x, max(depth.y, max(depth.z, depth.w)));
}

//-----------------------------------------------------------------------------
//      最大深度を保ちながら深度をダウンサンプルします.
//-----------------------------------------------------------------------------
float FarestDepthDownSample4(Texture2D<float> depthBuffer, SamplerState depthSampler, float2 uv)
{
    float4 depth0 = depthBuffer.GatherRed(depthSampler, uv, int2(0, 0));
    float4 depth1 = depthBuffer.GatherRed(depthSampler, uv, int2(0, 2));
    float4 depth2 = depthBuffer.GatherRed(depthSampler, uv, int2(2, 0));
    float4 depth3 = depthBuffer.GatherRed(depthSampler, uv, int2(2, 2));

    float4 maxDepth = max(depth0, max(depth1, max(depth2, depth3)));
    return max(maxDepth.x, max(maxDepth.y, max(maxDepth.z, maxDepth.w)));
}

//-----------------------------------------------------------------------------
//      モーションリプロジェクションを行います.
//-----------------------------------------------------------------------------
float2 MotionReprojection(float2 currUV, float2 motionVector)
{ return currUV + motionVector; }

//-----------------------------------------------------------------------------
//      ヒットポイントリプロジェクションを行います.
//-----------------------------------------------------------------------------
float2 HitPointReprojection
(
    float3   origVS,                // 反射レイの起点.
    float    reflectedRayLength,    // 衝突位置までの反射レイの長さ.
    float4x4 invView,               // ビュー行列の逆行列.
    float4x4 prevViewProj           // 前フレームのビュー射影行列.
)
{
    float3 posVS = origVS;
    float  primaryRayLength = length(posVS);
    posVS = normalize(posVS);   // 視線ベクトル方向.

    // Virtual Pointを求める.
    float rayLength = primaryRayLength + reflectedRayLength;
    posVS *= rayLength;
    
    // 前フレームでのUV座標を求める.
    float3 currPosWS = mul(invView, float4(posVS, 1.0f)).xyz;
    float4 prevPosCS = mul(prevViewProj, float4(currPosWS, 1.0f));
    float2 prevUV = (prevPosCS.xy / prevPosCS.w) * float2(0.5f, -0.5f) + 0.5f.xx;
    return prevUV;
}

//-----------------------------------------------------------------------------
//      リプロジェクションが有効かどうかチェックします.
//-----------------------------------------------------------------------------
bool IsValidReprojection
(
    int2        pixelPos,           // ピクセル位置 [0, 0] - [ScreenWidth, ScreenHeight]
    float3      prevPosWS,          // 前フレームのワールド空間位置.
    float3      prevNormal,         // 前フレームのワールド空間法線.
    float3      currPosWS,          // 現フレームのワールド空間位置.
    float3      currNormal,         // 現フレームのワールド空間法線.
    float       currLinearZ,        // 現フレームの線形深度.
    const int2  screenSize,         // スクリーンサイズ.
    const float distanceThreshold   // 平面距離の閾値.
)
{
    // 画面ないかどうか判定.
    if (any(pixelPos < int2(0, 0)) || any(pixelPos >= screenSize))
    { return false; }

    // 裏面ならヒストリーを棄却.
    if (dot(currNormal, prevNormal) < 0.0f)
    { return false; }

    // 平面距離が許容範囲かチェック.
    float3 posDiff      = currPosWS - prevPosWS;
    float  planeDist1   = abs(dot(posDiff, prevNormal));
    float  planeDist2   = abs(dot(posDiff, currNormal));
    float  maxPlaneDist = max(planeDist1, planeDist2);
    return (maxPlaneDist / currLinearZ) > distanceThreshold;
}

//-----------------------------------------------------------------------------
//      バイリニア補間データを取得します.
//-----------------------------------------------------------------------------
BilinearData GetBilinearFilter(float2 uv, float2 screenSize)
{
    BilinearData result;
    result.origin = floor(uv * screenSize - 0.5f);
    result.weight = frac (uv * screenSize - 0.5f);
    return result;
}

//-----------------------------------------------------------------------------
//      カスタムウェイトを考慮したバイリニア補間のウェイトを取得します.
//-----------------------------------------------------------------------------
float4 GetBlinearCustomWeigths(BilinearData f, float4 customWeights)
{
    float4 weights;
    weights.x = (1.0f - f.weight.x) * (1.0f - f.weight.y);
    weights.y = f.weight.x * (1.0f - f.weight.y);
    weights.z = (1.0f - f.weight.x) * f.weight.y;
    weights.w = f.weight.x * f.weight.y;
    return weights * customWeights;
}

//-----------------------------------------------------------------------------
//      カスタムウェイトによるバイリニア補間を適用します.
//-----------------------------------------------------------------------------
float ApplyBilienarCustomWeigths(float s00, float s10, float s01, float s11, float4 w, bool normalize = true)
{
    float r = s00 * w.x + s10 * w.y + s01 * w.z + s11 * w.w;
    return r * (normalize ? rcp(dot(w, 1.0f)) : 1.0f);
}

//-----------------------------------------------------------------------------
//      カスタムウェイトによるバイリニア補間を適用します.
//-----------------------------------------------------------------------------
float2 ApplyBilienarCustomWeigths(float2 s00, float2 s10, float2 s01, float2 s11, float4 w, bool normalize = true)
{
    float2 r = s00 * w.x + s10 * w.y + s01 * w.z + s11 * w.w;
    return r * (normalize ? rcp(dot(w, 1.0f)) : 1.0f);
}

//-----------------------------------------------------------------------------
//      カスタムウェイトによるバイリニア補間を適用します.
//-----------------------------------------------------------------------------
float3 ApplyBilienarCustomWeigths(float3 s00, float3 s10, float3 s01, float3 s11, float4 w, bool normalize = true)
{
    float3 r = s00 * w.x + s10 * w.y + s01 * w.z + s11 * w.w;
    return r * (normalize ? rcp(dot(w, 1.0f)) : 1.0f);
}

//-----------------------------------------------------------------------------
//      カスタムウェイトによるバイリニア補間を適用します.
//-----------------------------------------------------------------------------
float4 ApplyBilienarCustomWeigths(float4 s00, float4 s10, float4 s01, float4 s11, float4 w, bool normalize = true)
{
    float4 r = s00 * w.x + s10 * w.y + s01 * w.z + s11 * w.w;
    return r * (normalize ? rcp(dot(w, 1.0f)) : 1.0f);
}

//-----------------------------------------------------------------------------
//      Catmull-Rom フィルタリング.
//-----------------------------------------------------------------------------
float4 BicubicSampleCatmullRom(Texture2D map, SamplerState smp, float2 uv, float2 mapSize)
{
    const float2 kInvMapSize = rcp(mapSize);
    float2 samplePos = uv * mapSize;
    float2 tc = floor(samplePos - 0.5f) + 0.5f;
    float2 f  = samplePos - tc;
    float2 f2 = f * f;
    float2 f3 = f * f2;

    float2 w0 = f2 - 0.5f * (f3 + f);
    float2 w1 = 1.5f * f3 - 2.5f * f2 + 1;
    float2 w3 = 0.5f * (f3 - f2);
    float2 w2 = 1 - w0 - w1 - w3;

    float2 w12 = w1 + w2;

    float2 tc0  = (tc - 1) * kInvMapSize;
    float2 tc12 = (tc + w2 / w12) * kInvMapSize;
    float2 tc3  = (tc + 2) * kInvMapSize;

    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);

    result += map.SampleLevel(smp, float2(tc0.x,  tc0.y),  0) * (w0.x  * w0.y);
    result += map.SampleLevel(smp, float2(tc0.x,  tc12.y), 0) * (w0.x  * w12.y);
    result += map.SampleLevel(smp, float2(tc0.x,  tc3.y),  0) * (w0.x  * w3.y);

    result += map.SampleLevel(smp, float2(tc12.x, tc0.y),  0) * (w12.x * w0.y);
    result += map.SampleLevel(smp, float2(tc12.x, tc12.y), 0) * (w12.x * w12.y);
    result += map.SampleLevel(smp, float2(tc12.x, tc3.y),  0) * (w12.x * w3.y);

    result += map.SampleLevel(smp, float2(tc3.x,  tc0.y),  0) * (w3.x * w0.y);
    result += map.SampleLevel(smp, float2(tc3.x,  tc12.y), 0) * (w3.x * w12.y);
    result += map.SampleLevel(smp, float2(tc3.x,  tc3.y),  0) * (w3.x * w3.y);

    return max(result, 0.0f.xxxx);
}

#endif//ASDX_TEXTURE_UTIL_HLSLI
