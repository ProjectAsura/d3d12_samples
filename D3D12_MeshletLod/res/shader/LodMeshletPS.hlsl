

struct PSInput
{
    float4  Position    : SV_Position;
    float3  Normal      : NORMAL;
    float2  TexCoord    : TEXCOORD0;
    uint    MeshletId   : MESHLET_ID;
    uint    PrimitiveId : PRIMITIVE_ID;
    uint    LodIndex    : LOD_INDEX;
    uint    MaterialId  : MATERIAL_ID;
};

struct PSOutput
{
    float4 Color : SV_Target0;
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

///////////////////////////////////////////////////////////////////////////////
// LodRange structure
///////////////////////////////////////////////////////////////////////////////
struct LodRange
{
    uint    Offset;
    uint    Count;
};

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

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
ConstantBuffer<Constants>           g_Constants     : register(b0);
ConstantBuffer<TransParam>          g_TransParam    : register(b1);
StructuredBuffer<LodRange>          g_LodRanges     : register(t7);

PSOutput main(const PSInput input)
{
    PSOutput output = (PSOutput)0;
    
    bool wireframe = (g_Constants.Flags & 0x1);

    uint mode = (g_Constants.Flags >> 1) & 0xF;
    float3 N = normalize(input.Normal);
    //float3 T = normalize(input.Tangent);
    //float3 B = normalize(cross(N, T));
    
    float3 lightDir = normalize(g_TransParam.View._31_32_33);
    float NoL = saturate(dot(normalize(input.Normal), lightDir));
    
    float lighting = (wireframe) ? 1.0f : NoL;
    
    if (mode == 0)
    {
        output.Color.rgb = lighting.xxx;
        output.Color.a = 1.0f;
    }
    else if (mode == 1)
    {
        output.Color.rgb = N * 0.5f + 0.5f;
        output.Color.a   = 1.0f;
    }
    else if (mode == 2)
    {
        output.Color.rg = input.TexCoord;
        output.Color.ba = 1.0f.xx;
    }
    else if (mode == 3)
    {
        output.Color.rgb = ToSRGB(HueToRGB(input.MeshletId * 1.71f));
        output.Color.a   = 1.0f;
    }
    else if (mode == 4)
    {
        output.Color.rgb = ToSRGB(HueToRGB(input.PrimitiveId * 1.71f));
        output.Color.a   = 1.0f;
    }
    else if (mode == 5)
    {
        uint count = 0;
        uint stride = 0;
        g_LodRanges.GetDimensions(count, stride);

        float lod = float(input.LodIndex) / float(count);
        output.Color.rgb = ToSRGB(HueToRGB(lod)) * lighting;
        output.Color.a   = 1.0f;
    }
    else if (mode == 6)
    {
        output.Color.rgb = ToSRGB(HueToRGB(input.MaterialId * 1.71f)) * lighting;
        output.Color.a   = 1.0f;
    }
    
    return output;
}