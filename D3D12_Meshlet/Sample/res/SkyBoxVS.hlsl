//-----------------------------------------------------------------------------
// File : SkyBoxVS.hlsl
// Desc : Vertex Shader For Sky Box.
// Copyright(c) Pocol. All right reserved.
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
// CbSkyBox constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer CbSkyBox : register(b0)
{
    float4x4 World      : packoffset(c0);
    float4x4 View       : packoffset(c4);
    float4x4 Proj       : packoffset(c8);
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
VSOutput main(const VSInput input)
{
    VSOutput output = (VSOutput)0;

    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos = mul( World, localPos );
    float4 viewPos  = mul( View,  worldPos );
    float4 projPos  = mul( Proj,  viewPos );

    output.Position = projPos;
    output.TexCoord = worldPos.xyz;

    return output;
}
