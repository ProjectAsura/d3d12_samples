//-----------------------------------------------------------------------------
// File : DownScaledDepthPS.hlsl
// Desc : Down Scaled Depth Buffer Generation.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

/*
    注意：
    深度を出力するためにピクセルシェーダを起動する必要があるため，
    RenderTargetViewはバインドする必要があります.
    バインドしない場合はピクセルシェーダが起動されないため，深度が出力されません.
*/

// Reverse-Zが有効なら1
#define ENABLE_REVERSE_Z (0)

///////////////////////////////////////////////////////////////////////////////
// VSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

//-----------------------------------------------------------------------------
// Textures and Samplers.
//-----------------------------------------------------------------------------
Texture2D    g_FullSizeDepth : register(t0);
SamplerState g_PointSampler  : register(s0);

//-----------------------------------------------------------------------------
//     メインエントリーポイントです.
//-----------------------------------------------------------------------------
float main(const VSOutput input) : SV_DEPTH
{
    float4 depth = g_FullSizeDepth.GatherRed(g_PointSampler, input.TexCoord);

#if ENABLE_REVERSE_Z
    return min(min(depth.x, depth.y), min(depth.z, depth.w)); // 一番奥の値を返す.
#else
    return max(max(depth.x, depth.y), max(depth.z, depth.w)); // 一番奥の値を返す.
#endif
};
