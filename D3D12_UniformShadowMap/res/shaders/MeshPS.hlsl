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
    float4x4    World;
    float4x4    View;
    float4x4    Proj;
    float3      CameraPos;
    float       FieldOfView;
    float       NearClip;
    float       FarClip;
    float       TargetWidth;
    float       TargetHeight;
};

///////////////////////////////////////////////////////////////////////////////
// MaterialParam constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer MaterialParam : register(b1)
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
cbuffer LightParam : register(b2)
{
    float3 DirLightDir;
    float  DirLightIntensity;
    float3 DirLightColor;
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
    float  NdotV = saturate(dot(N, V));
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
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET0
{
    float3 gN = normalize(input.Normal);
    float3 gT = normalize(input.Tangent.xyz);
    float3 gB = normalize(cross(gN, gT) * input.Tangent.w);
 
    float3 tN = normalize(NormalMap.Sample(LinearClamp, input.TexCoord).xyz * 2.0f - 1.0f);

    float3 N = FromTangentSpaceToWorld(tN, gT, gB, gN);
    float3 T = RecalcTangent(N, gN);
    float3 B = cross(T, N);

    float4 output = 1.0f.xxxx;

    float3 V = normalize(input.WorldPos.xyz - CameraPos);
    float3 R = normalize(reflect(V, N));
 
    float NoV = saturate(dot(N, V));

    float4 bc  = BaseColorMap.Sample(LinearWrap, input.TexCoord);
    bc.rgb *= BaseColor;
    bc.a   *= Alpha;

    float3 orm = OrmMap.Sample(LinearWrap, input.TexCoord).rgb;
    orm.x *= Occlusion;
    orm.y *= Roughness;
    orm.z *= Metalness;

    float3 Kd = ToKd(bc.rgb, orm.z);
    float3 Ks = ToKs(bc.rgb, orm.z);
    
    float shadow = 1.0f;
 
    float3 lit = 0;
    lit += EvaluateDirectLight(N, -V, -DirLightDir, Kd, Ks, orm.y) * (DirLightColor * DirLightIntensity) * shadow;
    lit += EvaluateIBLDiffuse(N) * Kd * orm.x;
    lit += EvaluateIBLSpecular(NoV, N, R, Ks, orm.y) * orm.x;
    lit += EmissiveMap.Sample(LinearWrap, input.TexCoord).xyz * Emissive;
    output.rgb = lit;
    output.a   = bc.a;

    if (output.a < AlphaCutOff)
    { discard; }

    return output;
}