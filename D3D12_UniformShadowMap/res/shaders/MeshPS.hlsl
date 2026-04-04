//-----------------------------------------------------------------------------
// File : MeshPS.hlsl
// Desc : Pixel Shader For Model Drawing.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "asdxBRDF.hlsli"
#include "asdxTangentSpace.hlsli"
#include "asdxSamplers.hlsli"
#include "asdxColor.hlsli"

#define DEBUG_FLAG_ALIASING_ERROR   (0x1u << 0)
#define DEBUG_FLAG_SHADOW_TEXEL     (0x1u << 1)


///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4  Position    : SV_POSITION;
    float3  Normal      : NORMAL;
    float4  Tangent     : TANGENT;
    float2  TexCoord    : TEXCOORD0;
    float4  Color       : COLOR0;
    float4  WorldPos    : WORLD_POS;
};

///////////////////////////////////////////////////////////////////////////////
// SceneParam constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer SceneParam : register(b0)
{
    float4x4    View;
    float4x4    Proj;
    float3      CameraPos;
    float       FieldOfView;
    float       NearClip;
    float       FarClip;
    float       TargetWidth;
    float       TargetHeight;
};

////////////////////////////////////////////////////////////////////////////////
// ModelParam constant buffer.
////////////////////////////////////////////////////////////////////////////////
cbuffer ModelParam : register(b1)
{
    float4x4    World;
};

///////////////////////////////////////////////////////////////////////////////
// MaterialParam constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer MaterialParam : register(b2)
{
    float3  BaseColor;
    float   Alpha;
    float   Occlusion;
    float   Roughness;
    float   Metalness;
    float   Ior;
    float3  Emissive;
    float   AlphaCutOff;
};

///////////////////////////////////////////////////////////////////////////////
// LightParam constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer LightParam : register(b3)
{
    float3 DirLightDir;
    float  DirLightIntensity;
    float3 DirLightColor;
};

///////////////////////////////////////////////////////////////////////////////
// ShadowSceneParam constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer ShadowSceneParam : register(b4)
{
    float4x4 ShadowView;
    float4x4 ShadowProj;
    uint     ShadowDebugFlags;
    float    ShadowMapSize;
    uint2    ShadowReserved0;
};

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
Texture2D BaseColorMap  : register(t0);
Texture2D NormalMap     : register(t1);
Texture2D OrmMap        : register(t2);
Texture2D EmissiveMap   : register(t3);

Texture2D   DFGMap      : register(t4);     //!< DFGマップ.
TextureCube DiffuseLD   : register(t5);     //!< Diffuse LD.
TextureCube SpecularLD  : register(t6);     //!< Specular LD.
Texture2D   ShadowMap   : register(t7);

//-----------------------------------------------------------------------------
//      ディフューズIBLを評価します.
//-----------------------------------------------------------------------------
float3 EvaluateIBLDiffuse(float3 N)
{
    // Lambert BRDFはDFG項は積分すると1.0となるので，LD項のみを返却すれば良い
    return DiffuseLD.Sample(LinearWrap, N).rgb;
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
    float           roughness       // 線形ラフネス.
)
{
    float  a = roughness * roughness;
    float3 dominantR = GetSpecularDominantDir(N, R, a);

    float2 mapSize;
    float  mipLevels;
    SpecularLD.GetDimensions(0, mapSize.x, mapSize.y, mipLevels);
    float textureSize = max(mapSize.x, mapSize.y);

    // 関数を再構築.
    // L * D * (f0 * Gvis * (1 - Fc) + Gvis * Fc) * cosTheta / (4 * NdotL * NdotV).
    NdotV = max(NdotV, 0.5f / textureSize); // ゼロ除算が発生しないようにする.
    float  mipLevel = RoughnessToMipLevel(roughness, mipLevels); 
    float3 preLD    = SpecularLD.SampleLevel(LinearWrap, dominantR, mipLevel).xyz;

    // 事前積分したDFGをサンプルする.
    // Fc = ( 1 - HdotL )^5
    // PreIntegratedDFG.r = Gvis * (1 - Fc)
    // PreIntegratedDFG.g = Gvis * Fc
    float2 preDFG   = DFGMap.SampleLevel(LinearClamp, float2(NdotV, 1.0f - roughness), 0).xy;

    // LD * (f0 * Gvis * (1 - Fc) + Gvis * Fc)
    return preLD * (f0 * preDFG.x + preDFG.y);
}

//-----------------------------------------------------------------------------
//      直接光を評価します.
//-----------------------------------------------------------------------------
float3 EvaluateDirectLight(float3 N, float3 V, float3 L, float3 Kd, float3 Ks, float roughness)
{
    float3 H     = normalize(V + L);
    float  NdotV = abs(dot(N, V));
    float  LdotH = saturate(dot(L, H));
    float  NdotH = saturate(dot(N, H));
    float  NdotL = saturate(dot(N, L));
    float  VdotH = saturate(dot(V, H));
    float  a2    = max(roughness * roughness, 0.01);
    float  f90   = saturate(50.0f * dot(Ks, 0.33f));

    float3 diffuse = Kd / F_PI;
    float  D = D_GGX(NdotH, a2);
    float  G = G2_Smith(a2, NdotL, NdotV);
    float3 F = F_Schlick(Ks, f90, LdotH);
    float3 specular = (D * G * F) / F_PI;

    return (diffuse + specular) * NdotL;
}

//-----------------------------------------------------------------------------
//      シャドウマップのテクセルサイズをデバッグ表示します.
//-----------------------------------------------------------------------------
float3 DrawGrid
(
    float3 base_color,
    float2 texcoord,
    float  grid_line_width,
    float3 grid_color,
    float  spacing
)
{
    float2 scaledUV = texcoord * spacing;
    float2 dx = float2( ddx_fine(scaledUV.x), ddy_fine(scaledUV.x) );
    float2 dy = float2( ddx_fine(scaledUV.y), ddy_fine(scaledUV.y) );
    float2 m  = frac( scaledUV );

    if ( m.x < grid_line_width * length(dx) || m.y < grid_line_width * length(dy))
    { return grid_color; }
    else
    { return base_color; }
}

//-----------------------------------------------------------------------------
//      シャドウマップのエイリアシング誤差をデバッグ表示します.
//-----------------------------------------------------------------------------
float3 VisualizeError(float4 worldPos, float2 shadowUV)
{
    float2 mapSize;
    ShadowMap.GetDimensions(mapSize.x, mapSize.y);

    float2 ds = mapSize.x * ddx_fine(shadowUV.xy);
    float2 dt = mapSize.y * ddy_fine(shadowUV.xy);
    const float s = 0.1f; // [0, 10.0]の値を[0, 1]にマッピングするので0.1倍.
    float error = max(length(ds + dt), length(ds - dt)) * s;
    return HueToRGB(saturate(error));
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET0
{
    float3 gN = normalize(input.Normal);
    float3 gT = normalize(input.Tangent.xyz);
    float3 gB = normalize(cross(gT, gN) * input.Tangent.w);
 
    float3 tN = normalize(NormalMap.Sample(LinearClamp, input.TexCoord).xyz * 2.0f - 1.0f);

    float3 N = FromTangentSpaceToWorld(tN, gT, gB, gN);
    float3 T = RecalcTangent(N, gN);
    float3 B = normalize(cross(T, N));

    float4 output = 1.0f.xxxx;

    float3 V = normalize(CameraPos - input.WorldPos.xyz); // カメラに向かう方向.
    float3 R = normalize(reflect(-V, N));
    float3 L = normalize(-DirLightDir); // ライトに向かう方向にする.
 
    float NoV = abs(dot(N, V));

    float4 bc  = BaseColorMap.Sample(LinearWrap, input.TexCoord);
    bc.rgb *= BaseColor;
    bc.a   *= Alpha;

    float3 orm = OrmMap.Sample(LinearWrap, input.TexCoord).rgb;
    orm.x *= Occlusion;
    orm.y *= Roughness;
    orm.z *= Metalness;

    float3 Kd = ToKd(bc.rgb, orm.z);
    float3 Ks = ToKs(bc.rgb, orm.z);
    
    // ビュー射影空間に変換.
    float4 shadowViewPos = mul(ShadowView, input.WorldPos);
    float4 shadowProjPos = mul(ShadowProj, shadowViewPos);

    // 正規化デバイス座標系に変換.
    shadowProjPos.xyz /= shadowProjPos.w;
 
    // テクスチャ座標系に変換.
    float2 shadowUV = shadowProjPos.xy * float2(0.5f, -0.5f) + 0.5f;

    float shadow = ShadowMap.SampleCmpLevelZero(LessEqualSampler, shadowUV, saturate(shadowProjPos.z - 0.003f));
 
    float3 lit = 0;
    lit += EvaluateDirectLight(N, V, L, Kd, Ks, orm.y) * (DirLightColor * DirLightIntensity) * shadow;
    lit += EvaluateIBLDiffuse(N) * Kd * orm.x;
    lit += EvaluateIBLSpecular(NoV, N, R, Ks, orm.y) * orm.x;
    lit += EmissiveMap.Sample(LinearWrap, input.TexCoord).xyz * Emissive;
    output.rgb = lit;
    output.a   = bc.a;

    if (output.a < AlphaCutOff)
    { discard; }
 
    // エイリアシングエラーを表示.
    if (!!(ShadowDebugFlags & DEBUG_FLAG_ALIASING_ERROR))
        output.rgb = VisualizeError(input.WorldPos, shadowUV);

    // シャドウマップテクセルを表示.
    if (!!(ShadowDebugFlags & DEBUG_FLAG_SHADOW_TEXEL))
        output.rgb = DrawGrid(output.rgb, shadowUV, 2.0f, float3(1.0f, 0.0f, 0.0f), ShadowMapSize / 16.0f); // 16 pixel ごとにグリッドを表示.

    return SaturateFloat(output);
}