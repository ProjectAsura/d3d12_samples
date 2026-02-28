//-----------------------------------------------------------------------------
// File : SkySphereVS.hlsl
// Desc : Vertex Shader For SkySphere.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
};

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4      Position        : SV_POSITION;
    float2      TexCoord        : TEXCOORD;
    float3      Tangent         : TANGENT;
    float3      Binormal        : BINORMAL;
};

///////////////////////////////////////////////////////////////////////////////
// CbSkySphere structure
///////////////////////////////////////////////////////////////////////////////
cbuffer CbSkySphere : register(b0)
{
    float4x4 World      : packoffset(c0);
    float4x4 View       : packoffset(c4);
    float4x4 Proj       : packoffset(c8);
    float    SphereSize : packoffset(c12);
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
VSOutput main(const VSInput input)
{
    VSOutput output = (VSOutput)0;

    float4 localPos = float4(input.Position * SphereSize, 1.0f);
    float4 worldPos = mul(World, localPos);
    float4 viewPos  = mul(View, worldPos);
    float4 projPos  = mul(Proj, viewPos);

    float3 N = normalize(input.Normal);
    float3 T = normalize(input.Tangent);
    N = mul((float3x3)World, N);
    T = mul((float3x3)World, T);
    float3 B = cross(N, T);

    output.Position     = projPos;
    output.TexCoord     = input.TexCoord;
    output.Tangent      = T;
    output.Binormal     = B;

    return output;
}