//-------------------------------------------------------------------------------------------------
// File : SimplePS.hlsl
// Desc : Simple Pixel Shader.
// Copyright(c) Pocol. All right reserved.
//-------------------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4  Position : SV_POSITION;     // 位置座標です.
    float2  TexCoord : TEXCOORD;        // テクスチャ座標です.
    float3  Normal   : NORMAL;          // 法線ベクトルです.
    float4  WorldPos : WORLD_POS;       // ワールド空間での位置座標です.
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// PSOutput structure
///////////////////////////////////////////////////////////////////////////////////////////////////
struct PSOutput
{
    float4  Color : SV_TARGET0;     // 出力カラーです.
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// LightBuffer 
///////////////////////////////////////////////////////////////////////////////////////////////////
cbuffer LightBuffer : register( b1 )
{
    float3 LightPosition : packoffset( c0 );    // ライト位置です.
    float3 LightColor    : packoffset( c1 );    // ライトカラーです.
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// MaterialBuffer
///////////////////////////////////////////////////////////////////////////////////////////////////
cbuffer MaterialBuffer : register(b2)
{
    float3 Diffuse : packoffset( c0 );      // 拡散反射率です.
    float  Alpha   : packoffset( c0.w );    // 透過度です.
}

//-------------------------------------------------------------------------------------------------
// Textures and Samplers
//-------------------------------------------------------------------------------------------------
SamplerState WrapSmp  : register( s0 );
Texture2D    ColorMap : register( t0 );

//-------------------------------------------------------------------------------------------------
//      ピクセルシェーダのメインエントリーポイントです.
//-------------------------------------------------------------------------------------------------
PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput)0;

    float3 N = normalize(input.Normal);
    float3 L = normalize(LightPosition - input.WorldPos.xyz);

    float4 color   = ColorMap.Sample( WrapSmp, input.TexCoord );
    float3 diffuse = LightColor * Diffuse * saturate(dot(L, N));

    output.Color = float4( color.rgb * diffuse, color.a * Alpha );

    return output;
}