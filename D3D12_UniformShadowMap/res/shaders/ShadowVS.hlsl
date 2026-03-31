//-----------------------------------------------------------------------------
// File : ShadowVS.hlsl
// Desc : Vertex Shader For Shadow Drawing.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3  Position    : POSITION;
    float2  TexCoord    : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4  Position    : SV_POSITION;
    float2  TexCoord    : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////
// SceneParam constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer SceneParam : register(b0)
{
    float4x4    View;
    float4x4    Proj;
};

cbuffer ModelParam : register(b1)
{
    float4x4    World;
};

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
VSOutput main(const VSInput input)
{
    VSOutput output = (VSOutput)0;
    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos = mul(World, localPos);
    float4 viewPos  = mul(View, worldPos);
    float4 projPos  = mul(Proj, viewPos);

    output.Position = projPos;
    output.TexCoord = input.TexCoord;

    return output;
}
