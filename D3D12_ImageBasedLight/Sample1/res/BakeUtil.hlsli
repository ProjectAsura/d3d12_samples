//-----------------------------------------------------------------------------
// File : BakeUtil.hlsli
// Desc : IBL Bake Utility.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
static const uint SampleCount = 128;

#ifndef ENABLE_MIPMAP_FILTERING
#define ENABLE_MIPMAP_FILTERING     (1)
#endif//ENABLE_MIPMAP_FILTERING


//-----------------------------------------------------------------------------
//      Hammersley点群をサンプルします.
//-----------------------------------------------------------------------------
float2 Hammersley(uint i, uint N)
{
    float ri = reversebits(i) * 2.3283064365386963e-10f;
    return float2(float(i) / float(N), ri);
}

//-----------------------------------------------------------------------------
//      キューブマップのフェッチ方向を求めます.
//-----------------------------------------------------------------------------
float3 CalcDirection(float2 uv, const int faceIndex)
{
     float3 dir = 0;
     float2 pos = uv * 2.0f - 1.0f;

     switch(faceIndex)
     {
         case 0 : { dir = float3( 1.0f,   pos.y,  -pos.x); } break;
         case 1 : { dir = float3(-1.0f,   pos.y,   pos.x); } break;
         case 2 : { dir = float3( pos.x,   1.0f,  -pos.y); } break;
         case 3 : { dir = float3( pos.x,  -1.0f,   pos.y); } break;
         case 4 : { dir = float3( pos.x,  pos.y,    1.0f); } break;
         case 5 : { dir = float3(-pos.x,  pos.y,   -1.0f); } break;
     };

     return normalize(dir);
};

//-----------------------------------------------------------------------------
//      正規直交基底を求めます.
//-----------------------------------------------------------------------------
void TangentSpace(float3 N, out float3 T, out float3 B)
{
    // Tom Duff, James Burgess, Per Christensen, Christophe Hery, Andrew Kensler, Max Liani, and Ryusuke Villemin
    // "Building an Orthonormal Bais, Revisited",
    // Journal of Computer Graphics Techniques Vol.6, No.1, 2017.
    // Listing 3.参照.
    float s = (N.z >= 0.0f) ? 1.0f : -1.0f;
    float a = -1.0f / (s + N.z);
    float b = N.x * N.y * a;
    T = float3(1.0f + s * N.x * N.x * a, s * b, -s * N.x);
    B = float3(b, s + N.y * N.y * a, -N.y);
}

//-----------------------------------------------------------------------------
//      Lambert BRDFの形状に基づくサンプリングを行います.
//-----------------------------------------------------------------------------
float3 SampleLambert(float2 u, float3 N)
{
    float r   = sqrt(u.y);
    float phi = 2.0 * F_PI * u.x;

    float3 H;
    H.x = r * cos(phi);
    H.y = r * sin(phi);
    H.z = sqrt(1 - u.y);

    float3 T, B;
    TangentSpace(N, T, B);

    return normalize(T * H.x + B * H.y + N * H.z);
}

//-----------------------------------------------------------------------------
//      GGX BRDFの形状にもとづくサンプリングを行います.
//-----------------------------------------------------------------------------
float3 SampleGGX(float2 u, float a, float3 N)
{
    float phi = 2.0 * F_PI * u.x;
    float cosTheta = sqrt( (1.0 - u.y) / max(u.y * (a * a - 1.0) + 1.0, 1e-8f) );
    float sinTheta = sqrt( 1.0 - cosTheta * cosTheta );

    float3 H;
    H.x = sinTheta * cos( phi );
    H.y = sinTheta * sin( phi );
    H.z = cosTheta;

    float3 T, B;
    TangentSpace(N, T, B);

    return normalize(T * H.x + B * H.y + N * H.z);
}
