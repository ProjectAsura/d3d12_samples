//-----------------------------------------------------------------------------
// File : PhongPS.hlsl
// Desc : Phong Lighting Pixel Shader.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{   
    float4  Position : SV_POSITION;     // 位置座標です.
    float2  TexCoord : TEXCOORD;        // テクスチャ座標です.
    float3  Normal   : NORMAL;          // 法線ベクトルです.
    float3  WorldPos : WORLD_POS;       // ワールド空間の位置座標です.
};

///////////////////////////////////////////////////////////////////////////////
// PSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct PSOutput
{
    float4  Color : SV_TARGET0;     // 出力カラーです.
};

///////////////////////////////////////////////////////////////////////////////
// LightBuffer 
///////////////////////////////////////////////////////////////////////////////
cbuffer LightBuffer : register( b1 )
{
    float3 LightPosition  : packoffset( c0 );   // ライト位置です.
    float3 LightColor     : packoffset( c1 );   // ライトカラーです.
    float3 CameraPosition : packoffset( c2 );   // カメラ位置です.
};

///////////////////////////////////////////////////////////////////////////////
// MaterialBuffer
///////////////////////////////////////////////////////////////////////////////
cbuffer MaterialBuffer : register( b2 )
{
    float3 Diffuse      : packoffset( c0 );     // 拡散反射率です.
    float  Alpha        : packoffset( c0.w );   // 透過度です.
    float3 Specular     : packoffset( c1 );     // 鏡面反射率です.
    float  Shininess    : packoffset( c1.w );   // 鏡面反射強度です.
};

//-----------------------------------------------------------------------------
// Textures and Samplers
//-----------------------------------------------------------------------------
SamplerState WrapSmp  : register( s0 );
Texture2D    ColorMap : register( t0 );

//-----------------------------------------------------------------------------
//      ピクセルシェーダのメインエントリーポイントです.
//-----------------------------------------------------------------------------
PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput)0;

    float3 N = normalize(input.Normal);
    float3 L = normalize(LightPosition  - input.WorldPos);
    float3 V = normalize(CameraPosition - input.WorldPos);

    float3 R = normalize(-reflect(V, N));

    float4 color    = ColorMap.Sample( WrapSmp, input.TexCoord );
    float3 diffuse  = LightColor * Diffuse  * saturate(dot( L, N ));
    float3 specular = LightColor * Specular * pow( saturate(dot(L, R)), Shininess );

    output.Color = float4( color.rgb * ( diffuse + specular ), color.a * Alpha );
    return output;
}