//-----------------------------------------------------------------------------
// File : ColorTarget.cpp
// Desc : Color Target Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "ColorTarget.h"
#include "DescriptorPool.h"


///////////////////////////////////////////////////////////////////////////////
// ColorTarget class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ColorTarget::ColorTarget()
: m_pTarget     (nullptr)
, m_pHandleRTV  (nullptr)
, m_pPoolRTV    (nullptr)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ColorTarget::~ColorTarget()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool ColorTarget::Init
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

    assert(m_pHandleRTV == nullptr);
    assert(m_pPoolRTV   == nullptr);

    m_pPoolRTV = pPoolRTV;
    m_pPoolRTV->AddRef();

    m_pHandleRTV = m_pPoolRTV->AllocHandle();
    if (m_pHandleRTV == nullptr)
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
    desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format   = format;
    clearValue.Color[0] = 1.0f;
    clearValue.Color[1] = 1.0f;
    clearValue.Color[2] = 1.0f;
    clearValue.Color[3] = 1.0f;

    auto hr = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &clearValue,
        IID_PPV_ARGS(m_pTarget.GetAddressOf()));
    if (FAILED(hr))
    { return false; }

    m_ViewDesc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE2D;
    m_ViewDesc.Format                 = format;
    m_ViewDesc.Texture2D.MipSlice     = 0;
    m_ViewDesc.Texture2D.PlaneSlice   = 0;

    pDevice->CreateRenderTargetView(
        m_pTarget.Get(),
        &m_ViewDesc,
        m_pHandleRTV->HandleCPU);

    return true;
}

//-----------------------------------------------------------------------------
//      バックバッファから初期化処理を行います.
//-----------------------------------------------------------------------------
bool ColorTarget::InitFromBackBuffer
(
    ID3D12Device*   pDevice,
    DescriptorPool* pPoolRTV,
    uint32_t        index,
    IDXGISwapChain* pSwapChain
)
{
    if (pDevice == nullptr || pPoolRTV == nullptr || pSwapChain == nullptr)
    { return false; }

    assert(m_pHandleRTV == nullptr);
    assert(m_pPoolRTV   == nullptr);

    m_pPoolRTV = pPoolRTV;
    m_pPoolRTV->AddRef();

    m_pHandleRTV = m_pPoolRTV->AllocHandle();
    if (m_pHandleRTV == nullptr)
    { return false; }

    auto hr = pSwapChain->GetBuffer(index, IID_PPV_ARGS(m_pTarget.GetAddressOf()));
    if (FAILED(hr))
    { return false; }

    DXGI_SWAP_CHAIN_DESC desc;
    pSwapChain->GetDesc(&desc);

    m_ViewDesc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE2D;
    m_ViewDesc.Format                 = desc.BufferDesc.Format;
    m_ViewDesc.Texture2D.MipSlice     = 0;
    m_ViewDesc.Texture2D.PlaneSlice   = 0;

    pDevice->CreateRenderTargetView(
        m_pTarget.Get(),
        &m_ViewDesc,
        m_pHandleRTV->HandleCPU);

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void ColorTarget::Term()
{
    m_pTarget.Reset();

    if (m_pPoolRTV != nullptr && m_pHandleRTV != nullptr)
    {
        m_pPoolRTV->FreeHandle(m_pHandleRTV);
        m_pHandleRTV = nullptr;
    }

    if (m_pPoolRTV != nullptr)
    {
        m_pPoolRTV->Release();
        m_pPoolRTV = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      ディスクリプタハンドルを取得します.
//-----------------------------------------------------------------------------
DescriptorHandle* ColorTarget::GetHandleRTV() const
{ return m_pHandleRTV; }

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* ColorTarget::GetResource() const
{ return m_pTarget.Get(); }

//-----------------------------------------------------------------------------
//      リソース設定を取得します.
//-----------------------------------------------------------------------------
D3D12_RESOURCE_DESC ColorTarget::GetDesc() const
{
    if (m_pTarget == nullptr)
    { return D3D12_RESOURCE_DESC(); }

    return m_pTarget->GetDesc();
}

//-----------------------------------------------------------------------------
//      レンダーターゲットビューの設定を取得します.
//-----------------------------------------------------------------------------
D3D12_RENDER_TARGET_VIEW_DESC ColorTarget::GetViewDesc() const
{ return m_ViewDesc; }
