//-------------------------------------------------------------------------------------------------
// File : FullScreenVS.hlsl
// Desc : VertexShader For Full Screen Triangle.
// Copyright(c) Project Asura. All right reserved.
//-------------------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    noperspective float2 TexCoord : TEXCOORD;
};

//-------------------------------------------------------------------------------------------------
//      メインエントリーポイントです.
//-------------------------------------------------------------------------------------------------
VSOutput main(const VSInput input)
{
    VSOutput output = (VSOutput)0;

    output.Position = float4(input.Position, 0.0f, 1.0f);
    output.TexCoord = input.TexCoord;

    return output;
}