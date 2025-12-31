//-------------------------------------------------------------------------------------------------
// File : SimpleVS.hlsl
// Desc : Simple Vertex Shader.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Color    : VTX_COLOR;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 Color    : VTX_COLOR;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// TransformBuffer constant buffer
///////////////////////////////////////////////////////////////////////////////////////////////////
cbuffer TransformBuffer : register(b0)
{
    float4x4 World  : packoffset(c0);
    float4x4 View   : packoffset(c4);
    float4x4 Proj   : packoffset(c8);
};

//-------------------------------------------------------------------------------------------------
//      頂点シェーダのメインエントリーポイントです.
//-------------------------------------------------------------------------------------------------
VSOutput main(const VSInput input)
{
    VSOutput output = (VSOutput) 0;

    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos = mul(World, localPos);
    float4 viewPos  = mul(View,  worldPos);
    float4 projPos  = mul(Proj,  viewPos);

    output.Position = projPos;
    output.Normal   = input.Normal;
    output.TexCoord = input.TexCoord;
    output.Color    = input.Color;

    return output;
}