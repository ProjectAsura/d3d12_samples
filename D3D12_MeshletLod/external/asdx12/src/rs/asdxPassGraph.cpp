//-----------------------------------------------------------------------------
// File : asdxPassGraph.cpp
// Desc : Pass Graph System.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <atomic>
#include <map>
#include <fnd/asdxFrameHeap.h>
#include <fnd/asdxHash.h>
#include <fnd/asdxList.h>
#include <fnd/asdxStack.h>
#include <fnd/asdxThreadPool.h>
#include <fnd/asdxLogger.h>
#include <gfx/asdxCommandList.h>
#include <gfx/asdxDisposer.h>
#include <gfx/asdxGraphicsSystem.h>
#include <rs/asdxPassGraph.h>


// パスで生成可能な最大リソース数.
#define MAX_PASS_RESOURCE_COUNT (16)


namespace asdx {

//-----------------------------------------------------------------------------
// Forward Declarations.
//-----------------------------------------------------------------------------
class PassGraph;
class RenderPass;

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
static const auto RES_STATE_WRITE_RTV = D3D12_RESOURCE_STATE_RENDER_TARGET;
static const auto RES_STATE_WRITE_DSV = D3D12_RESOURCE_STATE_DEPTH_WRITE;
static const auto RES_STATE_WRITE_UAV = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
static const auto RES_STATE_READ      = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
static const auto RES_STATE_READ_DSV  = D3D12_RESOURCE_STATE_DEPTH_READ | RES_STATE_READ;


///////////////////////////////////////////////////////////////////////////////
// RESORUCE_INFO_FLAGS
///////////////////////////////////////////////////////////////////////////////
enum RESOURCE_INFO_FLAGS
{
    RESOURCE_INFO_FLAG_NONE                 = 0,            // 無し.
    RESOURCE_INFO_FLAG_STATE_COMMON         = 0x1 << 0,     // 共通ステート.
    RESOURCE_INFO_FLAG_STATE_READ           = 0x1 << 1,     // 読み取りステート.
    RESOURCE_INFO_FLAG_STATE_WRITE          = 0x1 << 2,     // 書き込みステート.
    RESOURCE_INFO_FLAG_BARRIER              = 0x1 << 3,     // パス実行前のバリア有効.
};

///////////////////////////////////////////////////////////////////////////////
// SYNC_FLAGS
///////////////////////////////////////////////////////////////////////////////
enum SYNC_FLAGS
{
    SYNC_FLAG_NONE = 0,
    SYNC_FLAG_GRAPHICS_TO_COMPUTE = 1,
    SYNC_FLAG_COMPUTE_TO_GRAPHICS = 2,
    SYNC_FLAG_COMPUTE_TO_COMPUTE  = 3,
};

//-----------------------------------------------------------------------------
//      RTV用ステートを取得します.
//-----------------------------------------------------------------------------
D3D12_RESOURCE_STATES GetStateRTV(const RESOURCE_INFO_FLAGS& flags)
{
    if (!!(flags & RESOURCE_INFO_FLAG_STATE_WRITE))
    { return RES_STATE_WRITE_RTV; }
    else if (!!(flags & RESOURCE_INFO_FLAG_STATE_READ))
    { return RES_STATE_READ; }

    return D3D12_RESOURCE_STATE_COMMON;
}

//-----------------------------------------------------------------------------
//      DSV用ステートを取得します.
//-----------------------------------------------------------------------------
D3D12_RESOURCE_STATES GetStateDSV(const RESOURCE_INFO_FLAGS& flags)
{
    if (!!(flags & RESOURCE_INFO_FLAG_STATE_WRITE))
    { return RES_STATE_WRITE_DSV; }
    else if (!!(flags & RESOURCE_INFO_FLAG_STATE_READ))
    { return RES_STATE_READ; }

    return D3D12_RESOURCE_STATE_COMMON;
}

//-----------------------------------------------------------------------------
//      UAV用ステートを取得します.
//-----------------------------------------------------------------------------
D3D12_RESOURCE_STATES GetStateUAV(const RESOURCE_INFO_FLAGS& flags)
{
    if (!!(flags & RESOURCE_INFO_FLAG_STATE_WRITE))
    { return RES_STATE_WRITE_UAV; }
    else if (!!(flags & RESOURCE_INFO_FLAG_STATE_READ))
    { return RES_STATE_READ; }

    return D3D12_RESOURCE_STATE_COMMON;
}

//-----------------------------------------------------------------------------
//      文字列をコピーします.
//-----------------------------------------------------------------------------
void CopyString(char* dst, const char* src, size_t maxSize)
{
    auto size = Min<size_t>(strlen(src), maxSize - 1);
    for(size_t i=0; i<size; ++i)
    { dst[i] = src[i]; }
    dst[size] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
// Transition structure
///////////////////////////////////////////////////////////////////////////////
struct Transition
{
    RESOURCE_INFO_FLAGS     Before;
    RESOURCE_INFO_FLAGS     After;
};


///////////////////////////////////////////////////////////////////////////////
// PassResource class
///////////////////////////////////////////////////////////////////////////////
class PassResource 
: public List<PassResource>::Node
, public Stack<PassResource>::Node
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    RESOURCE_INFO_FLAGS PrevState   = RESOURCE_INFO_FLAG_STATE_COMMON;  //!< 一時ステート
    bool                PrevCompute = false;
 
    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    PassResource()
    : List<PassResource>::Node()
    , Stack<PassResource>::Node()
    , m_RefCount    (0)
    , m_RTV         (nullptr)
    , m_DSV         (nullptr)
    , m_UAV         (nullptr)
    , m_SRV         (nullptr)
    , m_Resource    (nullptr)
    , m_Import      (false)
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~PassResource()
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      初期化処理を行います.
    //-------------------------------------------------------------------------
    bool Init(const PassResourceDesc& value, RenderPass* producer)
    {
        auto pDevice = GetD3D12Device();

        {
            D3D12_RESOURCE_DESC desc = {};
            switch(value.Dimension)
            {
            case PASS_RESOURCE_DIMENSION_BUFFER:
                { desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; }
                break;

            case PASS_RESOURCE_DIMENSION_1D:
                { desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D; }
                break;

            case PASS_RESOURCE_DIMENSION_2D:
                { desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; }
                break;

            case PASS_RESOURCE_DIMENSION_3D:
                { desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D; }
                break;
            }

            auto rtv = false;
            auto dsv = false;
            auto uav = false;

            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
            if (value.Usage & PASS_RESOURCE_USAGE_RTV)
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                rtv = true;
            }
            if (value.Usage & PASS_RESOURCE_USAGE_DSV)
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
                dsv = true;
            }
            if (value.Usage & PASS_RESOURCE_USAGE_UAV)
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                uav = true;
            }

            desc.Width              = value.Width;
            desc.Height             = value.Height;
            desc.DepthOrArraySize   = value.DepthOrArraySize;
            desc.MipLevels          = value.MipLevels;
            desc.Format             = value.Format;
            desc.Flags              = flags;
            desc.SampleDesc.Count   = 1;
            desc.SampleDesc.Quality = 0;
            desc.Layout             = (desc.Dimension == PASS_RESOURCE_DIMENSION_BUFFER) 
                                      ? D3D12_TEXTURE_LAYOUT_ROW_MAJOR 
                                      : D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE;

            D3D12_CLEAR_VALUE clearValue = {};
            clearValue.Format = value.Format;
            if (value.Usage & PASS_RESOURCE_USAGE_DSV)
            {
                clearValue.DepthStencil.Depth   = value.ClearValue.Depth;
                clearValue.DepthStencil.Stencil = value.ClearValue.Stencil;
            }
            else
            {
                clearValue.Color[0] = value.ClearValue.Color[0];
                clearValue.Color[1] = value.ClearValue.Color[1];
                clearValue.Color[2] = value.ClearValue.Color[2];
                clearValue.Color[3] = value.ClearValue.Color[3];
            }

            m_Stencil = false;
            if (value.Format == DXGI_FORMAT_D24_UNORM_S8_UINT)
            { m_Stencil = true; }
            else if (value.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
            { m_Stencil = true; }

            D3D12_HEAP_PROPERTIES props = {};
            props.Type                  = D3D12_HEAP_TYPE_DEFAULT;
            props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            props.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
            props.CreationNodeMask      = 0;
            props.VisibleNodeMask       = 0;

            auto hr = pDevice->CreateCommittedResource(
                &props,
                D3D12_HEAP_FLAG_NONE,
                &desc, 
                D3D12_RESOURCE_STATE_COMMON,
                &clearValue,
                IID_PPV_ARGS(&m_Resource));
            if (FAILED(hr))
            {
                ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. errcode = 0x%x", hr);
                return false;
            }

            PrevState = RESOURCE_INFO_FLAG_STATE_COMMON;

            if (rtv)
            {
                m_RTV = new IRenderTargetView* [value.DepthOrArraySize];

                for(auto i=0; i<value.DepthOrArraySize; ++i)
                {
                    if (!CreateRTV(value, i))
                    { return false; }
                }
            }
            else if (dsv)
            {
                m_DSV = new IDepthStencilView* [value.DepthOrArraySize];

                for(auto i=0; i<value.DepthOrArraySize; ++i)
                {
                    if (!CreateDSV(value, i))
                    { return false; }
                }
            }

            if (uav)
            {
                if (!CreateUAV(value))
                { return false; }
            }

            if (!CreateSRV(value))
            { return false; }
        }

        m_Import    = false;
        m_Desc      = value;
        m_Producer  = producer;

        return true;
    }

    //-------------------------------------------------------------------------
    //! @brief      終了処理を行います.
    //-------------------------------------------------------------------------
    void Term()
    {
        if (m_Import)
        {
            m_Resource      = nullptr;
            m_RTV = nullptr;
            m_DSV = nullptr;
            m_UAV = nullptr;
            m_SRV = nullptr;
            return;
        }

        for(auto i=0; i<m_Desc.DepthOrArraySize; ++i)
        {
            if (m_RTV[i] != nullptr)
            {
                m_RTV[i]->Release();
                m_RTV[i] = nullptr;
            }

            if (m_DSV != nullptr)
            {
                m_DSV[i]->Release();
                m_DSV[i] = nullptr;
            }
        }

        if (m_RTV != nullptr)
        {
            delete[] m_RTV;
            m_RTV = nullptr;
        }

        if (m_DSV != nullptr)
        {
            delete[] m_DSV;
            m_DSV = nullptr;
        }

        if (m_UAV != nullptr)
        {
            m_UAV->Release();
            m_UAV = nullptr;
        }

        if (m_SRV != nullptr)
        {
            m_SRV->Release();
            m_SRV = nullptr;
        }
    }

    //-------------------------------------------------------------------------
    //! @brief      解放処理を行います.
    //-------------------------------------------------------------------------
    void Release()
    {
        Term();
        delete this;
    }

    //-------------------------------------------------------------------------
    //! @brief      レンダーターゲットビューを取得します.
    //-------------------------------------------------------------------------
    const IRenderTargetView* GetRTV(uint16_t index) const
    {
        assert(index < m_Desc.DepthOrArraySize);
        if (m_RTV[index] != nullptr)
        { return m_RTV[index]; }

        return nullptr;
    }

    //-------------------------------------------------------------------------
    //! @brief      深度ステンシルビューを取得します.
    //-------------------------------------------------------------------------
    const IDepthStencilView* GetDSV(uint16_t index) const
    {
        assert(index < m_Desc.DepthOrArraySize);
        if (m_DSV[index] != nullptr)
        { return m_DSV[index]; }

        return nullptr;
    }

    //-------------------------------------------------------------------------
    //! @brief      UAVのディスクリプタハンドルを取得します.
    //-------------------------------------------------------------------------
    const IUnorderedAccessView* GetUAV() const
    {
        if (m_UAV != nullptr)
        { return m_UAV; }

        return nullptr;
    }

    //-------------------------------------------------------------------------
    //! @brief      SRVのディスクリプタハンドルを取得します.
    //-------------------------------------------------------------------------
    const IShaderResourceView* GetSRV() const
    {
        if (m_SRV != nullptr)
        { return m_SRV; }

        return nullptr;
    }

    //-------------------------------------------------------------------------
    //! @brief      構成設定を取得します.
    //-------------------------------------------------------------------------
    PassResourceDesc GetDesc() const
    { return m_Desc; }

    //-------------------------------------------------------------------------
    //! @brief      構成設定を取得します.
    //-------------------------------------------------------------------------
    D3D12_RESOURCE_DESC GetD3D12Desc() const
    {
        if (m_Resource != nullptr)
        { return m_Resource->GetDesc(); }
    
        return D3D12_RESOURCE_DESC();
    }

    //-------------------------------------------------------------------------
    //! @brief      ビューをクリアします.
    //-------------------------------------------------------------------------
    void ClearView(ID3D12GraphicsCommandList6* pCmd, ClearValue& value)
    {
        if (value.Type == CLEAR_TYPE_RTV)
        {
            for(auto i=0; i<m_Desc.DepthOrArraySize; ++i)
            {
                pCmd->ClearRenderTargetView(
                    m_RTV[i]->GetHandleCPU(),
                    value.Color,
                    0,
                    nullptr);
            }
        }
        else if (value.Type == CLEAR_TYPE_DSV)
        {
            auto flag = D3D12_CLEAR_FLAG_DEPTH;
            if (m_Stencil)
            { flag |= D3D12_CLEAR_FLAG_STENCIL;}

            for(auto i=0; i<m_Desc.DepthOrArraySize; ++i)
            {
                pCmd->ClearDepthStencilView(
                    m_DSV[i]->GetHandleCPU(),
                    flag,
                    value.Depth,
                    value.Stencil,
                    0,
                    nullptr);
            }
        }
        else if (value.Type == CLEAR_TYPE_UAV_FLOAT)
        {
            pCmd->ClearUnorderedAccessViewFloat(
                m_UAV->GetHandleGPU(),
                m_UAV->GetHandleCPU(),
                m_Resource,
                value.Float,
                0,
                nullptr);
        }
        else if (value.Type == CLEAR_TYPE_UAV_UINT)
        {
            pCmd->ClearUnorderedAccessViewUint(
                m_UAV->GetHandleGPU(),
                m_UAV->GetHandleCPU(),
                m_Resource,
                value.Uint,
                0,
                nullptr);
        }
    }

    //-------------------------------------------------------------------------
    //! @brief      リソースをインポートします.
    //-------------------------------------------------------------------------
    bool Import
    (
        ID3D12Resource*         pResource,
        D3D12_RESOURCE_STATES   state,
        IShaderResourceView*    pSRV,
        IUnorderedAccessView*   pUAV,
        IRenderTargetView**     pRTVs,
        IDepthStencilView**     pDSVs
    )
    {
        if (pResource == nullptr)
        {
            ELOG("Error : Invalid Argument.");
            return false;
        }

        m_Import   = true;
        m_Resource = pResource;

        auto desc = pResource->GetDesc();
        auto dimension = PASS_RESOURCE_DIMENSION_2D;

        switch(desc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            dimension = PASS_RESOURCE_DIMENSION_BUFFER;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            dimension = PASS_RESOURCE_DIMENSION_1D;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            dimension = PASS_RESOURCE_DIMENSION_2D;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            break;
        }

        if (state == D3D12_RESOURCE_STATE_COMMON)
        { PrevState = RESOURCE_INFO_FLAG_STATE_COMMON; }
        else if (!!(state & D3D12_RESOURCE_STATE_RENDER_TARGET)
        || !!(state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        || !!(state & D3D12_RESOURCE_STATE_DEPTH_WRITE))
        { PrevState = RESOURCE_INFO_FLAG_STATE_WRITE; }
        else if (!!(state & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        || !!(state & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))
        { PrevState = RESOURCE_INFO_FLAG_STATE_READ; }

        PrevCompute = false;

        uint8_t usage = PASS_RESOURCE_USAGE_NONE;
        if (pRTVs != nullptr)
        { usage = PASS_RESOURCE_USAGE_RTV; }
        else if (pDSVs != nullptr)
        { usage = PASS_RESOURCE_USAGE_DSV; }
        else if (pUAV != nullptr)
        { usage = PASS_RESOURCE_USAGE_UAV; }

        m_Desc.Dimension        = dimension;
        m_Desc.Width            = desc.Width;
        m_Desc.Height           = desc.Height;
        m_Desc.DepthOrArraySize = desc.DepthOrArraySize;
        m_Desc.MipLevels        = desc.MipLevels;
        m_Desc.Format           = desc.Format;
        m_Desc.Usage            = usage;
        m_RefCount              = 1;

        m_RTV = pRTVs;
        m_DSV = pDSVs;
        m_SRV = pSRV;
        m_UAV = pUAV;

        return true;
    }

    //-------------------------------------------------------------------------
    //! @brief      構成設定が合致するかどうかチェックします.
    //-------------------------------------------------------------------------
    bool Match(const PassResourceDesc& value) const
    { 
        return (m_Desc.Dimension         == value.Dimension)
            && (m_Desc.Width             == value.Width)
            && (m_Desc.Height            == value.Height)
            && (m_Desc.DepthOrArraySize  == value.DepthOrArraySize)
            && (m_Desc.MipLevels         == value.MipLevels)
            && (m_Desc.Format            == value.Format);
    }

    //-------------------------------------------------------------------------
    //! @brief      参照カウントを増やします.
    //-------------------------------------------------------------------------
    void Increment()
    {
        if (!m_Import)
        { m_RefCount++; }
    }

    //-------------------------------------------------------------------------
    //! @brief      参照カウントを減らします.
    //-------------------------------------------------------------------------
    void Decrement()
    {
        if (!m_Import)
        { m_RefCount--; }
    }

    //-------------------------------------------------------------------------
    //! @brief      参照カウントを取得します.
    //-------------------------------------------------------------------------
    int GetRefCount() const
    { return m_RefCount; }

    //-------------------------------------------------------------------------
    //! @brief      インポートリソースかどうかチェックします.
    //-------------------------------------------------------------------------
    bool IsImport() const
    { return m_Import; }

    //-------------------------------------------------------------------------
    //! @brief      生成パスを取得します.
    //-------------------------------------------------------------------------
    RenderPass* GetProducer() const
    { return m_Producer; }

    //-------------------------------------------------------------------------
    //! @brief      バリア情報を生成します.
    //-------------------------------------------------------------------------
    D3D12_RESOURCE_BARRIER CreateBarrier
    (
        RESOURCE_INFO_FLAGS prev,
        RESOURCE_INFO_FLAGS next,
        bool                compute,
        uint32_t            subResource = 0
    ) const
    {
        D3D12_RESOURCE_BARRIER result = {};

        if (m_Desc.Usage & PASS_RESOURCE_USAGE_RTV)
        {
            assert(!compute);
            auto before = GetStateRTV(prev);
            auto after  = GetStateRTV(next);

            result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            result.Transition.pResource   = m_Resource;
            result.Transition.StateBefore = before;
            result.Transition.StateAfter  = after;
            result.Transition.Subresource = subResource;

        }
        else if (m_Desc.Usage & PASS_RESOURCE_USAGE_DSV)
        {
            assert(!compute);
            auto before = GetStateDSV(prev);
            auto after  = GetStateDSV(next);

            result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            result.Transition.pResource   = m_Resource;
            result.Transition.StateBefore = before;
            result.Transition.StateAfter  = after;
            result.Transition.Subresource = subResource;
        }
        else if (m_Desc.Usage & PASS_RESOURCE_USAGE_UAV)
        {
            if (compute)
            {
                if (!!(next & RESOURCE_INFO_FLAG_STATE_WRITE))
                {
                    result.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                    result.UAV.pResource = m_Resource;
                }
            }
            else
            {
                auto before = GetStateUAV(prev);
                auto after  = GetStateUAV(next);

                result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                result.Transition.pResource   = m_Resource;
                result.Transition.StateBefore = before;
                result.Transition.StateAfter  = after;
                result.Transition.Subresource = subResource;
            }
        }

        return result;
    }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    std::atomic<int>        m_RefCount  = 1;
    IRenderTargetView**     m_RTV       = nullptr;
    IDepthStencilView**     m_DSV       = nullptr;
    IUnorderedAccessView*   m_UAV       = nullptr;
    IShaderResourceView*    m_SRV       = nullptr;
    ID3D12Resource*         m_Resource  = nullptr;
    PassResourceDesc        m_Desc      = {};
    bool                    m_Import    = false;
    bool                    m_Stencil   = false;
    RenderPass*             m_Producer  = nullptr;

    //=========================================================================
    // private methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      レンダーターゲットビューを生成します.
    //-------------------------------------------------------------------------
    bool CreateRTV(const PassResourceDesc& value, uint32_t arrayIndex)
    {
        D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
        switch(value.Dimension)
        {
        case PASS_RESOURCE_DIMENSION_BUFFER:
            { 
                viewDesc.ViewDimension          = D3D12_RTV_DIMENSION_BUFFER;
                viewDesc.Buffer.FirstElement    = 0;
                viewDesc.Buffer.NumElements     = uint32_t(value.Width / value.Stride);
            }
            break;

        case PASS_RESOURCE_DIMENSION_1D:
            {
                if (value.DepthOrArraySize > 1)
                {
                    viewDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                    viewDesc.Texture1DArray.ArraySize       = value.DepthOrArraySize;
                    viewDesc.Texture1DArray.FirstArraySlice = arrayIndex;
                    viewDesc.Texture1DArray.MipSlice        = 0;
                }
                else
                {
                    viewDesc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE1D;
                    viewDesc.Texture1D.MipSlice = 0;
                }
            }
            break;

        case PASS_RESOURCE_DIMENSION_2D:
            {
                {
                    if (value.DepthOrArraySize > 1)
                    {
                        viewDesc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                        viewDesc.Texture2DArray.ArraySize       = value.DepthOrArraySize;
                        viewDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                        viewDesc.Texture2DArray.MipSlice        = 0;
                        viewDesc.Texture2DArray.PlaneSlice      = 0;
                    }
                    else
                    {
                        viewDesc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE2D;
                        viewDesc.Texture2D.MipSlice     = 0;
                        viewDesc.Texture2D.PlaneSlice   = 0;
                    }
                }
            }
            break;

        case PASS_RESOURCE_DIMENSION_3D:
            {
                viewDesc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE3D;
                viewDesc.Texture3D.FirstWSlice  = arrayIndex;
                viewDesc.Texture3D.MipSlice     = 0;
                viewDesc.Texture3D.WSize        = value.DepthOrArraySize;
            }
            break;
        }

        if (!CreateRenderTargetView(m_Resource, &viewDesc, &m_RTV[arrayIndex]))
        {
            ELOG("Error : CreateRenderTargetView() Failed.");
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------
    //! @brief      深度ステンシルビューを生成します.
    //-------------------------------------------------------------------------
    bool CreateDSV(const PassResourceDesc& value, uint32_t arrayIndex)
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
        switch(value.Dimension)
        {
        case PASS_RESOURCE_DIMENSION_BUFFER:
            ELOG("Error : Invalid Argument.");
            return false;

        case PASS_RESOURCE_DIMENSION_1D:
            {
                if (value.DepthOrArraySize > 1)
                {
                    viewDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
                    viewDesc.Texture1DArray.ArraySize       = value.DepthOrArraySize;
                    viewDesc.Texture1DArray.FirstArraySlice = arrayIndex;
                    viewDesc.Texture1DArray.MipSlice        = 0;
                }
                else
                {
                    viewDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE1D;
                    viewDesc.Texture1D.MipSlice = 0;
                }
            }
            break;

        case PASS_RESOURCE_DIMENSION_2D:
            {
                if (value.DepthOrArraySize > 1)
                {
                    viewDesc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                    viewDesc.Texture2DArray.ArraySize       = value.DepthOrArraySize;
                    viewDesc.Texture2DArray.FirstArraySlice = arrayIndex;
                    viewDesc.Texture2DArray.MipSlice        = 0;
                }
                else
                {
                    viewDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
                    viewDesc.Texture2D.MipSlice = 0;
                }
            }
            break;

        case PASS_RESOURCE_DIMENSION_3D:
            ELOG("Error : Invalid Argument.");
            return false;
        }

        if (!CreateDepthStencilView(m_Resource, &viewDesc, &m_DSV[arrayIndex]))
        {
            ELOG("Error : CreateDepthStencilView() Failed.");
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------
    //! @brief      アンオーダードアクセスビューを生成します.
    //-------------------------------------------------------------------------
    bool CreateUAV(const PassResourceDesc& value)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
        switch(value.Dimension)
        {
        case PASS_RESOURCE_DIMENSION_BUFFER:
            {
                viewDesc.ViewDimension                  = D3D12_UAV_DIMENSION_BUFFER;
                viewDesc.Buffer.CounterOffsetInBytes    = 0;
                viewDesc.Buffer.FirstElement            = 0;
                viewDesc.Buffer.NumElements             = uint32_t(value.Width / value.Stride);
                viewDesc.Buffer.StructureByteStride     = value.Stride;
                viewDesc.Buffer.Flags                   = (value.Stride == 1) 
                                                            ? D3D12_BUFFER_UAV_FLAG_RAW 
                                                            : D3D12_BUFFER_UAV_FLAG_NONE;
            }
            break;

        case PASS_RESOURCE_DIMENSION_1D:
            {
                if (value.DepthOrArraySize > 1)
                {
                    viewDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                    viewDesc.Texture1DArray.ArraySize       = value.DepthOrArraySize;
                    viewDesc.Texture1DArray.FirstArraySlice = 0;
                    viewDesc.Texture1DArray.MipSlice        = 0;
                }
                else
                {
                    viewDesc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
                    viewDesc.Texture1D.MipSlice = 0;
                }
            }
            break;

        case PASS_RESOURCE_DIMENSION_2D:
            {
                if (value.DepthOrArraySize > 1)
                {
                    viewDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    viewDesc.Texture2DArray.ArraySize       = value.DepthOrArraySize;
                    viewDesc.Texture2DArray.FirstArraySlice = 0;
                    viewDesc.Texture2DArray.MipSlice        = 0;
                    viewDesc.Texture2DArray.PlaneSlice      = 0;
                }
                else
                {
                    viewDesc.ViewDimension          = D3D12_UAV_DIMENSION_TEXTURE2D;
                    viewDesc.Texture2D.MipSlice     = 0;
                    viewDesc.Texture2D.PlaneSlice   = 0;
                }
            }
            break;

        case PASS_RESOURCE_DIMENSION_3D:
            {
                viewDesc.ViewDimension          = D3D12_UAV_DIMENSION_TEXTURE3D;
                viewDesc.Texture3D.FirstWSlice  = 0;
                viewDesc.Texture3D.MipSlice     = 0;
                viewDesc.Texture3D.WSize        = value.DepthOrArraySize;
            }
            break;
        }

        if (!CreateUnorderedAccessView(m_Resource, nullptr, &viewDesc, &m_UAV))
        {
            ELOG("Error : CreateUnorderedAcessView() Failed.");
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------
    //! @brief      シェーダリソースビューを生成します.
    //-------------------------------------------------------------------------
    bool CreateSRV(const PassResourceDesc& value)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
        switch(value.Dimension)
        {
        case PASS_RESOURCE_DIMENSION_BUFFER:
            {
                viewDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
                viewDesc.Buffer.FirstElement        = 0;
                viewDesc.Buffer.NumElements         = uint32_t(value.Width / value.Stride);
                viewDesc.Buffer.StructureByteStride = value.Stride;
                viewDesc.Buffer.Flags               = (value.Stride == 1) 
                                                        ? D3D12_BUFFER_SRV_FLAG_RAW
                                                        : D3D12_BUFFER_SRV_FLAG_NONE;
            }
            break;

        case PASS_RESOURCE_DIMENSION_1D:
            {
                if (value.DepthOrArraySize > 1)
                {
                    viewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                    viewDesc.Texture1DArray.ArraySize           = value.DepthOrArraySize;
                    viewDesc.Texture1DArray.FirstArraySlice     = 0;
                    viewDesc.Texture1DArray.MipLevels           = value.MipLevels;
                    viewDesc.Texture1DArray.MostDetailedMip     = 0;
                    viewDesc.Texture1DArray.ResourceMinLODClamp = 0;
                }
                else
                {
                    viewDesc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE1D;
                    viewDesc.Texture1D.MipLevels            = value.MipLevels;
                    viewDesc.Texture1D.MostDetailedMip      = 0;
                    viewDesc.Texture1D.ResourceMinLODClamp  = 0;
                }
            }
            break;

        case PASS_RESOURCE_DIMENSION_2D:
            {
                if (value.DepthOrArraySize > 1)
                {
                    viewDesc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    viewDesc.Texture2DArray.ArraySize           = value.DepthOrArraySize;
                    viewDesc.Texture2DArray.FirstArraySlice     = 0;
                    viewDesc.Texture2DArray.MipLevels           = value.MipLevels;
                    viewDesc.Texture2DArray.MostDetailedMip     = 0;
                    viewDesc.Texture2DArray.PlaneSlice          = 0;
                    viewDesc.Texture2DArray.ResourceMinLODClamp = 0;
                }
                else
                {
                    viewDesc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
                    viewDesc.Texture2D.MipLevels            = value.MipLevels;
                    viewDesc.Texture2D.MostDetailedMip      = 0;
                    viewDesc.Texture2D.PlaneSlice           = 0;
                    viewDesc.Texture2D.ResourceMinLODClamp  = 0;
                }
            }
            break;

        case PASS_RESOURCE_DIMENSION_3D:
            {
                viewDesc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE3D;
                viewDesc.Texture3D.MipLevels            = value.MipLevels;
                viewDesc.Texture3D.MostDetailedMip      = 0;
                viewDesc.Texture3D.ResourceMinLODClamp  = 0;
            }
            break;
        }

        if (!CreateShaderResourceView(m_Resource, &viewDesc, &m_SRV))
        {
            ELOG("Error : CreateShaderResourceView() Failed.");
            return false;
        }

        return true;
    }
};


///////////////////////////////////////////////////////////////////////////////
// PassResourceRegistry class
///////////////////////////////////////////////////////////////////////////////
class PassResourceRegistry
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    PassResourceRegistry()
    : m_Capacity    (0)
    , m_Cache       ()
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~PassResourceRegistry()
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      初期化処理を行います.
    //-------------------------------------------------------------------------
    void Init(uint32_t capacity)
    {
        m_Capacity   = capacity;
        m_Cache.Clear();
    }

    //-------------------------------------------------------------------------
    //! @brief      取得または生成をおこないます.
    //-------------------------------------------------------------------------
    PassResource* GetOrCreate(const PassResourceDesc& value, RenderPass* producer)
    {
        // LRUキャッシュアルゴリズム.
        PassResource* node;
        if (Contains(value, &node))
        {
            Remove(node);
            PushBack(node);
        }
        else if(m_Cache.GetCount() < m_Capacity)
        {
            node = CreateResource(value, producer);
            PushBack(node);
        }
        else
        {
            auto head = PopFront();
            m_Dispoer.Push(head);

            node = CreateResource(value, producer);
            PushBack(node);
        }

        return node;
    }

    //-------------------------------------------------------------------------
    //! @brief      クリア処理を行います.
    //-------------------------------------------------------------------------
    void Clear()
    {
        // 全部を突っ込む.
        auto itr = m_Cache.GetHead();
        while(itr != nullptr)
        {
            auto node = itr;
            m_Dispoer.Push(node);

            if (!itr->HasNext())
            { break; }

            itr = itr->List<PassResource>::Node::GetNext();
        }

        m_Cache.Clear();

        // 強制破棄.
        m_Dispoer.Clear();
    }

    //-------------------------------------------------------------------------
    //! @brief      フレーム同期を行い，遅延解放を行います.
    //-------------------------------------------------------------------------
    void FrameSync()
    { m_Dispoer.FrameSync(); }

    //-------------------------------------------------------------------------
    //! @brief      リスト先頭ポインタを取得します.
    //-------------------------------------------------------------------------
    PassResource* GetHead() const
    { return m_Cache.GetHead(); }

    //-------------------------------------------------------------------------
    //! @brief      リスト末尾ポインタを取得します.
    //-------------------------------------------------------------------------
    PassResource* GetTail() const
    { return m_Cache.GetTail();}

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    Disposer<PassResource>  m_Dispoer;
    uint32_t                m_Capacity = 0;
    List<PassResource>      m_Cache;

    //=========================================================================
    // private methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      構成設定が合致するリソースが含まれるかチェックします.
    //-------------------------------------------------------------------------
    bool Contains(const PassResourceDesc& value, PassResource** node)
    {
        auto itr = m_Cache.GetHead();
        while(itr != nullptr)
        {
            if (itr->Match(value))
            {
                *node = itr;
                return true;
            }

            if (!itr->HasNext())
            { break; }

            itr = itr->List<PassResource>::Node::GetNext();
        }

        return false;
    }

    //-------------------------------------------------------------------------
    //! @brief      リストから削除します.
    //-------------------------------------------------------------------------
    void Remove(PassResource* node)
    { m_Cache.Remove(node); }

    //-------------------------------------------------------------------------
    //! @brief      リスト末尾に追加します.
    //-------------------------------------------------------------------------
    void PushBack(PassResource* node)
    { m_Cache.PushBack(node); }

    //-------------------------------------------------------------------------
    //! @brief      リスト先頭からポップします.
    //-------------------------------------------------------------------------
    PassResource* PopFront()
    { return m_Cache.PopFront(); }

    //-------------------------------------------------------------------------
    //! @brief      リソースを生成します.
    //-------------------------------------------------------------------------
    PassResource* CreateResource(const PassResourceDesc& value, RenderPass* producer)
    {
        auto resource = new(std::nothrow) PassResource();
        assert(resource != nullptr);

        if (!resource->Init(value, producer))
        {
            ELOG("Error : PassResource::Init() Failed.");
            assert(false);
            resource->Term();
            return nullptr;
        }

        return resource;
    }
};

///////////////////////////////////////////////////////////////////////////////
// PassGraphContext class
///////////////////////////////////////////////////////////////////////////////
class PassGraphContext : public IPassGraphContext
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    PassGraphContext(ID3D12GraphicsCommandList6* pCommandList)
    : m_CommandList (pCommandList)
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      RTV用ディスクリプタハンドルを取得します.
    //-------------------------------------------------------------------------
    const IRenderTargetView* GetRTV(PassResource* resource, uint16_t index) const override
    {
        assert(resource != nullptr);
        return resource->GetRTV(index);
    }

    //-------------------------------------------------------------------------
    //! @brief      DSV用ディスクリプタハンドルを取得します.
    //-------------------------------------------------------------------------
    const IDepthStencilView* GetDSV(PassResource* resource, uint16_t index) const override
    {
        assert(resource != nullptr);
        return resource->GetDSV(index);
    }

    //-------------------------------------------------------------------------
    //! @brief      UAV用ディスクリプタハンドルを取得します.
    //-------------------------------------------------------------------------
    const IUnorderedAccessView* GetUAV(PassResource* resource) const override
    {
        assert(resource != nullptr);
        return resource->GetUAV();
    }

    //-------------------------------------------------------------------------
    //! @brief      SRV用ディスクリプタハンドルを取得します.
    //-------------------------------------------------------------------------
    const IShaderResourceView* GetSRV(PassResource* resource) const override
    {
        assert(resource != nullptr);
        return resource->GetSRV();
    }

    //-------------------------------------------------------------------------
    //! @brief      構成設定を取得します.
    //-------------------------------------------------------------------------
    D3D12_RESOURCE_DESC GetDesc(PassResource* resource) const override
    {
        assert(resource != nullptr);
        return resource->GetD3D12Desc();
    }

    //-------------------------------------------------------------------------
    //! @brief      コマンドリストを取得します.
    //-------------------------------------------------------------------------
    ID3D12GraphicsCommandList6* GetCommandList() const
    { return m_CommandList; }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    ID3D12GraphicsCommandList6* m_CommandList   = nullptr;

    //=========================================================================
    // private methods.
    //=========================================================================
    /* NOTHING */
};

///////////////////////////////////////////////////////////////////////////////
// RenderPass class
///////////////////////////////////////////////////////////////////////////////
class RenderPass : public List<RenderPass>::Node, public IRunnable
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    ///////////////////////////////////////////////////////////////////////////
    // ClearInfo structure
    //////////////////////////////////////////////////////////////////////////
    struct ClearInfo
    {
        PassResource*   Resource   = nullptr;
        ClearValue      ClearValue = {};
    };

    ///////////////////////////////////////////////////////////////////////////
    // ResourceHolder structure
    ///////////////////////////////////////////////////////////////////////////
    struct ResourceHolder
    {
        PassResource*       Resource        = nullptr;
        uint8_t             Flags           = RESOURCE_INFO_FLAG_NONE;
        Transition          Barrier         = {};
    };

    //=========================================================================
    // public variables.
    //=========================================================================
    char            m_Tag[64]       = {};
    PassSetup       m_Setup         = nullptr;
    PassExecute     m_Execute       = nullptr;
    uint8_t         m_SyncFlag      = SYNC_FLAG_NONE;
    bool            m_AsyncCompute  = false;
    uint8_t         m_ResourceCount = 0;
    uint8_t         m_ClearCount    = 0;
    ResourceHolder  m_Holders    [MAX_PASS_RESOURCE_COUNT] = {};
    ClearInfo       m_Clears     [MAX_PASS_RESOURCE_COUNT] = {};

    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    RenderPass()
    : List<RenderPass>::Node()
    , m_Setup           (nullptr)
    , m_Execute         (nullptr)
    , m_SyncFlag        (SYNC_FLAG_NONE)
    , m_RefCount        (1)
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      参照カウントを増やします.
    //-------------------------------------------------------------------------
    void Increment()
    { m_RefCount++; }

    //-------------------------------------------------------------------------
    //! @brief      参照カウントを減らします.
    //-------------------------------------------------------------------------
    void Decrement()
    { m_RefCount--; }

    //-------------------------------------------------------------------------
    //! @brief      参照カウントを取得します.
    //-------------------------------------------------------------------------
    int GetRefCount() const
    { return m_RefCount; }

    //-------------------------------------------------------------------------
    //! @brief      リソースバリアを設定します.
    //-------------------------------------------------------------------------
    void ResourceBarrier(ID3D12GraphicsCommandList6* pCmd)
    {
        D3D12_RESOURCE_BARRIER barriers[MAX_PASS_RESOURCE_COUNT] = {};
        auto count = 0u;
        for(auto i=0u; i<m_ResourceCount; ++i)
        {
            if (m_Holders[i].Flags & RESOURCE_INFO_FLAG_BARRIER)
            {
                auto transition = m_Holders[i].Barrier;
                barriers[count] = m_Holders[i].Resource->CreateBarrier(
                    transition.Before, transition.After, m_AsyncCompute);
                count++;
            }
        }

        pCmd->ResourceBarrier(count, barriers);
    }

    //-------------------------------------------------------------------------
    //! @brief      ビューをクリアします.
    //-------------------------------------------------------------------------
    void ClearViews(ID3D12GraphicsCommandList6* pCmd)
    {
        for(auto i=0u; i<m_ClearCount; ++i)
        { m_Clears[i].Resource->ClearView(pCmd, m_Clears[i].ClearValue); }
    }

    //-------------------------------------------------------------------------
    //! @brief      コマンドリスト設定します.
    //-------------------------------------------------------------------------
    void SetCommandList(ID3D12GraphicsCommandList6* pCmd)
    { m_CommandList = pCmd; }

    //-------------------------------------------------------------------------
    //! @brief      コマンドリストを取得します.
    //-------------------------------------------------------------------------
    ID3D12CommandList* GetCommandList() const
    { return reinterpret_cast<ID3D12CommandList*>(m_CommandList); }

    //-------------------------------------------------------------------------
    //! @brief      ジョブを実行します.
    //-------------------------------------------------------------------------
    void Run() override
    {
        PassGraphContext context(m_CommandList);

        // リソースバリア設定.
        ResourceBarrier(m_CommandList);

        // クリア処理.
        ClearViews(m_CommandList);

        // パスを実行.
        if (m_Execute != nullptr)
        { m_Execute(&context); }
    }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    int                         m_RefCount    = 0;
    ID3D12GraphicsCommandList6* m_CommandList = nullptr;

    //=========================================================================
    // private methods.
    //=========================================================================
    /* NOTHING */
};

///////////////////////////////////////////////////////////////////////////////
// PassGraph class
///////////////////////////////////////////////////////////////////////////////
class PassGraph : public IPassGraph
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    PassGraph();

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~PassGraph();

    //-------------------------------------------------------------------------
    //! @brief      初期化処理を行います.
    //-------------------------------------------------------------------------
    bool Init(const PassGraphDesc& desc);

    //-------------------------------------------------------------------------
    //! @brief      解放処理を行います.
    //-------------------------------------------------------------------------
    void Release() override;

    //-------------------------------------------------------------------------
    //! @brief      パスを追加します.
    //-------------------------------------------------------------------------
    bool AddPass(const char* tag, PassSetup setup, PassExecute execute) override;

    //-------------------------------------------------------------------------
    //! @brief      ビルドします.
    //-------------------------------------------------------------------------
    void Compile() override;

    //-------------------------------------------------------------------------
    //! @brief      レンダーパスを実行します。
    //-------------------------------------------------------------------------
    WaitPoint Execute(const WaitPoint& waitPoint) override;

    //-------------------------------------------------------------------------
    //! @brief      ブラックボードを取得します.
    //-------------------------------------------------------------------------
    Blackboard& GetBlackboard()
    { return m_Blackboard; }

    //-------------------------------------------------------------------------
    //! @brief      ブラックボードを取得します.
    //-------------------------------------------------------------------------
    const Blackboard& GetBlackboard() const
    { return m_Blackboard; }

    //-------------------------------------------------------------------------
    //! @brief      リソースを確保します.
    //-------------------------------------------------------------------------
    PassResource* AllocResource(const PassResourceDesc& desc, RenderPass* producer);

    //-------------------------------------------------------------------------
    //! @brief      フレームヒープからメモリを確保します.
    //-------------------------------------------------------------------------
    template<typename T>
    T* FrameAlloc()
    {
        auto ptr = m_FrameHeap.Alloc<T>();
        assert(ptr != nullptr);
        return ptr;
    }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    FrameHeap               m_FrameHeap;
    PassResourceRegistry    m_Registry;
    List<RenderPass>        m_PassList;
    uint8_t                 m_BufferIndex           = 0;
    CommandList*            m_GraphicsCommandLists  = nullptr;
    CommandList*            m_ComputeCommandLists   = nullptr;
    uint32_t                m_MaxPassCount          = 0;
    IThreadPool*            m_ThreadPool            = nullptr;
    CommandQueue*           m_GraphicsQueue         = nullptr;
    CommandQueue*           m_ComputeQueue          = nullptr;
    Blackboard              m_Blackboard;

    //=========================================================================
    // private methods.
    //=========================================================================
    /* NOTHING */
};

///////////////////////////////////////////////////////////////////////////////
// PassGraphBuilder class
///////////////////////////////////////////////////////////////////////////////
class PassGraphBuilder : public IPassGraphBuilder
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    PassGraphBuilder(PassGraph* graph, RenderPass* pass)
    : m_Graph(graph)
    , m_Pass (pass)
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      非同期コンピュートを設定します.
    //-------------------------------------------------------------------------
    void AsyncComputeEnable(bool value) override
    { m_Pass->m_AsyncCompute = value; }

    //-------------------------------------------------------------------------
    //! @brief      Readリソースを登録します.
    //-------------------------------------------------------------------------
    PassResource* Read(PassResource* resource) override
    {
        uint8_t flag = RESOURCE_INFO_FLAG_STATE_READ;
        auto index = m_Pass->m_ResourceCount;

        m_Pass->m_Holders[index].Resource = resource;
        m_Pass->m_Holders[index].Flags    = flag;
        m_Pass->m_ResourceCount++;

        resource->Increment();
        return resource;
    }

    //-------------------------------------------------------------------------
    //! @brief      Writeリソースを登録します.
    //-------------------------------------------------------------------------
    PassResource* Write(PassResource* resource) override
    {
        uint8_t flag = RESOURCE_INFO_FLAG_STATE_WRITE;

        auto index = m_Pass->m_ResourceCount;

        m_Pass->m_Holders[index].Resource = resource;
        m_Pass->m_Holders[index].Flags    = flag;
        m_Pass->m_ResourceCount++;

        m_Pass->Increment();
        return resource;
    }

    //-------------------------------------------------------------------------
    //! @brief      リソースを生成します.
    //-------------------------------------------------------------------------
    PassResource* Create(const PassResourceDesc& desc) override
    {
        auto resource = m_Graph->AllocResource(desc, m_Pass);

        if (desc.InitState == PASS_RESOURCE_STATE_CLEAR)
        {
            auto index = m_Pass->m_ClearCount;
            m_Pass->m_Clears[index].Resource    = resource;
            m_Pass->m_Clears[index].ClearValue  = desc.ClearValue;
            m_Pass->m_ClearCount++;
        }

        return resource;
    }

    //-------------------------------------------------------------------------
    //! @brief      リソースをインポートします.
    //-------------------------------------------------------------------------
    PassResource* Import
    (
        ID3D12Resource*         resource,
        D3D12_RESOURCE_STATES   state,
        IShaderResourceView*    pSRV,
        IUnorderedAccessView*   pUAV,
        IRenderTargetView**     pRTVs,
        IDepthStencilView**     pDSVs
    ) override
    {
        auto importResource = m_Graph->FrameAlloc<PassResource>();
        if (!importResource->Import(
            resource,
            state,
            pSRV,
            pUAV,
            pRTVs,
            pDSVs))
        { return nullptr; }

        return importResource;
    }

    //-------------------------------------------------------------------------
    //! @brief      ブラックボードを取得します.
    //-------------------------------------------------------------------------
    Blackboard& GetBlackboard() override
    { return m_Graph->GetBlackboard(); }

    //-------------------------------------------------------------------------
    //! @brief      ブラックボードを取得します.
    //-------------------------------------------------------------------------
    const Blackboard& GetBlackboard() const override
    { return m_Graph->GetBlackboard(); }

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    PassGraph*  m_Graph = nullptr;
    RenderPass* m_Pass  = nullptr;

    //=========================================================================
    // private methods.
    //=========================================================================
    /* NOTHING */
};

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
PassGraph::PassGraph()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
PassGraph::~PassGraph()
{
    if (m_ThreadPool != nullptr)
    {
        m_ThreadPool->Release();
        m_ThreadPool = nullptr;
    }

    m_FrameHeap.Term();

    if (m_GraphicsCommandLists != nullptr)
    {
        for(auto i=0u; i<m_MaxPassCount; ++i)
        { m_GraphicsCommandLists[i].Term(); }

        delete[] m_GraphicsCommandLists;
        m_GraphicsCommandLists = nullptr;
    }

    if (m_ComputeCommandLists != nullptr)
    {
        for(auto i=0u; i<m_MaxPassCount; ++i)
        { m_ComputeCommandLists[i].Term(); }

        delete[] m_ComputeCommandLists;
        m_ComputeCommandLists = nullptr;
    }

    m_GraphicsQueue = nullptr;
    m_ComputeQueue  = nullptr;

    m_Registry.Clear();
}

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool PassGraph::Init(const PassGraphDesc& desc)
{
    if (!CreateThreadPool(desc.MaxThreadCount, &m_ThreadPool))
    {
        ELOG("Error : CreateThreadPool() Failed.");
        return false;
    }

    auto pDevice = GetD3D12Device();

    m_MaxPassCount = desc.MaxPassCount;

    // 必要なメモリを計算.
    auto frameHeapSize = sizeof(RenderPass)   * desc.MaxPassCount
                       + sizeof(PassResource) * desc.MaxResourceCount;

    if (!m_FrameHeap.Init(frameHeapSize))
    {
        ELOG("Error : FrameHeap::Init() Failed.");
        return false;
    }

    m_Registry.Init(desc.MaxResourceCount);

    m_GraphicsCommandLists = new(std::nothrow) CommandList[m_MaxPassCount];
    assert(m_GraphicsCommandLists != nullptr);
    if (m_GraphicsCommandLists == nullptr)
    {
        ELOG("Error : Out of Memory.");
        return false;
    }

    for(auto i=0u; i<m_MaxPassCount; ++i)
    {
        if (!m_GraphicsCommandLists[i].Init(
            pDevice,
            D3D12_COMMAND_LIST_TYPE_DIRECT))
        {
            ELOG("Error : CommandList::Init() Failed.");
            return false;
        }
    }

    m_ComputeCommandLists = new(std::nothrow) CommandList[m_MaxPassCount];
    assert(m_ComputeCommandLists != nullptr);
    if (m_ComputeCommandLists == nullptr)
    {
        ELOG("Error : Out of Memory.");
        return false;
    }

    for(auto i=0u; i<m_MaxPassCount; ++i)
    {
        if (!m_ComputeCommandLists[i].Init(
            pDevice,
            D3D12_COMMAND_LIST_TYPE_COMPUTE))
        {
            ELOG("Error : CommandList::Init() Failed.");
            return false;
        }
    }

    m_GraphicsQueue = desc.pGraphicsQueue;
    m_ComputeQueue  = desc.pComputeQueue;

    return true;
}

//-----------------------------------------------------------------------------
//      解放処理を行います.
//-----------------------------------------------------------------------------
void PassGraph::Release()
{ delete this; }

//-----------------------------------------------------------------------------
//      パスを追加します.
//-----------------------------------------------------------------------------
bool PassGraph::AddPass(const char* tag, PassSetup setup, PassExecute execute)
{
    assert(m_PassList.GetCount() < m_MaxPassCount);
    if (m_PassList.GetCount() >= m_MaxPassCount)
    {
        ELOG("Error : Invalid Operation.");
        return false;
    }

    auto pass = FrameAlloc<RenderPass>();
    pass->m_Setup   = setup;
    pass->m_Execute = execute;
    CopyString(pass->m_Tag, tag, 63);

    m_PassList.PushBack(pass);

    return true;
}

//-----------------------------------------------------------------------------
//      ビルドします.
//-----------------------------------------------------------------------------
void PassGraph::Compile()
{
    // 各パスについて処理.
    {
        auto itr = m_PassList.GetHead();
        while(itr != nullptr)
        {
            PassGraphBuilder builder(this, itr);
            itr->m_Setup(&builder);

            if (!itr->HasNext())
            { break; }

            itr = itr->GetNext();
        }
    }

    // 参照カウントゼロのリソースを格納するスタック. 
    Stack<PassResource> stack;

    // 参照カウントがゼロのリソースを見つける.
    {
        auto itr = m_Registry.GetHead();
        while(itr != nullptr)
        {
            if (itr->GetRefCount() == 0)
            {
                // スタックに積む.
                stack.Push(itr);
            }

            if (!itr->HasNext())
            { break; }

            itr = itr->List<PassResource>::Node::GetNext();
        }
    }

    // スタックが空でない場合
    while(!stack.IsEmpty())
    {
        // リソースをpopし，生成したproducerの参照カウントを下げる.
        auto resource = stack.Pop();
        auto producer = resource->GetProducer();
        producer->Decrement();

        // producerの参照カウントが0の場合.
        if (producer->GetRefCount() == 0)
        {
            // 読み取りするリソースの参照カウントを下げる.
            for(auto i=0u; i<producer->m_ResourceCount; ++i)
            {
                if (producer->m_Holders[i].Flags & RESOURCE_INFO_FLAG_STATE_READ)
                {
                    auto readResource = producer->m_Holders[i].Resource;
                    readResource->Decrement();

                    // 参照カウントがゼロのときにそれらのリソースをスタックに積む.
                    if (readResource->GetRefCount() == 0)
                    { stack.Push(readResource); }
                }
            }
        }
    }

    // バリアを解決.
    {
        auto itr = m_PassList.GetHead();
        while(itr != nullptr)
        {
            if (itr->GetRefCount() == 0)
            {
                auto node = itr;
                if (!itr->HasNext())
                { break; }

                itr = itr->GetNext();
            }

            auto count = itr->m_ResourceCount;

            // バリアだけを張るグラフィックスパスを直前に追加する.
            if (itr->m_AsyncCompute)
            {
                auto pass = FrameAlloc<RenderPass>();
                pass->m_AsyncCompute    = false;
                pass->m_ResourceCount   = count;
                pass->m_SyncFlag        = SYNC_FLAG_GRAPHICS_TO_COMPUTE;

                for(auto i=0u; i<count; ++i)
                {
                    auto resource  = itr->m_Holders[i].Resource;
                    auto prevState = resource->PrevState;
                    auto nextState = RESOURCE_INFO_FLAGS(itr->m_Holders[i].Flags);

                    pass->m_Holders[i].Flags            = RESOURCE_INFO_FLAG_BARRIER;
                    pass->m_Holders[i].Resource         = resource;
                    pass->m_Holders[i].Barrier.Before   = prevState;
                    pass->m_Holders[i].Barrier.After    = nextState;

                    resource->PrevState     = nextState;
                    resource->PrevCompute   = false;
                }

                // コンピュートパスの前に追加.
                m_PassList.InsertAfter(itr->GetPrev(), pass);
            }

            for(auto i=0u; i<count; ++i)
            {
                auto resource  = itr->m_Holders[i].Resource;
                auto prevState = resource->PrevState;
                auto nextState = RESOURCE_INFO_FLAGS(itr->m_Holders[i].Flags);

                // リソースを使っていた前パスがコンピュートなら同期をとる.
                if (resource->PrevCompute && !itr->m_AsyncCompute)
                { itr->m_SyncFlag = SYNC_FLAG_COMPUTE_TO_GRAPHICS; }
                else if (resource->PrevCompute && itr->m_AsyncCompute)
                { itr->m_SyncFlag = SYNC_FLAG_COMPUTE_TO_COMPUTE; }

                // ステートが違っていたらバリアを張る.
                if (prevState != nextState)
                {
                    itr->m_Holders[i].Flags |= RESOURCE_INFO_FLAG_BARRIER;
                    itr->m_Holders[i].Barrier.Before = prevState;
                    itr->m_Holders[i].Barrier.After  = nextState;

                    // 一時ステート更新.
                    resource->PrevState   = nextState;
                    resource->PrevCompute = itr->m_AsyncCompute;
                }
            }

            if (!itr->HasNext())
            { break; }

            // 次のパスへ.
            itr = itr->GetNext();
        }
    }
}

//-----------------------------------------------------------------------------
//      レンダーパスを実行します。
//-----------------------------------------------------------------------------
WaitPoint PassGraph::Execute(const WaitPoint& waitPoint)
{
    // コマンドリストをリセット.
    for(auto i=0u; i<m_MaxPassCount; ++i)
    {
        m_GraphicsCommandLists[i].Reset();
        m_ComputeCommandLists[i].Reset();
    }

    // 有効なコマンドリストの数.
    auto graphisIndex = 0u;
    auto computeIndex = 0u;

    auto itr = m_PassList.GetHead();
    while(itr != nullptr)
    {
        // カリング.
        if (itr->GetRefCount() == 0)
        {
            if (!itr->HasNext())
            { break; }

            itr = itr->GetNext();
        }

        // コマンドリスト割り当て
        ID3D12GraphicsCommandList6* pCmd = nullptr;
        if (!itr->m_AsyncCompute)
        {
            pCmd = m_GraphicsCommandLists[graphisIndex].GetCommandList();
            graphisIndex++;
        }
        else
        {
            pCmd = m_ComputeCommandLists[computeIndex].GetCommandList();
            computeIndex++;
        }

        // レンダリングパスにコマンドリスト設定.
        itr->SetCommandList(pCmd);

        // スレッド実行.
        m_ThreadPool->Push(itr);

        // 次が無ければ修正.
        if (!itr->HasNext())
        { break; }

        // 次のレンダーパスへ.
        itr = itr->GetNext();
    }

    // レンダリングパスの完了を待機.
    m_ThreadPool->Wait();

    // 前フレームのコマンドが完了するまで待機.
    if (waitPoint.IsValid())
    { m_GraphicsQueue->Sync(waitPoint); }

    WaitPoint graphicsWaitPoint = {};
    WaitPoint computeWaitPoint  = {};

    // コマンドキューに積む.
    auto ir = m_PassList.GetHead();
    while(itr != nullptr)
    {
        auto pCmd = itr->GetCommandList();

        switch(itr->m_SyncFlag)
        {
            case SYNC_FLAG_NONE:
            {
                // コマンドリスト実行.
                m_GraphicsQueue->Execute(1, &pCmd);
            }
            break;

            case SYNC_FLAG_GRAPHICS_TO_COMPUTE:
            {
                if (!graphicsWaitPoint.IsValid())
                { graphicsWaitPoint = m_GraphicsQueue->Signal(); }

                // グラフィックスキューの完了を待機.
                m_ComputeQueue->Wait(graphicsWaitPoint);

                // コマンドリスト実行.
                m_ComputeQueue->Execute(1, &pCmd);

                // 待機点を取得.
                computeWaitPoint = m_ComputeQueue->Signal();
            }
            break;

            case SYNC_FLAG_COMPUTE_TO_GRAPHICS:
            {
                if (!computeWaitPoint.IsValid())
                { computeWaitPoint = m_ComputeQueue->Signal(); }

                // コンピュートキューの完了を待機.
                m_GraphicsQueue->Wait(computeWaitPoint);

                // コマンドリスト実行.
                m_GraphicsQueue->Execute(1, &pCmd);
            }
            break;

            case SYNC_FLAG_COMPUTE_TO_COMPUTE:
            {
                // 直前に絶対実行しているので待機点があるはず.
                assert(computeWaitPoint.IsValid());

                // コンピュートキューの完了を待機.
                m_ComputeQueue->Wait(computeWaitPoint);

                // コマンドリスト実行.
                m_ComputeQueue->Execute(1, &pCmd);
            }
            break;
        }

        if (!itr->HasNext())
        { break; }

        itr = itr->GetNext();
    }

    graphicsWaitPoint = m_GraphicsQueue->Signal();

    // パスをクリア.
    m_PassList.Clear();

    // ダブルバッファリング.
    m_BufferIndex = (m_BufferIndex + 1) & 0x1;

    // ヒープリセット.
    m_FrameHeap.Reset();

    return graphicsWaitPoint;
}

//-----------------------------------------------------------------------------
//      リソースを確保します.
//-----------------------------------------------------------------------------
PassResource* PassGraph::AllocResource(const PassResourceDesc& desc, RenderPass* producer)
{ return m_Registry.GetOrCreate(desc, producer); }

//-----------------------------------------------------------------------------
//      パスグラフを生成します.
//-----------------------------------------------------------------------------
bool CreatePassGraph(const PassGraphDesc& desc, IPassGraph** ppGraph)
{
    auto instance = new(std::nothrow) PassGraph;
    if (instance == nullptr)
    {
        ELOG("Error : Out of Memory.");
        return false;
    }

    if (!instance->Init(desc))
    {
        ELOG("Error : Init() Failed.");
        return false;
    }

    *ppGraph = instance;

    return true;
}

//-----------------------------------------------------------------------------
//      構成設定を取得します.
//-----------------------------------------------------------------------------
PassResourceDesc GetDesc(PassResource* resource)
{
    if (resource == nullptr)
    { return PassResourceDesc(); }

    return resource->GetDesc();
}

} // namespace asdx
