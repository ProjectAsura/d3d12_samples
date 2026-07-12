//-----------------------------------------------------------------------------
// File : RasterScrollPS.hlsl
// Desc : Pixel Shader For Raster Scroll Effect.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "asdxSamplers.hlsli"


///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////
// CbParam constant buffers.
///////////////////////////////////////////////////////////////////////////////
cbuffer CbParam : register(b0)
{
    float2 Offset;
    float2 Scale;
    float3 Color;
    float  Alpha;
};

//-----------------------------------------------------------------------------
// Resources.
//-----------------------------------------------------------------------------
Texture2D ColorMap : register(t0);

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET0
{
    // 中心からの距離.
    float2 uv = input.TexCoord - 0.5f.xx;
    float len = length(uv);

    // 回転値.
    float rot = len * Scale.x + Offset.x;
 
    // 回転行列を適用.
    float s, c;
    sincos(rot, s, c);
    uv = mul(uv, float2x2(c, -s, s, c)) + 0.5f;

    // テクスチャをサンプリング.
    float3 texel = ColorMap.SampleLevel(LinearMirror, uv, 0.0f).rgb;

    // カラーをブレンド.
    float3 result = lerp(texel, Color, Alpha);

    // 結果を出力.
    return float4(result, 1.0f);
}
