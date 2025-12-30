//-----------------------------------------------------------------------------
// File : StructuredBuffer.cpp
// Desc : Structured Buffer Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <StructuredBuffer.h>
#include <DescriptorPool.h>
#include <Logger.h>


///////////////////////////////////////////////////////////////////////////////
// StructruedBuffer class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
StructuredBuffer::StructuredBuffer()
: m_pPool  (nullptr)
, m_pHandle(nullptr)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
StructuredBuffer::~StructuredBuffer()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool StructuredBuffer::Init
(
    ID3D12Device*                   pDevice,
    DescriptorPool*                 pPool,
    size_t                          count,
    size_t                          stride,
    const void*                     pInitData,
    DirectX::ResourceUploadBatch&   batch
)
{
    if (pInitData == nullptr)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    if (!Init(pDevice, pPool, count, stride))
    { return false; }

    auto size = count * stride;

    D3D12_SUBRESOURCE_DATA res = {};
    res.pData       = pInitData;
    res.RowPitch    = size;
    res.SlicePitch  = size;

    batch.Upload(m_pResource.Get(), 0, &res, 1);

    return true;
}

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool StructuredBuffer::Init
(
    ID3D12Device*   pDevice,
    DescriptorPool* pPool,
    size_t          count,
    size_t          stride
)
{
    if (pDevice == nullptr || pPool == nullptr || count == 0 || stride == 0)
    {
        ELOG("Error : Invalid Argument");
        return false;
    }

    assert(m_pPool   == nullptr);
    assert(m_pHandle == nullptr);

    // ディスクリプタプールを設定.
    m_pPool = pPool;
    m_pPool->AddRef();

    // ディスクリプタハンドルを取得.
    m_pHandle = pPool->AllocHandle();
    if (m_pHandle == nullptr)
    {
        ELOG("Error : DescriptorPool::AllocHandle() Failed.");
        return false;
    }

    // ヒーププロパティ.
    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type                   = D3D12_HEAP_TYPE_DEFAULT;
    prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
    prop.CreationNodeMask       = 1;
    prop.VisibleNodeMask        = 1;

    auto size = count * stride;

    // リソースの設定.
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment          = 0;
    desc.Width              = UINT64(size);
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = 1;
    desc.Format             = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    // リソースを生成.
    auto hr = pDevice->CreateCommittedResource(
        &prop, 
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(m_pResource.GetAddressOf()));
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Format                     = DXGI_FORMAT_UNKNOWN;
    viewDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
    viewDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Buffer.FirstElement        = 0;
    viewDesc.Buffer.NumElements         = UINT(count);
    viewDesc.Buffer.StructureByteStride = UINT(stride);
    viewDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;

    pDevice->CreateShaderResourceView(m_pResource.Get(), &viewDesc, m_pHandle->HandleCPU);

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void StructuredBuffer::Term()
{
    m_pResource.Reset();
    if (m_pPool != nullptr && m_pHandle != nullptr)
    {
        m_pPool->FreeHandle(m_pHandle);
        m_pHandle = nullptr;
    }

    if (m_pPool != nullptr)
    {
        m_pPool->Release();
        m_pPool = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* StructuredBuffer::GetResource() const
{ return m_pResource.Get(); }

//-----------------------------------------------------------------------------
//      CPUディスクリプタハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE StructuredBuffer::GetHandleCPU() const
{
    if (m_pHandle != nullptr)
    { return m_pHandle->HandleCPU; }

    return D3D12_CPU_DESCRIPTOR_HANDLE();
}

//-----------------------------------------------------------------------------
//      GPUディスクリプタハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE StructuredBuffer::GetHandleGPU() const
{
    if (m_pHandle != nullptr)
    { return m_pHandle->HandleGPU; }

    return D3D12_GPU_DESCRIPTOR_HANDLE();
}
