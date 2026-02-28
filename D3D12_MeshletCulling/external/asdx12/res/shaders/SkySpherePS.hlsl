//-----------------------------------------------------------------------------
// File : SkySpherePS.hlsl
// Desc : Pixel Shader For SkySphere.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4   Position       : SV_POSITION;
    float2   TexCoord       : TEXCOORD;
    float3   Tangent        : TANGENT;
    float3   Binormal       : BINORMAL;
};

//-----------------------------------------------------------------------------
// Textures and Samplers.
//-----------------------------------------------------------------------------
Texture2D    SphereMap : register(t0);
SamplerState SphereSmp : register(s0);

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET
{
    float3 color = SphereMap.Sample(SphereSmp, input.TexCoord).rgb;
    return float4(color, 1.0f);
}
