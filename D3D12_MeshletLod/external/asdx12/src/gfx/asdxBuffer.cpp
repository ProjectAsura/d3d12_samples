//-----------------------------------------------------------------------------
// File : asdxBuffer.cpp
// Desc : Buffer Wrapper.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cassert>
#include <gfx/asdxBuffer.h>
#include <gfx/asdxDevice.h>
#include <fnd/asdxLogger.h>


namespace asdx {

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
VertexBuffer::VertexBuffer()
{ memset(&m_View, 0, sizeof(m_View)); }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
VertexBuffer::~VertexBuffer()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool VertexBuffer::Init(uint64_t size, uint32_t stride)
{
    auto pDevice = GetD3D12Device();

    if (pDevice == nullptr || size == 0 || stride == 0)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type                   = D3D12_HEAP_TYPE_UPLOAD;
    prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
    prop.VisibleNodeMask        = 1;
    prop.CreationNodeMask       = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width              = size;
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.Format             = DXGI_FORMAT_UNKNOWN;
    desc.MipLevels          = 1;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    auto state = D3D12_RESOURCE_STATE_GENERIC_READ;
    auto flags = D3D12_HEAP_FLAG_NONE;

    auto hr = pDevice->CreateCommittedResource(
        &prop,
        flags,
        &desc,
        state,
        nullptr,
        IID_PPV_ARGS(m_pResource.GetAddress()));
    if ( FAILED(hr) )
    {
        ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
        return false;
    }

    m_View.BufferLocation   = m_pResource->GetGPUVirtualAddress();
    m_View.SizeInBytes      = UINT(size);
    m_View.StrideInBytes    = stride;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void VertexBuffer::Term()
{
    auto resource = m_pResource.Detach();
    Dispose(resource);
    memset(&m_View, 0, sizeof(m_View));
}

//-----------------------------------------------------------------------------
//      メモリマッピングを行います.
//-----------------------------------------------------------------------------
void* VertexBuffer::Map()
{
    if (m_pResource.GetPtr() == nullptr)
    { return nullptr; }

    void* ptr = nullptr;
    auto hr = m_pResource->Map(0, nullptr, &ptr);
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
        return nullptr;
    }

    return ptr;
}

//-----------------------------------------------------------------------------
//      メモリマッピングを解除します.
//-----------------------------------------------------------------------------
void VertexBuffer::Unmap()
{
    if (m_pResource.GetPtr() == nullptr)
    { return; }

    m_pResource->Unmap(0, nullptr);
}

//-----------------------------------------------------------------------------
//      頂点バッファビューを取得します.
//-----------------------------------------------------------------------------
D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetVBV() const
{ return m_View; }

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* VertexBuffer::GetResource() const
{ return m_pResource.GetPtr(); }


///////////////////////////////////////////////////////////////////////////////
// IndexBuffer class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
IndexBuffer::IndexBuffer()
{ memset(&m_View, 0, sizeof(m_View)); }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
IndexBuffer::~IndexBuffer()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool IndexBuffer::Init(uint64_t size, bool isShortFormat)
{
    auto pDevice = GetD3D12Device();

    if (pDevice == nullptr || size == 0)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type                   = D3D12_HEAP_TYPE_UPLOAD;
    prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
    prop.VisibleNodeMask        = 1;
    prop.CreationNodeMask       = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width              = size;
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.Format             = DXGI_FORMAT_UNKNOWN;
    desc.MipLevels          = 1;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    auto state = D3D12_RESOURCE_STATE_GENERIC_READ;
    auto flags = D3D12_HEAP_FLAG_NONE;

    auto hr = pDevice->CreateCommittedResource(
        &prop,
        flags,
        &desc,
        state,
        nullptr,
        IID_PPV_ARGS(m_pResource.GetAddress()));
    if ( FAILED(hr) )
    {
        ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. errode = 0x%x", hr);
        return false;
    }

    m_View.BufferLocation   = m_pResource->GetGPUVirtualAddress();
    m_View.SizeInBytes      = UINT(size);
    m_View.Format           = (isShortFormat) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void IndexBuffer::Term()
{
    auto resource = m_pResource.Detach();
    Dispose(resource);
    memset(&m_View, 0, sizeof(m_View));
}

//-----------------------------------------------------------------------------
//      メモリマッピングを行います.
//-----------------------------------------------------------------------------
void* IndexBuffer::Map()
{
    if (m_pResource.GetPtr() == nullptr)
    { return nullptr; }

    void* ptr = nullptr;
    auto hr = m_pResource->Map(0, nullptr, &ptr);
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
        return nullptr;
    }

    return ptr;
}

//-----------------------------------------------------------------------------
//      メモリマッピングを解除します.
//-----------------------------------------------------------------------------
void IndexBuffer::Unmap()
{
    if (m_pResource.GetPtr() == nullptr)
    { return; }

    m_pResource->Unmap(0, nullptr);
}

//-----------------------------------------------------------------------------
//      インデックスバッファビューを取得します.
//-----------------------------------------------------------------------------
D3D12_INDEX_BUFFER_VIEW IndexBuffer::GetIBV() const
{ return m_View; }

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* IndexBuffer::GetResource() const
{ return m_pResource.GetPtr(); }


///////////////////////////////////////////////////////////////////////////////
// ConstantBuffer class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ConstantBuffer::ConstantBuffer()
: m_Index(0)
{
    m_Dst[0] = nullptr;
    m_Dst[1] = nullptr;
    m_View[0] = nullptr;
    m_View[1] = nullptr;
    m_Resource[0] = nullptr;
    m_Resource[1] = nullptr;
}

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ConstantBuffer::~ConstantBuffer()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool ConstantBuffer::Init(uint64_t size)
{
    if ( size == 0 )
    {
        ELOG( "Error : Invalid Argument." );
        return false;
    }

    auto rest = size % 256;
    if ( rest != 0 )
    {
        ELOG( "Error : ConstantBuffer must be 256 byte alignment., (size %% 256) = %u", rest );
        return false;
    }

    auto pDevice = GetD3D12Device();

    D3D12_HEAP_PROPERTIES props = {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        1,
        1
    };

    D3D12_RESOURCE_DESC desc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        size,
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        { 1, 0 },
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_NONE
    };

    for(auto i=0; i<2; ++i)
    {
        auto hr = pDevice->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_Resource[i].GetAddress()));
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr );
            return false;
        }

        m_Resource[i]->SetName(L"asdxConstantBuffer");

        hr = m_Resource[i]->Map( 0, nullptr, reinterpret_cast<void**>( &m_Dst[i] ) );
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr );
            return false;
        }

        D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
        viewDesc.SizeInBytes    = static_cast<uint32_t>(size);
        viewDesc.BufferLocation = m_Resource[i]->GetGPUVirtualAddress();

        if (!CreateConstantBufferView(m_Resource[i].GetPtr(), &viewDesc, m_View[i].GetAddress()))
        {
            ELOGA("Erorr : CreateConstantBufferView() Failed.");
            return false;
        }
    }

    m_Size = size;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void ConstantBuffer::Term()
{
    for(auto i=0; i<2; ++i)
    {
        m_View[i].Reset();
        auto ptr = m_Resource[i].Detach();
        Dispose(ptr);
        m_Dst[i] = nullptr;
    }

    m_Size  = 0;
    m_Index = 0;
}

//-----------------------------------------------------------------------------
//      更新処理を行います.
//-----------------------------------------------------------------------------
void ConstantBuffer::Update(const void* pSrc, uint64_t size, uint64_t srcOffset, uint64_t dstOffset)
{
    auto dst = m_Dst[m_Index] + dstOffset;
    auto src = reinterpret_cast<const uint8_t*>(pSrc) + srcOffset;
    auto copy_size = (size > m_Size) ? m_Size : size;

    memcpy(dst, src, copy_size);
}

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* ConstantBuffer::GetResource() const
{ return m_Resource[m_Index].GetPtr(); }

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* ConstantBuffer::GetResource(uint32_t index) const
{
    assert(index < 2);
    return m_Resource[index].GetPtr();
}

//-----------------------------------------------------------------------------
//      定数バッファビューを取得します.
//-----------------------------------------------------------------------------
IConstantBufferView* ConstantBuffer::GetCBV() const
{ return m_View[m_Index].GetPtr(); }

//-----------------------------------------------------------------------------
//      定数バッファビューを取得します.
//-----------------------------------------------------------------------------
IConstantBufferView* ConstantBuffer::GetCBV(uint32_t index) const
{
    assert(index < 2);
    return m_View[index].GetPtr();;
}

//-----------------------------------------------------------------------------
//      バッファを入れ替えます.
//-----------------------------------------------------------------------------
void ConstantBuffer::SwapBuffer()
{ m_Index = (m_Index + 1) & 0x1; }

//-----------------------------------------------------------------------------
//      メモリマッピングを行います.
//-----------------------------------------------------------------------------
void* ConstantBuffer::Map(uint32_t index)
{
    assert(index < 2);
    void* pData = nullptr;
    auto hr = m_Resource[index]->Map(0, nullptr, &pData);
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
        return nullptr;
    }

    return pData;
}

//-----------------------------------------------------------------------------
//      メモリマッピングを解除します.
//-----------------------------------------------------------------------------
void ConstantBuffer::Unmap(uint32_t index)
{
    assert(index < 2);
    m_Resource[index]->Unmap(0, nullptr);
}


///////////////////////////////////////////////////////////////////////////////
// ByteAddressBuffer class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ByteAddressBuffer::ByteAddressBuffer()
: m_Resource()
, m_View    ()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ByteAddressBuffer::~ByteAddressBuffer()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool ByteAddressBuffer::Init(uint64_t size, D3D12_RESOURCE_STATES state)
{
    auto pDevice = GetD3D12Device();

    if (pDevice == nullptr || size == 0)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    auto rest = size % 4;
    if (rest != 0)
    {
        size += rest;
    }

    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type                   = D3D12_HEAP_TYPE_DEFAULT;
    prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
    prop.VisibleNodeMask        = 1;
    prop.CreationNodeMask       = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width              = size;
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.Format             = DXGI_FORMAT_UNKNOWN;
    desc.MipLevels          = 1;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    auto flags = D3D12_HEAP_FLAG_NONE;

    auto hr = pDevice->CreateCommittedResource(
        &prop,
        flags,
        &desc,
        state,
        nullptr,
        IID_PPV_ARGS(m_Resource.GetAddress()));
    if ( FAILED(hr) )
    {
        ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Format                     = DXGI_FORMAT_R32_TYPELESS;
    viewDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
    viewDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.Buffer.FirstElement        = 0;
    viewDesc.Buffer.NumElements         = UINT(size / 4);
    viewDesc.Buffer.StructureByteStride = 0;
    viewDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_RAW;

    if (!CreateShaderResourceView(m_Resource.GetPtr(), &viewDesc, m_View.GetAddress()))
    {
        ELOGA("Error : CreateShaderResourceView() Failed.");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool ByteAddressBuffer::Init
(
    ID3D12GraphicsCommandList*  pCmdList,
    uint64_t                    size,
    const void*                 pInitData
)
{
    if (!Init(size, D3D12_RESOURCE_STATE_COMMON))
    { return false; }

    UpdateBuffer(pCmdList, m_Resource.GetPtr(), pInitData);

    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_Resource.GetPtr();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_GENERIC_READ;
        barrier.Transition.Subresource  = 0;

        pCmdList->ResourceBarrier(1, &barrier);
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void ByteAddressBuffer::Term()
{
    auto resource = m_Resource.Detach();
    Dispose(resource);
    m_View.Reset();
}

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* ByteAddressBuffer::GetResource() const
{ return m_Resource.GetPtr(); }

//-----------------------------------------------------------------------------
//      シェーダリソースビューを取得します.
//-----------------------------------------------------------------------------
IShaderResourceView* ByteAddressBuffer::GetSRV() const
{ return m_View.GetPtr(); }

//-----------------------------------------------------------------------------
//      UAVバリアを設定します.
//-----------------------------------------------------------------------------
void ByteAddressBuffer::UAVBarrier(ID3D12GraphicsCommandList* pCmdList)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type            = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags           = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource   = m_Resource.GetPtr();

    pCmdList->ResourceBarrier(1, &barrier);
}


///////////////////////////////////////////////////////////////////////////////
// StructuredBuffer class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
StructuredBuffer::StructuredBuffer()
: m_Resource   ()
, m_View       ()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
StructuredBuffer::~StructuredBuffer()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool StructuredBuffer::Init(uint64_t count, uint32_t stride, D3D12_RESOURCE_STATES state)
{
    auto pDevice = GetD3D12Device();

    if (pDevice == nullptr || count == 0 || stride == 0)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    uint64_t size = count * stride;
    auto rest = size % 4;
    if (rest != 0)
    {
        size += rest;
    }

    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type                   = D3D12_HEAP_TYPE_DEFAULT;
    prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
    prop.VisibleNodeMask        = 1;
    prop.CreationNodeMask       = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width              = size;
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.Format             = DXGI_FORMAT_UNKNOWN;
    desc.MipLevels          = 1;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    auto flags = D3D12_HEAP_FLAG_NONE;

    auto hr = pDevice->CreateCommittedResource(
        &prop,
        flags,
        &desc,
        state,
        nullptr,
        IID_PPV_ARGS(m_Resource.GetAddress()));
    if ( FAILED(hr) )
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
    viewDesc.Buffer.StructureByteStride = stride;
    viewDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;

    if (!CreateShaderResourceView(m_Resource.GetPtr(), &viewDesc, m_View.GetAddress()))
    {
        ELOGA("Error : CreateShaderResourceView() Failed.");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool StructuredBuffer::Init
(
    ID3D12GraphicsCommandList*  pCmdList,
    uint64_t                    count,
    uint32_t                    stride,
    const void*                 pInitData
)
{
    if (!Init(count, stride, D3D12_RESOURCE_STATE_COMMON))
    { return false;  }

    UpdateBuffer(pCmdList, m_Resource.GetPtr(), pInitData);

    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_Resource.GetPtr();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_GENERIC_READ;
        barrier.Transition.Subresource  = 0;

        pCmdList->ResourceBarrier(1, &barrier);
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void StructuredBuffer::Term()
{
    auto resource = m_Resource.Detach();
    Dispose(resource);
    m_View.Reset();
}

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* StructuredBuffer::GetResource() const
{ return m_Resource.GetPtr(); }

//-----------------------------------------------------------------------------
//      ビューを取得します.
//-----------------------------------------------------------------------------
IShaderResourceView* StructuredBuffer::GetSRV() const
{ return m_View.GetPtr(); }

//-----------------------------------------------------------------------------
//      UAVバリアを設定します.
//-----------------------------------------------------------------------------
void StructuredBuffer::UAVBarrier(ID3D12GraphicsCommandList* pCmdList)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type            = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags           = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource   = m_Resource.GetPtr();

    pCmdList->ResourceBarrier(1, &barrier);
}


///////////////////////////////////////////////////////////////////////////////
// BufferUpdate class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool BufferUpdater::Init(ID3D12Device* pDevice, uint64_t size)
{
    if (pDevice == nullptr || size == 0)
    {
        ELOG("Error : Invalid Argument.");
        return false;
    }

    D3D12_HEAP_PROPERTIES props = {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        1,
        1
    };

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width              = size;
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = 1;
    desc.Format             = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc         = { 1, 0 };
    desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    for(auto i=0; i<2; ++i)
    {
        auto hr = pDevice->CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(m_Resource[i].GetAddress()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateCommittedReosurce() Failed. errcode = 0x%x", hr);
            return false;
        }

        hr = m_Resource[i]->Map(0, nullptr, reinterpret_cast<void**>(&m_AddressCPU[i]));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Resource::Map() Failed. errcode = 0x%x", hr);
            return false;
        }
    }

    m_Size  = size;
    m_Offset = 0;
    m_Index  = 0;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void BufferUpdater::Term()
{
    for(auto i=0; i<2; ++i)
    {
        m_Resource[i].Reset();
        m_AddressCPU[i] = nullptr;
    }

    m_Size   = 0;
    m_Offset = 0;
    m_Index  = 0;
}

//-----------------------------------------------------------------------------
//      バッファを更新します.
//-----------------------------------------------------------------------------
bool BufferUpdater::Update
(
    ID3D12GraphicsCommandList*  pCommandList,
    ID3D12Resource*             pDstResource,
    uint64_t                    dstOffset,
    void*                       pSrcResource,
    uint64_t                    size
)
{
    if (m_Offset + size >= m_Size)
    { return false; }

    auto srcOffset = m_Offset;
    m_Offset += size;

    auto dstAddr = m_AddressCPU[m_Index] + srcOffset;
    memcpy(dstAddr, pSrcResource, size);

    pCommandList->CopyBufferRegion(
        pDstResource,
        dstOffset,
        m_Resource[m_Index].GetPtr(),
        srcOffset,
        size);
    return true;
}

//-----------------------------------------------------------------------------
//      バッファを入れ替えます.
//-----------------------------------------------------------------------------
void BufferUpdater::SwapBuffers()
{
    m_Index  = (m_Index + 1) & 0x1;
    m_Offset = 0;
}

//-----------------------------------------------------------------------------
//      バッファ番号を取得します.
//-----------------------------------------------------------------------------
uint8_t BufferUpdater::GetIndex() const
{ return m_Index; }

} // namespace asdx
