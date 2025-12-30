//-----------------------------------------------------------------------------
// File : DepthTarget.cpp
// Desc : Depth Target Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "DepthTarget.h"
#include "DescriptorPool.h"


///////////////////////////////////////////////////////////////////////////////
// DepthTarget class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
DepthTarget::DepthTarget()
: m_pTarget     (nullptr)
, m_pHandleDSV  (nullptr)
, m_pPoolDSV    (nullptr)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
DepthTarget::~DepthTarget()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool DepthTarget::Init
(
    ID3D12Device*   pDevice,
    DescriptorPool* pPoolRTV,
    uint32_t        width,
    uint32_t        height,
    DXGI_FORMAT     format
)
{
    if (pDevice == nullptr || pPoolRTV == nullptr || width == 0 || height == 0)
    { return false; }

    assert(m_pHandleDSV == nullptr);
    assert(m_pPoolDSV   == nullptr);

    m_pPoolDSV = pPoolRTV;
    m_pPoolDSV->AddRef();

    m_pHandleDSV = m_pPoolDSV->AllocHandle();
    if (m_pHandleDSV == nullptr)
    { return false; }

    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type                   = D3D12_HEAP_TYPE_DEFAULT;
    prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
    prop.CreationNodeMask       = 1;
    prop.VisibleNodeMask        = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = 0;
    desc.Width              = UINT64(width);
    desc.Height             = height;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = 1;
    desc.Format             = format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format               = format;
    clearValue.DepthStencil.Depth   = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    auto hr = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(m_pTarget.GetAddressOf()));
    if (FAILED(hr))
    { return false; }

    m_ViewDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
    m_ViewDesc.Format             = format;
    m_ViewDesc.Texture2D.MipSlice = 0;
    m_ViewDesc.Flags              = D3D12_DSV_FLAG_NONE;

    pDevice->CreateDepthStencilView(
        m_pTarget.Get(),
        &m_ViewDesc,
        m_pHandleDSV->HandleCPU);

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void DepthTarget::Term()
{
    m_pTarget.Reset();

    if (m_pPoolDSV != nullptr && m_pHandleDSV != nullptr)
    {
        m_pPoolDSV->FreeHandle(m_pHandleDSV);
        m_pHandleDSV = nullptr;
    }

    if (m_pPoolDSV != nullptr)
    {
        m_pPoolDSV->Release();
        m_pPoolDSV = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      ディスクリプタハンドル(DSV用)を取得します.
//-----------------------------------------------------------------------------
DescriptorHandle* DepthTarget::GetHandleDSV() const
{ return m_pHandleDSV; }

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* DepthTarget::GetResource() const
{ return m_pTarget.Get(); }

//-----------------------------------------------------------------------------
//      リソース設定を取得します.
//-----------------------------------------------------------------------------
D3D12_RESOURCE_DESC DepthTarget::GetDesc() const
{
    if (m_pTarget == nullptr)
    { return D3D12_RESOURCE_DESC(); }

    return m_pTarget->GetDesc();
}

//-----------------------------------------------------------------------------
//      深度ステンシルビューの設定を取得します.
//-----------------------------------------------------------------------------
D3D12_DEPTH_STENCIL_VIEW_DESC DepthTarget::GetViewDesc() const
{ return m_ViewDesc; }
