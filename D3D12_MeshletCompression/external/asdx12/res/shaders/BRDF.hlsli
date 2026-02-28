//-----------------------------------------------------------------------------
// File : BRDF.hlsli
// Desc : BRDF.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#ifndef ASDX_BRDF_HLSLI
#define ASDX_BRDF_HLSLI

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "Math.hlsli"


//-----------------------------------------------------------------------------
//      ラフネスからスペキュラー指数を求めます.
//-----------------------------------------------------------------------------
float ToSpecularPower(float roughness)
{ return 2.0f / min(0.99999f, max(0.0002f, roughness * roughness)) - 2.0f; }

//-----------------------------------------------------------------------------
//      スペキュラー指数からラフネス値を求めます.
//-----------------------------------------------------------------------------
float ToRoughness(float specularPower)
{
    // Dimiatr Lazarov, "Physically-based lighting in Call of Duty: Black Ops",
    // SIGGRAPH 2011 Cources: Advances in Real-Time Rendering in 3D Graphics.
    return sqrt(2.0f / (specularPower + 2.0f));
}

//-----------------------------------------------------------------------------
//      ディフューズ反射率を求めます.
//-----------------------------------------------------------------------------
float3 ToKd(float3 baseColor, float metallic)
{ return (1.0f - metallic) * baseColor; }

//-----------------------------------------------------------------------------
//      スペキュラー反射率を求めます.
//-----------------------------------------------------------------------------
float3 ToKs(float3 baseColor, float metallic)
{ return lerp(0.03f, baseColor, metallic); }

//-----------------------------------------------------------------------------
//      非金属向け.
//-----------------------------------------------------------------------------
float3 ToKsDielectics(float3 baseColor, float metallic, float reflectance)
{ return lerp(0.16f * reflectance * reflectance, baseColor, metallic); }

//-----------------------------------------------------------------------------
//      金属向け.
//-----------------------------------------------------------------------------
float3 ToKsConductors(float3 baseColor, float metallic)
{ return baseColor * metallic; }

//-----------------------------------------------------------------------------
//      90度入射におけるフレネル反射率を求めます.
//-----------------------------------------------------------------------------
float CalcF90(in float3 f0)
{ return saturate(50.0f * dot(f0, 0.33f)); }

//-----------------------------------------------------------------------------
//      Schlickによるフレネル反射の近似値を求める.
//-----------------------------------------------------------------------------
float3 F_Schlick(in float3 f0, in float f90, in float u)
{ return f0 + (f90 - f0) * Pow5(1.0f - u); }

//-----------------------------------------------------------------------------
//      Schlickによるフレネル反射の近似値を求める.
//-----------------------------------------------------------------------------
float F_Schlick(in float f0, in float f90, in float u)
{ return f0 + (f90 - f0) * Pow5(1.0f - u); }

//-----------------------------------------------------------------------------
//      フレネル項を計算します.
//-----------------------------------------------------------------------------
float3 F_Schlick(const float3 f0, float VoH)
{
    float f = Pow5(1.0f - VoH);
    return f + f0 * (1.0f - f);
}

//-----------------------------------------------------------------------------
//      フレネル項を計算します.
//-----------------------------------------------------------------------------
float F_Schlick(const float f0, float VoH)
{
    float f = Pow5(1.0f - VoH);
    return f + f0 * (1.0f - f);
}

//-----------------------------------------------------------------------------
//      ディフューズの支配的な方向を求めます.
//-----------------------------------------------------------------------------
float3 GetDiffuseDominantDir(float3 N, float3 V, float NoV, float roughness)
{
    float a = 10.2341f * roughness - 1.51174f;
    float b = -0.511705f * roughness + 0.755868f;
    float lerpFactor = saturate((NoV * a + b) * roughness);
    return lerp(N, V, lerpFactor);
}

//-----------------------------------------------------------------------------
//      スペキュラーの支配的な方向を求めます.
//-----------------------------------------------------------------------------
float3 GetSpecularDominantDir(float3 N, float3 R, float roughness)
{
    float smoothness = saturate(1.0f - roughness);
    float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
    return lerp(N, R, lerpFactor);
}

//-----------------------------------------------------------------------------
//      スペキュラーローブの半角の正接を求めます.
//-----------------------------------------------------------------------------
float GetSpecularLobeTanHalfAngle(float linearRoughness, float percentOfVolume /*= 0.75*/)
{
    // Moving Frostbite to PBR v3.2 p.72
    float a = linearRoughness * linearRoughness;
    return a * percentOfVolume / ( 1.0 - percentOfVolume );
}

//-----------------------------------------------------------------------------
//      スペキュラーローブの半角を求めます.
//-----------------------------------------------------------------------------
float GetSpecularLobeHalfAngle(float linearRoughess, float percentOfVolume /*= 0.75f*/)
{
    // Moving Frostbite to PBR v3.2 p.72
    float tangent = GetSpecularLobeTanHalfAngle(linearRoughess, percentOfVolume);
    return atan(tangent);
}

//-----------------------------------------------------------------------------
//      スペキュラーAOを計算します.
//-----------------------------------------------------------------------------
float CalcSpecularAO(float NoV, float ao, float roughness)
{
    // Moving Frostbite to PBR v3.2 Listing 26.
    return saturate(Pow(max(NoV + ao, 0.0f), exp2(-16.0f * roughness - 1.0f)) - 1.0f + ao);
}

//-----------------------------------------------------------------------------
//      水平スペキュラーAOを計算します.
//-----------------------------------------------------------------------------
float CalcHorizonAO(float RoN)
{
    // CalcSpecularAOがパフォーマンス的に困る場合に，簡易な近似として使用する.
    float horizon = min(1.0f + RoN, 1.0f);
    return horizon * horizon;
}

//-----------------------------------------------------------------------------
//      マイクロシャドウを計算します.
//-----------------------------------------------------------------------------
float ApplyMicroShadow(float ao, float NoL, float shadow)
{
    // See, "The Technical Art of Uncharted 4"
    float aperture = 2.0 * ao * ao;
    float microShadow = saturate(NoL + aperture - 1.0);
    return shadow * microShadow;
}

//-----------------------------------------------------------------------------
//      Lambert Diffuseを求めます.
//-----------------------------------------------------------------------------
float LambertDiffuse(float NoL)
{ return NoL / F_PI; }

//-----------------------------------------------------------------------------
//      Half-Lambert Diffuseを求めます.
//-----------------------------------------------------------------------------
float HalfLambertDiffuse(float NoL)
{
    float  v = NoL * 0.5f + 0.5f;
    return (v * v) * (3.0f / (4.0f * F_PI));
}

//-----------------------------------------------------------------------------
//      Disney Diffuseを求めます.
//-----------------------------------------------------------------------------
float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float roughness)
{
    float  energyBias   = lerp(0.0f, 0.5f, roughness);
    float  energyFactor = lerp(1.0f, 1.0f / 1.51f, roughness);
    float  fd90 = energyBias + 2.0f * LdotH * LdotH * roughness;
    float  lightScatter = F_Schlick(1.0f, fd90, NdotL).r;
    float  viewScatter  = F_Schlick(1.0f, fd90, NdotV).r;

    return lightScatter * viewScatter * energyFactor;
}

//-----------------------------------------------------------------------------
//      Phong Specularを求めます.
//-----------------------------------------------------------------------------
float PhongSpecular(float3 N, float3 V, float3 L, float shininess)
{
    float3 R = -V + (2.0f * dot(N, V) * N);
    return Pow(max(dot(L, R), 0.0f), shininess) * ((shininess + 2.0f) / (2.0 * F_PI));
}

//-----------------------------------------------------------------------------
//      SmithによるG2項を求めます.
//-----------------------------------------------------------------------------
float G2_Smith(float a, float NoL, float NoV)
{
    // [Lagarde 2014] Sebastien Lagarde, Charles de Rousier,
    // "Moving Frostbite to Physically Based Rendering 3.0",
    // p.12, Listing 2.
    float a2 = a * a;
    float GGXV = NoV * sqrt(a2 + NoL * (NoL - a2 * NoL));
    float GGXL = NoL * sqrt(a2 + NoV * (NoV - a2 * NoV));

    // NOTE: "1.0f / (4.0f * NoL * NoV)"が考慮されている値なので，
    //       返却値にこれらを掛ける必要はありません.
    return 0.5f / (GGXV + GGXL);
}

//-----------------------------------------------------------------------------
//      GGXのD項を求めます.
//-----------------------------------------------------------------------------
float D_GGX(float NoH, float a)
{
    float a2 = Pow2(a);
    float d = (NoH * a2 - NoH) * NoH + 1.0f;
    return a2 / (F_PI * Pow2(d));
}

//-----------------------------------------------------------------------------
//      D項を計算します.
//-----------------------------------------------------------------------------
float D_Ashikhmin(float linearRoughness, float NoH)
{
    // Ashkhmin 2007, "Distribution-based BRDFs".
    float a2 = linearRoughness * linearRoughness;
    float cos2h = NoH * NoH;
    float sin2h = max(1.0f - cos2h, 0.0078125); // 2^(-14/2), so sin2h^2 0 in fp16
    float sin4h = sin2h * sin2h;
    float cot2 = -cos2h / (a2 * sin2h);
    return 1.0f / (F_PI * (4.0f * a2 + 1.0f) * sin4h) * (4.0f * exp(cot2) + sin4h);
}

//-----------------------------------------------------------------------------
//      D項を計算します.
//-----------------------------------------------------------------------------
float D_Charlie(float linearRoughness, float NoH)
{
    // Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF".
    float invAlpha = 1.0f / linearRoughness;
    float cos2h = NoH * NoH;
    float sin2h = max(1.0f - cos2h, 0.0078125f); // 2^(-14/2), so sin2h^2 0 in fp16
    return (2.0f + invAlpha) * Pow(sin2h, invAlpha * 0.5f) / (2.0f * F_PI);
}

//-----------------------------------------------------------------------------
//      V項を計算します.
//-----------------------------------------------------------------------------
float V_Neubelt(float NoV, float NoL)
{
    // Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for THe Order: 1886".
    return SaturateHalf(1.0f / (4.0f * (NoL + NoV - NoL * NoV)));
}

//-----------------------------------------------------------------------------
//      布用ディフューズ項を評価します.
//-----------------------------------------------------------------------------
float3 EvaluateClothDiffuse(float3 diffuseColor, float sheen, float3 subsurfaceColor, float NoL)
{ 
    float diffuse = 1.0f / F_PI;
    diffuse *= saturate((NoL + 0.5f) / 2.25f);
    float3 result = diffuseColor * diffuse;
    result *= saturate(subsurfaceColor + NoL);
    return result;
}

//-----------------------------------------------------------------------------
//      布用スペキュラー項を評価します.
//-----------------------------------------------------------------------------
float3 EvaluateClothSpecular
(
    float   sheen,
    float   clothness,
    float   NoH,
    float   NoL,
    float   NoV
)
{
    float  D = D_Charlie(clothness, NoH);
    float  V = V_Neubelt(NoV, NoL);
    float3 F = float3(sheen, sheen, sheen);
    return (D * V * F) * NoL;
}

//-----------------------------------------------------------------------------
//      ヘアのスペキュラー項を求めます.
//-----------------------------------------------------------------------------
float ScheuermannSingleSpecularTerm(float3 T, float3 H, float exponent)
{
    // Thorsten Scheuermann, "Hair Rendering and Shading", ShaderX 3, p.244　参照.
    float ToH   = dot(T, H);
    float sinTH = sqrt(1.0f - ToH * ToH);
    return Pow(sinTH, exponent);
}

//-----------------------------------------------------------------------------
//      ヘアのスペキュラー減衰を求めます.
//-----------------------------------------------------------------------------
float ScheuermannSpecularAttenuation(float NoL)
{
    // Thorsten Scheuermann, "Hair Rendering and Shading", ShaderX 3. p.246 参照.
    return saturate(1.75f * NoL + 0.25f);
}

//-----------------------------------------------------------------------------
//      Kajiya-Keyディフューズ項を評価します.
//-----------------------------------------------------------------------------
float EvaluateScheuermannDiffuse(float NoL)
{
    // Thorsten Scheuermann, "Hair Rendering and Shading", ShaderX 3, p.243 参照.
    return saturate(0.75f * NoL + 0.25f);
}

//-----------------------------------------------------------------------------
//      Scheuermannスペキュラー項を評価します.
//-----------------------------------------------------------------------------
float3 EvaluateScheuermannSpecular
(
    float3  T,              // 接線ベクトル.
    float3  N,              // 法線ベクトル.
    float3  H,              // 視線ベクトルとライトベクトルのハーフベクトル.
    float   NoL,            // 法線とライトベクトルの内積.
    float4  specularColor0, // RGB : スペキュラーカラー, A : 反射強度.
    float4  specularColor1, // RGB : スペキュラーカラー, A : 反射強度.
    float   specularShift0, // シフト量.
    float   specularShift1, // シフト量.
    float   shiftValue,     // シフトテクスチャの値.
    float   noise           // ノイズテクスチャの値.
)
{
    float3 T0 = ShiftTangent(T, N, specularShift0 + shiftValue);
    float3 T1 = ShiftTangent(T, N, specularShift1 + shiftValue);

    float3 specular0 = specularColor0.rgb * ScheuermannSingleSpecularTerm(T0, H, specularColor0.a);
    float3 specular1 = specularColor1.rgb * ScheuermannSingleSpecularTerm(T1, H, specularColor1.a);

    specular1 *= noise;

    return (specular0 + specular1) * ScheuermannSpecularAttenuation(NoL);
}

//-----------------------------------------------------------------------------
//      Kajiya-Kay BRDFを評価します.
//-----------------------------------------------------------------------------
float3 EvaluateKajiyaKay
(
    float3  T,              // 接線ベクトル.
    float3  N,              // 法線ベクトル.
    float3  V,              // 視線ベクトル.
    float3  L,              // ライトベクトル.
    float3  Kd,             // ディフューズカラー.
    float3  Ks,             // スペキュラーカラー.
    float   noise,          // ノイズテクスチャの値.
    float   primaryScale,   // プライマリーハイライト強度.
    float   secondaryWidth  // セカンダリーハイライト幅.
)
{
    // James T. Kajiya, Timothy L. Kay, "RENDERING FUR WITH THREE DIMENSIONAL TEXTURES",
    // Computer Graphics, Volume 23, Number 3, July 1989,
    // Diffuse  は Equation (14) 参照.
    // Specular は Equation (16) 参照.

    float SpecularPower0  = 80.0f * primaryScale;
    float SpecularPower1  = max(0.04f, SpecularPower0 / secondaryWidth * 4.0f);
    float Normalize0      = (SpecularPower0 + 2.0f) / (2.0f * F_PI);
    float Normalize1      = (SpecularPower1 + 2.0f) / (2.0f * F_PI);

    float cosTL = dot(T, L);
    float sinTL = ToSin(cosTL);

    float diffuse = max(sinTL, 0.0f);
    float alpha   = radians(noise * 10.0f); // チルト角(5 - 10 度)

    float cosTRL = -cosTL;
    float sinTRL =  sinTL;
    float cosTV  = dot(T, V);
    float sinTV  = ToSin(cosTV);

    // プライマリーカラーを求める.
    float cosTRL0   = cosTRL * cos(2.0f * alpha) - sinTRL * sin(2.0f * alpha);
    float sinTRL0   = ToSin(cosTRL0);
    float specular0 = max(0, cosTRL0 * cosTV + sinTRL0 * sinTV);

    // セカンダリーカラーを求める.
    float cosTRL1   = cosTRL * cos(-3.0f * alpha) - sinTRL * sin(-3.0f * alpha);
    float sinTRL1   = ToSin(cosTRL1);
    float specular1 = max(0, cosTRL1 * cosTV + sinTRL1 * sinTV);

    // スペキュラー値.
    float power0 = Pow(specular0, SpecularPower0) * Normalize0;
    float power1 = Pow(specular1, SpecularPower1) * Normalize1;

    // レンダリング方程式の余弦項.
    float NoL = saturate(dot(N, L));

    // BRDFを評価.
    float3 fd = Kd * diffuse / F_PI;
    float3 fs = SaturateHalf(Ks * (power0 + power1) * 0.5f);  // 2灯焚いているので2で割る(=0.5を掛ける).

    return (fd + fs) * NoL;
}

//-----------------------------------------------------------------------------
//      V項を計算します.
//-----------------------------------------------------------------------------
float V_Kelemen(float LoH)
{ return 0.25f / (LoH * LoH); }

//-----------------------------------------------------------------------------
//      クリアコートのラフネスを求めます.
//-----------------------------------------------------------------------------
float GetClearCoatRoughness(float clearCoatRoughness)
{ return lerp(0.089f, 0.6f, clearCoatRoughness); }

//-----------------------------------------------------------------------------
//      クリアコートのフレネル項を求めます.
//-----------------------------------------------------------------------------
float GetClearCoatFresnel(float LoH, float clearCoat)
{ return F_Schlick(0.04f, 1.0f, LoH) * clearCoat; }

//-----------------------------------------------------------------------------
//      異方性GGXのD項を求めます.
//-----------------------------------------------------------------------------
float D_GGXAnisotropic
(
    float  at,
    float  ab,
    float  NoH,
    float3 H,
    float3 T,
    float3 B
)
{
    //float  at  = max(linearRoughness * (1.0f + anisotropy), 0.001f);
    //float  ab  = max(linearRoughness * (1.0f - anisotropy), 0.001f);
    float  ToH = dot(T, H);
    float  BoH = dot(B, H);
    float  a2  = at * ab;
    float3 v   = float3(ab * ToH, at * BoH, a2 * NoH);
    float  v2  = dot(v, v);
    float  w2  = a2 / v2;

    return a2 * w2 * w2;
}

//-----------------------------------------------------------------------------
//      異方性GGXのV項を求めます.
//-----------------------------------------------------------------------------
float V_SmithGGXHeightCorrelatedAnisotropic
(
    float at,
    float ab,
    float3 V,
    float3 L,
    float3 T,
    float3 B,
    float NoV,
    float NoL
)
{
    //float  at  = max(linearRoughness * (1.0f + anisotropy), 0.001f);
    //float  ab  = max(linearRoughness * (1.0f - anisotropy), 0.001f);
    float ToV = dot(T, V);
    float BoV = dot(B, V);
    float ToL = dot(T, L);
    float BoL = dot(B, L);

    float lambdaV = NoL * length(float3(at * ToV, ab * BoV, NoV));
    float lambdaL = NoV * length(float3(at * ToL, ab * BoL, NoL));
    float v = 0.5f / (lambdaV + lambdaL);
    return SaturateHalf(v);
}

//-----------------------------------------------------------------------------
//      計算により眼球用の高さを求めます.
//-----------------------------------------------------------------------------
float ProcedualHeightForEye(float radius, float anteriorChamberDepth)
{
    // Jorge Jimenez, Javier von der Pahlen,
    // "Next-Generation Character Rendering", GDC 2013
    // Eye Rendering セクション参照.
    //const float anteriorChamberDepth = 3.23f; // 3.23[nm] from [Lackner 2005]
    return anteriorChamberDepth * saturate(1.0f - 18.4f * radius * radius);
}

//-----------------------------------------------------------------------------
//      屈折ベクトルを計算します.
//-----------------------------------------------------------------------------
float3 CalcRefraction(float n1, float n2, float3 N, float3 V)
{
    float NdotV = dot(N, V);
    float n = (NdotV > 0.0f) ? (n1 / n2) : (n2 / n1); 

    // "Real-time Rendering Third Edition", 9.5 Refraction, p.396
    // 式(9.31), 式(9.32)参照.
    // 相対屈折率 n = (媒質1の屈折率 n1 / 媒質2の屈折率 n2).
    float w = n * NdotV;
    float k = sqrt(1.0f + (w - n) * (w + n));
    return (w - k) * N - n * V;
}

//-----------------------------------------------------------------------------
//      視差マッピングによる屈折後のテクスチャ座標を計算します.
//-----------------------------------------------------------------------------
float2 ParallaxRefraction
(
    float2      texcoord,               // テクスチャ座標.
    float       height,                 // 高さ.
    float       parallaxScale,          // 視差スケール.
    float3      viewW,                  // ワールド空間の視線ベクトル.
    float3x3    world                   // ワールド行列.
)
{
    // Jorge Jimenez, Javier von der Pahlen,
    // "Next-Generation Character Rendering", GDC 2013
    // Eye Rendering セクション参照.
    float2 viewL = mul(viewW, (float3x2)world);
    float2 offset = height * viewL;
    offset.y = -offset.y;
    return texcoord - parallaxScale * offset;
}

//-----------------------------------------------------------------------------
//      物理ベースによる屈折後のテクスチャ座標を計算します.
//-----------------------------------------------------------------------------
float2 PhysicallyBasedRefraction
(
    float2      texcoord,           // テクスチャ座標.
    float       height,             // 高さ.
    float       mask,               // 網膜から強膜への補間値.
    float       n1,                 // 媒質1の屈折率.
    float       n2,                 // 媒質2の屈折率.
    float3      normalW,            // 法線ベクトル.
    float3      viewW,              // ワールド空間での視線ベクトル.
    float3      frontNormalW,       // 眼球の視線ベクトル
    float3x3    world               // ワールド行列
)
{
    // 屈折ベクトルを求める.
    float3 refractedW = CalcRefraction(n1, n2, normalW, viewW);

    // Jorge Jimenez, Javier von der Pahlen,
    // "Next-Generation Character Rendering", GDC 2013
    // Eye Rendering セクション参照.
    float  cosAlpha = dot(frontNormalW, -refractedW);
    float  dist     = height / cosAlpha;
    float3 offsetW  = dist * refractedW;

    // ローカルに変換
    float2 offsetL = mul(offsetW, world).xy;

    return texcoord + float2(mask, -mask) * offsetL;
}

//-----------------------------------------------------------------------------
//      薄ガラスを評価します.
//-----------------------------------------------------------------------------
void EvaluateThinGlass
(
    in  float   eta,                // 屈折率.
    in  float   NoV,                // 法線と視線ベクトルの内積.
    in  float3  baseColor,          // ベースカラー.
    out float3  transmittance,      // トランスミッタンス.
    out float3  reflectance,        // リフレクタンス.
    out float3  absorptionRatio     // 吸収率.
)
{
    float sinTheta2 = 1.0f - NoV * NoV;

    const float sinRefractedTheta2 = sinTheta2 / (eta * eta);
    const float cosRefractedTheta  = sqrt(1 - sinRefractedTheta2);

    const float q0 = mad(eta, cosRefractedTheta, -NoV);
    const float q1 = mad(eta, cosRefractedTheta,  NoV);
    const float q2 = mad(eta, NoV, -cosRefractedTheta);
    const float q3 = mad(eta, NoV,  cosRefractedTheta);

    const float r0 = q0 / q1;
    const float r1 = q2 / q3;

    // 入射面におけるフレネルリフレクタンス.
    const float R0 = 0.5 * saturate(r0 * r0 + r1 * r1);
    // 入射面におけるフレネルトランスミッタンス.
    const float T0 = 1 - R0;

    const float3 R = float3(R0, R0, R0);
    const float3 T = float3(T0, T0, T0);
    const float3 C = float3(cosRefractedTheta, cosRefractedTheta, cosRefractedTheta);

    // 吸収を考慮するための係数.
    const float3 K = Pow(max(baseColor, 0.001), 1 / C);
    const float3 RK = R * K;

    transmittance   = saturate(T * T * K / (1 - RK * RK));
    reflectance     = saturate(RK  * transmittance + R);
    absorptionRatio = saturate(-(1 + RK) * transmittance + T);

    // reflectanceを出力カラーに乗算.
    // transmittanceは合成する背景色を下記式を用いて変更.
    // backGround = lerp(1.0, transmittance, alpha);
    // これを使ってアルファブレンディングする.
}

//-----------------------------------------------------------------------------
//      クリアコートの直接光を評価します.
//-----------------------------------------------------------------------------
float3 EvaluateDirectLightClearCoat
(
    float3  N,
    float3  L,
    float3  V,
    float3  Kd,
    float3  Ks,
    float   roughness,
    float   clearCoatStrength,
    float   clearCoatRoughness
)
{
    float NoV = abs(dot(N, V));
    float3 H  = normalize(V + L);
    float LoH = saturate(dot(L, H));
    float NoL = saturate(dot(N, L));
    float NoH = saturate(dot(N, H));
    float HoV = saturate(dot(H, V));
    float a2  = max(roughness * roughness, 0.01f);
    float f90 = saturate(50.0f * dot(Ks, 0.33f));

    float3 Fd = (Kd / F_PI);
    float  D = D_GGX(NoH, a2);
    float  G = G2_Smith(a2, NoL, NoV);
    float3 F = F_Schlick(Ks, f90, LoH);
    float3 Fr = (D * G * F) / (4.0f * NoV * NoL);

    float coatingPerceptualRoughness = GetClearCoatRoughness(clearCoatRoughness);
    float coatingRoughness = coatingPerceptualRoughness * coatingPerceptualRoughness;

    float  Dc  = D_GGX(NoH, coatingRoughness);
    float  Gc  = V_Kelemen(LoH);
    float  Fc  = GetClearCoatFresnel(LoH, clearCoatStrength);
    float  Frc = (Dc * Gc * Fc) / (4.0f * NoV * NoL); 
    float  t   = max(1.0f - Fc, 0.0f);

    return ((Fd + Fr * (1.0f - Fc)) * (1.0f - Fc) + Frc) * NoL;
}

//-----------------------------------------------------------------------------
//      Lambert BRDFの形状にもとづくサンプリングを行います.
//-----------------------------------------------------------------------------
float3 SampleLambert(float2 u)
{
    // PDF = NoL / F_PI
    return UniformSampleHemisphere(u);
}

//-----------------------------------------------------------------------------
//      Phong BRDFの形状にもとづくサンプリングを行います.
//-----------------------------------------------------------------------------
float3 SamplePhong(float2 u, float shininess)
{
    float phi = 2.0f * F_PI * u.x;
    float cosTheta = pow(1.0f - u.y, 1.0f / (shininess + 1.0f));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // PDF = normalizationTerm * pow(cosTheta, shininess)
    //     = ((1.0f + shininess) / (2.0f * F_PI)) * pow(cosTheta, shininess)
    return float3(
        sinTheta * cos(phi),
        sinTheta * sin(phi),
        cosTheta);
}

//-----------------------------------------------------------------------------
//      GGX BRDFの形状にもとづくサンプリングを行います.
//-----------------------------------------------------------------------------
float3 SampleGGX(float2 u, float a)
{
    // a = linearRoughness * linearRoughness とします.
    float phi = 2.0 * F_PI * u.x;
    float cosTheta = sqrt( (1.0 - u.y) / (u.y * (a * a - 1.0) + 1.0) );
    float sinTheta = sqrt( 1.0 - cosTheta * cosTheta );

    // PDF = D * NdotH;
    // Jacobian  = 1.0f / (4.0f * VdotH);
    // Final PDF = D * NdotH / (4.0f * VdotH).
    return float3(
        sinTheta * cos(phi),
        sinTheta * sin(phi),
        cosTheta);
}

//-----------------------------------------------------------------------------
//      可視法線分布を考慮したGGX BRDFの形状にもとづくサンプリングを行います.
//-----------------------------------------------------------------------------
float3 SampleVndfGGX(float2 u, float2 a, float3 wi)
{
    // a = linearRoughness * linearRoughness とします.
    float3 V = normalize(float3(wi.xy * a, wi.z));

    // [Dupuy 2023] Jonathan Dupuy, Anis Benyoub,
    // "Sampling Visible GGX Normals with Spherical Caps",
    // High-Performance Graphics 2023.
    float phi = 2.0f * F_PI * u.x;
    float z = (1.0f - u.y) * (1.0f + V.z) - V.z;
    float sinTheta = sqrt(saturate(1.0f - z * z));
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float3 H = float3(x, y, z) + V;

    H = normalize(float3(H.xy * a, H.z));

    // Final PDF = G1(N, H) * D(N) / (4.0f * NoH);
    // cf. [Dupuy 2023] Equation (20).
    return H;
}

//-----------------------------------------------------------------------------
//      バランスヒューリスティック.
//-----------------------------------------------------------------------------
float BalanceHeuristic(float nf, float pf, float ng, float pg)
{ return (nf * pf) / (nf * pf + ng * pg); }

//-----------------------------------------------------------------------------
//      パワーヒューリスティック.
//-----------------------------------------------------------------------------
float PowerHeuristic(float nf, float pf, float ng, float pg)
{ 
    float f = nf * pf;
    float g = ng * pg;
    return (f * f) / (f * f + g * g);
}

#endif//ADX_BRDF_HLSLI
