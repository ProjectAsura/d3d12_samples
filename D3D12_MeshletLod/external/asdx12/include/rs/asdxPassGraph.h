//-----------------------------------------------------------------------------
// File : asdxPassGraph.h
// Desc : Pass Graph System.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>
#include <d3d12.h>
#include <fnd/asdxMath.h>
#include <fnd/asdxFunction.h>
#include <gfx/asdxTarget.h>
#include <gfx/asdxCommandQueue.h>
#include <rs/asdxBlackboard.h>

namespace asdx {

//-----------------------------------------------------------------------------
// Forward Declarations.
//-----------------------------------------------------------------------------
struct IPassGraphBuilder;
struct IPassGraphContext;
class  PassResource;

using PassSetup     = Action<IPassGraphBuilder*>;
using PassExecute   = Action<IPassGraphContext*>;


///////////////////////////////////////////////////////////////////////////////
// PASS_RESOURCE_DIMENSION enum
///////////////////////////////////////////////////////////////////////////////
enum PASS_RESOURCE_DIMENSION
{
    PASS_RESOURCE_DIMENSION_BUFFER, //!< バッファです.
    PASS_RESOURCE_DIMENSION_1D,     //!< 1次元テクスチャです.
    PASS_RESOURCE_DIMENSION_2D,     //!< 2次元テクスチャです.
    PASS_RESOURCE_DIMENSION_3D,     //!< 3次元テクスチャです.
};

///////////////////////////////////////////////////////////////////////////////
// PASS_RESOURCE_STATE enum
///////////////////////////////////////////////////////////////////////////////
enum PASS_RESOURCE_STATE
{
    PASS_RESOURCE_STATE_NONE,       //!< 何もしません.
    PASS_RESOURCE_STATE_CLEAR,      //!< リソースをクリアします.
};

///////////////////////////////////////////////////////////////////////////////
// PASS_RESOURCE_USAGE enum
///////////////////////////////////////////////////////////////////////////////
enum PASS_RESOURCE_USAGE
{
    PASS_RESOURCE_USAGE_NONE    = 0x0,  //!< SRVとして利用します.
    PASS_RESOURCE_USAGE_RTV     = 0x1,  //!< SRV + RTVとして利用します.
    PASS_RESOURCE_USAGE_DSV     = 0x2,  //!< SRV + DSVとして利用します.
    PASS_RESOURCE_USAGE_UAV     = 0x4,  //!< SRV + UAVとして利用します.
};

///////////////////////////////////////////////////////////////////////////////
// CLEAR_TYPE
///////////////////////////////////////////////////////////////////////////////
enum CLEAR_TYPE
{
    CLEAR_TYPE_RTV,         //!< RTVをクリアします.
    CLEAR_TYPE_DSV,         //!< DSVをクリアします.
    CLEAR_TYPE_UAV_FLOAT,   //!< UAVを浮動小数でクリアします.
    CLEAR_TYPE_UAV_UINT     //!< UAVを符号なし整数でクリアします.
};

////////////////////////////////////////////////////////////////////////////////
// ClearValue structure
///////////////////////////////////////////////////////////////////////////////
struct ClearValue
{
    CLEAR_TYPE Type;            //!< クリアタイプです.
    union
    {
        float Color[4];         //!< クリアカラーです.
        struct 
        {
            float   Depth;      //!< クリア深度です.
            uint8_t Stencil;    //!< クリアステンシルです.
        };
        float    Float[4];      //!< クリアに使用する浮動小数値です.
        uint32_t Uint[4];       //!< クリアに使用する符号なし整数値です.
    };
};

///////////////////////////////////////////////////////////////////////////////
// PassResourceDesc enum
///////////////////////////////////////////////////////////////////////////////
struct PassResourceDesc
{
    PASS_RESOURCE_DIMENSION Dimension           = PASS_RESOURCE_DIMENSION_2D;       //!< 次元です.
    uint64_t                Width               = 1920;                             //!< 横幅です.
    uint32_t                Height              = 1080;                             //!< 縦幅です.
    uint16_t                DepthOrArraySize    = 1;                                //!< 奥行または配列数です.
    uint16_t                MipLevels           = 1;                                //!< ミップレベル数です.
    DXGI_FORMAT             Format              = DXGI_FORMAT_UNKNOWN;              //!< フォーマットです.
    ClearValue              ClearValue          = {};                               //!< クリア値です.
    uint32_t                Stride              = 1;                                //!< 構造体のサイズです.
    PASS_RESOURCE_STATE     InitState           = PASS_RESOURCE_STATE_NONE;         //!< 初期ステートです.
    uint8_t                 Usage               = PASS_RESOURCE_USAGE_RTV;          //!< 使用用途です.
};


///////////////////////////////////////////////////////////////////////////////
// IPassGraphBuilder interface
///////////////////////////////////////////////////////////////////////////////
struct IPassGraphBuilder
{
    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    virtual ~IPassGraphBuilder()
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      非同期コンピュートフラグを設定します.
    //!
    //! @param[in]      value       設定するフラグです.
    //-------------------------------------------------------------------------
    virtual void AsyncComputeEnable(bool value) = 0;

    //-------------------------------------------------------------------------
    //! @brief      読み取りリソースを設定します.
    //!
    //! @param[in]      resource    読み取りするリソースです.
    //! @return     読み取りリソースを返却します.
    //-------------------------------------------------------------------------
    virtual PassResource* Read(PassResource* resource) = 0;

    //-------------------------------------------------------------------------
    //! @brief      書き込みリソースを設定します.
    //!
    //! @param[in]      resource    書き込みするリソースです.
    //! @return     書き込みリソースを返却します.
    //-------------------------------------------------------------------------
    virtual PassResource* Write(PassResource* resource) = 0;

    //-------------------------------------------------------------------------
    //! @brief      リソースを作成します.
    //!
    //! @param[in]      desc        リソース設定です.
    //! @return     生成リソースを返却します.
    //-------------------------------------------------------------------------
    virtual PassResource* Create(const PassResourceDesc& desc) = 0;

    //-------------------------------------------------------------------------
    //! @brief      ブラックボードを取得します.
    //!
    //! @return     ブラックボードを返却します.
    //-------------------------------------------------------------------------
    virtual Blackboard& GetBlackboard() = 0;

    //-------------------------------------------------------------------------
    //! @brief      ブラックボードを取得します.
    //!
    //! @return     ブラックボードを返却します.
    //-------------------------------------------------------------------------
    virtual const Blackboard& GetBlackboard() const = 0;

    //-------------------------------------------------------------------------
    //! @brief      リソースをインポートします.
    //!
    //! @param[in]      resource    インポートするリソースです.
    //! @param[in]      state       リソースステートです.
    //! @param[in]      pSRV        SRVです.
    //! @param[in]      pUAV        UAVです.
    //! @param[in]      pRTVs       RTV配列です.
    //! @param[in]      pDSVs       DSV配列です.
    //-------------------------------------------------------------------------
    virtual PassResource* Import(
        ID3D12Resource*         resource,
        D3D12_RESOURCE_STATES   state,
        IShaderResourceView*    pSRV,
        IUnorderedAccessView*   pUAV,
        IRenderTargetView**     pRTVs,
        IDepthStencilView**     pDSVs) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// IPassGraphContext interface
///////////////////////////////////////////////////////////////////////////////
struct IPassGraphContext
{
    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    virtual ~IPassGraphContext()
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      RTV用ディスクリプタハンドルを取得します.
    //!
    //! @param[in]      resource        リソースです.
    //! @param[in]      index           インデックス.
    //! @return     CPUディスクリプタハンドルを返却します.
    //-------------------------------------------------------------------------
    virtual const IRenderTargetView* GetRTV(PassResource* resource, uint16_t index = 0) const = 0;

    //-------------------------------------------------------------------------
    //! @brief      RTV用ディスクリプタハンドルを取得します.
    //!
    //! @param[in]      resource        リソースです.
    //! @param[in]      index           インデックス.
    //! @return     CPUディスクリプタハンドルを返却します.
    //-------------------------------------------------------------------------
    virtual const IDepthStencilView* GetDSV(PassResource* resource, uint16_t index = 0) const = 0;

    //-------------------------------------------------------------------------
    //! @brief      UAV用ディスクリプタハンドルを取得します.
    //!
    //! @param[in]      resource        リソースです.
    //! @return     GPUディスクリプタハンドルを返却します.
    //-------------------------------------------------------------------------
    virtual const IUnorderedAccessView* GetUAV(PassResource* resource) const = 0;

    //-------------------------------------------------------------------------
    //! @brief      SRV用ディスクリプタハンドルを取得します.
    //!
    //! @param[in]      resource        リソースです.
    //! @return     GPUディスクリプタハンドルを返却します.
    //-------------------------------------------------------------------------
    virtual const IShaderResourceView* GetSRV(PassResource* resource) const = 0;

    //-------------------------------------------------------------------------
    //! @brief      構成設定を取得します.
    //!
    //! @param[in]      resource        リソースです.
    //! @return     構成設定を返却します.
    //-------------------------------------------------------------------------
    virtual D3D12_RESOURCE_DESC GetDesc(PassResource* resource) const = 0;

    //-------------------------------------------------------------------------
    //! @brief      グラフィックスコマンドリストを取得します.
    //!
    //! @return     グラフィックスコマンドリストを返却します.
    //-------------------------------------------------------------------------
    virtual ID3D12GraphicsCommandList6* GetCommandList() const = 0;
};

///////////////////////////////////////////////////////////////////////////////
// IPassGraph interface
///////////////////////////////////////////////////////////////////////////////
struct IPassGraph
{
    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    virtual ~IPassGraph()
    { /* DO_NOTHING */ }

    //-------------------------------------------------------------------------
    //! @brief      破棄処理を行います.
    //-------------------------------------------------------------------------
    virtual void Release() = 0;

    //-------------------------------------------------------------------------
    //! @brief      レンダーパスを追加します.
    //!
    //! @param[in]      tag         タグ名です.
    //! @param[in]      setup       セットアップ処理です.
    //! @param[in]      execute     実行処理です.
    //! @retval true    追加に成功しました.
    //! @retval false   追加に失敗しました.
    //-------------------------------------------------------------------------
    virtual bool AddPass(const char* tag, PassSetup setup, PassExecute execute) = 0;

    //-------------------------------------------------------------------------
    //! @brief      コンパイルします.
    //-------------------------------------------------------------------------
    virtual void Compile() = 0;

    //-------------------------------------------------------------------------
    //! @brief      レンダーパスを実行します.
    //!
    //! @param[in]      value       前フレームの待機ポイントです.
    //! @return     待機ポイントを返却します.
    //-------------------------------------------------------------------------
    virtual WaitPoint Execute(const WaitPoint& value) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// PassGraphDesc structure
///////////////////////////////////////////////////////////////////////////////
struct PassGraphDesc
{
    uint32_t        MaxPassCount;       //!< 最大パス数です.
    uint32_t        MaxResourceCount;   //!< 最大リソース数です.
    uint8_t         MaxThreadCount;     //!< 最大スレッド数です.
    CommandQueue*   pGraphicsQueue;     //!< グラフィックスキューです.
    CommandQueue*   pComputeQueue;      //!< コンピュートキューです.
};

//-----------------------------------------------------------------------------
//! @brief      パスグラフを生成します.
//!
//! @param[in]      desc        構成設定です.
//! @param[out]     ppGraph     パスグラフの格納先です.
//! @retval true    生成に成功.
//! @retval false   生成に失敗.
//-----------------------------------------------------------------------------
bool CreatePassGraph(const PassGraphDesc& desc, IPassGraph** ppGraph);

//-----------------------------------------------------------------------------
//! @brief      構成設定を取得します.
//! 
//! @param[in]      pResource   パスリソースです.
//! @return     構成設定を返却します.
//-----------------------------------------------------------------------------
PassResourceDesc GetDesc(PassResource* pResource);

} // namespace asdx
