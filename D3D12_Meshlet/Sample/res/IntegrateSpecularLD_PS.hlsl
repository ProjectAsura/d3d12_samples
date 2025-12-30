//-----------------------------------------------------------------------------
// File : IntegrateSpecularLD_PS.hlsl
// Desc : Pixel Shader For Integrate Specular LD Term.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "BRDF.hlsli"
#include "BakeUtil.hlsli"


///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

///////////////////////////////////////////////////////////////////////////////
// CbBake buffer
///////////////////////////////////////////////////////////////////////////////
cbuffer CbBake : register(b0)
{
    int     FaceIndex : packoffset(c0);     // 出力キューブマップの面番号.
    float   Roughness : packoffset(c0.y);   // ラフネス(= 線形ラフネス^2).
    float   Width     : packoffset(c0.z);   // 入力キューブマップのサイズ.
    float   MipCount  : packoffset(c0.w);   // 入力キューブマップのミップレベル数.
};

//-----------------------------------------------------------------------------
// Textures and Samplers.
//-----------------------------------------------------------------------------
TextureCube  IBLCube : register(t0);
SamplerState IBLSmp  : register(s0);


//-----------------------------------------------------------------------------
//      スペキュラーのLD項を積分します.
//-----------------------------------------------------------------------------
float3 IntegrateSpecularCube
(
    in float3 V,
    in float3 N,
    in float  a,
    in float  width,
    in float  mipCount
)
{
    float3 acc       = 0.0f;
    float  accWeight = 0.0f;

    float omegaP = (4.0f * F_PI) / (6.0f * width * width);
    float bias   = 1.0f;

    for(uint i=0; i<SampleCount; ++i)
    {
        // 超一様分布列を取得.
        float2 u = Hammersley(i, SampleCount);

        // BRDFにもとづく重点サンプリング.
        float3 H = SampleGGX(u, a, N);

        // ライトベクトルを求める.
        float3 L = normalize(2 * dot( V, H ) * H - V);

        float NdotL = saturate(dot(N, L));
        if (NdotL > 0)
        {
        #ifdef ENABLE_MIPMAP_FILTERING
            // ミップマップフィルタ重点サンプリング.
            float pdf      = D_GGX(NdotL, a) * NdotL;
            float omegaS   = 1.0f / max(SampleCount * pdf, 1e-8f);
            float l        = 0.5f * (log2(omegaS) - log2(omegaP)) + bias;
            float mipLevel = clamp(l, 0, mipCount);

            acc += IBLCube.SampleLevel(IBLSmp, L, mipLevel).rgb * NdotL;
            accWeight += NdotL;
        #else
            acc += IBLCube.Sample(IBLSmp, L).rgb * NdotL;
            accWeight += NdotL;
        #endif
        }
    }

    if (accWeight == 0.0f)
    { return acc; }

    return acc / accWeight;
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET
{
    float3 output = 0;
    float3 dir = CalcDirection(input.TexCoord, FaceIndex);

    if (Roughness == 0.0f)
    { output = IBLCube.SampleLevel(IBLSmp, dir, 0.0f); }
    else
    { output = IntegrateSpecularCube(dir, dir, Roughness, Width, MipCount); }

    return float4(output, 1.0f);
}
