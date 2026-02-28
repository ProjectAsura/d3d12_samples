//-----------------------------------------------------------------------------
// File : SimplePS.hlsl
// Desc : Simple Pixel Shader.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// MSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct MSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR0;
};

///////////////////////////////////////////////////////////////////////////////
// PSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct PSOutput
{
    float4 Color : SV_TARGET0;
};

///////////////////////////////////////////////////////////////////////////////
// Constants structure
///////////////////////////////////////////////////////////////////////////////
struct Constants
{
    uint    MeshletCount;
    uint    InstanceId;
    float   MinContribution;
    uint    Flags;
};

///////////////////////////////////////////////////////////////////////////////
// TransParam structure
///////////////////////////////////////////////////////////////////////////////
struct TransParam
{
    float4x4 View;
    float4x4 Proj;
};

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
ConstantBuffer<Constants>           g_Constants     : register(b0);
ConstantBuffer<TransParam>          g_TransParam    : register(b1);


//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
PSOutput main(const MSOutput input)
{
    PSOutput output = (PSOutput)0;
    if (WaveActiveAllTrue(!!((g_Constants.Flags >> 1) & 0x1)))
    {
        float3 lightDir = normalize(g_TransParam.View._31_32_33);
        output.Color.rgb = saturate(dot(normalize(input.Normal), lightDir)).xxx;
        output.Color.a = 1.0f;
    }
    else
    {
        output.Color = input.Color;
    }
    
    return output;
}