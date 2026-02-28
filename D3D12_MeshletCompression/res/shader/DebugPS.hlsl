//-----------------------------------------------------------------------------
// File : SimplePS.hlsl
// Desc : Simple Pixel Shader.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// MSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct MSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
    float2 TexCoord : TEXCOORD0;
    nointerpolation uint MeshletId   : MESHLET_ID;
    nointerpolation uint PrimitiveId : PRIMITIVE_ID;
};

///////////////////////////////////////////////////////////////////////////////
// PSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct PSOutput
{
    float4 Color : SV_TARGET0;
};

///////////////////////////////////////////////////////////////////////////////
// Constants structure
///////////////////////////////////////////////////////////////////////////////
struct Constants
{
    uint    MeshletCount;
    uint    InstanceId;
    float   MinContribution;
    uint    Flags;
};

///////////////////////////////////////////////////////////////////////////////
// TransParam structure
///////////////////////////////////////////////////////////////////////////////
struct TransParam
{
    float4x4 View;
    float4x4 Proj;
};

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
ConstantBuffer<Constants>           g_Constants     : register(b0);
ConstantBuffer<TransParam>          g_TransParam    : register(b1);

//-----------------------------------------------------------------------------
//      色相からRGB値を求めます.
//-----------------------------------------------------------------------------
float3 HueToRGB(float hue) 
{
    // https://www.ronja-tutorials.com/post/041-hsv-colorspace/
    hue = frac(hue); //only use fractional part of hue, making it loop
    float r = -1.0f + abs(hue * 6.0f - 3.0f); //red
    float g =  2.0f - abs(hue * 6.0f - 2.0f); //green
    float b =  2.0f - abs(hue * 6.0f - 4.0f); //blue
    return saturate(float3(r, g, b)); //clamp between 0 and 1
}

//-----------------------------------------------------------------------------
//      リニアからSRGBへの変換.
//-----------------------------------------------------------------------------
float3 ToSRGB(float3 color)
{
    float3 result;
    result.x  = (color.x < 0.0031308) ? 12.92 * color.x : 1.055 * pow(abs(color.x), 1.0f / 2.4) - 0.05f;
    result.y  = (color.y < 0.0031308) ? 12.92 * color.y : 1.055 * pow(abs(color.y), 1.0f / 2.4) - 0.05f;
    result.z  = (color.z < 0.0031308) ? 12.92 * color.z : 1.055 * pow(abs(color.z), 1.0f / 2.4) - 0.05f;

    return result;
}

float3 Pow2(float3 value)
{
    return value * value;
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


void CotangentFrame(float3 N, float3 p, float2 uv, out float3 outT, out float3 outB)
{
    float3 dp1  = ddx(p);
    float3 dp2  = ddy(p);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);

    float3 dp2perp = cross(dp2, N);
    float3 dp1perp = cross(N, dp1);
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
     
    float invmax = rsqrt(max(dot(T, T), dot(B, B)));
    outT = normalize(T * invmax);
    outB = normalize(B * invmax);
}


//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
PSOutput main(const MSOutput input)
{
    PSOutput output = (PSOutput)0;
    
    uint mode = (g_Constants.Flags >> 1) & 0xF;
    float3 N = normalize(input.Normal);
    float3 T = normalize(input.Tangent);
    float3 B = normalize(cross(N, T));
    
    float3 calcT, calcB;
    CotangentFrame(N, input.Position.xyz, input.TexCoord, calcT, calcB);
    
    if (mode == 0)
    {
        float3 lightDir = normalize(g_TransParam.View._31_32_33);
        output.Color.rgb = saturate(dot(normalize(input.Normal), lightDir)).xxx;
        output.Color.a = 1.0f;
    }
    else if (mode == 1)
    {
        output.Color.rgb = N * 0.5f + 0.5f;
        output.Color.a   = 1.0f;
    }
    else if (mode == 2)
    {
        output.Color.rgb = T * 0.5f + 0.5f;
        output.Color.a   = 1.0f;
    }
    else if (mode == 3)
    {
        output.Color.rgb = B * 0.5f + 0.5f;
        output.Color.a   = 1.0f;
    }
    else if (mode == 4)
    {
        output.Color.rgb = calcT * 0.5f + 0.5f;
        output.Color.a   = 1.0f;
    }
    else if (mode == 5)
    {
        output.Color.rgb = calcB * 0.5f + 0.5f;
        output.Color.a   = 1.0f;
    }
    else if (mode == 6)
    {
        output.Color.rg = input.TexCoord;
        output.Color.ba = 1.0f.xx;
    }
    else if (mode == 7)
    {
        output.Color.rgb = ToSRGB(HueToRGB(input.MeshletId * 1.71f));
        output.Color.a   = 1.0f;
    }
    else if (mode == 8)
    {
        output.Color.rgb = ToSRGB(HueToRGB(input.PrimitiveId * 1.71f));
        output.Color.a   = 1.0f;
    }
    
    return output;
}