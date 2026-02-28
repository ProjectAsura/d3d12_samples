//-----------------------------------------------------------------------------
// File : CrossBilateralFilterCS.hlsl
// Desc : Cross-Bilateral Filter.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "Math.hlsli"

#define KERNEL_RADIUS (8)

///////////////////////////////////////////////////////////////////////////////
// CbBlur constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer CbBlur : register(b0)
{
    float2  Offset          : packoffset(c0);
    float2  InvTargetSize   : packoffset(c0.z);
    uint2   DispatchDim     : packoffset(c1);
    float   Sharpness       : packoffset(c1.z);
};

//-----------------------------------------------------------------------------
// Resources.
//-----------------------------------------------------------------------------
RWTexture2D<float4>     Output          : register(u0);
Texture2D               Input           : register(t0); // RGB : Color, A : Depth
SamplerState            PointSampler    : register(s0);
SamplerState            LinearSampler   : register(s1);

//-----------------------------------------------------------------------------
//      バイラテラルフィルタの重みを求めます.
//-----------------------------------------------------------------------------
float CrossBilateralWeight(float r, float d, float d0)
{
    // 参考. "Stable SSAO in Battlefield 3 with Scelective Temporal Filtering", GDC 2012,
    // https://www.ea.com/frostbite/news/stable-ssao-in-battlefield-3-with-selective-temporal-filtering

    // fxcで最適化される
    const float BlurSigma = ((float)KERNEL_RADIUS + 1.0f) * 0.5f;
    const float BlurFallOff = 1.0 / (2.0f * BlurSigma * BlurSigma);

    // dとd0は線形深度値とする.
    float dz = (d0 - d) * Sharpness;
    return exp2(-r * r * BlurFallOff - dz * dz);
}

//-----------------------------------------------------------------------------
//      バイラテラルフィルタ実行します.
//-----------------------------------------------------------------------------
float4 CrossBilateralFilter(float2 texcoord, float2 offset)
{
    float4 output = (float4)0;

    float4 center_color = Input.SampleLevel(PointSampler, texcoord, 0);
    float3 total_color  = center_color.rgb;
    float  total_weight = 1.0f;

    float r = 1;
    [unroll] for(; r<=KERNEL_RADIUS/2; r+=1)
    {
        float2 uv = r * offset + texcoord;
        float4 color = Input.SampleLevel(PointSampler, uv, 0);
          
        float w = CrossBilateralWeight(r, color.a, center_color.a);
        total_color  += w * color.rgb;
        total_weight += w;
    }

    for(; r<=KERNEL_RADIUS; r+=2)
    {
        float2 uv = r * offset + texcoord;
        float4 color = Input.SampleLevel(LinearSampler, uv, 0);
          
        float w = CrossBilateralWeight(r, color.a, center_color.a);
        total_color  += w * color.rgb;
        total_weight += w;
    }
    
    output.rgb = total_color / total_weight;
    output.a   = center_color.a;

    return output;
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
[numthreads(8, 8, 1)]
void main
(
    uint3 dispatchId : SV_DispatchThreadID,
    uint  groupIndex : SV_GroupIndex
)
{
    uint2 id = RemapLane8x8(dispatchId.xy, groupIndex);

    float2 uv = id * InvTargetSize;

    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);

    result += CrossBilateralFilter(uv,  Offset);
    result += CrossBilateralFilter(uv, -Offset);

    Output[id] = result;
}
