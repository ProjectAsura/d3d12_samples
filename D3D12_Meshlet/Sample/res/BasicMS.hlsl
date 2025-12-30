//-----------------------------------------------------------------------------
// File : BasicMS.hlsl
// Desc : Vertex Shader.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// MSInput structure
///////////////////////////////////////////////////////////////////////////////
struct MSInput
{
    float3  Position;  // 位置座標です.
    float3  Normal;    // 法線ベクトルです.
    float2  TexCoord;  // テクスチャ座標です.
    float3  Tangent;   // 接線ベクトルです.
};

///////////////////////////////////////////////////////////////////////////////
// MSOutput structure
///////////////////////////////////////////////////////////////////////////////
struct MSOutput
{
    float4      Position        : SV_POSITION;  // 位置座標です.
    float2      TexCoord        : TEXCOORD;     // テクスチャ座標です.
    float3      WorldPos        : WORLD_POS;    // ワールド空間の位置座標です.
    float3      Tangent         : TANGENT;      // 接線ベクトルです.
    float3      Bitangent       : BITANGENT;    // 従接線ベクトルです.
    float3      Normal          : NORMAL;       // 法線ベクトルです.
};

///////////////////////////////////////////////////////////////////////////////
// Transform structure
///////////////////////////////////////////////////////////////////////////////
struct TransformParam
{
    float4x4 View;  // ビュー行列です.
    float4x4 Proj;  // 射影行列です.
};

///////////////////////////////////////////////////////////////////////////////
// MeshParam structure
///////////////////////////////////////////////////////////////////////////////
struct MeshParam
{
    float4x4 World; // ワールド行列です.
};

///////////////////////////////////////////////////////////////////////////////
// Meshlet structure
///////////////////////////////////////////////////////////////////////////////
struct Meshlet
{
    uint VertexOffset;      // 頂点番号オフセット.
    uint VertexCount;       // 出力頂点数.
    uint PrimitiveOffset;   // プリミティブ番号オフセット.
    uint PrimitiveCount;    // 出力プリミティブ数.
};

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------
ConstantBuffer<TransformParam> CbTransform  : register(b0);
ConstantBuffer<MeshParam>      CbMesh       : register(b1);
StructuredBuffer<MSInput>      Vertices     : register(t0);
StructuredBuffer<uint>         Indices      : register(t1);
StructuredBuffer<Meshlet>      Meshlets     : register(t2);
StructuredBuffer<uint>         Primitives   : register(t3);


//------------------------------------------------------------------------------
//      パッキングされたインデックスデータを展開する.
//------------------------------------------------------------------------------
uint3 UnpackPrimitiveIndex(uint packedIndex)
{
    return uint3(
        packedIndex & 0x3FF,
        (packedIndex >> 10) & 0x3FF,
        (packedIndex >> 20) & 0x3FF);
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
[numthreads(128, 1, 1)]
[outputtopology("triangle")]
void main
(
    uint groupThreadId  : SV_GroupThreadID,
    uint groupId        : SV_GroupID,
    out vertices MSOutput   verts[64],
    out indices  uint3      polys[126]
)
{
    Meshlet m = Meshlets[groupId];

    SetMeshOutputCounts(m.VertexCount, m.PrimitiveCount);

    if (groupThreadId < m.PrimitiveCount)
    {
        uint packedIndex     = Primitives[m.PrimitiveOffset + groupThreadId];
        polys[groupThreadId] = UnpackPrimitiveIndex(packedIndex);
    }
    
    if (groupThreadId < m.VertexCount)
    {
        uint     index  = Indices[m.VertexOffset + groupThreadId];
        MSInput  input  = Vertices[index];
        MSOutput output = (MSOutput)0;

        float4 localPos = float4(input.Position, 1.0f);
        float4 worldPos = mul(CbMesh.World,     localPos);
        float4 viewPos  = mul(CbTransform.View, worldPos);
        float4 projPos  = mul(CbTransform.Proj, viewPos);

        output.Position = projPos;
        output.TexCoord = input.TexCoord;
        output.WorldPos = worldPos.xyz;

        // 基底ベクトル.
        float3 N = normalize(mul((float3x3)CbMesh.World, input.Normal));
        float3 T = normalize(mul((float3x3)CbMesh.World, input.Tangent));
        float3 B = normalize(cross(N, T));

        output.Tangent      = T;
        output.Bitangent    = B;
        output.Normal       = N;

        verts[groupThreadId] = output;
    }
}