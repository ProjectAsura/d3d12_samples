//-----------------------------------------------------------------------------
// File : FilmGrainPS.hlsl
// Desc : Film Grain Effect.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "asdxMath.hlsli"
#include "asdxColor.hlsli"
#include "asdxSamplers.hlsli"
#include "asdxRandom.hlsli"


///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////
// CbParam constant buffer. 
///////////////////////////////////////////////////////////////////////////////
cbuffer CbParam : register(b0)
{
    float  Time;
    float  Sigma;
    float2 Reserved;
};

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
Texture2D ColorMap  : register(t0);

//-----------------------------------------------------------------------------
//      ガウス関数.
//-----------------------------------------------------------------------------
float3 Gaussian(float3 value, float sigma)
{
    float norm = 1.0f / (sqrt(2.0f * F_PI) * sigma);
    return norm * exp(-0.5f * Pow2(value) / Pow2(sigma));
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET0
{
    float4 color = ColorMap.SampleLevel(LinearClamp, input.TexCoord, 0.0f);
 
    // 適当なノイズを作る.
    float3 noise;
    noise.x = FracSin(input.TexCoord, Time);
    noise.y = FracSin(input.TexCoord, Time * 1.3f);
    noise.z = FracSin(input.TexCoord, Time * 1.5f);

    // 適当な偏りを作る.
    noise = Gaussian(noise, Pow2(Sigma));
    noise = saturate(noise);

    // オーバーレイでノイズを加える.
    color.rgb = BlendOverlay(color.rgb, noise);

    // 結果を出力.
    return color;
}
