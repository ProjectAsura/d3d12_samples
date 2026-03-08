//-----------------------------------------------------------------------------
// File : Math.hlsli
// Desc : Math Utility.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#ifndef ASDX_MATH_HLSLI
#define ASDX_MATH_HLSLI

// HLSLのhalf型はfloatマッピングされてしまうため，16bitのまま扱えるようにdefineする. サフィックスはhを使う.
#define half    min16float
#define half2   min16float2
#define half3   min16float3
#define half4   min16float4
#define half2x2 min16float2x2
#define half2x3 min16float2x3
#define half3x2 min16float3x2
#define half3x3 min16float3x3
#define half3x4 min16float3x4
#define half4x2 min16float4x2
#define half4x3 min16float4x3
#define half4x4 min16float4x4

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
static const float HALF_MAX  = 65504.0f;         // 半精度浮動小数の最大値.
static const float FLT_NAN   = 0x7fc00000;       // qNaN
static const float FLT_MAX   = 3.402823466e+38f; // 浮動小数の最大値.
static const float F_PI      = 3.1415926535897932384626433832795f;
static const float F_2PI     = 6.283185307179586476925286766559f;
static const float F_1DIVPI  = 0.31830988618379067153776752674503f;
static const float F_1DIV2PI = 0.15915494309189533576888376337251f;
static const float F_PIDIV2  = 1.5707963267948966192313216916398f;
static const float F_DITHER_LIST[4][4] = {
    { 0.37647f, 0.87450f, 0.50196f, 0.99000f },
    { 0.62352f, 0.12549f, 0.75294f, 0.25098f },
    { 0.43921f, 0.93725f, 0.31372f, 0.81568f },
    { 0.68627f, 0.18823f, 0.56470f, 0.06274f },
};

//-----------------------------------------------------------------------------
//      半精度浮動小数の最大値未満に飽和させます.
//-----------------------------------------------------------------------------
float SaturateHalf(float value)
{ return clamp(value, 0.0f, HALF_MAX); }

//-----------------------------------------------------------------------------
//      半精度浮動小数の最大値未満に飽和させます.
//-----------------------------------------------------------------------------
float2 SaturateHalf(float2 value)
{ return clamp(value, float(0).xx, HALF_MAX.xx); }

//-----------------------------------------------------------------------------
//      半精度浮動小数の最大値未満に飽和させます.
//-----------------------------------------------------------------------------
float3 SaturateHalf(float3 value)
{ return clamp(value, float(0).xxx, HALF_MAX.xxx); }

//-----------------------------------------------------------------------------
//      半精度浮動小数の最大値未満に飽和させます.
//-----------------------------------------------------------------------------
float4 SaturateHalf(float4 value)
{ return clamp(value, float(0).xxxx, HALF_MAX.xxxx); }

//-----------------------------------------------------------------------------
//      精度浮動小数の最大値未満に飽和させます.
//-----------------------------------------------------------------------------
float SaturateFloat(float value)
{ return clamp(value, 0.0f, FLT_MAX); }

//-----------------------------------------------------------------------------
//      単精度浮動小数の最大値未満に飽和させます.
//-----------------------------------------------------------------------------
float2 SaturateFloat(float2 value)
{ return clamp(value, float(0.0f).xx, FLT_MAX.xx); }

//-----------------------------------------------------------------------------
//      単精度浮動小数の最大値未満に飽和させます.
//-----------------------------------------------------------------------------
float3 SaturateFloat(float3 value)
{ return clamp(value, float(0.0f).xxx, FLT_MAX.xxx); }

//-----------------------------------------------------------------------------
//      単精度浮動小数の最大値未満に飽和させます.
//-----------------------------------------------------------------------------
float4 SaturateFloat(float4 value)
{ return clamp(value, float(0.0f).xxxx, FLT_MAX.xxxx); }

//-----------------------------------------------------------------------------
//      2乗計算を行います.
//-----------------------------------------------------------------------------
float Sq(float x)
{ return x * x; }

//-----------------------------------------------------------------------------
//      2乗計算を行います.
//-----------------------------------------------------------------------------
float Pow2(float x)
{ return x * x; }

//-----------------------------------------------------------------------------
//      2乗計算を行います.
//-----------------------------------------------------------------------------
float2 Pow2(float2 x)
{ return x * x; }

//-----------------------------------------------------------------------------
//      2乗計算を行います.
//-----------------------------------------------------------------------------
float3 Pow2(float3 x)
{ return x * x; }

//-----------------------------------------------------------------------------
//      2乗計算を行います.
//-----------------------------------------------------------------------------
float4 Pow2(float4 x)
{ return x * x; }

//-----------------------------------------------------------------------------
//      4乗計算を行います.
//-----------------------------------------------------------------------------
float Pow4(float x)
{
    float x2 = x * x;
    return x2 * x2;
}

//-----------------------------------------------------------------------------
//      4乗計算を行います.
//-----------------------------------------------------------------------------
float2 Pow4(float2 x)
{
    float2 x2 = x * x;
    return x2 * x2;
}

//-----------------------------------------------------------------------------
//      4乗計算を行います.
//-----------------------------------------------------------------------------
float3 Pow4(float3 x)
{
    float3 x2 = x * x;
    return x2 * x2;
}

//-----------------------------------------------------------------------------
//      4乗計算を行います.
//-----------------------------------------------------------------------------
float4 Pow4(float4 x)
{
    float4 x2 = x * x;
    return x2 * x2;
}

//-----------------------------------------------------------------------------
//      5乗計算を行います.
//-----------------------------------------------------------------------------
float Pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

//-----------------------------------------------------------------------------
//      5乗計算を行います.
//-----------------------------------------------------------------------------
float2 Pow5(float2 x)
{
    float2 x2 = x * x;
    return x2 * x2 * x;
}

//-----------------------------------------------------------------------------
//      5乗計算を行います.
//-----------------------------------------------------------------------------
float3 Pow5(float3 x)
{
    float3 x2 = x * x;
    return x2 * x2 * x;
}

//-----------------------------------------------------------------------------
//      5乗計算を行います.
//-----------------------------------------------------------------------------
float4 Pow5(float4 x)
{
    float4 x2 = x * x;
    return x2 * x2 * x;
}

//-----------------------------------------------------------------------------
//      累乗計算を行います.
//-----------------------------------------------------------------------------
float Pow(float a, float b)
{ return SaturateFloat(pow(a, b)); }

//-----------------------------------------------------------------------------
//      累乗計算を行います.
//-----------------------------------------------------------------------------
float2 Pow(float2 a, float2 b)
{ return SaturateFloat(pow(a, b)); }

//-----------------------------------------------------------------------------
//      累乗計算を行います.
//-----------------------------------------------------------------------------
float3 Pow(float3 a, float3 b)
{ return SaturateFloat(pow(a, b)); }

//-----------------------------------------------------------------------------
//      累乗計算を行います.
//-----------------------------------------------------------------------------
float4 Pow(float4 a, float4 b)
{ return SaturateFloat(pow(a, b)); }

//-----------------------------------------------------------------------------
//      再マッピングします.
//-----------------------------------------------------------------------------
float Remap(float value, float oldMin, float oldMax, float newMin, float newMax)
{ return clamp((value - oldMin) * ((newMax - newMin) / (oldMax - oldMin)) + newMin, newMin, newMax); }

//-----------------------------------------------------------------------------
//      再マッピングします.
//-----------------------------------------------------------------------------
float2 Remap(float2 value, float2 oldMin, float2 oldMax, float2 newMin, float2 newMax)
{ return clamp((value - oldMin) * ((newMax - newMin) / (oldMax - oldMin)) + newMin, newMin, newMax); }

//-----------------------------------------------------------------------------
//      再マッピングします.
//-----------------------------------------------------------------------------
float3 Remap(float3 value, float3 oldMin, float3 oldMax, float3 newMin, float3 newMax)
{ return clamp((value - oldMin) * ((newMax - newMin) / (oldMax - oldMin)) + newMin, newMin, newMax); }

//-----------------------------------------------------------------------------
//      再マッピングします.
//-----------------------------------------------------------------------------
float4 Remap(float4 value, float4 oldMin, float4 oldMax, float4 newMin, float4 newMax)
{ return clamp((value - oldMin) * ((newMax - newMin) / (oldMax - oldMin)) + newMin, newMin, newMax); }

//-----------------------------------------------------------------------------
//      最大コンポーネントの値を取得します.
//-----------------------------------------------------------------------------
float Max2(float2 value)
{ return max(value.x, value.y); }

//-----------------------------------------------------------------------------
//      最大コンポーネントの値を取得します.
//-----------------------------------------------------------------------------
float Max3(float3 value)
{ return max(value.x, max(value.y, value.z)); }

//-----------------------------------------------------------------------------
//      最大コンポーネントの値を取得します.
//-----------------------------------------------------------------------------
float Max4(float4 value)
{ return max(value.x, max(value.y, max(value.z, value.w))); }

//-----------------------------------------------------------------------------
//      最小コンポーネントの値を取得します.
//-----------------------------------------------------------------------------
float Min2(float2 value)
{ return min(value.x, value.y); }

//-----------------------------------------------------------------------------
//      最小コンポーネントの値を取得します.
//-----------------------------------------------------------------------------
float Min3(float3 value)
{ return min(value.x, min(value.y, value.z)); }

//-----------------------------------------------------------------------------
//      最小コンポーネントの値を取得します.
//-----------------------------------------------------------------------------
float Min4(float4 value)
{ return min(value.x, min(value.y, min(value.z, value.w))); }

//-----------------------------------------------------------------------------
//      スロープ関数.
//-----------------------------------------------------------------------------
float LinearStep(float edge0, float edge1, float x)
{ return (x - edge0) / (edge1 - edge0); }

//-----------------------------------------------------------------------------
//      スロープ関数.
//-----------------------------------------------------------------------------
float2 LinearStep(float2 edge0, float2 edge1, float2 x)
{ return (x - edge0) / (edge1 - edge0); }

//-----------------------------------------------------------------------------
//      スロープ関数.
//-----------------------------------------------------------------------------
float3 LinearStep(float3 edge0, float3 edge1, float3 x)
{ return (x - edge0) / (edge1 - edge0); }

//-----------------------------------------------------------------------------
//      スロープ関数.
//-----------------------------------------------------------------------------
float4 LinearStep(float4 edge0, float4 edge1, float4 x)
{ return (x - edge0) / (edge1 - edge0); }

//-----------------------------------------------------------------------------
//      3次エルミート補間.
//-----------------------------------------------------------------------------
float SmoothStep(float a, float b, float x)
{
    x = LinearStep(a, b, x);
    return (x * x * (3.0f - 2.0f * x));
}

//-----------------------------------------------------------------------------
//      3次エルミート補間.
//-----------------------------------------------------------------------------
float2 SmoothStep(float2 a, float2 b, float2 x)
{
    x = LinearStep(a, b, x);
    return (x * x * (3.0f - 2.0f * x));
}

//-----------------------------------------------------------------------------
//      3次エルミート補間.
//-----------------------------------------------------------------------------
float3 SmoothStep(float3 a, float3 b, float3 x)
{
    x = LinearStep(a, b, x);
    return (x * x * (3.0f - 2.0f * x));
}

//-----------------------------------------------------------------------------
//      3次エルミート補間.
//-----------------------------------------------------------------------------
float4 SmoothStep(float4 a, float4 b, float4 x)
{
    x = LinearStep(a, b, x);
    return (x * x * (3.0f - 2.0f * x));
}

//-----------------------------------------------------------------------------
//      5次エルミート補間.
//-----------------------------------------------------------------------------
float SmootherStep(float a, float b, float x)
{
    x = LinearStep(a, b, x);
    return (x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f));
}

//-----------------------------------------------------------------------------
//      5次エルミート補間.
//-----------------------------------------------------------------------------
float2 SmootherStep(float2 a, float2 b, float2 x)
{
    x = LinearStep(a, b, x);
    return (x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f));
}

//-----------------------------------------------------------------------------
//      5次エルミート補間.
//-----------------------------------------------------------------------------
float3 SmootherStep(float3 a, float3 b, float3 x)
{
    x = LinearStep(a, b, x);
    return (x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f));
}

//-----------------------------------------------------------------------------
//      5次エルミート補間.
//-----------------------------------------------------------------------------
float4 SmootherStep(float4 a, float4 b, float4 x)
{
    x = LinearStep(a, b, x);
    return (x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f));
}

//-----------------------------------------------------------------------------
//      [0, 1]のスロープ関数.
//-----------------------------------------------------------------------------
float linearstep(float edge0, float edge1, float x)
{ return saturate(LinearStep(edge0, edge1, x)); }

//-----------------------------------------------------------------------------
//      [0, 1]のスロープ関数.
//-----------------------------------------------------------------------------
float2 linearstep(float2 edge0, float2 edge1, float2 x)
{ return saturate(LinearStep(edge0, edge1, x)); }

//-----------------------------------------------------------------------------
//      [0, 1]のスロープ関数.
//-----------------------------------------------------------------------------
float3 linearstep(float3 edge0, float3 edge1, float3 x)
{ return saturate(LinearStep(edge0, edge1, x)); }

//-----------------------------------------------------------------------------
//      [0, 1]のスロープ関数.
//-----------------------------------------------------------------------------
float4 linearstep(float4 edge0, float4 edge1, float4 x)
{ return saturate(LinearStep(edge0, edge1, x)); }

//-----------------------------------------------------------------------------
//      [0, 1]の5次エルミート補間.
//-----------------------------------------------------------------------------
float smootherstep(float a, float b, float x)
{
    x = saturate(LinearStep(a, b, x));
    return (x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f));
}

//-----------------------------------------------------------------------------
//      [0, 1]の5次エルミート補間.
//-----------------------------------------------------------------------------
float2 smootherstep(float2 a, float2 b, float2 x)
{
    x = saturate(LinearStep(a, b, x));
    return (x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f));
}

//-----------------------------------------------------------------------------
//      [0, 1]範囲の5次エルミート補間.
//-----------------------------------------------------------------------------
float3 smootherstep(float3 a, float3 b, float3 x)
{
    x = saturate(LinearStep(a, b, x));
    return (x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f));
}

//-----------------------------------------------------------------------------
//      [0, 1]範囲の5次エルミート補間.
//-----------------------------------------------------------------------------
float4 smootherstep(float4 a, float4 b, float4 x)
{
    x = saturate(LinearStep(a, b, x));
    return (x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f));
 }

//-----------------------------------------------------------------------------
//      平方根を近似計算します.
//-----------------------------------------------------------------------------
float FastSqrt(float x)
{
    // [Drobot2014a] Low Level Optimizations for GCN
    // https://blog.selfshadow.com/publications/s2016-shading-course/activision/s2016_pbs_activision_occlusion.pdf slide 63
    return asfloat(0x1fbd1df5 + (asint(x) >> 1));
}

//-----------------------------------------------------------------------------
//      アークコサインを近似計算します.
//-----------------------------------------------------------------------------
float FastAcos(float x)
{
    // [Lagarde 2014] Sebastien Lagarde
    // "Inverse trigonometric functions GPU optimization for AMD GCN architecture"
    // https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
    
    // max absolute error 9.0x10^-3
    // Eberly's polynomial degree 1 - respect bounds
    // 4 VGPR, 12 FR (8 FR, 1 QR), 1 scalar
    // input [-1, 1] and output [0, PI]
    x = abs(x);
    float ret = -0.156583 * x + F_PIDIV2;
    ret *= FastSqrt(1.0 - x);
    return (x >= 0) ? ret : F_PI - ret;
}

//-----------------------------------------------------------------------------
//      アークサインを近似計算します.
//-----------------------------------------------------------------------------
float FastAsin(float x)
{
    // [Lagarde 2014] Sebastien Lagarde
    // "Inverse trigonometric functions GPU optimization for AMD GCN architecture"
    // https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
    
    // Same cost as Acos + 1 FR
    // Same error
    // input [-1, 1] and output [-PI/2, PI/2]
    return F_PIDIV2 - FastAcos(x);
}

//-----------------------------------------------------------------------------
//      正の値のアークタンジェントを近似計算します.
//-----------------------------------------------------------------------------
float FastAtanPos(float x)
{
    // [Lagarde 2014] Sebastien Lagarde
    // "Inverse trigonometric functions GPU optimization for AMD GCN architecture"
    // https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
    
    // max absolute error 1.3x10^-3
    // Eberly's odd polynomial degree 5 - respect bounds
    // 4 VGPR, 14 FR (10 FR, 1 QR), 2 scalar
    // input [0, infinity] and output [0, PI/2]
    float t0 = (x < 1.0f) ? x : 1.0f / x;
    float t1 = t0 * t0;
    float poly = 0.0872929f;
    poly = -0.301895f + poly * t1;
    poly = 1.0f + poly * t1;
    poly = poly * t0;
    return (x < 1.0f) ? poly : F_PIDIV2 - poly;
}

//-----------------------------------------------------------------------------
//      アークタンジェントを近似計算します.
//-----------------------------------------------------------------------------
float FastAtan(float x)
{
    // [Lagarde 2014] Sebastien Lagarde
    // "Inverse trigonometric functions GPU optimization for AMD GCN architecture"
    // https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/

    // 4 VGPR, 16 FR (12 FR, 1 QR), 2 scalar
    // input [-infinity, infinity] and output [-PI/2, PI/2]
    float t0 = FastAtanPos(abs(x));
    return (x < 0.0f) ? -t0 : t0;
}

//-----------------------------------------------------------------------------
//      RGBE形式に圧縮します.
//-----------------------------------------------------------------------------
float4 EncodeHDR(float3 value)
{
    value = 65536.0f;
    float3 exponent  = clamp(ceil(log2(value)), -128.0f, 127.0f);
    float  component = Max3(exponent);
    float  range     = exp2(component);
    float3 mantissa  = saturate(value / range);
    return float4(mantissa, (component + 128.0f) / 256.0f);
}

//-----------------------------------------------------------------------------
//      RGBE形式を展開します.
//-----------------------------------------------------------------------------
float3 DecodeHDR(float4 value)
{
    float  exponent = value.a * 256.0f - 128.0f;
    float3 mantissa = value.rgb;
    return exp2(exponent) * mantissa * 65536.0f;
}

//-----------------------------------------------------------------------------
//      接線空間からワールド空間に変換します.
//-----------------------------------------------------------------------------
float3 FromTangentSpaceToWorld(float3 value, float3 T, float3 B, float3 N)
{ return normalize(value.x * T + value.y * B + value.z * N); }

//-----------------------------------------------------------------------------
//      八面体ラップ処理を行います.
//-----------------------------------------------------------------------------
float2 OctWrap(float2 v)
{
#if __HLSL_VERSION >= 2021
    return (1.0f - abs(v.yx)) * select(v.xy >= 0.0f, 1.0f, -1.0f);
#else
    return (1.0f - abs(v.yx)) * (v.xy >= 0.0f ? 1.0f : -1.0f);
#endif
}

//-----------------------------------------------------------------------------
//      法線ベクトルをパッキングします.
//-----------------------------------------------------------------------------
float2 PackNormal(float3 normal)
{
    // Octahedron normal vector encoding.
    // https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
    float3 n = normal / (abs(normal.x) + abs(normal.y) + abs(normal.z));
#if __HLSL_VERSION >= 2021
    n.xy = select(n.z >= 0.0f, n.xy, OctWrap(n.xy));
#else
    n.xy = (n.z >= 0.0f) ? n.xy : OctWrap(n.xy);
#endif
    return n.xy * 0.5f + 0.5f;
}

//-----------------------------------------------------------------------------
//      法線ベクトルをアンパッキングします.
//-----------------------------------------------------------------------------
float3 UnpackNormal(float2 packed)
{
    // Octahedron normal vector encoding.
    // https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
    float2 encoded = packed * 2.0f - 1.0f;
    float3 n = float3(encoded.x, encoded.y, 1.0f - abs(encoded.x) - abs(encoded.y));
    float  t = saturate(-n.z);
#if __HLSL_VERSION >= 2021
    n.xy += select(n.xy >= 0.0f, -t, t);
#else
    n.xy += (n.xy >= 0.0f) ? -t : t;
#endif
    return normalize(n);
}

//-----------------------------------------------------------------------------
//      ハードウェアから出力された深度を線形深度に変換します.
//-----------------------------------------------------------------------------
float ToLinearDepth(float hardwareDepth, float nearClip, float farClip)
{ return nearClip / (hardwareDepth * (nearClip - farClip) + farClip); }

//-----------------------------------------------------------------------------
//      ハードウェアから出力された深度をビュー空間深度に変換します.
//-----------------------------------------------------------------------------
float ToViewDepth(float hardwareDepth, float nearClip, float farClip)
{ return -nearClip * farClip / (hardwareDepth * (nearClip - farClip) + farClip); }

//-----------------------------------------------------------------------------
//      ハードウェアから出力されたリバース深度を線形深度に変換します.
//-----------------------------------------------------------------------------
float ToLinearDepthFromReverseZ(float hardwareDepth, float nearClip, float farClip)
{ return -nearClip / (hardwareDepth * (nearClip - farClip) + nearClip); }

//-----------------------------------------------------------------------------
//      ハードウェアから出力されたリバース深度をビュー空間深度に変換します.
//-----------------------------------------------------------------------------
float ToViewDepthFromReverseZ(float hardwareDepth, float nearClip, float farClip)
{ return -farClip * nearClip / (hardwareDepth * (nearClip - farClip) + nearClip); }

//-----------------------------------------------------------------------------
//      farClip = ∞とするリバースZ深度バッファの値からビュー空間深度値を求めます.
//-----------------------------------------------------------------------------
float ToViewDepthFromReverseZ(float hardwareDepth, float nearClip)
{ return -nearClip / hardwareDepth; }

//-----------------------------------------------------------------------------
//      ビュー空間深度から線形深度に変換します.
//-----------------------------------------------------------------------------
float ToLinearDepth(float viewDepth, float farClip)
{ return viewDepth / farClip; }

//-----------------------------------------------------------------------------
//      線形深度からビュー空間深度に変換します.
//-----------------------------------------------------------------------------
float ToViewDepth(float linearDepth, float farClip)
{ return linearDepth * farClip; }

//-----------------------------------------------------------------------------
//      ビュー空間から射影空間に変換します.
//-----------------------------------------------------------------------------
float3 ToProjPos(float3 viewPos, float4 param)
{
    // param.x = 1.0f / proj[0][0];
    // param.y = 1.0f / proj[1][1];
    // param.z = nearClip;
    // param.w = farClip;
    return float3(
        -viewPos.x / (viewPos.z * param.x),
        -viewPos.y / (viewPos.z * param.y),
        -(param.w / (param.z - param.w)) * (1.0f + (param.z / viewPos.z)));
}

//-----------------------------------------------------------------------------
//      ビュー空間位置からテクスチャ座標を求めます.
//-----------------------------------------------------------------------------
float2 ToTexCoord(float3 viewPos, float2 param)
{
    // param.x = 1.0f / proj[0][0];
    // param.y = 1.0f / proj[1][1];
    float2 p;
    p.x = (-viewPos.x / (viewPos.z * param.x)) * ( 0.5f) + 0.5f;
    p.y = (-viewPos.y / (viewPos.z * param.y)) * (-0.5f) + 0.5f;
    return p;
}

//-----------------------------------------------------------------------------
//      ビュー空間位置を求めます.
//-----------------------------------------------------------------------------
float3 ToViewPos(float2 uv, float viewZ, float2 param)
{
    // param.x = 1.0f / proj[0][0];
    // param.y = 1.0f / proj[1][1];
    float2 p = uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    p *= -viewZ * param;
    return float3(p, viewZ);
}

//-----------------------------------------------------------------------------
//      最小となる差分値を求めます.
//-----------------------------------------------------------------------------
float3 MinDiff(float3 p, float3 r, float3 l)
{
    float3 v1 = r - p;
    float3 v2 = p - l;
#if __HLSL_VERSION >= 2021
    return select(dot(v1, v1) < dot(v2, v2),  v1, v2);
#else
    return (dot(v1, v1) < dot(v2, v2)) ? v1 : v2;
#endif
}

//-----------------------------------------------------------------------------
//      法線ベクトルを再構築します.
//-----------------------------------------------------------------------------
float3 ToNormal(float3 p0, float3 pr, float3 pl, float3 pt, float3 pb)
{ 
    // p0 : 中心位置.
    // pr : p0 + (1, 0);
    // pl : p0 - (1, 0);
    // pt : p0 + (0, 1);
    // pb : p0 - (0, 1);
    return normalize(cross(MinDiff(p0, pr, pl), MinDiff(p0, pt, pb)));
}

//-----------------------------------------------------------------------------
//      Hammersleyサンプリング.
//-----------------------------------------------------------------------------
float2 Hammersley(uint i, uint N)
{
    float ri = reversebits(i) * 2.3283064365386963e-10f;
    return float2(float(i) / float(N), ri);
}

//-----------------------------------------------------------------------------
//      R1数列.
//-----------------------------------------------------------------------------
float R1Sequence(float number)
{
    // Martin Roberts, "The Unreasonable Effectiveness of Quasirandom Sequences", 2018
    // http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/

    // g  = 1.6180339887498948482;
    // a1 = 1 / g = 0.6180339887498948482
    return frac(0.5f + number * 0.6180339887498948482);
}

//-----------------------------------------------------------------------------
//      R2数列.
//-----------------------------------------------------------------------------
float2 R2Sequence(float number)
{
    // g = 1.32471795724474602596
    // a1 = 1 / g       = 0.75487766624669276005
    // a2 = 1 / (g * g) = 0.5698402909980532659114;
    return frac(0.5f.xx + number * float2(0.75487766624669276005, 0.5698402909980532659114));
}

//-----------------------------------------------------------------------------
//      法線マップを合成します.
//-----------------------------------------------------------------------------
float3 BlendNormal(float3 n1, float3 n2)
{
    float3 t = n1 * float3( 2.0f,  2.0f, 2.0f) + float3(-1.0f, -1.0f,  0.0f);
    float3 u = n2 * float3(-2.0f, -2.0f, 2.0f) + float3( 1.0f,  1.0f, -1.0f);
    float3 r = t * dot(t, u) - u * t.z;
    return normalize(r);
}

//-----------------------------------------------------------------------------
//      ベント反射ベクトルを計算します.
//-----------------------------------------------------------------------------
float3 BentReflection(float3 T, float3 B, float3 N, float3 V, float anisotropy)
{
#if __HLSL_VERSION >= 2021
    float3 anisotropicDirection = select(anisotropy >= 0.0f,  B, T);
#else
    float3 anisotropicDirection = (anisotropy >= 0.0f) ? B : T;
#endif
    float3 anisotropicTangent   = cross(anisotropicDirection, V);
    float3 anisotropicNormal    = cross(anisotropicTangent, anisotropicDirection);
    float3 bentNormal           = normalize(lerp(N, anisotropicNormal, anisotropy));
    return reflect(V, bentNormal);
}

//-----------------------------------------------------------------------------
//      余弦の値から正弦を求めます.
//-----------------------------------------------------------------------------
float ToSin(float cosine)
{ return sqrt(1.0f - cosine * cosine); }

//-----------------------------------------------------------------------------
//      接線をシフトします.
//-----------------------------------------------------------------------------
float3 ShiftTangent(float3 T, float3 N, float shiftAngle)
{
    // ShaderX 3の式ではなく，数学的に正しく回転を扱う式に変更.
    float cosTX = cos(shiftAngle);
    float sinTX = sqrt(1.0f - cosTX * cosTX);
    float cosTN = dot(T, N);
    float sinTN = sqrt(1.0f - cosTN * cosTN);
    float3 X = (cosTX * sinTN - cosTN * sinTX) * T + sinTN * sinTX * N;
    return normalize(X);
}

//-----------------------------------------------------------------------------
//      疑似油膜表現.
//-----------------------------------------------------------------------------
float3 FakeFilm(float3 V, float3 N, float mask, float thickness, float ior)
{
    // 高木康行, "モンスターハンター：ワールド アーティストによるシェーダ作成", CEDEC 2018.
    float cos0 = abs(dot(V, N));

    cos0 *= mask;
    float  tr = cos0 * thickness - ior;
    float3 n_color = (cos((tr * 35.0) * float3(0.71,0.87,1.0)) * -0.5) + 0.5;
    n_color = lerp(n_color, float3(0.5, 0.5, 0.5), tr);
    n_color *= n_color*2.0f;
    return n_color;
}

//-----------------------------------------------------------------------------
//      ヒートマップの色を取得します.
//-----------------------------------------------------------------------------
float3 HeatMap(float v) 
{
    float3 r = v * 2.1f - float3(1.8f, 1.14f, 0.3f);
    return 1.0f - r * r;
}

//-----------------------------------------------------------------------------
//      三角ノイズを計算します.
//-----------------------------------------------------------------------------
float TriangleNoise(float2 n, float time)
{
    // triangle noise, in [-1.0..1.0[ range
    float v = 0.07 * frac(time);
    n += float2(v, v);
    n  = frac(n * float2(5.3987, 5.4421));
    n += dot(n.yx, n.xy + float2(21.5351, 14.3137));

    float xy = n.x * n.y;
    // compute in [0..2[ and remap to [-1.0..1.0[
    return frac(xy * 95.4307) + frac(xy * 75.04961) - 1.0;
}

//-----------------------------------------------------------------------------
//      グラディエントノイズを計算します.
//-----------------------------------------------------------------------------
float InterleavedGradientNoise(const float2 n)
{ return frac(52.982919 * frac(dot(float2(0.06711, 0.00584), n))); }

//-----------------------------------------------------------------------------
//      Jimenezによるディザーを計算します.
//-----------------------------------------------------------------------------
float4 DitherJimenez(float2 uv, float time, float4 rgba)
{
    // Jimenez 2014, "Next Generation Post-Processing in Call of Duty"
    float noise = InterleavedGradientNoise(uv.xy + time);
    // remap from [0..1[ to [-1..1[
    noise = (noise * 2.0) - 1.0;
    return float4(rgba.rgb + noise / 255.0, rgba.a);
}

//------------------------------------------------------------------------------
//      Gjølによるディザーを計算します.
//------------------------------------------------------------------------------
float4 DitherTriangleNoise(float4 rgba, float2 uv, float2 screenSize, float time)
{
    // Gjøl 2016, "Banding in Games: A Noisy Rant", http://loopit.dk/banding_in_games.pdf.
    return rgba + TriangleNoise(uv * screenSize, time) / 255.0;
}

//-----------------------------------------------------------------------------
//      GjølによるRGBディザーを計算します.
//-----------------------------------------------------------------------------
float4 DitherTriangleNoiseRGB(float4 rgba, float2 uv, float2 screenSize, float time)
{
    // Gjøl 2016, "Banding in Games: A Noisy Rant", http://loopit.dk/banding_in_games.pdf.
    float2 st = uv * screenSize;
    float3 dither = float3(
            TriangleNoise(st, time),
            TriangleNoise(st + 0.1337, time),
            TriangleNoise(st + 0.3141, time)) / 255.0;
    return float4(rgba.rgb + dither, rgba.a + dither.x);
}


//-----------------------------------------------------------------------------
//      接線を再計算します.
//-----------------------------------------------------------------------------
float3 RecalcTangent(float3 normalMappedN, float3 T)
{
    // Johon Isidoro, Chris Brenman, "Per-Pixel Strand Based Anisotropic Lighting",
    // Direct3D ShaderX Vertex and Pixel Shader Tips and Tricks, pp.376-382, Wordware Publishing Inc.
    return normalize(T - dot(T, normalMappedN) * normalMappedN);
}

//-----------------------------------------------------------------------------
//      疑似乱数を生成します.
//-----------------------------------------------------------------------------
float Random(float2 value)
{ return frac(sin(dot(value.xy, float2(12.9898, 78.233))) * 43758.5453); }

//-----------------------------------------------------------------------------
//      バリューノイズを生成します.
//-----------------------------------------------------------------------------
float ValueNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    
    float2 s = smoothstep(0.0, 1.0, f);
    float nx0 = lerp(Random(i + float2(0.0, 0.0)), Random(i + float2(1.0, 0.0)), s.x);
    float nx1 = lerp(Random(i + float2(0.0, 1.0)), Random(i + float2(1.0, 1.0)), s.x);
    return lerp(nx0, nx1, s.y);
}

//-----------------------------------------------------------------------------
//      オーバーレイします.
//-----------------------------------------------------------------------------
float Overlay(float v0, float v1)
{ return v0 * (v0 + 2.f * v1 * (1.f - v0)); }

//-----------------------------------------------------------------------------
//      オーバーレイします.
//-----------------------------------------------------------------------------
float2 Overlay(float2 v0, float2 v1)
{ return v0 * (v0 + 2.f.xx * v1 * (1.f.xx - v0)); }

//-----------------------------------------------------------------------------
//      オーバーレイします.
//-----------------------------------------------------------------------------
float3 Overlay(float3 v0, float3 v1)
{ return v0 * (v0 + 2.f.xxx * v1 * (1.f.xxx - v0)); }

//-----------------------------------------------------------------------------
//      オーバーレイします.
//-----------------------------------------------------------------------------
float4 Overlay(float4 v0, float4 v1)
{ return v0 * (v0 + 2.f.xxxx * v1 * (1.f.xxxx - v0)); }

//-----------------------------------------------------------------------------
//      3要素の合計を求めます.
//-----------------------------------------------------------------------------
float Sum(float3 v)
{ return v.x + v.y + v.z; }

//-----------------------------------------------------------------------------
//      4要素の合計を求めます.
//-----------------------------------------------------------------------------
float Sum(float4 v)
{ return v.x + v.y + v.z + v.w; }

//-----------------------------------------------------------------------------
//      UVアニメーションを行います.
//-----------------------------------------------------------------------------
float2 UVAnimation(float2 uv, float2 scale, float2 offset, float rotate)
{
    float2 st = uv * scale;
    float s, c;
    sincos(rotate, s, c);

    float2 temp = st;
    st.x = temp.x * c - temp.y * s;
    st.y = temp.x * s + temp.y * c;
    st += offset;
    return st;
}

//-----------------------------------------------------------------------------
//        Toksvigフィルタを適用します.
//-----------------------------------------------------------------------------
float ToksvigRoughness(float3 normal, float roughness)
{
    float length_normal = length(normal);
    float shininess     = 1.0f - roughness;
    float toksvig_shininess = length_normal * shininess / (length_normal + shininess * (1.0f - length_normal));
    return 1.0f - toksvig_shininess;
}

//-----------------------------------------------------------------------------
//      Tokuyoshi-Kaplanyanフィルタを適用します.
//-----------------------------------------------------------------------------
float TokuyoshiRoughness(float3 normal, float roughness, float sigma2, float kappa)
{
    // Yusuke Tokuyoshi and Anton S. Kaplanyan, "Improved Geometric Specular Antialiasing",
    // ACM SIGGRAPH Symposium on Interactive 3D Graphics and Games 2019, 
    // sigma2 : screen-space variance.
    // kappa  : clamping threshold.
    // ※ sigma^2 = 0.25, kappa = 0.18 in the paper.
    float3 dndu = ddx(normal);
    float3 dndv = ddy(normal);
    float variance = sigma2 * (dot(dndu, dndu) + dot(dndv, dndv));
    float kernelRoughness2 = min(2.0f * variance, kappa);
    return sqrt(saturate(roughness * roughness + kernelRoughness2));
}

//-----------------------------------------------------------------------------
//      リニアからSRGBへの変換.
//-----------------------------------------------------------------------------
float3 Linear_To_SRGB(float3 color)
{
    float3 result;
    result.x  = (color.x < 0.0031308) ? 12.92 * color.x : 1.055 * pow(abs(color.x), 1.0f / 2.4) - 0.05f;
    result.y  = (color.y < 0.0031308) ? 12.92 * color.y : 1.055 * pow(abs(color.y), 1.0f / 2.4) - 0.05f;
    result.z  = (color.z < 0.0031308) ? 12.92 * color.z : 1.055 * pow(abs(color.z), 1.0f / 2.4) - 0.05f;

    return result;
}

//-----------------------------------------------------------------------------
//      SRGBからリニアへの変換.
//-----------------------------------------------------------------------------
float3 SRGB_To_Linear(float3 color)
{
    float3 result;
    result.x = (color.x < 0.0405f) ? color.x / 12.92f : pow((abs(color.x) + 0.055) / 1.055f, 2.4f);
    result.y = (color.y < 0.0405f) ? color.y / 12.92f : pow((abs(color.y) + 0.055) / 1.055f, 2.4f);
    result.z = (color.z < 0.0405f) ? color.z / 12.92f : pow((abs(color.z) + 0.055) / 1.055f, 2.4f);
    
    return result;
}

//-----------------------------------------------------------------------------
//      BT.601での輝度値を求めます.
//-----------------------------------------------------------------------------
float LuminanceBT601(float3 value)
{ return dot(value, float3(0.299f, 0.587f, 0.114f)); }

//-----------------------------------------------------------------------------
//      BT.709での輝度値を求めます.
//-----------------------------------------------------------------------------
float LuminanceBT709(float3 value)
{ return dot(value, float3(0.2126f, 0.7152f, 0.0722f)); }

//-----------------------------------------------------------------------------
//      HLGでの輝度値を求めます.
//-----------------------------------------------------------------------------
float LuminanceHLG(float3 value)
{ return dot(value, float3(0.2627f, 0.6780f, 0.0593f)); }

//-----------------------------------------------------------------------------
//      ITU-R BT.709からAdobeRGBへの変換.
//-----------------------------------------------------------------------------
float3 BT709_To_AdobeRGB(float3 color)
{
    const float3x3 conversion = 
    {
        0.715126f, 0.284874f, -0.000000f,
        0.000000f, 1.000000f, 0.000000f,
        0.000000f, 0.041162f, 0.958838f
    };
    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      ITU-R BT.709からDCI-P3への変換.
//-----------------------------------------------------------------------------
float3 BT709_To_DCI_P3(float3 color)
{
    const float3x3 conversion =
    {
        0.898952f, 0.194049f, 0.000000f,
        0.031821f, 0.926804f, 0.000000f,
        0.019654f, 0.083296f, 1.047586f
    };
    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      ITU-R BT.709からITU-R BT.2020への変換.
//-----------------------------------------------------------------------------
float3 BT709_To_BT2020(float3 color)
{
    const float3x3 conversion =
    {
          0.627404f, 0.329283f, 0.043313f,
          0.069097f, 0.919540f, 0.011362f,
          0.016391f, 0.088013f, 0.895595f
     };
     return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      ITU-R BT.709からCIE 1931 XYZ表色系への変換.
//-----------------------------------------------------------------------------
float3 BT709_To_XYZ(float3 color)
{
    const float3x3 conversion = 
    {
        0.412391f, 0.357584f, 0.180481f,
        0.212639f, 0.715169f, 0.072192f,
        0.019331f, 0.119195f, 0.950532f
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      ITU-R BT.20202からCIE 1931 XYZ表色系への変換.
//-----------------------------------------------------------------------------
float3 BT2020_To_XYZ(float3 color)
{
    const float3x3 conversion = 
    {
        0.636958f, 0.144617f, 0.168881f,
        0.262700f, 0.677998f, 0.059302f,
        0.000000f, 0.028073f, 1.060985f
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      DCI-P3からCIE 1931 XYZ表色系への変換.
//-----------------------------------------------------------------------------
float3 DCI_P3_To_XYZ(float3 color)
{
    const float3x3 conversion = 
    {
        0.445170f, 0.277134f, 0.172283f,
        0.209492f, 0.721595f, 0.068913f,
        0.000000f, 0.047061f, 0.907355f
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      AbodeRGBからCIE 1931 XYZ表色系への変換.
//-----------------------------------------------------------------------------
float3 AdobeRGB_To_XYZ(float3 color)
{
    const float3x3 conversion = 
    {
        0.576669f, 0.185558f, 0.188229f,
        0.297345f, 0.627363f, 0.075291f,
        0.027031f, 0.070689f, 0.991337f
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      ACES AP0からCIE 1931 XYZ表色系への変換.
//-----------------------------------------------------------------------------
float3 AP0_To_XYZ(float3 color)
{
    const float3x3 conversion = 
    {
        0.9525523959, 0.0000000000, 0.0000936786,
        0.3439664498, 0.7281660966, -0.0721325464,
        0.0000000000, 0.0000000000, 1.0088251844
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      ACES AP1からCIE 1931 XYZ表色系への変換.
//-----------------------------------------------------------------------------
float3 AP1_To_XYZ(float3 color)
{
    const float3x3 conversion = 
    {
        0.6624541811, 0.1340042065, 0.1561876870,
        0.2722287168, 0.6740817658, 0.0536895174,
       -0.0055746495, 0.0040607335, 1.0103391003
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      CIE xyY表色系からCIE 1931 XYZ表色系への変換.
//-----------------------------------------------------------------------------
float3 xyY_To_XYZ(float3 color)
{
    float3 xyY;
    float div = color.x + color.y + color.z;
    div = max(div, 1e-10);
    xyY.x = color.x / div;
    xyY.y = color.y / div;
    xyY.z = color.y;
    return xyY;
}

//-----------------------------------------------------------------------------
//      CIE 1931 XYZ表色系からITU-R BT.709への変換.
//-----------------------------------------------------------------------------
float3 XYZ_To_BT709(float3 color)
{
    const float3x3 conversion = 
    {
        3.240970f, -1.537383f, -0.498611f,
        -0.969244f, 1.875968f,  0.041555f,
        0.055630f, -0.203977f,  1.056972f
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      CIE 1931 XYZ表色系からITU-R BT.2020への変換.
//-----------------------------------------------------------------------------
float3 XYZ_To_BT2020(float3 color)
{
    const float3x3 conversion = 
    {
         1.716651f, -0.355671f, -0.253366f,
        -0.666684f,  1.616481f,  0.015769f,
         0.017640f, -0.042771f,  0.942103f
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      CIE 1931 XYZ表色系からDCI-P3への変換.
//-----------------------------------------------------------------------------
float3 XYZ_To_DCI_P3(float3 color)
{
    const float3x3 conversion = 
    {
         2.725394f, -1.018003f, -0.440163f,
        -0.795168f,  1.689732f,  0.022647f,
         0.041242f, -0.087639f,  1.100930f
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      CIE 1931 XYZ表色系からAdobeRGBへの変換.
//-----------------------------------------------------------------------------
float3 XYZ_To_AdobeRGB(float3 color)
{
    const float3x3 conversion = 
    {
         2.041588f, -0.565007f, -0.344731f,
        -0.969244f,  1.875968f,  0.041555f,
         0.013444f, -0.118362f,  1.015175f
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      CIE 1931 XYZ表色系からACES AP0への変換.
//-----------------------------------------------------------------------------
float3 XYZ_To_AP0(float3 color)
{
    const float3x3 conversion = 
    {
         1.0498110175, 0.0000000000, -0.0000974845,
        -0.4959030231, 1.3733130458,  0.0982400361,
         0.0000000000, 0.0000000000,  0.9912520182
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      CIE 1931 XYZ表色系からACES AP1への変換.
//-----------------------------------------------------------------------------
float3 XYZ_To_AP1(float3 color)
{
    const float3x3 conversion = 
    {
         1.6410233797, -0.3248032942, -0.2364246952,
        -0.6636628587,  1.6153315917,  0.0167563477,
         0.0117218943, -0.0082844420,  0.9883948585
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      CIE 1931 XYZ表色系からCIE xyYへの変換.
//-----------------------------------------------------------------------------
float3 XYZ_To_xyY(float3 color)
{
    float3 XYZ;
    XYZ.x = color.x * color.z / max(color.y, 1e-10);
    XYZ.y = color.z;
    XYZ.z = (1.0f - color.x - color.y) * color.z / max(color.y, 1e-10);
    
    return XYZ;
}

//-----------------------------------------------------------------------------
//      ITU-R BT709からCIE xyYへの変換.
//-----------------------------------------------------------------------------
float3 BT709_To_xyY(float3 color)
{
    float3 XYZ = BT709_To_XYZ(color);
    return XYZ_To_xyY(XYZ);
}

//-----------------------------------------------------------------------------
//      ITU-R BT.2020からCIE xyY表色系に変換します.
//-----------------------------------------------------------------------------
float3 BT2020_To_xyY(float3 color)
{
    float3 XYZ = BT2020_To_XYZ(color);
    return XYZ_To_xyY(XYZ);
}

//-----------------------------------------------------------------------------
//      CIE xyY表色系からITU-R BT.709への変換.
//-----------------------------------------------------------------------------
float3 xyY_To_BT709(float3 color)
{
    float3 XYZ = xyY_To_XYZ(color);
    return XYZ_To_BT709(XYZ);
};

//-----------------------------------------------------------------------------
//       CIE xyY表色系からAdobeRGBへの変換.
//-----------------------------------------------------------------------------
float3 xyY_To_AdobeRGB(float3 color)
{
    float3 XYZ = xyY_To_XYZ(color);
    return XYZ_To_AdobeRGB(XYZ);
}

//-----------------------------------------------------------------------------
//       CIE xyY表色系からDCI-P3への変換.
//-----------------------------------------------------------------------------
float3 xyY_To_DCI_P3(float3 color)
{
    float3 XYZ = xyY_To_XYZ(color);
    return XYZ_To_DCI_P3(XYZ);
}

//-----------------------------------------------------------------------------
//      CIE xyY表色系からITU-R BT.2020に変換します.
//-----------------------------------------------------------------------------
float3 xyY_To_BT2020(float3 color)
{
    float3 XYZ = xyY_To_XYZ(color);
    return XYZ_To_BT2020(XYZ);
}

//-----------------------------------------------------------------------------
//      ACES AP0 から ACES AP1への変換です.
//-----------------------------------------------------------------------------
float3 AP0_To_AP1(float3 color)
{
    const float3x3 conversion = 
    {
         1.4514393161, -0.2365107469, -0.2149285693,
        -0.0765537734,  1.1762296998, -0.0996759264,
         0.0083161484, -0.0060324498,  0.9977163014
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      ACES AP1からACES AP0への変換です.
//-----------------------------------------------------------------------------
float3 AP1_To_AP0(float3 color)
{
    const float3x3 conversion =
    {
         0.6954522414, 0.1406786965, 0.1638690622,
         0.0447945634, 0.8596711185, 0.0955343182,
        -0.0055258826, 0.0040252103, 1.0015006723
    };

    return mul(conversion, color);
}

//-----------------------------------------------------------------------------
//      ディザ処理.
//-----------------------------------------------------------------------------
void Dithering(float2 sv_position, float alpha)
{
    uint2 screenPos = (uint2)fmod(sv_position, 4.0f);
    if (alpha < F_DITHER_LIST[screenPos.x][screenPos.y])
    { discard; }
}
//-----------------------------------------------------------------------------
//      Ray vs Sphereの交差判定を行います.
//-----------------------------------------------------------------------------
bool RaySphereHit(float3 rayOrigin, float3 rayDir, float3 sphereCenter, float sphereRadius)
{
    float3 closestPointOnRay = max(0, dot(sphereCenter - rayOrigin, rayDir)) * rayDir;
    float3 centerToRay = rayOrigin + closestPointOnRay - sphereCenter;
    return dot(centerToRay, centerToRay) <= Sq(sphereRadius);
}

//-----------------------------------------------------------------------------
//      Ray vs AABBの交差判定を行います.
//-----------------------------------------------------------------------------
bool RayBoxHit(float3 rayOrigin, float3 invRayDir, float3 boxMin, float3 boxMax)
{
    float3 t0 = (boxMin - rayOrigin) * invRayDir;
    float3 t1 = (boxMax - rayOrigin) * invRayDir;
    float3 tmin = min(t0, t1);
    float3 tmax = max(t0, t1);
    return Max3(tmin) <= Min3(tmax);
}

//-----------------------------------------------------------------------------
//      AABBとの交差判定を行い, 交点までの距離を返却します.
//-----------------------------------------------------------------------------
float IntersectAABB(float3 dir, float3 orig, float3 mini, float3 maxi)
{
    float3 invDir = rcp(dir);
    float3 p0 = (mini - orig) * invDir;
    float3 p1 = (maxi - orig) * invDir;
    float3 t = min(p0, p1);
    return Max3(t);
}

//-----------------------------------------------------------------------------
//      グロシネスに変換します.
//-----------------------------------------------------------------------------
float RoughnessToGlossiness(float roughness)
{ return saturate(1.0f - roughness); }

//-----------------------------------------------------------------------------
//      ラフネスに変換します.
//-----------------------------------------------------------------------------
float GlossinessToRoughness(float glossiness)
{ return saturate(1.0f - glossiness); }

//-----------------------------------------------------------------------------
//      PBR RoughnessからTradiational Specular Powerに変換します.
//-----------------------------------------------------------------------------
float GlossinessToSpecularPower(float glossiness)
{
    // Sebastien Lagarade, "Adopting a physically based shading model", 
    // https://seblagarde.wordpress.com/2011/08/17/hello-world/
    // ※有効範囲は[2, 2048]まで.
    return exp2(10.0f * glossiness + 1.0f);
}

//-----------------------------------------------------------------------------
//      Traditional Specular Power から PBR Glossinessに変換します.
//-----------------------------------------------------------------------------
float SpecularPowerToGlossiness(float specularPower)
{ return log2(specularPower) * 0.01f - 1.0f; }

//-----------------------------------------------------------------------------
//      Traditional Specular Power から　PBR Roughnessに変換します.
//-----------------------------------------------------------------------------
float SpecularPowerToRoughness(float specularPower)
{ return SpecularPowerToRoughness(SpecularPowerToGlossiness(specularPower)); }

//-----------------------------------------------------------------------------
//      正規直交基底を求めます.
//-----------------------------------------------------------------------------
void CalcONB(float3 N, out float3 T, out float3 B)
{
    float sig = sign(N.z);
    float a = -1.0f / (sig + N.z);
    float b = N.x * N.y * a;
    T = float3(1.0f + sig * N.x * N.x * a, sig * b, -sig * N.x);
    B = float3(b, sig + N.y * N.y * a, -N.y);
}

//-----------------------------------------------------------------------------
//      半球を一様サンプリングします.
//-----------------------------------------------------------------------------
float3 UniformSampleHemisphere(float2 u)
{
    float z = u.x;
    float r = sqrt(max(0.0f, 1.0f - z * z));
    float phi = 2.0f * F_PI * u.y;

    return float3(r * cos(phi), r * sin(phi), z);
    // Pdf = z * (1.0f / F_PI);
}

//-----------------------------------------------------------------------------
//      全球を一様サンプリングします.
//-----------------------------------------------------------------------------
float3 UniformSampleSphere(float2 u)
{
    float z = 1.0f - 2.0f * u.x;
    float r = sqrt(max(0.0f, 1.0f - z * z));
    float phi = 2.0f * F_PI * u.y;

    return float3(r * cos(phi), r * sin(phi), z);
    // Pdf = 1.0f / (4.0f * F_PI);
}

//-----------------------------------------------------------------------------
//      円盤を一様サンプリングします.
//-----------------------------------------------------------------------------
float2 UniformSampleDisk(float2 u)
{
    float r = sqrt(u.x);
    float theta = 2.0f * F_PI * u.y;

    return float2(r * cos(theta), r * sin(theta));
}

//-----------------------------------------------------------------------------
//      円錐を一様サンプリングします.
//-----------------------------------------------------------------------------
float3 UniformSampleCone(float2 u, float cosThetaMax)
{
    float cosTheta = (1.0f - u.x) + u.x * cosThetaMax;
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    float phi =  u.y * 2.0f * F_PI;

    return float3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta);
}

//-----------------------------------------------------------------------------
//      三角形を一様サンプリングします.
//-----------------------------------------------------------------------------
float3 UniformSampleTriangle(float2 u)
{
    float su = sqrt(u.x);
    float2 b = float2(1.0f - su, u.y * su);
    return float3(1.0f - b.x - b.y, b.x, b.y);
}

//-----------------------------------------------------------------------------
//      X軸を取得します.
//-----------------------------------------------------------------------------
float3 GetAxisX(float4x4 view)
{ return view._11_12_13; } // CPU側と比べて転置しているので注意.

//-----------------------------------------------------------------------------
//      Y軸を取得します.
//-----------------------------------------------------------------------------
float3 GetAxisY(float4x4 view)
{ return view._21_22_23; } // CPU側と比べて転置しているので注意.

//-----------------------------------------------------------------------------
//      Z軸を取得します.
//-----------------------------------------------------------------------------
float3 GetAxisZ(float4x4 view)
{ return view._31_32_33; } // CPU側と比べて転置しているので注意.

//-----------------------------------------------------------------------------
//      カメラ位置を取得します.
//-----------------------------------------------------------------------------
float3 GetPosition(float4x4 view)
{
    return -view._11_12_13 * view._14
           -view._21_22_23 * view._24
           -view._31_32_33 * view._34;
}

//-----------------------------------------------------------------------------
//      レイの方向を求めます.
//-----------------------------------------------------------------------------
float3 CalcRayDir(float2 pixel, float4x4 view, float4x4 proj)
{
    // pixel : [-1, 1]の正規化されているピクセル座標とします(y方向は反転補正済みの前提).
    // view : ビュー行列.
    // proj : 透視投影行列.

    float aspect  = proj._22 / proj._11;
    float tanFovY = 1.0f / proj._22;

    return normalize(
        (view._11_12_13 * pixel.x * tanFovY * aspect)
      + (view._21_22_23 * pixel.y * tanFovY)
      - (view._31_32_33)); // 右手系なので，レイの進む向きは-Z方向.
}


//-----------------------------------------------------------------------------
//      X軸周りの回転行列を生成します.
//-----------------------------------------------------------------------------
float4x4 CreateRotationX(float radian)
{
    float sinRad, cosRad;
    sincos(radian, sinRad, cosRad);
    return float4x4(
        1.0f,     0.0f,     0.0f,   0.0f,
        0.0f,    cosRad,  sinRad,   0.0f,
        0.0f,   -sinRad,  cosRad,   0.0f,
        0.0f,      0.0f,    0.0f,   1.0f);
}

//-----------------------------------------------------------------------------
//      Y軸周りの回転行列を生成します.
//-----------------------------------------------------------------------------
float4x4 CreateRotationY(float radian)
{
    float sinRad, cosRad;
    sincos(radian, sinRad, cosRad);
    return float4x4(
        cosRad,     0.0f,   -sinRad,    0.0f,
          0.0f,     1.0f,      0.0f,    0.0f,
        sinRad,     0.0f,    cosRad,    0.0f,
          0.0f,     0.0f,      0.0f,    1.0f);
}

//-----------------------------------------------------------------------------
//      Z軸周りの回転行列を生成します.
//-----------------------------------------------------------------------------
float4x4 CreateRotationZ(float radian)
{
    float sinRad, cosRad;
    sincos(radian, sinRad, cosRad);
    return float4x4(
      cosRad,   sinRad,     0.0f,   0.0f,
     -sinRad,   cosRad,     0.0f,   0.0f,
        0.0f,     0.0f,     1.0f,   0.0f,
        0.0f,     0.0f,     0.0f,   1.0f);
}

//-----------------------------------------------------------------------------
//      パッキングされたプリミティブ番号を展開します.
//-----------------------------------------------------------------------------
uint3 UnpackPrimitiveIndex(uint packed)
{
    return uint3(
        packed & 0x3FF,
        (packed >> 10) & 0x3FF,
        (packed >> 20) & 0x3FF);
}

//-----------------------------------------------------------------------------
//      R8G8B8A8_SNORMを展開します.
//-----------------------------------------------------------------------------
float4 UnpackSnorm4(uint packed)
{
    float4 v;
    v.x = float((packed >> 0) & 0xFF);
    v.y = float((packed >> 8) & 0xFF);
    v.z = float((packed >> 16) & 0xFF);
    v.w = float((packed >> 24) & 0xFF);
    v = v / 255.0;
    v = v * 2.0 - 1.0;
    return v;
}

//-----------------------------------------------------------------------------
//      R8G8B8A8_UNORMを展開します.
//-----------------------------------------------------------------------------
float4 UnpackUnorm4(uint packed)
{
    float4 v;
    v.x = float((packed >> 0) & 0xFF);
    v.y = float((packed >> 8) & 0xFF);
    v.z = float((packed >> 16) & 0xFF);
    v.w = float((packed >> 24) & 0xFF);
    v = v / 255.0;
    return v;
}

//-----------------------------------------------------------------------------
//      R16G16_FLOATを展開します.
//-----------------------------------------------------------------------------
float2 UnpackHalf2(uint packed)
{
    float2 result;
    result.x = f16tof32(packed & 0xffff);
    result.y = f16tof32((packed >> 16) & 0xffff);
    return result;
}

//-----------------------------------------------------------------------------
//      R16G16_UINTを展開します.
//-----------------------------------------------------------------------------
uint2 UnpackUshort2(uint packed)
{
    return uint2(
        packed & 0xffff,
        (packed >> 16) & 0xffff);
}

//-----------------------------------------------------------------------------
//      R16G16B16A16_UINTを展開します.
//-----------------------------------------------------------------------------
uint4 UnpackUshort4(uint2 packed)
{
    return uint4(
        UnpackUshort2(packed.x),
        UnpackUshort2(packed.y));
}

//-----------------------------------------------------------------------------
//      パッキングされた接線空間を展開します.
//-----------------------------------------------------------------------------
void UnpackTN
(
    in  uint    encodedTBN,     // 圧縮している接線空間(32bit).
    out float3  tangent,        // 接線ベクトル.
    out float3  normal          // 法線ベクトル.
)
{
    // Hawar Doghramachi and Jean-Normand Bucci, 
    // "Deferred+: Next-Gen Culling and Rendering for the Dawn Engine"
    // GPU Zen, pp.89-91, 2017.
    uint2 packedN;
    packedN.x = encodedTBN & 0x3FF;
    packedN.y = (encodedTBN >> 10) & 0x3FF;

    // octahedron normal vector decoding.
    normal = UnpackNormal(packedN / 1023.0f);
    
    uint cosAngleUint = (encodedTBN >> 20) & 0xFF;
    uint compIndex    = (encodedTBN >> 28) & 0x3;

    // get reference vector
    float3 refVector;
    if (compIndex == 0)
    { refVector = float3(1.0f, 0.0f, 0.0f); }
    else if (compIndex == 1)
    { refVector = float3(0.0f, 1.0f, 0.0f); }
    else
    { refVector = float3(0.0f, 0.0f, 1.0f); }

    // decode tangent
    float cosAngle = (float(cosAngleUint) / 255.0f) * 2.0f - 1.0f;
    float sinAngle = sqrt(saturate(1.0f - (cosAngle * cosAngle)));
    
    uint handedness = (encodedTBN >> 30) & 0x3;

    sinAngle = ((handedness & 0x2) == 0) ? -sinAngle : sinAngle;
    float3 orthoA = normalize(cross(normal, refVector));
    float3 orthoB = cross(normal, orthoA);
    tangent = normalize((cosAngle * orthoA) + (sinAngle * orthoB));
}

//-----------------------------------------------------------------------------
//      パッキングされた接線空間を展開します.
//-----------------------------------------------------------------------------
void UnpackTBN
(
    in  uint    encodedTBN,     // 圧縮している接線空間(32bit).
    out float3  tangent,        // 接線ベクトル.
    out float3  bitangent,      // 従接線ベクトル.
    out float3  normal          // 法線ベクトル.
)
{
    // Hawar Doghramachi and Jean-Normand Bucci, 
    // "Deferred+: Next-Gen Culling and Rendering for the Dawn Engine"
    // GPU Zen, pp.89-91, 2017.
    uint2 packedN;
    packedN.x = encodedTBN & 0x3FF;
    packedN.y = (encodedTBN >> 10) & 0x3FF;

    // octahedron normal vector decoding.
    normal = UnpackNormal(packedN / 1023.0f);
    
    uint cosAngleUint = (encodedTBN >> 20) & 0xFF;
    uint compIndex    = (encodedTBN >> 28) & 0x3;

    // get reference vector
    float3 refVector;
    if (compIndex == 0)
    { refVector = float3(1.0f, 0.0f, 0.0f); }
    else if (compIndex == 1)
    { refVector = float3(0.0f, 1.0f, 0.0f); }
    else
    { refVector = float3(0.0f, 0.0f, 1.0f); }

    // decode tangent
    float cosAngle = (float(cosAngleUint) / 255.0f) * 2.0f - 1.0f;
    float sinAngle = sqrt(saturate(1.0f - (cosAngle * cosAngle)));
    
    uint handedness = (encodedTBN >> 30) & 0x3;

    sinAngle = ((handedness & 0x2) == 0) ? -sinAngle : sinAngle;
    float3 orthoA = normalize(cross(normal, refVector));
    float3 orthoB = cross(normal, orthoA);
    tangent = normalize((cosAngle * orthoA) + (sinAngle * orthoB));

    // decode bitangent
    bitangent = cross(normal, tangent);
    bitangent = ((handedness & 0x1) == 0) ? bitangent : -bitangent;
}

//-----------------------------------------------------------------------------
//      接線空間を32bitにパッキングします.
//-----------------------------------------------------------------------------
uint PackTBN(float3 normal, float3 tangent, uint binormalHandedness)
{
    // Hawar Doghramachi and Jean-Normand Bucci, 
    // "Deferred+: Next-Gen Culling and Rendering for the Dawn Engine"
    // GPU Zen, pp.89-91, 2017.
    uint result = 0;
    float2 packNormal = PackNormal(normal);
    result |= uint(packNormal.x * 1023.0f);
    result |= uint(packNormal.y * 1023.0f) << 10;

    float3 tangentAbs = abs(tangent);
    float maxComp = Max3(tangentAbs);

    float3 refVector;
    uint compIndex = 0;
    if (maxComp == tangentAbs.x)
    {
        refVector = float3(1.0f, 0.0f, 0.0f);
        compIndex = 0;
    }
    else if (maxComp == tangentAbs.y)
    {
        refVector = float3(0.0f, 1.0f, 0.0f);
        compIndex = 1;
    }
    else
    {
        refVector = float3(0.0f, 0.0f, 1.0f);
        compIndex = 2;
    }
    
    float3 orthoA = normalize(cross(normal, refVector));
    float3 orthoB = cross(normal, orthoA);
    uint cosAngle = uint((dot(tangent, orthoA) * 0.5f + 0.5f) * 255.0f);
    uint tangentHandedness = (dot(tangent, orthoB) > 0.0001f) ? 1 : 0;
    
    result |= (cosAngle  & 0xff) << 20;
    result |= (compIndex & 0x3)  << 28;
    result |= (tangentHandedness  & 0x1) << 30;
    result |= (binormalHandedness & 0x1) << 31;
    
    return result;
}

//-----------------------------------------------------------------------------
//      R10G10B10A2_UINTフォーマットを展開します.
//-----------------------------------------------------------------------------
uint4 DecodeR10G10B10A2(uint packed)
{
    return uint4(
        packed & 0x3FF,
        (packed >> 10) & 0x3FF,
        (packed >> 20) & 0x3FF,
        (packed >> 30) & 0x2);
}

//-----------------------------------------------------------------------------
//      法錐が縮退しているかどうかチェックします.
//-----------------------------------------------------------------------------
bool IsConeDegenerate(uint packedCone)
{ return (packedCone >> 24) == 0xff; }

//-----------------------------------------------------------------------------
//      法錐カリングを行います.
//-----------------------------------------------------------------------------
bool NormalConeCulling(float4 normalCone, float3 viewDir)
{
    // normalConeはワールド変換済みとします.
    return dot(normalCone.xyz, -viewDir) > normalCone.w;
}

//-----------------------------------------------------------------------------
//      視錐台の中にスフィアが含まれるかどうかチェックします.
//-----------------------------------------------------------------------------
bool Contains(float4 planes[6], float4 sphere)
{
    // sphereは事前に位置座標がワールド変換済み，半径もスケール適用済みとします.
    float4 center = float4(sphere.xyz, 1.0f);

    for(int i=0; i<6; ++i)
    {
        if (dot(center, planes[i]) < -sphere.w)
        {
            // カリングする.
            return false;
        }
    }

    // カリングしない.
    return true;
}

//-----------------------------------------------------------------------------
//      ポリゴン単位のバックフェースカリングを行います.
//-----------------------------------------------------------------------------
bool OrientationCulling(float3 p0, float3 p1, float3 p2)
{
    float det = determinant(float3x3(p0, p1, p2));
    return -det <= 0.0f; // true でカリング.
}

//-----------------------------------------------------------------------------
//      極小プリミティブカリングを行います.
//-----------------------------------------------------------------------------
bool SmallPrimitiveCulling(float2 vmin, float2 vmax)
{
    // vmin, vmaxは NDC(正規化デバイス座標系).
    return any(round(vmin) == round(vmax)); // true でカリング.
}

//-----------------------------------------------------------------------------
//      ポリゴン単位の錐台カリングを行います.
//-----------------------------------------------------------------------------
bool FrustumCulling(float3 p0, float3 p1, float3 p2)
{
    // p0, p1, p2 は NDC(正規化デバイス座標系).
    float3 pmin = min(p0, min(p1, p2));
    float3 pmax = max(p0, max(p1, p2));

    // true でカリング.
    return (any(pmax.xy < -1.0f) || any(pmin.xy > 1.0f) || pmax.z < 0.0f || pmax.z > 1.0f);
}

//-----------------------------------------------------------------------------
//      スフィアマップのテクスチャ座標を求めます.
//-----------------------------------------------------------------------------
float2 ToSphereMapCoord(float3 reflectDir)
{
    float theta = acos(reflectDir.y);
    float phi   = atan2(reflectDir.z, reflectDir.x);
    if (reflectDir.z < 0.0f)
    { phi += F_2PI; }

    return float2(phi * F_1DIV2PI, theta * F_1DIVPI);
}

//-----------------------------------------------------------------------------
//      スフィアマップのテクスチャ座標からキューブマップサンプリング方向を求めます.
//-----------------------------------------------------------------------------
float3 FromSphereMapCoord(float2 uv)
{
    float phi   = uv.x * F_2PI;
    float theta = uv.y * F_PI;

    float sinTheta, cosTheta;
    sincos(theta, sinTheta, cosTheta);

    float sinPhi, cosPhi;
    sincos(phi, sinPhi, cosPhi);

    return float3(
        cosPhi * sinTheta,
        sinPhi * sinTheta,
        cosTheta);
}

//-----------------------------------------------------------------------------
//      入力のうち低い方の16ビットを1つおきのビットごとに分離する.
//-----------------------------------------------------------------------------
uint Part1By1(uint x)
{
    x &= 0x0000ffff;                  // x = ---- ---- ---- ---- fedc ba98 7654 3210
    x = (x ^ (x <<  8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
    x = (x ^ (x <<  4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
    x = (x ^ (x <<  2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
    x = (x ^ (x <<  1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
    return x;
}

//-----------------------------------------------------------------------------
//      入力のうち低い方の10ビットを2つおきのビットごとに分離する.
//-----------------------------------------------------------------------------
uint Part1By2(uint x)
{
    x &= 0x000003ff;                  // x = ---- ---- ---- ---- ---- --98 7654 3210
    x = (x ^ (x << 16)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
    x = (x ^ (x <<  8)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
    x = (x ^ (x <<  4)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
    x = (x ^ (x <<  2)) & 0x09249249; // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
    return x;
}

//-----------------------------------------------------------------------------
//      1つおきのビットをコンパクションします.
//-----------------------------------------------------------------------------
uint Compact1By1(uint x)
{
    x &= 0x55555555;                  // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
    x = (x ^ (x >>  1)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
    x = (x ^ (x >>  2)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
    x = (x ^ (x >>  4)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
    x = (x ^ (x >>  8)) & 0x0000ffff; // x = ---- ---- ---- ---- fedc ba98 7654 3210
    return x;
}

//-----------------------------------------------------------------------------
//      2つおきのビットをコンパクションします.
//-----------------------------------------------------------------------------
uint Compact1By2(uint x)
{
    x &= 0x09249249;                  // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
    x = (x ^ (x >>  2)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
    x = (x ^ (x >>  4)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
    x = (x ^ (x >>  8)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
    x = (x ^ (x >> 16)) & 0x000003ff; // x = ---- ---- ---- ---- ---- --98 7654 3210
    return x;
}

//-----------------------------------------------------------------------------
//      2次元のモートンコードをエンコードします.
//-----------------------------------------------------------------------------
uint EncodeMorton2(uint x, uint y)
{ return (Part1By1(y) << 1) | Part1By1(x); }

//-----------------------------------------------------------------------------
//      3次元のモートンコードをエンコードします.
//-----------------------------------------------------------------------------
uint EncodeMorton3(uint x, uint y, uint z)
{ return (Part1By2(z) << 2) | (Part1By2(y) << 1) | Part1By2(x); }

//-----------------------------------------------------------------------------
//      2次元のモートンコードをデコードします.
//-----------------------------------------------------------------------------
uint2 DecodeMorton2(uint code)
{
    return uint2(
        Compact1By1(code >> 0),
        Compact1By1(code >> 1));
}

//-----------------------------------------------------------------------------
//      3次元のモートンコードをデコードします.
//-----------------------------------------------------------------------------
uint3 DecodeMorton3(uint code)
{
    return uint3(
        Compact1By2(code >> 0),
        Compact1By2(code >> 1),
        Compact1By2(code >> 2));
}

//-----------------------------------------------------------------------------
//      スクリーン内にテクスチャ座標が治まっているかどうかチェックします.
//-----------------------------------------------------------------------------
float IsInScreen(float2 uv)
{ return float(all(saturate(uv) == uv)); }

//-----------------------------------------------------------------------------
//      ビットフィールドを抽出します.
//-----------------------------------------------------------------------------
uint BitfieldExtract(uint src, uint offset, uint bits)
{
    uint mask = (1u << bits) - 1u;
    return (src >> offset) & mask;
}

//-----------------------------------------------------------------------------
//      ビットフィールドを挿入します.
//-----------------------------------------------------------------------------
uint BitfieldInsert(uint src, uint insert, uint bits) 
{
    uint mask = (1u << bits) - 1u;
    return (insert & mask) | (src & (~mask));
}

//-----------------------------------------------------------------------------
//      モートンオーダーにリマップします.
//-----------------------------------------------------------------------------
uint2 RemapLane8x8(uint2 dispatchId, uint groupIndex)
{
    uint2 remappedId;
    remappedId.x = BitfieldInsert(BitfieldExtract(groupIndex, 2u, 3u), groupIndex, 1u);
    remappedId.y = BitfieldInsert(BitfieldExtract(groupIndex, 3u, 3u), BitfieldExtract(groupIndex, 1u, 2u), 2u);

    uint2  dispatchGroupId = uint2(dispatchId) / 8; // 8未満切り捨て.
    return (dispatchGroupId * 8) + remappedId;
}

//-----------------------------------------------------------------------------
//      RGBからYCoCgに変換します.
//-----------------------------------------------------------------------------
float4 RGBToYCoCg(float4 value)
{
    float Y  = dot(value.rgb, float3( 1, 2,  1)) * 0.25;
    float Co = dot(value.rgb, float3( 2, 0, -2)) * 0.25 + (0.5 * 256.0 / 255.0);
    float Cg = dot(value.rgb, float3(-1, 2, -1)) * 0.25 + (0.5 * 256.0 / 255.0);
    return float4(Y, Co, Cg, value.a);
}

//-----------------------------------------------------------------------------
//      YCoCgからRGBに変換します.
//-----------------------------------------------------------------------------
float4 YCoCgToRGB(float4 YCoCg)
{
    float Y  = YCoCg.x;
    float Co = YCoCg.y - (0.5 * 256.0 / 255.0);
    float Cg = YCoCg.z - (0.5 * 256.0 / 255.0);
    float R  = Y + Co - Cg;
    float G  = Y + Cg;
    float B  = Y - Co - Cg;
    return float4(R, G, B, YCoCg.a);
}

//-----------------------------------------------------------------------------
//      [0, 1]の値をカラー化します.
//-----------------------------------------------------------------------------
float3 ColorizeZucconi(float x)
{
    // https://www.shadertoy.com/view/ls2Bz1
    // Original solution converts visible wavelengths of light (400-700 nm) (represented as x = [0; 1]) to RGB colors
    x = saturate(x) * 0.85;

    const float3 c1 = float3( 3.54585104, 2.93225262, 2.41593945 );
    const float3 x1 = float3( 0.69549072, 0.49228336, 0.27699880 );
    const float3 y1 = float3( 0.02312639, 0.15225084, 0.52607955 );

    const float3 c2 = float3( 3.90307140, 3.21182957, 3.96587128 );
    const float3 x2 = float3( 0.11748627, 0.86755042, 0.66077860 );
    const float3 y2 = float3( 0.84897130, 0.88445281, 0.73949448 );

    // https://developer.nvidia.com/sites/all/modules/custom/gpugems/books/GPUGems/gpugems_ch08.html.
    return saturate(1.0f.xxx - Pow2(c1 * (x - x1)) - y1)
         + saturate(1.0f.xxx - Pow2(c2 * (x - x2)) - y2);
}

//-----------------------------------------------------------------------------
//      Karisのアンチファイアフライウェイトを計算します.
//-----------------------------------------------------------------------------
float KarisAntiFireflyWeight(float3 value, float exposure)
{ return rcp(4.0f + LuminanceBT709(value) * exposure); }

//-----------------------------------------------------------------------------
//      Karisのアンチファイアフライウェイトを計算します.
//-----------------------------------------------------------------------------
float KarisAntiFireflyWeightY(float luma, float exposure)
{ return rcp(4.0f + luma * exposure); }

//-----------------------------------------------------------------------------
//      先鋭化フィルタを適用します.
//-----------------------------------------------------------------------------
float4 ApplySharpening
(
    float4  center,     // 中心ピクセル.
    float4  top,        // 上ピクセル.
    float4  left,       // 左ピクセル.
    float4  right,      // 右ピクセル.
    float4  bottom,     // 下ピクセル.
    float   weight,     // 中心の重み (デフォルト4.0)
    float   tolerance   // 許容値 (デフォルト0.1)
)
{
    // https://en.wikipedia.org/wiki/Unsharp_masking

    float4 result = RGBToYCoCg(center);

    float unsharpenMask;
    unsharpenMask  = weight * result.x;
    unsharpenMask -= RGBToYCoCg(top).x;
    unsharpenMask -= RGBToYCoCg(bottom).x;
    unsharpenMask -= RGBToYCoCg(left).x;
    unsharpenMask -= RGBToYCoCg(right).x;

    const float invWeight = SaturateFloat(1.0f / weight);
    result.x = clamp(
        result.x + invWeight * unsharpenMask,
        (1.0f - tolerance) * result.x, 
        (1.0f + tolerance) * result.x);

    return YCoCgToRGB(result);
}

#endif//ASDX_MATH_HLSLI
