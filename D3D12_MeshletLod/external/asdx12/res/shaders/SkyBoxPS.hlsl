//-----------------------------------------------------------------------------
// File : SkyBoxPS.hlsl
// Desc : Pixel Shader For SkyBox.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};

//-----------------------------------------------------------------------------
// Textures and Samplers.
//-----------------------------------------------------------------------------
TextureCube  CubeMap : register(t0);
SamplerState CubeSmp : register(s0);

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET
{
    float3 color = CubeMap.Sample(CubeSmp, input.TexCoord).rgb;
    return float4(color, 1.0f);
}
