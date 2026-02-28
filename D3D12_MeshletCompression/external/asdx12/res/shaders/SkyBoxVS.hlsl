//-----------------------------------------------------------------------------
// File : SkyBoxVS.hlsl
// Desc : Vertex Shader For SkyBox.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;
};

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};

///////////////////////////////////////////////////////////////////////////////
// CbSkyBox structure
///////////////////////////////////////////////////////////////////////////////
cbuffer CbSkyBox : register(b0)
{
    float4x4 World      : packoffset(c0);
    float4x4 View       : packoffset(c4);
    float4x4 Proj       : packoffset(c8);
    float3   CameraPos  : packoffset(c12);
    float    BoxSize    : packoffset(c12.w);
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
VSOutput main(const VSInput input)
{
    VSOutput output = (VSOutput)0;

    float4 localPos = float4( input.Position * BoxSize, 1.0f );
    float4 worldPos = mul( World, localPos );
    float4 viewPos  = mul( View,  worldPos );
    float4 projPos  = mul( Proj,  viewPos );

    output.Position = projPos;
    output.TexCoord = worldPos.xyz - CameraPos;

    return output;
}