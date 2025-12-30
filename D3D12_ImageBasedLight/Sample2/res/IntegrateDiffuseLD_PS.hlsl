//-----------------------------------------------------------------------------
// File : IntegrateDiffuseLD_PS.hlsl
// Desc : Pixel Shader For Integrate Diffuse LD Term.
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
    int     FaceIndex : packoffset(c0);     // キューブマップの面番号.
    float   Roughness : packoffset(c0.y);   // 線形ラフネス.
    float   Width     : packoffset(c0.z);   // 入力キューブマップのサイズ
    float   MipCount  : packoffset(c0.w);   // 入力キューブマップのミップレベル数.
};

//-----------------------------------------------------------------------------
// Textures and Samplers.
//-----------------------------------------------------------------------------
TextureCube  IBLCube : register(t0);
SamplerState IBLSmp  : register(s0);


//-----------------------------------------------------------------------------
//      ディフューズのLD項を積分します.
//-----------------------------------------------------------------------------
float3 IntegrateDiffuseCube(in float3 N, in float width, in float mipCount)
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
        float3 L = SampleLambert(u, N);

        float NdotL = saturate(dot(N, L));
        if (NdotL > 0.0f)
        {
        #ifdef ENABLE_MIPMAP_FILTERING
            // ミップマップフィルタ重点サンプリング.
            float pdf      = NdotL / F_PI;
            float omegaS   = 1.0f / max(SampleCount * pdf, 1e-8f);
            float l        = 0.5f * (log2(omegaS) - log2(omegaP)) + bias;
            float mipLevel = clamp(l, 0.0f, mipCount);

            acc += IBLCube.SampleLevel(IBLSmp, L, mipLevel).rgb;
            accWeight += 1.0f;
        #else
            acc += IBLCube.Sample(IBLSmp, L).rgb;
            accWeight += 1.0f;
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
    float3 dir    = CalcDirection(input.TexCoord, FaceIndex);
    float3 output = IntegrateDiffuseCube(dir, Width, MipCount);
    
    return float4(output, 1.0f);
}