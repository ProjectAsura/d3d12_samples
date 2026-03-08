//-----------------------------------------------------------------------------
// File : ShapeVS.hlsl
// Desc : Vertex Shader For Debug Shape.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// VSInput structure
///////////////////////////////////////////////////////////////////////////////
struct VSInput
{
    float3 Position : POSITION;
};

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
};

///////////////////////////////////////////////////////////////////////////////
// CbCamera constant buffers.
///////////////////////////////////////////////////////////////////////////////
cbuffer CbCamera : register(b0)
{
    float4x4 View;
    float4x4 Proj;
};

///////////////////////////////////////////////////////////////////////////////
// CbShape constant buffers.
///////////////////////////////////////////////////////////////////////////////
cbuffer CbShape : register(b1)
{
    float4x4 World;
    float4   Color;
};

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
VSOutput main(const VSInput input)
{
    VSOutput output = (VSOutput)0;
    
    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos = mul(World, localPos);
    float4 viewPos  = mul(View,  worldPos);
    float4 projPos  = mul(Proj,  viewPos);

    output.Position = projPos;

    return output;
}
