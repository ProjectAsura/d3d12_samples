//-----------------------------------------------------------------------------
// File : asdxTexture.cpp
// Desc : Texture.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cassert>
#include <vector>
#include <gfx/asdxTexture.h>
#include <gfx/asdxDevice.h>
#include <gfx/asdxCommandList.h>
#include <fnd/asdxLogger.h>


#if (D3D12_SDK_VERSION >= 613 || D3D12_PREVIEW_SDK_VERSION >= 710)
#define ASDX_ENABLE_GPU_UPLOAD_HEAPS    (1)
#else
#define ASDX_ENABLE_GPU_UPLOAD_HEAPS    (0)
#endif

namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// Texture class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Texture::Texture()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Texture::~Texture()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool Texture::Init(ID3D12GraphicsCommandList* pCmdList, const ResTexture& resource)
{
    auto pDevice = GetD3D12Device();

    auto dimension  = D3D12_RESOURCE_DIMENSION_UNKNOWN;
    auto isCube     = false;
    auto depth      = 1;
    auto format     = DXGI_FORMAT(resource.Format);

#if ASDX_IS_SCARLETT
    auto mostDetailedMip = resourc.MipMapCount - 1;
#else
    auto mostDetailedMip = 0u;
#endif

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    bool gpuUploadHeapsSupported     = false;
    bool isUnifiedMemoryArchitecture = false;

    auto heapType  = D3D12_HEAP_TYPE_DEFAULT;
    auto initState = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D12_FEATURE_DATA_ARCHITECTURE architecture = {};
    if (SUCCEEDED(pDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &architecture, sizeof(architecture))))
    {
        if (architecture.UMA)
        {
            isUnifiedMemoryArchitecture = true;
            initState = D3D12_RESOURCE_STATE_COMMON;
        }
    }

#if ASDX_ENABLE_GPU_UPLOAD_HEAPS
    D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = {};
    if (SUCCEEDED(pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16))))
    {
        if (options16.GPUUploadHeapSupported)
        {
            gpuUploadHeapsSupported = true;
            heapType  = D3D12_HEAP_TYPE_GPU_UPLOAD;
            initState = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        }
    }
#endif

    ID3D12Resource* pResource = nullptr;
    D3D12_HEAP_PROPERTIES props = {
        heapType,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        1,
        1
    };

    switch(resource.Dimension)
    {
    case TEXTURE_DIMENSION_1D:
        {
            dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            if (resource.SurfaceCount > 1)
            {
                viewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                viewDesc.Format                             = format;
                viewDesc.Texture1DArray.ArraySize           = resource.SurfaceCount;
                viewDesc.Texture1DArray.FirstArraySlice     = 0;
                viewDesc.Texture1DArray.MipLevels           = resource.MipMapCount;
                viewDesc.Texture1DArray.MostDetailedMip     = mostDetailedMip;
                viewDesc.Texture1DArray.ResourceMinLODClamp = 0;
            }
            else
            {
                viewDesc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE1D;
                viewDesc.Format                         = format;
                viewDesc.Texture1D.MipLevels            = resource.MipMapCount;
                viewDesc.Texture1D.MostDetailedMip      = mostDetailedMip;
                viewDesc.Texture1D.ResourceMinLODClamp  = 0;
            }
        }
        break;

    case TEXTURE_DIMENSION_2D:
        {
            dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            if (resource.SurfaceCount > 1)
            {
                viewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                viewDesc.Format                             = format;
                viewDesc.Texture2DArray.ArraySize           = resource.SurfaceCount;
                viewDesc.Texture2DArray.FirstArraySlice     = 0;
                viewDesc.Texture2DArray.MipLevels           = resource.MipMapCount;
                viewDesc.Texture2DArray.MostDetailedMip     = mostDetailedMip;
                viewDesc.Texture2DArray.PlaneSlice          = 0;
                viewDesc.Texture2DArray.ResourceMinLODClamp = 0;
            }
            else
            {
                viewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2D;
                viewDesc.Format                             = format;
                viewDesc.Texture2D.MipLevels                = resource.MipMapCount;
                viewDesc.Texture2D.MostDetailedMip          = mostDetailedMip;
                viewDesc.Texture2D.PlaneSlice               = 0;
                viewDesc.Texture2D.ResourceMinLODClamp      = 0;
            }
        }
        break;

    case TEXTURE_DIMENSION_3D:
        {
            dimension   = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            depth       = resource.Depth;

            viewDesc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE3D;
            viewDesc.Format                         = format;
            viewDesc.Texture3D.MipLevels            = resource.MipMapCount;
            viewDesc.Texture3D.MostDetailedMip      = 0;
            viewDesc.Texture3D.ResourceMinLODClamp  = 0.0f;
        }
        break;

    case TEXTURE_DIMENSION_CUBE:
        {
            dimension   = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            depth       = resource.SurfaceCount;

            if (resource.SurfaceCount > 6)
            {
                viewDesc.ViewDimension                           = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                viewDesc.Format                                  = format;
                viewDesc.TextureCubeArray.First2DArrayFace       = 0;
                viewDesc.TextureCubeArray.MipLevels              = resource.MipMapCount;
                viewDesc.TextureCubeArray.MostDetailedMip        = 0;
                viewDesc.TextureCubeArray.NumCubes               = resource.SurfaceCount / 6;
                viewDesc.TextureCubeArray.ResourceMinLODClamp    = 0.0f;
            }
            else
            {
                viewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURECUBE;
                viewDesc.Format                             = format;
                viewDesc.TextureCube.MipLevels              = resource.MipMapCount;
                viewDesc.TextureCube.MostDetailedMip        = 0;
                viewDesc.TextureCube.ResourceMinLODClamp    = 0;
            }
        }
        break;
    }

    D3D12_RESOURCE_DESC desc = {
        dimension,
        0,
        resource.Width,
        resource.Height,
        UINT16(depth),
        UINT16(resource.MipMapCount),
        format,
        { 1, 0 },
        D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_RESOURCE_FLAG_NONE
    };

    auto hr = pDevice->CreateCommittedResource(
        &props,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        initState,
        nullptr,
        IID_PPV_ARGS(&pResource));
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12Device::CreateCommitedResource() Failed. errcode = 0x%x", hr);
        return false;
    }

    // シェーダリソースビューの生成.
    {
        if (!CreateShaderResourceView(pResource, &viewDesc, m_View.GetAddress()))
        {
            pResource->Release();
            pResource = nullptr;
            return false;
        }
    }

    // 直接書き込める場合.
    if (gpuUploadHeapsSupported || isUnifiedMemoryArchitecture)
    {
        auto count = resource.MipMapCount * resource.SurfaceCount;
        for(auto i=0u; i<count; ++i)
        {
            auto srcPtr        = resource.pResources[i].pPixels;
            auto srcRowPitch   = resource.pResources[i].Pitch;
            auto srcDepthPitch = srcRowPitch * resource.pResources[i].Height * resource.Depth;

            D3D12_BOX dstBox = {};
            dstBox.left     = 0;
            dstBox.right    = resource.pResources[i].Width;
            dstBox.top      = 0;
            dstBox.bottom   = resource.pResources[i].Height;
            dstBox.front    = 0;
            dstBox.back     = resource.Depth;

            pResource->WriteToSubresource(i, &dstBox, srcPtr, srcRowPitch, srcDepthPitch);
        }
    }
    else
    {
        // コピーコマンドを使ってアップロード.
        UpdateTexture(pCmdList, pResource, &resource);
    }

    // ステート遷移.
    if (initState != D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                   = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource    = pResource;
        barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore  = initState;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

        pCmdList->ResourceBarrier(1, &barrier);
    }

    pResource->Release();
    pResource = nullptr;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void Texture::Term()
{ m_View.Reset(); }

//-----------------------------------------------------------------------------
//      ビューを取得します.
//-----------------------------------------------------------------------------
IShaderResourceView* Texture::GetSRV() const
{ return m_View.GetPtr(); }

//-----------------------------------------------------------------------------
//      デバッグ名を設定します.
//-----------------------------------------------------------------------------
void Texture::SetName(const char* name)
{
    m_View->GetResource()
          ->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(strlen(name)), name);
}

} // namespace asdx
