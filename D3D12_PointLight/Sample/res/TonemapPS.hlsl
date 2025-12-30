//-----------------------------------------------------------------------------
// File : TonemapPS.hlsl
// Desc : Pixel Shader For Tonemap.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
static const int COLOR_SPACE_BT709      = 0;
static const int COLOR_SPACE_BT2100_PQ  = 1;

static const int TONEMAP_NONE     = 0;
static const int TONEMAP_REINHARD = 1;
static const int TONEMAP_GT       = 2;


///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;      // 位置座標です.
    float2 TexCoord : TEXCOORD;         // テクスチャ座標です.
};

///////////////////////////////////////////////////////////////////////////////
// CbTonemap constant buffer
///////////////////////////////////////////////////////////////////////////////
cbuffer CbTonemap : register(b0)
{
    int     TonemapType;    // トーンマップのタイプです.
    int     ColorSpace;     // 出力色空間です.
    float   BaseLuminance;  // 基準輝度値です(単位は[nit]).
    float   MaxLuminance;   // 最大輝度値です(単位は[nit]).
};

//-----------------------------------------------------------------------------
// Textures and Sampler
//-----------------------------------------------------------------------------
Texture2D       ColorMap : register(t0);
SamplerState    ColorSmp : register(s0);


//-----------------------------------------------------------------------------
//      色域変換を行います.
//-----------------------------------------------------------------------------
float4 ColorSpaceConvert(float4 color)
{
    float4 result = 0;

    switch (ColorSpace)
    {
    // ITU-R BT.709
    case COLOR_SPACE_BT709:
        {
            // 従来通り，そのまま出力します.
            result = color;
        }
        break;

    // ITU-R BT.2100
    case COLOR_SPACE_BT2100_PQ:
        {
            // BT.709 ---> BT.2100
            static const float3x3 conversion =
            {
                 0.627404f, 0.329283f, 0.043313f,
                 0.069097f, 0.919540f, 0.011362f,
                 0.016391f, 0.088013f, 0.895595f
            };
            result.rgb = mul(conversion, color.rgb);
        }
        break;
    }

    return result;
}

//-----------------------------------------------------------------------------
//      Reinhardトーンマップを適用します.
//-----------------------------------------------------------------------------
float3 ReinhardTonemap(float3 color)
{
    float Lz = MaxLuminance / BaseLuminance;
    float k = BaseLuminance * Lz / (BaseLuminance - Lz);
    return color.rgb * k / (color.rgb + float3(k, k, k));
}

//-----------------------------------------------------------------------------
//      GTトーンマップを適用します.
//-----------------------------------------------------------------------------
float3 GtTonemap(float3 color)
{
    float k = MaxLuminance / BaseLuminance;

    // 制御パラメータ.
    float P = k;
    float a = 1.0f;
    float m = 0.22f;
    float l = 0.4f;
    float c = 1.33f;
    float b = 0.0f;

    float3 x = color.rgb;
    float l0 = ((P - m) * l) / a;

    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    float3 w0 = 1.0 - smoothstep(0.0f, m, x);
    float3 w2 = step(m + l0, x);
    float3 w1 = 1.0f - w0 - w2;

    float3 T = m * pow(x / m, c) + b;
    float3 S = P - (P - S1) * exp(CP * (x - S0));
    float3 L = m + a * (x - m);

    return T * w0 + L * w1 + S * w2;
}

//-----------------------------------------------------------------------------
//      ITU-R BT.709 OETFを適用します.
//-----------------------------------------------------------------------------
float3 OETF_BT709(float3 color)
{
    float3 result;
    result.r = (color.r <= 0.018f) ? 4.500 * color.r : (1.0f + 0.099) * pow(abs(color.r), 0.45f) - 0.099f;
    result.g = (color.g <= 0.018f) ? 4.500 * color.g : (1.0f + 0.099) * pow(abs(color.g), 0.45f) - 0.099f;
    result.b = (color.b <= 0.018f) ? 4.500 * color.b : (1.0f + 0.099) * pow(abs(color.b), 0.45f) - 0.099f;
    return result;
}

//-----------------------------------------------------------------------------
//      ITU-R BT.2100 PQ System OETFを適用します.
//-----------------------------------------------------------------------------
float3 OETF_BT2100_PQ(float3 color)
{
    float m1 = 2610.0 / 4096.0 / 4;
    float m2 = 2523.0 / 4096.0 * 128;
    float c1 = 3424.0 / 4096.0;
    float c2 = 2413.0 / 4096.0 * 32;
    float c3 = 2392.0 / 4096.0 * 32;
    float3 cp = pow(abs(color.rgb), m1);
    return pow((c1 + c2 * cp) / (1 + c3 * cp), m2);
}

//-----------------------------------------------------------------------------
//      トーンマッピングを行います.
//-----------------------------------------------------------------------------
float4 Tonemapping(float4 color)
{
    float4 result = 0;

    switch (TonemapType)
    {
    // トーンマップなし.
    case TONEMAP_NONE:
        result.rgb = color.rgb;
        break;

    // Reinhardトーンマップ.
    case TONEMAP_REINHARD:
        result.rgb = ReinhardTonemap(color.rgb);
        break;

    // GTトーンマップ.
    case TONEMAP_GT:
        result.rgb = GtTonemap(color.rgb);
        break;
    }

    return result;
}

//-----------------------------------------------------------------------------
//      OETF(Opt Electronic Transfer Function)を適用します.
//-----------------------------------------------------------------------------
float4 ApplyOETF(float4 color)
{
    float4 result = 0;

    switch (ColorSpace)
    {
    // ITU-R BT.709 OEFT
    case COLOR_SPACE_BT709:
        result.rgb = OETF_BT709(color.rgb);
        break;

    // ITU-R BT.2100 PQ OETF
    case COLOR_SPACE_BT2100_PQ:
        result.rgb = OETF_BT2100_PQ(color.rgb);
        break;
    }

    return result;
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
float4 main(const VSOutput input) : SV_TARGET0
{
    // テクスチャカラーを取得.
    float4 result = ColorMap.Sample(ColorSmp, input.TexCoord);

    // 色空間を変換.
    result = ColorSpaceConvert(result);

    // トーンマッピング.
    result = Tonemapping(result);

    // OETF適用.
    result = ApplyOETF(result);

    return result;
}
