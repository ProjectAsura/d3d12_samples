//-----------------------------------------------------------------------------
// File : VignettingPS.hlsl
// Desc : Vignetting Effect.
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
// CbParam constant buffer. 
///////////////////////////////////////////////////////////////////////////////
cbuffer CbParam : register(b0)
{
    float   AspectRatio;
    float   Strength;
    float2  Reserved;
};

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
Texture2D ColorMap  : register(t0);


//-----------------------------------------------------------------------------
//      Natural Vignetting を計算します.
//-----------------------------------------------------------------------------
float ComputeNaturalVignetting
(
    float2  uv,                 // テクスチャ座標.
    float   aspectRatio,        // アスペクト比.
    float2  sensorSize_mm,      // センサーサイズ(単位:mm)
    float   focalLength_mm,     // 焦点距離(単位:mm)
    float   strength            // ヴィネットの強さ.
)
{
    // センサー中心を原点とした実寸(mm)
    float2 p = (uv - 0.5f);
    p.x *= aspectRatio;
    p *= sensorSize_mm;

    // 2乗値を求める.
    const float r2 = dot(p, p);
    const float f2 = Pow2(focalLength_mm);

    // コサイン4乗則.
    const float attenuation = Pow2(f2 / (r2 + f2));

    // 強さを制御できるように線形補間した結果を返却.
    return lerp(1.0f, attenuation, saturate(strength));
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET0
{
    float4 color = ColorMap.SampleLevel(LinearClamp, input.TexCoord, 0.0f);
 
    // ヴィネッティングを計算
    float attenuation = ComputeNaturalVignetting(
        input.TexCoord,
        AspectRatio,
        float2(36.0f, 24.0f),   // フルサイズセンサー (36mm x 24mm)
        35.0f,                  // 35mm レンズ.
        Strength);
 
    // ヴィネッティングを適用し弱める.
    color.rgb *= attenuation;

    // 結果を出力.
    return color;
}
