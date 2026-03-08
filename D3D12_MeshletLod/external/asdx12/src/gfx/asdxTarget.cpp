//-----------------------------------------------------------------------------
// File : asdxTarget.cpp
// Desc : Target Wrapper.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cassert>
#include <gfx/asdxTarget.h>
#include <gfx/asdxDevice.h>
#include <fnd/asdxLogger.h>


namespace /* anonymous */ {

//-----------------------------------------------------------------------------
//      sRGBフォーマットに変換します.
//-----------------------------------------------------------------------------
DXGI_FORMAT GetSRGBFormat(DXGI_FORMAT value)
{
    DXGI_FORMAT result = value;

    switch( value )
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        { result = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; }
        break;

    case DXGI_FORMAT_BC1_UNORM:
        { result = DXGI_FORMAT_BC1_UNORM_SRGB; }
        break;

    case DXGI_FORMAT_BC2_UNORM:
        { result = DXGI_FORMAT_BC2_UNORM_SRGB; }
        break;

    case DXGI_FORMAT_BC3_UNORM:
        { result = DXGI_FORMAT_BC3_UNORM_SRGB; }
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
        { result = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; }
        break;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
        { result = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB; }
        break;

    case DXGI_FORMAT_BC7_UNORM:
        { result = DXGI_FORMAT_BC7_UNORM_SRGB; }
        break;
    }

    return result;
}

//-----------------------------------------------------------------------------
//      深度フォーマットからリソースフォーマットに変換します.
//-----------------------------------------------------------------------------
DXGI_FORMAT GetResourceFormat(DXGI_FORMAT value, bool isStencil)
{
    DXGI_FORMAT result = value;

    switch(value)
    {
    case DXGI_FORMAT_D16_UNORM:
        { result = DXGI_FORMAT_R16_UNORM; }
        break;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            if (!isStencil)
                result = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            else
                result = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        }
        break;

    case DXGI_FORMAT_D32_FLOAT:
        { result = DXGI_FORMAT_R32_FLOAT; }
        break;

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        {
            if (!isStencil)
                result = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
            else
                result = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
        }
        break;
    }

    return result;
}

//-----------------------------------------------------------------------------
//      非sRGBフォーマットに変換します.
//-----------------------------------------------------------------------------
DXGI_FORMAT GetNoSRGBFormat(DXGI_FORMAT value)
{
    DXGI_FORMAT result = value;

    switch( value )
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        { result = DXGI_FORMAT_R8G8B8A8_UNORM; }
        break;

    case DXGI_FORMAT_BC1_UNORM_SRGB:
        { result = DXGI_FORMAT_BC1_UNORM; }
        break;

    case DXGI_FORMAT_BC2_UNORM_SRGB:
        { result = DXGI_FORMAT_BC2_UNORM; }
        break;

    case DXGI_FORMAT_BC3_UNORM_SRGB:
        { result = DXGI_FORMAT_BC3_UNORM; }
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        { result = DXGI_FORMAT_B8G8R8A8_UNORM; }
        break;

    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        { result = DXGI_FORMAT_B8G8R8X8_UNORM; }
        break;

    case DXGI_FORMAT_BC7_UNORM_SRGB:
        { result = DXGI_FORMAT_BC7_UNORM; }
        break;
    }

    return result;
}

//-----------------------------------------------------------------------------
//      sRGBフォーマットかどうか?
//-----------------------------------------------------------------------------
bool IsSRGBFormat(DXGI_FORMAT value)
{
    bool result = false;

    switch( value )
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        { result = true; }
        break;
    }

    return result;
}

} // namespace /* anonymouus */


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// ColorTarget class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ColorTarget::ColorTarget()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ColorTarget::~ColorTarget()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理です.
//-----------------------------------------------------------------------------
bool ColorTarget::Init(const TargetDesc* pDesc)
{
    if (pDesc == nullptr)
    {
        ELOGA("Error : Invalid Argument");
        return false;
    }

    if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        ELOGA("Error : Invalid Resource Dimension.");
        return false;
    }

    HRESULT hr = S_OK;

    {
        auto format = GetNoSRGBFormat(pDesc->Format);

        D3D12_HEAP_PROPERTIES props = {
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            1,
            1
        };

        D3D12_RESOURCE_DESC desc = {
            pDesc->Dimension,
            pDesc->Alignment,
            pDesc->Width,
            pDesc->Height,
            pDesc->DepthOrArraySize,
            pDesc->MipLevels,
            format,
            pDesc->SampleDesc,
            D3D12_TEXTURE_LAYOUT_UNKNOWN,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
        };

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format   = format,
        clearValue.Color[0] = pDesc->ClearColor[0];
        clearValue.Color[1] = pDesc->ClearColor[1];
        clearValue.Color[2] = pDesc->ClearColor[2];
        clearValue.Color[3] = pDesc->ClearColor[3];

        hr = GetD3D12Device()->CreateCommittedResource( 
            &props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            pDesc->InitState,
            &clearValue,
            IID_PPV_ARGS(m_pResource.GetAddress()));
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr );
            return false;
        }
    }

#if ASDX_IS_SCARLETT
    auto mostDetailedMip = (pDesc->MipLevels - 1);
#else
    auto mostDetailedMip = 0u;
#endif

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    D3D12_RENDER_TARGET_VIEW_DESC   rtv_desc = {};
    rtv_desc.Format = pDesc->Format;
    srv_desc.Format = pDesc->Format; 
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;


    if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        rtv_desc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE3D;
        rtv_desc.Texture3D.FirstWSlice  = 0;
        rtv_desc.Texture3D.MipSlice     = 0;
        rtv_desc.Texture3D.WSize        = pDesc->DepthOrArraySize;

        srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE3D;
        srv_desc.Texture3D.MipLevels            = pDesc->MipLevels;
        srv_desc.Texture3D.MostDetailedMip      = mostDetailedMip;
        srv_desc.Texture3D.ResourceMinLODClamp  = 0;
    }
    else if(pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        if ( pDesc->DepthOrArraySize > 1 )
        {
            if ( pDesc->SampleDesc.Count > 1 )
            {
                rtv_desc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                rtv_desc.Texture2DMSArray.ArraySize       = pDesc->DepthOrArraySize;
                rtv_desc.Texture2DMSArray.FirstArraySlice = 0;

                srv_desc.ViewDimension                    = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                srv_desc.Texture2DMSArray.ArraySize       = pDesc->DepthOrArraySize;
                srv_desc.Texture2DMSArray.FirstArraySlice = 0;
            }
            else
            {
                rtv_desc.ViewDimension                   = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                rtv_desc.Texture2DArray.ArraySize        = pDesc->DepthOrArraySize;
                rtv_desc.Texture2DArray.FirstArraySlice  = 0;
                rtv_desc.Texture2DArray.MipSlice         = 0;
                rtv_desc.Texture2DArray.PlaneSlice       = 0;

                srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srv_desc.Texture2DArray.ArraySize           = pDesc->DepthOrArraySize;
                srv_desc.Texture2DArray.FirstArraySlice     = 0;
                srv_desc.Texture2DArray.MipLevels           = 0;
                srv_desc.Texture2DArray.PlaneSlice          = 0;
                srv_desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
                srv_desc.Texture2DArray.MostDetailedMip     = mostDetailedMip;
            }
        }
        else
        {
            if ( pDesc->SampleDesc.Count > 1 )
            {
                rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                rtv_desc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtv_desc.Texture2D.MipSlice = 0;

                srv_desc.Texture2D.MipLevels            = pDesc->MipLevels;
                srv_desc.Texture2D.MostDetailedMip      = mostDetailedMip;
                srv_desc.Texture2D.PlaneSlice           = 0;
                srv_desc.Texture2D.ResourceMinLODClamp  = 0.0f;
                srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
            }
        }
    }
    else if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
    {
        if (pDesc->DepthOrArraySize > 1)
        {
            rtv_desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            rtv_desc.Texture1DArray.ArraySize       = pDesc->DepthOrArraySize;
            rtv_desc.Texture1DArray.FirstArraySlice = 0;
            rtv_desc.Texture1DArray.MipSlice        = 0;

            srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            srv_desc.Texture1DArray.ArraySize           = pDesc->DepthOrArraySize;
            srv_desc.Texture1DArray.FirstArraySlice     = 0;
            srv_desc.Texture1DArray.MipLevels           = pDesc->MipLevels;
            srv_desc.Texture1DArray.MostDetailedMip     = mostDetailedMip;
            srv_desc.Texture1DArray.ResourceMinLODClamp = 0;
        }
    }
    else if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        // Not Support Buffer.
        assert(false);
        ELOGA("Error : Not Support Buffer Type.");
        return false;
    }

    if (!CreateRenderTargetView(m_pResource.GetPtr(), &rtv_desc, m_pRTV.GetAddress()))
    {
        ELOG("Error : CreateRenderTargetView() Failed.");
        return false;
    }

    if (!CreateShaderResourceView(m_pResource.GetPtr(), &srv_desc, m_pSRV.GetAddress()))
    {
        ELOG("Error : CreateShaderResourceView() Failed.");
        return false;
    }

    memcpy(&m_Desc, pDesc, sizeof(m_Desc));

    m_PrevState = m_Desc.InitState;

    return true;
}

//-----------------------------------------------------------------------------
//      初期化処理です.
//-----------------------------------------------------------------------------
bool ColorTarget::Init
(
    IDXGISwapChain* pSwapChain,
    uint32_t        backBufferIndex
)
{
    HRESULT hr = S_OK;

    hr = pSwapChain->GetBuffer( backBufferIndex, IID_PPV_ARGS(m_pResource.GetAddress()));
    if ( FAILED( hr ) )
    {
        ELOG( "Error : IDXGISwapChain::GetBuffer() Failed. errcode = 0x%x", hr );
        return false;
    }

    auto desc = m_pResource->GetDesc();
    if ( desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D )
    {
        ELOG( "Error : Invalid Resource Dimension. ");
        return false;
    }

#if ASDX_IS_SCARLETT
    auto mostDetailedMip = (pDesc->MipLevels - 1);
#else
    auto mostDetailedMip = 0u;
#endif

    D3D12_RENDER_TARGET_VIEW_DESC   rtv_desc = {};
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    rtv_desc.Format = desc.Format;
    srv_desc.Format = desc.Format;
    srv_desc.Shader4ComponentMapping =D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if ( desc.DepthOrArraySize > 1 )
    {
        if ( desc.SampleDesc.Count >= 1 )
        {
            rtv_desc.Texture2DMSArray.ArraySize       = desc.DepthOrArraySize;
            rtv_desc.Texture2DMSArray.FirstArraySlice = 0;
            rtv_desc.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;

            srv_desc.Texture2DMSArray.ArraySize       = desc.DepthOrArraySize;
            srv_desc.Texture2DMSArray.FirstArraySlice = 0;
            srv_desc.ViewDimension                    = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
        }
        else
        {
            rtv_desc.Texture2DArray.ArraySize        = desc.DepthOrArraySize;
            rtv_desc.Texture2DArray.FirstArraySlice  = 0;
            rtv_desc.Texture2DArray.MipSlice         = 0;
            rtv_desc.Texture2DArray.PlaneSlice       = 0;
            rtv_desc.ViewDimension                   = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;

            srv_desc.Texture2DArray.ArraySize           = desc.DepthOrArraySize;
            srv_desc.Texture2DArray.FirstArraySlice     = 0;
            srv_desc.Texture2DArray.MipLevels           = desc.MipLevels;
            srv_desc.Texture2DArray.MostDetailedMip     = mostDetailedMip;
            srv_desc.Texture2DArray.PlaneSlice          = 0;
            srv_desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
        }
    }
    else
    {
        if ( desc.SampleDesc.Count <= 1 )
        {
            rtv_desc.Texture2D.MipSlice     = 0;
            rtv_desc.Texture2D.PlaneSlice   = 0;
            rtv_desc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE2D;
                
            srv_desc.Texture2D.MipLevels            = desc.MipLevels;
            srv_desc.Texture2D.MostDetailedMip      = mostDetailedMip;
            srv_desc.Texture2D.PlaneSlice           = 0;
            srv_desc.Texture2D.ResourceMinLODClamp  = 0.0f;
            srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
        }
        else
        {
            rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        }
    }

    if (!CreateRenderTargetView(m_pResource.GetPtr(), &rtv_desc, m_pRTV.GetAddress()))
    {
        ELOG("Error : CreateRenderTargetView() Failed.");
        return false;
    }

    if (!CreateShaderResourceView(m_pResource.GetPtr(), &srv_desc, m_pSRV.GetAddress()))
    {
        ELOG("Error : CreateShaderResourceView() Failed.");
        return false;
    }

    m_PrevState = D3D12_RESOURCE_STATE_COMMON;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void ColorTarget::Term()
{
    m_pSRV.Reset();
    m_pRTV.Reset();

    auto resource = m_pResource.Detach();
    Dispose(resource);

    memset(&m_Desc, 0, sizeof(m_Desc));
}

//-----------------------------------------------------------------------------
//      リサイズ処理を行います.
//-----------------------------------------------------------------------------
bool ColorTarget::Resize(uint32_t width, uint32_t height)
{
    auto desc = m_Desc;
    Term();

    desc.Width  = width;
    desc.Height = height;
    return Init(&desc);
}

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* ColorTarget::GetResource() const
{ return m_pResource.GetPtr(); }

//-----------------------------------------------------------------------------
//      レンダーターゲットビューを取得します.
//-----------------------------------------------------------------------------
IRenderTargetView* ColorTarget::GetRTV() const
{ return m_pRTV.GetPtr(); }

//-----------------------------------------------------------------------------
//      シェーダリソースビューを取得します.
//-----------------------------------------------------------------------------
IShaderResourceView* ColorTarget::GetSRV() const
{ return m_pSRV.GetPtr(); }

//-----------------------------------------------------------------------------
//      構成設定を取得します.
//-----------------------------------------------------------------------------
TargetDesc ColorTarget::GetDesc() const
{ return m_Desc; }

//-----------------------------------------------------------------------------
//      sRGBフラグを取得します.
//-----------------------------------------------------------------------------
bool ColorTarget::IsSRGB() const
{ return IsSRGBFormat(m_Desc.Format); }

//-----------------------------------------------------------------------------
//      デバッグ名を設定します.
//-----------------------------------------------------------------------------
void ColorTarget::SetName(LPCWSTR tag)
{
    if (m_pResource)
    { m_pResource->SetName(tag); }
}


///////////////////////////////////////////////////////////////////////////////
// DepthTarget class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
DepthTarget::DepthTarget()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
DepthTarget::~DepthTarget()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool DepthTarget::Init(const TargetDesc* pDesc)
{
    if (pDesc == nullptr)
    {
        ELOGA("Error : Invalid Argument");
        return false;
    }

    if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER
     || pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        ELOGA("Error : Invalid Resource Dimension");
        return false;
    }

    HRESULT hr = S_OK;

    auto format = GetResourceFormat(pDesc->Format, false);
    {
        D3D12_HEAP_PROPERTIES props = {
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            1,
            1
        };

        D3D12_RESOURCE_DESC desc = {
            pDesc->Dimension,
            pDesc->Alignment,
            pDesc->Width,
            pDesc->Height,
            pDesc->DepthOrArraySize,
            pDesc->MipLevels,
            pDesc->Format,
            pDesc->SampleDesc,
            D3D12_TEXTURE_LAYOUT_UNKNOWN,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        };

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format               = pDesc->Format;
        clearValue.DepthStencil.Depth   = pDesc->ClearDepth;
        clearValue.DepthStencil.Stencil = pDesc->ClearStencil;

        hr = GetD3D12Device()->CreateCommittedResource( 
            &props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            pDesc->InitState,
            &clearValue,
            IID_PPV_ARGS(m_pResource.GetAddress()));
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr );
            return false;
        }
    }

    #if ASDX_IS_SCARLETT
        auto mostDetailedMip = (pDesc->MipLevels - 1);
    #else
        auto mostDetailedMip = 0u;
    #endif

    D3D12_DEPTH_STENCIL_VIEW_DESC   dsv_desc = {};
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    dsv_desc.Format = pDesc->Format;
    srv_desc.Format = format;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        if (pDesc->DepthOrArraySize > 1)
        {
            if (pDesc->SampleDesc.Count > 1)
            {
                dsv_desc.ViewDimension                      = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                dsv_desc.Texture2DMSArray.ArraySize         = pDesc->DepthOrArraySize;
                dsv_desc.Texture2DMSArray.FirstArraySlice   = 0;

                srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                srv_desc.Texture2DMSArray.ArraySize         = pDesc->DepthOrArraySize;
                srv_desc.Texture2DMSArray.FirstArraySlice   = 0;
            }
            else
            {
                dsv_desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                dsv_desc.Texture2DArray.ArraySize       = pDesc->DepthOrArraySize;
                dsv_desc.Texture2DArray.FirstArraySlice = 0;
                dsv_desc.Texture2DArray.MipSlice        = 0;

                srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srv_desc.Texture2DArray.ArraySize           = pDesc->DepthOrArraySize;
                srv_desc.Texture2DArray.FirstArraySlice     = 0;
                srv_desc.Texture2DArray.MipLevels           = pDesc->MipLevels;
                srv_desc.Texture2DArray.MostDetailedMip     = mostDetailedMip;
                srv_desc.Texture2DArray.PlaneSlice          = 0;
                srv_desc.Texture2DArray.ResourceMinLODClamp = 0;
            }
        }
        else
        {
            if (pDesc->SampleDesc.Count > 1)
            {
                dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                dsv_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsv_desc.Texture2D.MipSlice = 0;

                srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
                srv_desc.Texture2D.MipLevels            = pDesc->MipLevels;
                srv_desc.Texture2D.MostDetailedMip      = mostDetailedMip;
                srv_desc.Texture2D.PlaneSlice           = 0;
                srv_desc.Texture2D.ResourceMinLODClamp  = 0;
            }
        }

    }
    else if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
    {
        if (pDesc->DepthOrArraySize > 1)
        {
            dsv_desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            dsv_desc.Texture1DArray.ArraySize       = pDesc->DepthOrArraySize;
            dsv_desc.Texture1DArray.FirstArraySlice = 0;
            dsv_desc.Texture1DArray.MipSlice        = 0;

            srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            srv_desc.Texture1DArray.ArraySize           = pDesc->DepthOrArraySize;
            srv_desc.Texture1DArray.FirstArraySlice     = 0;
            srv_desc.Texture1DArray.MipLevels           = pDesc->MipLevels;
            srv_desc.Texture1DArray.MostDetailedMip     = mostDetailedMip;
            srv_desc.Texture1DArray.ResourceMinLODClamp = 0;
        }
        else
        {
            dsv_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE1D;
            dsv_desc.Texture1D.MipSlice = 0;

            srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE1D;
            srv_desc.Texture1D.MipLevels            = pDesc->MipLevels;
            srv_desc.Texture1D.MostDetailedMip      = mostDetailedMip;
            srv_desc.Texture1D.ResourceMinLODClamp  = 0;
        }
    }
    else if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        // Not Support Buffer.
        assert(false);
        ELOGA("Error : Not Support Buffer Type.");
        return false;
    }

    if (!CreateDepthStencilView(m_pResource.GetPtr(), &dsv_desc, m_pDSV.GetAddress()))
    {
        ELOG("Error : CreateDepthStencilView() Failed.");
        return false;
    }

    if (!CreateShaderResourceView(m_pResource.GetPtr(), &srv_desc, m_pSRV.GetAddress()))
    {
        ELOG("Error : CreateShaderResourceView() Failed.");
        return false;
    }

    memcpy(&m_Desc, pDesc, sizeof(m_Desc));

    m_PrevState = m_Desc.InitState;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void DepthTarget::Term()
{
    m_pDSV.Reset();
    m_pSRV.Reset();

    auto resource = m_pResource.Detach();
    Dispose(resource);

    memset(&m_Desc, 0, sizeof(m_Desc));
}

//-----------------------------------------------------------------------------
//      リサイズ処理を行います.
//-----------------------------------------------------------------------------
bool DepthTarget::Resize(uint32_t width, uint32_t height)
{
    auto desc = m_Desc;
    Term();

    desc.Width  = width;
    desc.Height = height;
    return Init(&desc);
}

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* DepthTarget::GetResource() const
{ return m_pResource.GetPtr(); }

//-----------------------------------------------------------------------------
//      深度ステンシルビューを取得します.
//-----------------------------------------------------------------------------
IDepthStencilView* DepthTarget::GetDSV() const
{ return m_pDSV.GetPtr(); }

//-----------------------------------------------------------------------------
//      シェーダリソースビューを取得します.
//-----------------------------------------------------------------------------
IShaderResourceView* DepthTarget::GetSRV() const
{ return m_pSRV.GetPtr(); }

//-----------------------------------------------------------------------------
//      構成設定を取得します
//-----------------------------------------------------------------------------
TargetDesc DepthTarget::GetDesc() const
{ return m_Desc; }

//-----------------------------------------------------------------------------
//      デバッグ名を設定します.
//-----------------------------------------------------------------------------
void DepthTarget::SetName(LPCWSTR tag)
{
    if (m_pResource)
    { m_pResource->SetName(tag); }
}


///////////////////////////////////////////////////////////////////////////////
// ComputeTarget class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ComputeTarget::ComputeTarget()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ComputeTarget::~ComputeTarget()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool ComputeTarget::Init(const TargetDesc* pDesc, uint32_t stride)
{
    if (pDesc == nullptr)
    {
        ELOGA("Error : Invalid Argument");
        return false;
    }

    if (pDesc->SampleDesc.Count > 1)
    {
        ELOGA("Error : Invalid Argument");
        return false;
    }

    HRESULT hr = S_OK;

    {
        D3D12_HEAP_PROPERTIES props = {
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            1,
            1
        };

        auto layout = (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
            ? D3D12_TEXTURE_LAYOUT_ROW_MAJOR
            : D3D12_TEXTURE_LAYOUT_UNKNOWN;

        D3D12_RESOURCE_DESC desc = {
            pDesc->Dimension,
            pDesc->Alignment,
            pDesc->Width,
            pDesc->Height,
            pDesc->DepthOrArraySize,
            pDesc->MipLevels,
            pDesc->Format,
            pDesc->SampleDesc,
            layout,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        };

        hr = GetD3D12Device()->CreateCommittedResource( 
            &props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            pDesc->InitState,
            nullptr,
            IID_PPV_ARGS(m_pResource.GetAddress()));
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr );
            return false;
        }
    }

#if ASDX_IS_SCARLETT
    auto mostDetailedMip = (pDesc->MipLevels - 1);
#else
    auto mostDetailedMip = 0u;
#endif

    D3D12_SHADER_RESOURCE_VIEW_DESC     srv_desc = {};
    D3D12_UNORDERED_ACCESS_VIEW_DESC    uav_desc = {};
    uav_desc.Format = pDesc->Format;
    srv_desc.Format = pDesc->Format; 
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        uav_desc.ViewDimension          = D3D12_UAV_DIMENSION_TEXTURE3D;
        uav_desc.Texture3D.FirstWSlice  = 0;
        uav_desc.Texture3D.MipSlice     = 0;
        uav_desc.Texture3D.WSize        = pDesc->DepthOrArraySize;

        srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE3D;
        srv_desc.Texture3D.MipLevels            = pDesc->MipLevels;
        srv_desc.Texture3D.MostDetailedMip      = mostDetailedMip;
        srv_desc.Texture3D.ResourceMinLODClamp  = 0;
    }
    else if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        if (pDesc->DepthOrArraySize > 1)
        {
            uav_desc.ViewDimension                      = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uav_desc.Texture2DArray.ArraySize           = pDesc->DepthOrArraySize;
            uav_desc.Texture2DArray.FirstArraySlice     = 0;
            uav_desc.Texture2DArray.MipSlice            = 0;
            uav_desc.Texture2DArray.PlaneSlice          = 0;

            srv_desc.Texture2DArray.ArraySize           = pDesc->DepthOrArraySize;
            srv_desc.Texture2DArray.FirstArraySlice     = 0;
            srv_desc.Texture2DArray.MipLevels           = 0;
            srv_desc.Texture2DArray.PlaneSlice          = 0;
            srv_desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            srv_desc.Texture2DArray.MostDetailedMip     = mostDetailedMip;
            srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        }

        else
        {
            uav_desc.ViewDimension          = D3D12_UAV_DIMENSION_TEXTURE2D;
            uav_desc.Texture2D.MipSlice     = 0;
            uav_desc.Texture2D.PlaneSlice   = 0;

            srv_desc.Texture2D.MipLevels            = pDesc->MipLevels;
            srv_desc.Texture2D.MostDetailedMip      = mostDetailedMip;
            srv_desc.Texture2D.PlaneSlice           = 0;
            srv_desc.Texture2D.ResourceMinLODClamp  = 0.0f;
            srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;

        }
    }
    else if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
    {
        if (pDesc->DepthOrArraySize > 1)
        {
            uav_desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            uav_desc.Texture1DArray.ArraySize       = pDesc->DepthOrArraySize;
            uav_desc.Texture1DArray.FirstArraySlice = 0;
            uav_desc.Texture1DArray.MipSlice        = 0;

            srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            srv_desc.Texture1DArray.ArraySize           = pDesc->DepthOrArraySize;
            srv_desc.Texture1DArray.FirstArraySlice     = 0;
            srv_desc.Texture1DArray.MipLevels           = pDesc->MipLevels;
            srv_desc.Texture1DArray.MostDetailedMip     = mostDetailedMip;
            srv_desc.Texture1DArray.ResourceMinLODClamp = 0;
        }
        else
        {
            uav_desc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
            uav_desc.Texture1D.MipSlice = 0;

            srv_desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE1D;
            srv_desc.Texture1D.MipLevels            = pDesc->MipLevels;
            srv_desc.Texture1D.MostDetailedMip      = mostDetailedMip;
            srv_desc.Texture1D.ResourceMinLODClamp  = 0;
        }
    }
    else if (pDesc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        auto elements = (stride == 0) ? pDesc->Width : pDesc->Width / stride;

        uav_desc.ViewDimension                  = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.CounterOffsetInBytes    = 0;
        uav_desc.Buffer.FirstElement            = 0;
        uav_desc.Buffer.NumElements             = UINT(elements);
        uav_desc.Buffer.StructureByteStride     = stride;
        uav_desc.Buffer.Flags                   = (stride == 0) ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

        srv_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.FirstElement        = 0;
        srv_desc.Buffer.NumElements         = UINT(elements);
        srv_desc.Buffer.StructureByteStride = stride;
        srv_desc.Buffer.Flags               = (stride == 0) ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;
    }

    if (!CreateUnorderedAccessView(m_pResource.GetPtr(), nullptr, &uav_desc, m_pUAV.GetAddress()))
    {
        ELOG("Error : CreateUnorderedAccessView() Failed.");
        return false;
    }

    if (!CreateShaderResourceView(m_pResource.GetPtr(), &srv_desc, m_pSRV.GetAddress()))
    {
        ELOG("Error : CreateShaderResourceView() Failed.");
        return false;
    }

    memcpy(&m_Desc, pDesc, sizeof(m_Desc));

    m_PrevState = m_Desc.InitState;
    m_Stride    = stride;

    return true;
}

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool ComputeTarget::Init(ColorTarget& target)
{
    m_pResource = target.GetResource();
    m_pSRV      = target.GetSRV();

    auto desc = target.GetDesc();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = desc.Format;

    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        uav_desc.ViewDimension          = D3D12_UAV_DIMENSION_TEXTURE3D;
        uav_desc.Texture3D.FirstWSlice  = 0;
        uav_desc.Texture3D.MipSlice     = 0;
        uav_desc.Texture3D.WSize        = desc.DepthOrArraySize;
    }
    else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        if (desc.DepthOrArraySize > 1)
        {
            uav_desc.ViewDimension                      = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uav_desc.Texture2DArray.ArraySize           = desc.DepthOrArraySize;
            uav_desc.Texture2DArray.FirstArraySlice     = 0;
            uav_desc.Texture2DArray.MipSlice            = 0;
            uav_desc.Texture2DArray.PlaneSlice          = 0;
        }

        else
        {
            uav_desc.ViewDimension          = D3D12_UAV_DIMENSION_TEXTURE2D;
            uav_desc.Texture2D.MipSlice     = 0;
            uav_desc.Texture2D.PlaneSlice   = 0;
        }
    }
    else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
    {
        if (desc.DepthOrArraySize > 1)
        {
            uav_desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            uav_desc.Texture1DArray.ArraySize       = desc.DepthOrArraySize;
            uav_desc.Texture1DArray.FirstArraySlice = 0;
            uav_desc.Texture1DArray.MipSlice        = 0;
        }
        else
        {
            uav_desc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
            uav_desc.Texture1D.MipSlice = 0;
        }
    }
    else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        // Not Support Buffer.
        assert(false);
        ELOGA("Error : Not Support Buffer Type.");
        return false;
    }

    if (!CreateUnorderedAccessView(m_pResource.GetPtr(), nullptr, &uav_desc, m_pUAV.GetAddress()))
    {
        ELOG("Error : CreateUnorderedAccessView() Failed.");
        return false;
    }

    memcpy(&m_Desc, &desc, sizeof(m_Desc));

    m_PrevState = m_Desc.InitState;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void ComputeTarget::Term()
{
    m_pUAV.Reset();
    m_pSRV.Reset();

    auto resource = m_pResource.Detach();
    Dispose(resource);
 
    memset(&m_Desc, 0, sizeof(m_Desc));
    m_Stride = 0;
}

//-----------------------------------------------------------------------------
//      リサイズ処理を行います.
//-----------------------------------------------------------------------------
bool ComputeTarget::Resize(uint32_t width, uint32_t height, uint16_t depth)
{
    auto desc   = m_Desc;
    auto stride = m_Stride;
    Term();

    desc.Width            = width;
    desc.Height           = height;
    desc.DepthOrArraySize = depth;
    return Init(&desc, stride);
}

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* ComputeTarget::GetResource() const
{ return m_pResource.GetPtr(); }

//-----------------------------------------------------------------------------
//      アンオーダードアクセスビューを取得します.
//-----------------------------------------------------------------------------
const IUnorderedAccessView* ComputeTarget::GetUAV() const
{ return m_pUAV.GetPtr(); }

//-----------------------------------------------------------------------------
//      シェーダリソースビューを取得します.
//-----------------------------------------------------------------------------
const IShaderResourceView* ComputeTarget::GetSRV() const
{ return m_pSRV.GetPtr(); }

//-----------------------------------------------------------------------------
//      構成設定を取得します.
//-----------------------------------------------------------------------------
TargetDesc ComputeTarget::GetDesc() const
{ return m_Desc; }

//-----------------------------------------------------------------------------
//      ストライドサイズを取得します.
//-----------------------------------------------------------------------------
uint32_t ComputeTarget::GetStride() const
{ return m_Stride; }

//-----------------------------------------------------------------------------
//      デバッグ名を設定します.
//-----------------------------------------------------------------------------
void ComputeTarget::SetName(LPCWSTR tag)
{
    if (m_pResource)
    { m_pResource->SetName(tag); }
}

//-----------------------------------------------------------------------------
//      UAVバリアを設定します.
//-----------------------------------------------------------------------------
void ComputeTarget::UAVBarrier(ID3D12GraphicsCommandList* pCmdList)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type            = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource   = m_pResource.GetPtr();

    pCmdList->ResourceBarrier(1, &barrier);
}

} // namespace asdx
