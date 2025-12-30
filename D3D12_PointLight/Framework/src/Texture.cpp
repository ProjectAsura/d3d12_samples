//-----------------------------------------------------------------------------
// File : Texture.cpp
// Desc : Texture Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <Texture.h>
#include <DDSTextureLoader.h>
#include <DescriptorPool.h>
#include <Logger.h>


namespace {

//-----------------------------------------------------------------------------
//      SRGBフォーマットに変換します.
//-----------------------------------------------------------------------------
DXGI_FORMAT ConvertToSRGB(DXGI_FORMAT format)
{
    DXGI_FORMAT result = format;
    switch(format)
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

    default:
        break;
    }

    return result;
}

} // namespace


///////////////////////////////////////////////////////////////////////////////
// Texture class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Texture::Texture()
: m_pTex    (nullptr)
, m_pHandle (nullptr)
, m_pPool   (nullptr)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Texture::~Texture()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Texture::Init
(
    ID3D12Device*                   pDevice,
    DescriptorPool*                 pPool,
    const wchar_t*                  filename,
    DirectX::ResourceUploadBatch&   batch
)
{ return Init(pDevice, pPool, filename, false, batch); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Texture::Init
(
    ID3D12Device*                   pDevice,
    DescriptorPool*                 pPool,
    const wchar_t*                  filename,
    bool                            isSRGB,
    DirectX::ResourceUploadBatch&   batch
)
{
    // 引数チェック.
    if (pDevice == nullptr || pPool == nullptr || filename == nullptr)
    {
        ELOG( "Error : Invalid Argument." );
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
    { return false; }

    // ファイルからテクスチャを生成.
    bool isCube = false;
    auto hr = DirectX::CreateDDSTextureFromFile(
        pDevice,
        batch,
        filename,
        m_pTex.GetAddressOf(),
        true,
        0,
        nullptr,
        &isCube);
    if (FAILED(hr))
    {
        ELOG( "Error : DirectX::CreateDDSTextureFromFile() Failed. filename = %ls, retcode = 0x%x", filename, hr );
        return false;
    }

    // シェーダリソースビューの設定を求める.
    auto viewDesc = GetViewDesc(isCube);

    // SRGBフォーマットに変換します.
    if (isSRGB)
    { viewDesc.Format = ConvertToSRGB(viewDesc.Format); }
    
    // シェーダリソースビューを生成します.
    pDevice->CreateShaderResourceView(m_pTex.Get(), &viewDesc, m_pHandle->HandleCPU);

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Texture::Init(
    ID3D12Device*               pDevice,
    DescriptorPool*             pPool,
    const D3D12_RESOURCE_DESC*  pDesc,
    D3D12_RESOURCE_STATES       initState,
    bool                        isCube
)
{ return Init(pDevice, pPool, pDesc, initState, isCube, false); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Texture::Init
(
    ID3D12Device*               pDevice,
    DescriptorPool*             pPool,
    const D3D12_RESOURCE_DESC*  pDesc,
    D3D12_RESOURCE_STATES       initState,
    bool                        isCube,
    bool                        isSRGB
)
{
    if (pDevice == nullptr || pPool == nullptr || pDesc == nullptr)
    { return false; }

    assert(m_pPool   == nullptr);
    assert(m_pHandle == nullptr);

    // ディスクリプタプールを設定.
    m_pPool = pPool;
    m_pPool->AddRef();

    // ディスクリプタハンドルを取得.
    m_pHandle = pPool->AllocHandle();
    if (m_pHandle == nullptr)
    { return false; }

    D3D12_HEAP_PROPERTIES prop = {};
    prop.Type                   = D3D12_HEAP_TYPE_DEFAULT;
    prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
    prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.CreationNodeMask       = 0;
    prop.VisibleNodeMask        = 0;

    auto hr = pDevice->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        pDesc,
        initState,
        nullptr,
        IID_PPV_ARGS(m_pTex.GetAddressOf())
    );
    if (FAILED(hr))
    {
        ELOG( "Error : ID3D12Device::CreateCommittedResource() Failed. retcode = 0x%x", hr );
        return false;
    }

    // シェーダリソースビューの設定を求める.
    auto viewDesc = GetViewDesc(isCube);

    // SRGBフォーマットに変換します.
    if (isSRGB)
    { viewDesc.Format = ConvertToSRGB(viewDesc.Format); }
    
    // シェーダリソースビューを生成します.
    pDevice->CreateShaderResourceView(m_pTex.Get(), &viewDesc, m_pHandle->HandleCPU);

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void Texture::Term()
{
    m_pTex.Reset();

    // ディスクリプタハンドルを解放.
    if (m_pHandle != nullptr && m_pPool != nullptr)
    {
        m_pPool->FreeHandle(m_pHandle);
        m_pHandle = nullptr;
    }

    // ディスクリプタプールを解放.
    if (m_pPool != nullptr)
    {
        m_pPool->Release();
        m_pPool = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      CPUディスクリプタハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetHandleCPU() const
{
    if (m_pHandle != nullptr)
    { return m_pHandle->HandleCPU; }

    return D3D12_CPU_DESCRIPTOR_HANDLE();
}

//-----------------------------------------------------------------------------
//      GPUディスクリプタハンドルを取得します.
//-----------------------------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetHandleGPU() const
{
    if (m_pHandle != nullptr)
    { return m_pHandle->HandleGPU; }

    return D3D12_GPU_DESCRIPTOR_HANDLE();
}

//-----------------------------------------------------------------------------
//      リソースを取得します.
//-----------------------------------------------------------------------------
ID3D12Resource* Texture::GetResource() const
{ return m_pTex.Get();  }

//-----------------------------------------------------------------------------
//      シェーダリソースビューの設定を求めます.
//-----------------------------------------------------------------------------
D3D12_SHADER_RESOURCE_VIEW_DESC Texture::GetViewDesc(bool isCube)
{
    auto desc = m_pTex->GetDesc();
    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};

    viewDesc.Format                  = desc.Format;
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    switch(desc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        {
            // バッファは対象外とします.
            abort();    // アプリを止める.
        }
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        {
            if (desc.DepthOrArraySize > 1)
            {
                viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;

                viewDesc.Texture1DArray.MostDetailedMip     = 0;
                viewDesc.Texture1DArray.MipLevels           = desc.MipLevels;
                viewDesc.Texture1DArray.FirstArraySlice     = 0;
                viewDesc.Texture1DArray.ArraySize           = desc.DepthOrArraySize;
                viewDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
            }
            else
            {
                viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;

                viewDesc.Texture1D.MostDetailedMip      = 0;
                viewDesc.Texture1D.MipLevels            = desc.MipLevels;
                viewDesc.Texture1D.ResourceMinLODClamp  = 0.0f;
            }
        }
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        {
            if (isCube)
            {
                if (desc.DepthOrArraySize > 6)
                {
                    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;

                    viewDesc.TextureCubeArray.MostDetailedMip       = 0;
                    viewDesc.TextureCubeArray.MipLevels             = desc.MipLevels;
                    viewDesc.TextureCubeArray.First2DArrayFace      = 0;
                    viewDesc.TextureCubeArray.NumCubes              = (desc.DepthOrArraySize / 6);
                    viewDesc.TextureCubeArray.ResourceMinLODClamp   = 0.0f;
                }
                else
                {
                    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;

                    viewDesc.TextureCube.MostDetailedMip        = 0;
                    viewDesc.TextureCube.MipLevels              = desc.MipLevels;
                    viewDesc.TextureCube.ResourceMinLODClamp    = 0.0f;
                }
            }
            else
            {
                if (desc.DepthOrArraySize > 1)
                {
                    if (desc.MipLevels > 1)
                    {
                        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;

                        viewDesc.Texture2DMSArray.FirstArraySlice   = 0;
                        viewDesc.Texture2DMSArray.ArraySize         = desc.DepthOrArraySize;
                    }
                    else
                    {
                        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;

                        viewDesc.Texture2DArray.MostDetailedMip     = 0;
                        viewDesc.Texture2DArray.MipLevels           = desc.MipLevels;
                        viewDesc.Texture2DArray.FirstArraySlice     = 0;
                        viewDesc.Texture2DArray.ArraySize           = desc.DepthOrArraySize;
                        viewDesc.Texture2DArray.PlaneSlice          = 0;
                        viewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
                    }
                }
                else
                {
                    if (desc.MipLevels > 1)
                    {
                        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

                        viewDesc.Texture2D.MostDetailedMip      = 0;
                        viewDesc.Texture2D.MipLevels            = desc.MipLevels;
                        viewDesc.Texture2D.PlaneSlice           = 0;
                        viewDesc.Texture2D.ResourceMinLODClamp  = 0.0f;
                    }
                }
            }
        }
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        {
            viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;

            viewDesc.Texture3D.MostDetailedMip      = 0;
            viewDesc.Texture3D.MipLevels            = desc.MipLevels;
            viewDesc.Texture3D.ResourceMinLODClamp  = 0.0f;
        }
        break;

    default:
        {
            // 想定外
            abort();    // アプリを止める.
        }
        break;
    }

    return viewDesc;
}
