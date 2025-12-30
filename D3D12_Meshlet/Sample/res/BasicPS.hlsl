//-----------------------------------------------------------------------------
// File : BasicPS.hlsl
// Desc : Pixel Shader.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "BRDF.hlsli"


///////////////////////////////////////////////////////////////////////////////
// MSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct MSOutput
{
    float4  Position    : SV_POSITION;  // 位置座標です.
    float2  TexCoord    : TEXCOORD;     // テクスチャ座標です.
    float3  WorldPos    : WORLD_POS;    // ワールド空間の位置座標です.
    float3  Tangent     : TANGENT;      // 接線ベクトルです.
    float3  Bitangent   : BITANGENT;    // 従接線ベクトルです.
    float3  Normal      : NORMAL;       // 法線ベクトルです.    
};

///////////////////////////////////////////////////////////////////////////////
// PSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct PSOutput
{
    float4  Color : SV_TARGET0;     // 出力カラーです.
};

///////////////////////////////////////////////////////////////////////////////
// CbLight constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer CbLight : register(b1)
{
    float   TextureSize     : packoffset(c0);   // テクスチャサイズです.
    float   MipCount        : packoffset(c0.y); // ミップカウントです.
    float   LightIntensity  : packoffset(c0.z); // ライト強度(スケール値).
    float3  LightDirection  : packoffset(c1);   // ディレクショナルライトの方向.
};

///////////////////////////////////////////////////////////////////////////////
// CbCamera constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer CbCamera : register(b2)
{
    float3 CameraPosition : packoffset(c0);     // カメラ位置です.
}

//-----------------------------------------------------------------------------
// Textures and Samplers
//-----------------------------------------------------------------------------
// DFG項.
Texture2D    DFGMap         : register(t0);
SamplerState DFGSmp         : register(s0);

// ディフューズLD項.
TextureCube  DiffuseLDMap   : register(t1);
SamplerState DiffuseLDSmp   : register(s1);

// スペキュラーLD項.
TextureCube  SpecularLDMap  : register(t2);
SamplerState SpecularLDSmp  : register(s2);

// ベースカラーマップ.
Texture2D    BaseColorMap    : register(t3);
SamplerState BaseColorSmp    : register(s3);

// メタリックマップ.
Texture2D    MetallicMap     : register(t4);
SamplerState MetallicSmp     : register(s4);

// ラフネスマップ.
Texture2D    RoughnessMap    : register(t5);
SamplerState RoughnessSmp    : register(s5);

// 法線マップ.
Texture2D    NormalMap       : register(t6);
SamplerState NormalSmp       : register(s6);


//-----------------------------------------------------------------------------
//      スペキュラーの支配的な方向を求めます.
//-----------------------------------------------------------------------------
float3 GetSpecularDomiantDir(float3 N, float3 R, float roughness)
{
    float smoothness = saturate(1.0f - roughness);
    float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
    return lerp(N, R, lerpFactor);
}

//-----------------------------------------------------------------------------
//      ディフューズIBLを評価します.
//-----------------------------------------------------------------------------
float3 EvaluateIBLDiffuse(float3 N)
{
    // Lambert BRDFはDFG項は積分すると1.0となるので，LD項のみを返却すれば良い
    return DiffuseLDMap.Sample(DiffuseLDSmp, N).rgb;
}

//-----------------------------------------------------------------------------
//      線形ラフネスからミップレベルを求めます.
//-----------------------------------------------------------------------------
float RoughnessToMipLevel(float linearRoughness, float mipCount)
{
    return (mipCount - 1) * linearRoughness;
}

//-----------------------------------------------------------------------------
//      スペキュラーIBLを評価します.
//-----------------------------------------------------------------------------
float3 EvaluateIBLSpecular
(
    float           NdotV,          // 法線ベクトルと視線ベクトルの内積.
    float3          N,              // 法線ベクトル.
    float3          R,              // 反射ベクトル.
    float3          f0,             // フレネル項
    float           roughness,      // 線形ラフネス.
    float           textureSize,    // テクスチャサイズ.
    float           mipCount        // ミップレベル数.
)
{
    float  a = roughness * roughness;
    float3 dominantR = GetSpecularDomiantDir(N, R, a);

    // 関数を再構築.
    // L * D * (f0 * Gvis * (1 - Fc) + Gvis * Fc) * cosTheta / (4 * NdotL * NdotV).
    NdotV = max(NdotV, 0.5f / textureSize); // ゼロ除算が発生しないようにする.
    float  mipLevel = RoughnessToMipLevel(roughness, mipCount);
    float3 preLD    = SpecularLDMap.SampleLevel(SpecularLDSmp, dominantR, mipLevel).xyz;
    
    // 事前積分したDFGをサンプルする.
    // Fc = ( 1 - HdotL )^5
    // PreIntegratedDFG.r = Gvis * (1 - Fc)
    // PreIntegratedDFG.g = Gvis * Fc
    float2 preDFG   = DFGMap.SampleLevel(DFGSmp, float2(NdotV, roughness), 0).xy;

    // LD * (f0 * Gvis * (1 - Fc) + Gvis * Fc)
    return preLD * (f0 * preDFG.x + preDFG.y);
}


//-----------------------------------------------------------------------------
//      ピクセルシェーダのメインエントリーポイントです.
//-----------------------------------------------------------------------------
PSOutput main(MSOutput input)
{
    PSOutput output = (PSOutput)0;
    
    float3x3 invTangentBasis = transpose(float3x3(input.Tangent, input.Bitangent, input.Normal));

    float3 V = normalize(input.WorldPos.xyz - CameraPosition);
    float3 N = NormalMap.Sample(NormalSmp, input.TexCoord).xyz * 2.0f - 1.0f;
    N = mul(invTangentBasis, N);
    float3 R = normalize(reflect(V, N));

    float NV = saturate(dot(N, V));

    float3 baseColor = BaseColorMap.Sample(BaseColorSmp, input.TexCoord).rgb;
    float  metallic  = MetallicMap .Sample(MetallicSmp,  input.TexCoord).r;
    float  roughness = RoughnessMap.Sample(RoughnessSmp, input.TexCoord).r;

    float3 Kd = baseColor * (1.0f - metallic);
    float3 Ks = baseColor * metallic;

    float3 lit = 0;
    lit += EvaluateIBLDiffuse(N) * Kd;
    lit += EvaluateIBLSpecular(NV, N, R, Ks, roughness, TextureSize, MipCount);

    output.Color.rgb = lit * LightIntensity;
    output.Color.a   = 1.0f;

    return output;
}