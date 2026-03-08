//-----------------------------------------------------------------------------
// File : ImGuiVS.hlsl
// Desc : Vertex Shader For ImGui.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float4 Position : POSITION;
    float2 TexCoord : TEXCOORD0;   
    float4 Color    : COLOR0;
};

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;  
    float4 Color    : COLOR0;
};

///////////////////////////////////////////////////////////////////////////////
// CbImGui constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer CbImGui : register(b0)
{
    float4x4 Proj;
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(Proj, float4(input.Position.xy, 0.0f, 1.0f));
    output.Color    = input.Color;
    output.TexCoord = input.TexCoord;
    return output;
}
