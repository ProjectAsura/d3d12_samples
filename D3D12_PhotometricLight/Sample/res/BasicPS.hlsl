//-----------------------------------------------------------------------------
// File : BasicPS.hlsl
// Desc : Pixel Shader.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "BRDF.hlsli"

#ifndef MIN_DIST
#define MIN_DIST (0.01)
#endif//MIN_DIST


///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4      Position        : SV_POSITION;          // 位置座標です.
    float2      TexCoord        : TEXCOORD;             // テクスチャ座標です.
    float3      WorldPos        : WORLD_POS;            // ワールド空間の位置座標です.
    float3x3    InvTangentBasis : INV_TANGENT_BASIS;    // 接線空間への基底変換行列の逆行列です.
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
    float3      LightPosition       : packoffset(c0);
    float       LightInvSqrRadius   : packoffset(c0.w);
    float3      LightColor          : packoffset(c1);
    float       LightIntensity      : packoffset(c1.w);
    float3      LightForward        : packoffset(c2);
    float       LightAngleScale     : packoffset(c2.w);
    float       LightAngleOffset    : packoffset(c3);
    int3        Reserved            : packoffset(c3.y);
};

///////////////////////////////////////////////////////////////////////////////
// CameraBuffer
///////////////////////////////////////////////////////////////////////////////
cbuffer CbCamera : register(b2)
{
    float3 CameraPosition : packoffset(c0);     // カメラ位置です.
}

//-----------------------------------------------------------------------------
// Textures and Samplers
//-----------------------------------------------------------------------------
SamplerState    IESSmp       : register(s0);
Texture2D       IESMap       : register(t0);

Texture2D       BaseColorMap : register(t1);
SamplerState    BaseColorSmp : register(s1);

Texture2D       MetallicMap  : register(t2);
SamplerState    MetallicSmp  : register(s2);

Texture2D       RoughnessMap : register(t3);
SamplerState    RoughnessSmp : register(s3);

Texture2D       NormalMap    : register(t4);
SamplerState    NormalSmp    : register(s4);


//-----------------------------------------------------------------------------
//      距離を元に滑らかに減衰させます.
//-----------------------------------------------------------------------------
float SmoothDistanceAttenuation
(
    float squaredDistance,      // ライトへの距離の2乗.
    float invSqrAttRadius       // ライト半径の2乗の逆数.
)
{
    float  factor = squaredDistance * invSqrAttRadius;
    float  smoothFactor = saturate(1.0f - factor * factor);
    return smoothFactor * smoothFactor;
}

//-----------------------------------------------------------------------------
//      距離減衰を求めます.
//-----------------------------------------------------------------------------
float GetDistanceAttenuation
(
    float3  unnormalizedLightVector,    // ライト位置とオブジェクト位置の差分ベクトル.
    float   invSqrAttRadius             // ライト半径の2乗の逆数.
)
{
    float sqrDist = dot(unnormalizedLightVector, unnormalizedLightVector);
    float attenuation = 1.0f / (max(sqrDist, MIN_DIST * MIN_DIST));
    attenuation *= SmoothDistanceAttenuation(sqrDist, invSqrAttRadius);

    return attenuation;
}

//-----------------------------------------------------------------------------
//      角度減衰を求めます.
//-----------------------------------------------------------------------------
float GetAngleAttenuation
(
    float3  unnormalizedLightVector,    // ライト位置とオブジェクト位置の差分ベクトル.
    float3  lightDir,                   // 正規化済みのライトベクトル.
    float   lightAngleScale,            // スポットライトの角度減衰スケール.
    float   lightAngleOffset            // スポットライトの角度オフセット.
)
{
    // CPU側で次の値を計算しておく.
    // float lightAngleScale = 1.0f / max(0.001f, (cosInner - cosOuter));
    // float lightAngleOffset  = -cosOuter * lightAngleScale;
    // 上記における
    // * cosInnerは内角の余弦.
    // * cosOuterは外角の余弦.
    // とします.

    float cd = dot(lightDir, unnormalizedLightVector);
    float attenuation = saturate(cd * lightAngleScale + lightAngleOffset);

    // 滑らかに変化させる.
    attenuation *= attenuation;

    return attenuation;
}

//-----------------------------------------------------------------------------
//      IESプロファイルによる減衰を求めます.
//-----------------------------------------------------------------------------
float GetIESProfileAttenuation(float3 lightDir, float3 lightForward)
{
    // 角度を求める.
    float  thetaCoord   = dot(lightDir, lightForward) * 0.5f + 0.5f;
    float  tangentAngle = atan2(lightDir.y, lightDir.x);
    float  phiCoord     = (tangentAngle / F_PI) * 0.5f + 0.5;
    float2 texCoord     = float2(thetaCoord, phiCoord);

    // IESテクスチャから正規化された光度を取り出す.
    return IESMap.SampleLevel(IESSmp, texCoord, 0).r;
}

//-----------------------------------------------------------------------------
//      フォトメトリックライトを評価します.
//-----------------------------------------------------------------------------
float3 EvaluatePhotometricLight
(
    float3      N,                  // 法線ベクトル.
    float3      worldPos,           // ワールド空間のオブジェクト位置.
    float3      lightPos,           // ライト位置.
    float3      lightForward,       // ライトの照射方向.
    float3      lightColor,         // ライトカラー.
    float       lightAngleScale,    // ライトの角度スケール.
    float       lightAngleOffset,   // ライトの角度オフセット.
    float       lightInvSqrRadius   // ライト半径2乗の逆数.
)
{
    float3 unnormalizedLightVector = (lightPos - worldPos);
    float3 L = normalize(unnormalizedLightVector);
    float att = 1.0f;
    att *= GetDistanceAttenuation(unnormalizedLightVector, lightInvSqrRadius);
    att *= GetAngleAttenuation(-L, lightForward, lightAngleScale, lightAngleOffset);
    att *= GetIESProfileAttenuation(L, lightForward);

    return saturate(dot(N, L)) * lightColor * att / (4.0f * F_PI);
}

//-----------------------------------------------------------------------------
//      ピクセルシェーダのメインエントリーポイントです.
//-----------------------------------------------------------------------------
PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput)0;

    float3 L = normalize(LightPosition  - input.WorldPos);
    float3 V = normalize(CameraPosition - input.WorldPos);
    float3 H = normalize(V + L);
    float3 N = NormalMap.Sample(NormalSmp, input.TexCoord).xyz * 2.0f - 1.0f;
    N = mul(input.InvTangentBasis, N);

    float NV = saturate(dot(N, V));
    float NH = saturate(dot(N, H));
    float NL = saturate(dot(N, L));

    float3 baseColor = BaseColorMap.Sample(BaseColorSmp, input.TexCoord).rgb;
    float  metallic  = MetallicMap .Sample(MetallicSmp,  input.TexCoord).r;
    float  roughness = RoughnessMap.Sample(RoughnessSmp, input.TexCoord).r;

    float3 Kd      = baseColor * (1.0f - metallic);
    float3 diffuse = ComputeLambert(Kd);

    float3 Ks       = baseColor * metallic;
    float3 specular = ComputeGGX(Ks, roughness, NH, NV, NL);

    float3 BRDF = (diffuse + specular);
    float3 lit  = EvaluatePhotometricLight(
        N,
        input.WorldPos,
        LightPosition,
        LightForward,
        LightColor,
        LightAngleScale,
        LightAngleOffset,
        LightInvSqrRadius) * LightIntensity;

    output.Color.rgb = lit * BRDF;
    output.Color.a   = 1.0f;

    return output;
}