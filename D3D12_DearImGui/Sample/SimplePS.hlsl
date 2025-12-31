//-----------------------------------------------------------------------------
// File : SimplePS.hlsl
// Desc : Simple Pixel Shader
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Color    : VTX_COLOR;
};

///////////////////////////////////////////////////////////////////////////////
// PSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct PSOutput
{
    float4 Color : SV_TARGET0;
};

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
Texture2D       ColorMap : register(t0);
SamplerState    Sampler  : register(s0);

//-----------------------------------------------------------------------------
//      ピクセルシェーダのメインエントリーポイントです.
//-----------------------------------------------------------------------------
PSOutput main(const VSOutput input)
{
    PSOutput output = (PSOutput)0;
    output.Color = ColorMap.Sample(Sampler, input.TexCoord) * input.Color;
    return output;
}
