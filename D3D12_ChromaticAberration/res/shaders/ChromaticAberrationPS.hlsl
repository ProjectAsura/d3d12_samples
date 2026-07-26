//-----------------------------------------------------------------------------
// File : ChromaticAberrationPS.hlsl
// Desc : Magnification Chromatic Aberration.
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
    float AspectRatio;
    float Power;
    float Scale;
    uint  Resolution;
};

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
Texture2D ColorMap : register(t0);

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET
{
    float2 v = (input.TexCoord - 0.5f.xx);
    v.x *= AspectRatio;

    // サンプリング方向.
    float2 dir = normalize(v) * pow(length(v), Power);

    float3 output = 0.0f.xxx;
    output += ColorMap.SampleLevel(LinearClamp, input.TexCoord - dir * Scale * 0.0f, 0.0f).rgb * float3(0.1625f, 0.0f,  0.0f);
    output += ColorMap.SampleLevel(LinearClamp, input.TexCoord - dir * Scale * 1.0f, 0.0f).rgb * float3(0.5625f, 0.0f,  0.0f);
    output += ColorMap.SampleLevel(LinearClamp, input.TexCoord - dir * Scale * 2.0f, 0.0f).rgb * float3(0.25f,   0.25f, 0.0f);
    output += ColorMap.SampleLevel(LinearClamp, input.TexCoord - dir * Scale * 3.0f, 0.0f).rgb * float3(0.0f,    0.5f,  0.0f);
    output += ColorMap.SampleLevel(LinearClamp, input.TexCoord - dir * Scale * 4.0f, 0.0f).rgb * float3(0.0f,    0.25f, 0.25f);
    output += ColorMap.SampleLevel(LinearClamp, input.TexCoord - dir * Scale * 5.0f, 0.0f).rgb * float3(0.0f,    0.0f,  0.5f);
    output += ColorMap.SampleLevel(LinearClamp, input.TexCoord - dir * Scale * 6.0f, 0.0f).rgb * float3(0.025f,  0.0f,  0.25f);

    return float4(output, 1.0f);
}
