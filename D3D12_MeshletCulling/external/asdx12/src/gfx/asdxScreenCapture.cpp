//-----------------------------------------------------------------------------
// File : asdxScreenCapture.cpp
// Desc : Screen Capture.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <gfx/asdxScreenCpature.h>
#include <gfx/asdxDevice.h>
#include <fnd/asdxLogger.h>


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// CaptureResource class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
CaptureResource::CaptureResource()
{
    memset(&m_CopyDst, 0, sizeof(m_CopyDst));
    memset(&m_CopySrc, 0, sizeof(m_CopySrc));
}

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
CaptureResource::~CaptureResource()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理です.
//-----------------------------------------------------------------------------
bool CaptureResource::Init(ID3D12Device* pDevice, ID3D12Resource* pSource)
{
    D3D12_HEAP_PROPERTIES srcHeapProps = {};
    D3D12_HEAP_FLAGS srcHeapFlags;
    auto hr = pSource->GetHeapProperties(&srcHeapProps, &srcHeapFlags);
    if (FAILED(hr))
    {
        ELOGA("Error : ID3D12Resource::GetHeapProperties() Failed. errcode = 0x%x", hr);
        return false;
    }

    if (srcHeapProps.Type == D3D12_HEAP_TYPE_READBACK)
    {
        // 読み取りできるやつだから作る必要なし.
        ELOGA("Error : Heap Props is READ_BACK. This resource is capturable, so we don't create.");
        return false;
    }

    auto desc = pSource->GetDesc();

    if (desc.MipLevels != 1)
    {
        // ミップ複数の場合もサポートしない.
        ELOGA("Error : Unsupported Resources Settings. MipLevels = %u", desc.MipLevels);
        return false;
    }

    UINT   rowCount     = 0;
    UINT64 rowPitch     = 0;
    UINT64 totalBytes   = 0;
    pDevice->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, &rowCount, &rowPitch, &totalBytes);

    // 256 byte alignment.
    rowPitch = (rowPitch + 255) &~0xFFu;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension            = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width                = rowPitch * desc.Height;
    bufferDesc.Height               = 1;
    bufferDesc.DepthOrArraySize     = 1;
    bufferDesc.MipLevels            = 1;
    bufferDesc.Format               = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count     = 1;
    bufferDesc.SampleDesc.Quality   = 0;
    bufferDesc.Layout               = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags                = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES dstHeapProps = {};
    dstHeapProps.Type                   = D3D12_HEAP_TYPE_READBACK;
    dstHeapProps.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    dstHeapProps.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
    dstHeapProps.CreationNodeMask       = 1;
    dstHeapProps.VisibleNodeMask        = 1;

    hr = pDevice->CreateCommittedResource(
        &dstHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(m_Resource.GetAddress()));
    if (FAILED(hr))
    {
        ELOGA("Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
        return false;
    }

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footPrint = {};
    footPrint.Footprint.Width       = UINT(desc.Width);
    footPrint.Footprint.Height      = desc.Height;
    footPrint.Footprint.Depth       = 1;
    footPrint.Footprint.RowPitch    = UINT(rowPitch);
    footPrint.Footprint.Format      = desc.Format;

    m_CopyDst.pResource         = m_Resource.GetPtr();
    m_CopyDst.Type              = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    m_CopyDst.PlacedFootprint   = footPrint;

    m_CopySrc.pResource         = pSource;
    m_CopySrc.Type              = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    m_CopySrc.SubresourceIndex  = 0;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理です.
//-----------------------------------------------------------------------------
void CaptureResource::Term()
{
    auto resource = m_Resource.Detach();
    Dispose(resource);
    memset(&m_CopyDst, 0, sizeof(m_CopyDst));
    memset(&m_CopySrc, 0, sizeof(m_CopySrc));
}

//-----------------------------------------------------------------------------
//      リードバックコマンドを発行します.
//-----------------------------------------------------------------------------
void CaptureResource::ReadBack(ID3D12GraphicsCommandList* pCmdList)
{
    // バリアは自前で張ってね. リゾルブもしません.
    pCmdList->CopyTextureRegion(&m_CopyDst, 0, 0, 0, &m_CopySrc, nullptr);
}

//-----------------------------------------------------------------------------
//      構成設定を取得します.
//-----------------------------------------------------------------------------
D3D12_RESOURCE_DESC CaptureResource::GetDesc() const
{ return m_Resource->GetDesc(); }

//-----------------------------------------------------------------------------
//      メモリマッピングを行います.
//-----------------------------------------------------------------------------
uint8_t* CaptureResource::Map()
{
    void* ptr = nullptr;
    auto hr = m_Resource->Map(0, nullptr, &ptr);
    if (FAILED(hr))
    { return nullptr; }

    return reinterpret_cast<uint8_t*>(ptr);
}

//-----------------------------------------------------------------------------
//      メモリマッピングを解除します.
//-----------------------------------------------------------------------------
void CaptureResource::Unmap()
{ m_Resource->Unmap(0, nullptr); }

} // namespace asdx
