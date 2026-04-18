//-----------------------------------------------------------------------------
// File : RasterScrollPS.hlsl
// Desc : Pixel Shader For Raster Scroll Effect.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "asdxMath.hlsli"
#include "asdxSamplers.hlsli"


///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////
// CbParam constant buffers.
///////////////////////////////////////////////////////////////////////////////
cbuffer CbParam : register(b0)
{
    float4 Color;
    uint   Pattern;
    float2 Control;
    float  AspectRatio;
};

//-----------------------------------------------------------------------------
//      回転処理を行います.
//-----------------------------------------------------------------------------
float2 Rotate(float2 p, float a)
{
    float s, c;
    sincos(a, s, c);
    return float2(c * p.x - s * p.y, s * p.x + c * p.y);
}

//-----------------------------------------------------------------------------
//      円形の符号付き距離関数です.
//-----------------------------------------------------------------------------
float SdfCircle(float2 p, float r)
{
    return length(p) - r;
}

//-----------------------------------------------------------------------------
//      四角形の符号付き距離関数です.
//-----------------------------------------------------------------------------
float SdfBox(float2 p, float2 size)
{
    float2 d = abs(p) - size;
    return length(max(d, 0.0f)) + min(max(d.x, d.y), 0.0f);
}

//-----------------------------------------------------------------------------
//      ひし形の符号付き距離関数です.
//-----------------------------------------------------------------------------
float SdfDiamond(float2 p, float r)
{
    p = abs(p);
    return (p.x + p.y) - r;
}

//-----------------------------------------------------------------------------
//      多角形の符号付き距離関数です.
//-----------------------------------------------------------------------------
float SdfPolygon(float2 p, int n, float r)
{
    float angle = F_2PI / float(n);
    float a = atan2(p.y, p.x) - F_PI * 0.5f;
    float d = cos(floor(0.5f + a / angle) * angle - a) * length(p);
    return d - r;
}

//-----------------------------------------------------------------------------
//      星形の符号付き距離関数です.
//-----------------------------------------------------------------------------
float SdfStar(float2 p, float r)
{
    p.y = -p.y; // 上下反転.
    const float k1x = 0.809016994; // cos(π/ 5).
    const float k2x = 0.309016994; // sin(π/10).
    const float k1y = 0.587785252; // sin(π/ 5).
    const float k2y = 0.951056516; // cos(π/10).
    const float k1z = 0.726542528; // tan(π/ 5).
    const float2 v1 = float2(k1x, -k1y);
    const float2 v2 = float2(-k1x, -k1y);
    const float2 v3 = float2(k2x, -k2y);
    
    p.x = abs(p.x);
    p -= 2.0f * max(dot(v1, p), 0.0f) * v1;
    p -= 2.0f * max(dot(v2, p), 0.0f) * v2;
    p.x = abs(p.x);
    p.y -= r;
    return length(p - v3 * clamp(dot(p, v3), 0.0f, k1z * r))
           * sign(p.y * v3.x - p.x * v3.y);
}

//-----------------------------------------------------------------------------
//      形状を結合します
//-----------------------------------------------------------------------------
float SdfUnion(float lhs, float rhs)
{
    return min(lhs, rhs);
}

//-----------------------------------------------------------------------------
//      基本形状から別の形状でくりぬきます.
//-----------------------------------------------------------------------------
float SdfSubtract(float lhs, float rhs)
{
    return max(lhs, -rhs);
}

//-----------------------------------------------------------------------------
//      2つの形状が重なっている共通部分だけを抽出します.
//-----------------------------------------------------------------------------
float SdfIntersection(float lhs, float rhs)
{
    return max(lhs, rhs);
}

//-----------------------------------------------------------------------------
//      形状をドーナツ化します.
//-----------------------------------------------------------------------------
float SdfDonut(float sdf, float r)
{
    return abs(sdf) - r;
}

//-----------------------------------------------------------------------------
//      SDFをリピート化します.
//-----------------------------------------------------------------------------
float2 SdfRepeat(float2 p, float2 c)
{
    return fmod(p + 0.5f * c, c) - 0.5f * c;
}

//-----------------------------------------------------------------------------
//      アンチエイリアス処理を行います.
//-----------------------------------------------------------------------------
float SdfAntiAlias(float2 uv, float sdf)
{
    const float distPerPixel = abs(ddx_fine(uv)) + abs(ddy_fine(uv));
    const float distInPixels = sdf / distPerPixel;
    return saturate(0.5f - distInPixels);
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET0
{
    float2 uv = (input.TexCoord - 0.5f.xx);
    uv.x *= AspectRatio;  // アスペクト比を考慮.

    float sdf = 0.0f;
    switch(Pattern)
    {
    case 0:
    default:
        sdf = SdfCircle(uv, Control.x);
        break;

    case 1:
        sdf = SdfBox(uv, Control.yx);
        break;

    case 2:
        sdf = SdfDiamond(uv, Control.x);
        break;
 
    case 3:
        sdf = SdfPolygon(uv, 3, Control.x);
        break;
 
    case 4:
        sdf = SdfPolygon(uv, 4, Control.x);
        break;

    case 5:
        sdf = SdfPolygon(uv, 5, Control.x);
        break;
 
    case 6:
        sdf = SdfPolygon(uv, 6, Control.x);
        break;

    case 7:
        sdf = SdfPolygon(uv, 7, Control.x);
        break;

    case 8:
        sdf = SdfPolygon(uv, 8, Control.x);
        break;
 
    case 9:
        sdf = SdfPolygon(uv, 9, Control.x);
        break;

    case 10:
        sdf = SdfStar(uv, Control.x * 3.0f);
        break;
    }


    float4 result = (sdf <= 0.0f) ? 0.0f.xxxx : Color;
    result.w = 1.0f - saturate(SdfAntiAlias(input.TexCoord, sdf));

    return result;
}
