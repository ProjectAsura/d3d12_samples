//----------------------------------------------------------------------------
// File : SpericalHarmonics.hlsli
// Desc : Sperical Harmonics Utility.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#ifndef ASDX_SPHERICAL_HARMONICS_HLSLI
#define ASDX_SPHERICAL_HARMONCIS_HLSLI

///////////////////////////////////////////////////////////////////////////////
// SH2 - Spherical Harmonics L1
///////////////////////////////////////////////////////////////////////////////
struct SH2
{
    float3 Coeffient[4];    //!< 係数.
};

///////////////////////////////////////////////////////////////////////////////
// SH3 - Spherical Harmonics L2
///////////////////////////////////////////////////////////////////////////////
struct SH3
{
    float3 Coeffient[9];    //!< 係数.
};

//-----------------------------------------------------------------------------
//      方向ベクトルから球面調和関数の係数ベクトルを求めます.
//-----------------------------------------------------------------------------
void EvaluateSH_L1(float3 dir, out float sh[4])
{
    sh[0] =  0.282095f;
    sh[1] = -0.488603f * dir.y;
    sh[2] =  0.488603f * dir.z;
    sh[3] = -0.488603f * dir.x;
}

//-----------------------------------------------------------------------------
//      放射輝度から放射照度を得るためにクランプコサインローブで畳み込みます.
//-----------------------------------------------------------------------------
void ApplyDiffuseConvoluitonSH_L1(inout float sh[4])
{
    // Yuriy O'Donnel, "Precomputed Global Illumination in Forstbite",
    // GDC 2018, Slide 46.

    const float A0 = 0.886227f; // pi/sqrt(fourPi).
    const float A1 = 1.023326f; // sqrt(pi/3).
    sh[0] *= A0;
    sh[1] *= A1;
    sh[2] *= A1;
    sh[3] *= A1;
}

//-----------------------------------------------------------------------------
//      放射照度を考慮した球面調和関数の係数ベクトルを求めます.
//-----------------------------------------------------------------------------
void EvaluateDiffuseSH_L1(float3 dir, out float sh[4])
{
    // Yuriy O'Donnel, "Precomputed Global Illumination in Forstbite",
    // GDC 2018, Slide 47.

    const float AY0 = 0.25f; // A0 * Y0 = pi/sqrt(fourPi) * sqrt(1/fourPi).
    const float AY1 = 0.50f; // A1 * Y1 = sqrt(pi/3) * sqrt(3/fourPi).
    sh[0] = AY0;
    sh[1] = AY1 * dir.y;
    sh[2] = AY1 * dir.z;
    sh[3] = AY1 * dir.x;
}

//-----------------------------------------------------------------------------
//      球面調和関数の係数ベクトルから放射照度を求めます.
//-----------------------------------------------------------------------------
float IrradianceSH_L1(float3 n, float4 sh)
{
    return (0.5f + dot(sh.yzw, n)) * sh.x * 2.0f;
}

//-----------------------------------------------------------------------------
//      Geomerics方式で球面調和関数を評価します.
//-----------------------------------------------------------------------------
float IrradianceSH_NonLinearL1(float3 n, float sh[4])
{
    // William Joseph, "球面調和関数データからの拡散反射光の再現", CEDEC 2015,
    // https://cedil.cesa.or.jp/cedil_sessions/view/1329
    float  L0 = sh[0];
    float3 L1 = float3(sh[1], sh[2], sh[3]);
    float  modL1 = length(L1);
    if (modL1 == 0.0f)
    { return 0.0f; }

    float q = saturate(0.5f + 0.5f * dot(n, normalize(L1)));
    float r = modL1 / L0;
    float p = 1.0f + 2.0f * r;
    float a = (1.0f - r) / (1.0f + r);

    return L0 * lerp((1.0f + p) * pow(q, p), 1.0f, a);
}

//SH2 ToSH2(float3 dir, float3 diffuseLight)
//{
//    // SH係数を取得.
//    float sh[4];
//    EvaluateDiffuseSH_L1(dir, sh);

//    // SHに射影.
//    SH2 result = (SH2)0;
//    result.SH.r = sh * diffuseLight.r;
//    result.SH.g = sh * diffuseLight.g;
//    result.SH.b = sh * diffuseLight.b;
//    return result;
//}

//float3 FromSH2(SH2 sh, float3 normal)
//{
//    return float3(
//        IrradianceSH_L1(normal, sh.R),
//        IrradianceSH_L1(normal, sh.G),
//        IrradianceSH_L1(normal, sh.B));
//}

//float3 FromSH2NonLinear(SH2 sh, float3 normal)
//{
//    return float3(
//        IrradianceSH_NonLinearL1(normal, sh.R),
//        IrradianceSH_NonLinearL1(normal, sh.G),
//        IrradianceSH_NonLinearL1(normal, sh.B));
//}


#endif//ASDX_SPHERICAL_HARMONICS_HLSLI