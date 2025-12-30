//-----------------------------------------------------------------------------
// File : TonemapVS.hlsl
// Desc : Vertex Shader For Tonemap
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float2 Position : POSITION;     // 位置座標です.
    float2 TexCoord : TEXCOORD;     // テクスチャ座標です.
};

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

//-----------------------------------------------------------------------------
//      メインエントリーポイント.
//-----------------------------------------------------------------------------
VSOutput main(const VSInput input)
{
    VSOutput output = (VSOutput)0;

    output.Position = float4(input.Position, 0.0f, 1.0f);
    output.TexCoord = input.TexCoord;

    return output;
}