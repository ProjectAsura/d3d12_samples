//-----------------------------------------------------------------------------
// File : ShadowPS.hlsl
// Desc : Pixel Shader For Shadow Map.
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
    float4  Position    : SV_POSITION;
    float2  TexCoord    : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////
// MaterialParam constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer MaterialParam : register(b1)
{
    float3  BaseColor;
    float   Alpha;
    float   Occlusion;
    float   Roughness;
    float   Metalness;
    float   Ior;
    float3  Emissive;
    float   AlphaCutOff;
};

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
Texture2D BaseColorMap  : register(t0);

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
void main(const VSOutput input)
{
    float alpha  = BaseColorMap.Sample(LinearWrap, input.TexCoord).a * Alpha;
    if (alpha < AlphaCutOff)
        discard;
}
