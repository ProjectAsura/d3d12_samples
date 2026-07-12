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
    float2 uv = input.TexCoord;

    // ピクセルのY座標に応じて，X座標をずらす.
    uv.x += sin(uv.y * Scale.y + Offset.y);

    // テクスチャをサンプル.
    float3 texel = ColorMap.SampleLevel(LinearMirror, uv, 0.0f).rgb;

    // カラーをブレンド.
    float3 result = lerp(texel, Color, Alpha);

    // 結果を出力.
    return float4(result, 1.0f);
}
