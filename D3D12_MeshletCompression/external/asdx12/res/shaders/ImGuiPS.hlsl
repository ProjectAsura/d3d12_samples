//-----------------------------------------------------------------------------
// File : ImGuiPS.hlsl
// Desc : Pixel Shader For ImGui.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// PSInput structure
///////////////////////////////////////////////////////////////////////////////
struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR0;
};

//-----------------------------------------------------------------------------
// Texture and Samplers.
//-----------------------------------------------------------------------------
Texture2D       Texture0 : register(t0);
SamplerState    Sampler0 : register(s0);

//-----------------------------------------------------------------------------
//      SRGBからリニアへの変換.
//-----------------------------------------------------------------------------
float3 SRGB_To_Linear(float3 color)
{
    float3 result;
    result.x = (color.x < 0.0405f) ? color.x / 12.92f : pow((abs(color.x) + 0.055) / 1.055f, 2.4f);
    result.y = (color.y < 0.0405f) ? color.y / 12.92f : pow((abs(color.y) + 0.055) / 1.055f, 2.4f);
    result.z = (color.z < 0.0405f) ? color.z / 12.92f : pow((abs(color.z) + 0.055) / 1.055f, 2.4f);
    
    return result;
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(PSInput input) : SV_TARGET
{
    float4 texel = Texture0.Sample(Sampler0, input.TexCoord);

    float4 result;
    result.rgb = SRGB_To_Linear(input.Color.rgb) * texel.rgb;
    result.a   = input.Color.a * texel.a;

    return result;
}