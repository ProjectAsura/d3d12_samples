//-----------------------------------------------------------------------------
// File : MeshVS.hlsl
// Desc : Vertex Shader For Model Drawing.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3  Position    : POSITION;
    float3  Normal      : NORMAL;
    float4  Tangent     : TANGENT;
    float2  TexCoord    : TEXCOORD0;
    float4  Color       : COLOR0;
};

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4  Position    : SV_POSITION;
    float3  Normal      : NORMAL;
    float4  Tangent     : TANGENT;
    float2  TexCoord    : TEXCOORD0;
    float4  Color       : COLOR0;
    float4  WorldPos    : WORLD_POS;
};

///////////////////////////////////////////////////////////////////////////////
// SceneParam constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer SceneParam : register(b0)
{
    float4x4    View;
    float4x4    Proj;
    float3      CameraPos;
    float       FieldOfView;
    float       NearClip;
    float       FarClip;
    float       TargetWidth;
    float       TargetHeight;
};

///////////////////////////////////////////////////////////////////////////////
// ModelParam constant buffer.
///////////////////////////////////////////////////////////////////////////////
cbuffer ModelParam : register(b1)
{
    float4x4    World;
};

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
VSOutput main(const VSInput input)
{
    VSOutput output = (VSOutput)0;
    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos = mul(World, localPos);
    float4 viewPos  = mul(View, worldPos);
    float4 projPos  = mul(Proj, viewPos);

    float3 worldNormal  = normalize(mul((float3x3)World, input.Normal));
    float3 worldTangent = normalize(mul((float3x3)World, input.Tangent.xyz));

    output.Position     = projPos;
    output.Normal       = worldNormal;
    output.Tangent      = float4(worldTangent, input.Tangent.w);
    output.TexCoord     = input.TexCoord;
    output.Color        = input.Color;
    output.WorldPos     = worldPos;

    return output;
}
