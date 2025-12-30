//-----------------------------------------------------------------------------
// File : IntegrateDFG_PS.hlsl
// Desc : Pixel Shader For Integrate DFG Term.
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

//-----------------------------------------------------------------------------
//      DFG項を積分します.
//-----------------------------------------------------------------------------
float3 IntegrateDFG_Only(float NdotV, float roughness)
{
    float3 N = float3(0.0f, 0.0f, 1.0f);
    float3 V = float3(sqrt(1.0f - NdotV * NdotV), 0.0f, NdotV);
    float  a = roughness * roughness;

    float3 acc = 0;
    const uint count = 1024;

    for(uint i=0; i<count; ++i)
    {
        float2 u     = Hammersley(i, SampleCount);
        float3 H     = SampleGGX(u, a, N);
        float3 L     = normalize(2 * dot( V, H ) * H - V);
        float  NdotL = dot(N, L);

        if (NdotL > 0.0f)
        {
            float NdotH = saturate(dot(N, H));
            float VdotH = saturate(dot(V, H));

            float G    = G2_Smith(NdotL, NdotV, roughness);
            float GVis = G * VdotH / max(NdotV * NdotH, 1e-8f);
            float Fc   = pow(1.0f - VdotH, 5.0f);

            acc.x += (1 - Fc) * GVis;
            acc.y += Fc * GVis;
        }
    }

    return acc / float(count);
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET
{
    float  NdotV     = input.TexCoord.x;
    float  roughness = input.TexCoord.y;
    float3 output    = IntegrateDFG_Only(NdotV, roughness);

    return float4(output, 1.0f);
}