//-----------------------------------------------------------------------------
// File : GGXVS.hlsl
// Desc : GGX Lighting Vertex Shader.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3  Position : POSITION;    // 位置座標です.
    float3  Normal   : NORMAL;      // 法線ベクトルです.
    float2  TexCoord : TEXCOORD;    // テクスチャ座標です.
    float3  Tangent  : TANGENT;     // 接線ベクトルです.
};

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
// Transform constant buffer
///////////////////////////////////////////////////////////////////////////////
cbuffer Transform : register( b0 )
{
    float4x4 World : packoffset( c0 ); // ワールド行列です.
    float4x4 View  : packoffset( c4 ); // ビュー行列です.
    float4x4 Proj  : packoffset( c8 ); // 射影行列です.
}

//-----------------------------------------------------------------------------
//      頂点シェーダのメインエントリーポイントです.
//-----------------------------------------------------------------------------
VSOutput main( VSInput input )
{
    VSOutput output = (VSOutput)0;

    float4 localPos = float4( input.Position, 1.0f );
    float4 worldPos = mul( World, localPos );
    float4 viewPos  = mul( View,  worldPos );
    float4 projPos  = mul( Proj,  viewPos );

    output.Position = projPos;
    output.TexCoord = input.TexCoord;
    output.Normal   = normalize( mul((float3x3)World, input.Normal ) );
    output.WorldPos = worldPos.xyz;

    return output;
}