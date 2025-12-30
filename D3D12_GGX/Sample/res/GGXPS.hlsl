//-----------------------------------------------------------------------------
// File : GGXPS.hlsl
// Desc : GGX Lighting Pixel Shader.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constant Values
//-----------------------------------------------------------------------------
static const float F_PI = 3.141596535f;

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
    float3 BaseColor : packoffset( c0 );    // 基本色.
    float  Alpha     : packoffset( c1.w );  // 透過度.
    float  Roughness : packoffset( c1 );    // 面の粗さです(範囲は[0, 1]).
    float  Metallic  : packoffset( c1.y );  // メタリック.
};

//-----------------------------------------------------------------------------
// Textures and Samplers
//-----------------------------------------------------------------------------
SamplerState WrapSmp  : register( s0 );
Texture2D    ColorMap : register( t0 );

//-----------------------------------------------------------------------------
//      Schlickによるフレネル項の近似式.
//-----------------------------------------------------------------------------
float3 SchlickFresnel(float3 specular, float VH)
{
    return specular + (1.0f - specular) * pow((1.0f - VH), 5.0f);
}

//-----------------------------------------------------------------------------
//      GGXによる法線分布関数.
//-----------------------------------------------------------------------------
float D_GGX(float m2, float NH)
{
    float f = (NH * m2 - NH) * NH + 1;
    return m2 / (F_PI * f * f);
}

//-----------------------------------------------------------------------------
//      Height Correlated Smithによる幾何減衰項.
//-----------------------------------------------------------------------------
float G2_Smith(float NL, float NV, float m2)
{
    float NL2 = NL * NL;
    float NV2 = NV * NV;

    float Lambda_V = (-1.0f + sqrt(m2 * (1.0f - NL2) / NL2 + 1.0f)) * 0.5f;
    float Lambda_L = (-1.0f + sqrt(m2 * (1.0f - NV2) / NV2 + 1.0f)) * 0.5f;
    return 1.0f / (1.0f + Lambda_V + Lambda_V);
}

//-----------------------------------------------------------------------------
//      ピクセルシェーダのメインエントリーポイントです.
//-----------------------------------------------------------------------------
PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput)0;

    float3 N = normalize(input.Normal);
    float3 L = normalize(LightPosition  - input.WorldPos);
    float3 V = normalize(CameraPosition - input.WorldPos);
    float3 H = normalize(V + L);

    float NV = saturate(dot(N, V));
    float NH = saturate(dot(N, H));
    float NL = saturate(dot(N, L));
    float VH = saturate(dot(V, H));

    float4 color    = ColorMap.Sample(WrapSmp, input.TexCoord);
    float3 Kd       = BaseColor * (1.0f - Metallic);
    float3 diffuse  = Kd * (1.0 / F_PI);

    float3 Ks = BaseColor * Metallic;
    float  a  = Roughness * Roughness;
    float  m2 = a * a;
    float  D  = D_GGX(m2, NH);
    float  G2 = G2_Smith(NL, NV, m2);
    float3 Fr = SchlickFresnel(Ks, NL);

    float3 specular = (D * G2 * Fr) / (4.0f * NV * NL);

    output.Color = float4( color.rgb * ( diffuse + specular ) * NL, color.a * Alpha );
    return output;
}
