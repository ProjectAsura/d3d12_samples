//-----------------------------------------------------------------------------
// File : Mesh.cpp
// Desc : Mesh Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <Mesh.h>
#include <Logger.h>


namespace {

//-----------------------------------------------------------------------------
//      ディスクリプタを設定します.
//-----------------------------------------------------------------------------
void SetDescriptor
(
    ID3D12GraphicsCommandList6*     pCmd,
    const MeshDescriptorEntry&      entry,
    const StructuredBuffer&         buffer
)
{
    if (entry.DescriptorTable)
    {
        pCmd->SetGraphicsRootDescriptorTable(
            entry.ParamIndex,
            buffer.GetHandleGPU());
    }
    else
    {
        pCmd->SetGraphicsRootShaderResourceView(
            entry.ParamIndex,
            buffer.GetResource()->GetGPUVirtualAddress());
    }
}

} // namespace


///////////////////////////////////////////////////////////////////////////////
// Mesh class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Mesh::Mesh()
: m_MaterialId      (UINT32_MAX)
, m_IndexCount      (0)
, m_MeshlsetCount   (0)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Mesh::~Mesh()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Mesh::Init(ID3D12Device* pDevice, const ResMesh& resource)
{
    if (pDevice == nullptr)
    { return false; }

    if (!m_VB.Init<MeshVertex>(
        pDevice, resource.Vertices.size(), resource.Vertices.data()))
    { return false; }

    if (!m_IB.Init(
        pDevice, resource.Indices.size(), resource.Indices.data()))
    { return false; }

    m_MaterialId = resource.MaterialId;
    m_IndexCount = uint32_t(resource.Indices.size());

    return true;
}

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Mesh::Init
(
    ID3D12Device*                   pDevice,
    DescriptorPool*                 pPool,
    const ResMesh&                  resource,
    DirectX::ResourceUploadBatch&   batch
)
{
    if (pDevice == nullptr)
    {
        ELOG("Error : Invalid Argument");
        return false;
    }

    if (!m_Vertices.Init(
        pDevice,
        pPool,
        resource.Vertices.size(),
        sizeof(MeshVertex),
        resource.Vertices.data(),
        batch))
    {
        ELOG("Error : Vertex Buffer Initialize Failed.");
        return false;
    }

    if (!m_UniqueVertexIndices.Init(
        pDevice,
        pPool,
        resource.UniqueVertexIndices.size(),
        sizeof(uint32_t),
        resource.UniqueVertexIndices.data(),
        batch))
    {
        ELOG("Error : Unique Vertex Indices Initialize Failed.");
        return false;
    }

    if (!m_PrimitiveIndices.Init(
        pDevice,
        pPool,
        resource.PrimitiveIndices.size(),
        sizeof(ResPrimitiveIndex),
        resource.PrimitiveIndices.data(),
        batch))
    {
        ELOG("Error : Primitive Indices Initialize Failed.");
        return false;
    }

    if (!m_Meshlests.Init(
        pDevice,
        pPool,
        resource.Meshlets.size(),
        sizeof(ResMeshlet),
        resource.Meshlets.data(),
        batch))
    {
        ELOG("Error : Meshlets Initialize Failed.");
        return false;
    }

    m_MaterialId    = resource.MaterialId;
    m_MeshlsetCount = uint32_t(resource.Meshlets.size());

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void Mesh::Term()
{
    m_VB.Term();
    m_IB.Term();
    m_MaterialId = UINT32_MAX;
    m_IndexCount = 0;

    m_Vertices              .Term();
    m_UniqueVertexIndices   .Term();
    m_PrimitiveIndices      .Term();
    m_Meshlests             .Term();
    m_MeshlsetCount = 0;
}

//-----------------------------------------------------------------------------
//      描画処理を行います.
//-----------------------------------------------------------------------------
void Mesh::Draw(ID3D12GraphicsCommandList* pCmdList)
{
    auto VBV = m_VB.GetView();
    auto IBV = m_IB.GetView();
    pCmdList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    pCmdList->IASetVertexBuffers(0, 1, &VBV);
    pCmdList->IASetIndexBuffer(&IBV);
    pCmdList->DrawIndexedInstanced(m_IndexCount, 1, 0, 0, 0);
}

//-----------------------------------------------------------------------------
//      描画処理を行います.
//-----------------------------------------------------------------------------
void Mesh::Dispatch
(
    ID3D12GraphicsCommandList6*     pCmdList,
    const MeshDescriptorSetting&    setting
)
{
    SetDescriptor(pCmdList, setting.Vertices, m_Vertices);
    SetDescriptor(pCmdList, setting.UniqueVertexIndices, m_UniqueVertexIndices);
    SetDescriptor(pCmdList, setting.PrimitiveIndices, m_PrimitiveIndices);
    SetDescriptor(pCmdList, setting.Meshlets, m_Meshlests);
    pCmdList->DispatchMesh(m_MeshlsetCount, 1, 1);
}

//-----------------------------------------------------------------------------
//      マテリアルIDを取得します.
//-----------------------------------------------------------------------------
uint32_t Mesh::GetMaterialId() const
{ return m_MaterialId; }
