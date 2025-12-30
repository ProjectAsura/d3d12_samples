//-----------------------------------------------------------------------------
// File : SphereToCubePS.hlsl
// Desc : Convert from sphere map to cube map.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

//-----------------------------------------------------------------------------
// Textures and Samplers.
//-----------------------------------------------------------------------------
Texture2D       SphereMap : register(t0);
SamplerState    SphereSmp : register(s0);

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET0
{
    // スフィアマップをサンプルします.
    return SphereMap.SampleLevel(SphereSmp, input.TexCoord, 0.0f);
}
