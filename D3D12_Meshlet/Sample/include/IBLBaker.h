//-----------------------------------------------------------------------------
// File : IBLBake.h
// Desc : Bake IBL.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <VertexBuffer.h>
#include <ConstantBuffer.h>
#include <DescriptorPool.h>


///////////////////////////////////////////////////////////////////////////////
// IBLBake class
///////////////////////////////////////////////////////////////////////////////
class IBLBaker
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    static const int    DFGTextureSize = 512;
    static const int    LDTextureSize  = 128;
    static const int    MipCount       = 7;

    //=========================================================================
    // public methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    IBLBaker();

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~IBLBaker();

    //-------------------------------------------------------------------------
    //! @brief      初期化処理を行います.
    //!
    //! @param[in]      pDevice     デバイスです.
    //! @param[in]      pPoolRes    リソース用ディスクリプタプールです.
    //! @param[in]      pPoolRTV    レンダーターゲットビュー用ディスクリプタプールです.
    //! @retval true    初期化成功.
    //! @retval false   初期化失敗.
    //-------------------------------------------------------------------------
    bool Init(
        ID3D12Device*   pDevice,
        DescriptorPool* pPoolRes,
        DescriptorPool* pPoolRTV);

    //-------------------------------------------------------------------------
    //! @brief      終了処理を行います.
    //-------------------------------------------------------------------------
    void Term();

    //-------------------------------------------------------------------------
    //! @brief      DFG項の積分を行います.
    //!
    //! @param[in]      pCmdList        コマンドリストです.
    //-------------------------------------------------------------------------
    void IntegrateDFG(ID3D12GraphicsCommandList*  pCmdList);

    //-------------------------------------------------------------------------
    //! @brief      LD項の積分を行います.
    //!
    //! @param[in]      pCmdList        コマンドリストです.
    //! @param[in]      handleCubeMap   入力キューブマップです.
    //-------------------------------------------------------------------------
    void IntegrateLD(
        ID3D12GraphicsCommandList*  pCmdList,
        uint32_t                    mapSize,
        uint32_t                    mipCount,
        D3D12_GPU_DESCRIPTOR_HANDLE handleCubeMap);

    //-------------------------------------------------------------------------
    //! @brief      DFGテクスチャのCPUディスクリプタハンドルを取得します.
    //!
    //! @return     DFGテクスチャのCPUディスクリプタハンドルを返却します.
    //-------------------------------------------------------------------------
    D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU_DFG() const;

    //-------------------------------------------------------------------------
    //! @brief      DiffuseLDテクスチャのCPUディスクリプタハンドルを取得します.
    //!
    //! @return     DiffuseLDテクスチャのCPUディスクリプタハンドルを返却します.
    //-------------------------------------------------------------------------
    D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU_DiffuseLD() const;

    //-------------------------------------------------------------------------
    //! @brief      SpecularLDテクスチャのCPUディスクリプタハンドルを取得します.
    //!
    //! @return     SpecularLDテクスチャのCPUディスクリプタハンドルを返却します.
    //-------------------------------------------------------------------------
    D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU_SpecularLD() const;

    //-------------------------------------------------------------------------
    //! @brief      DFGテクスチャのGPUディスクリプタハンドルを取得します.
    //!
    //! @return     DFGテクスチャのGPUディスクリプタハンドルを返却します.
    //-------------------------------------------------------------------------
    D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU_DFG() const;

    //-------------------------------------------------------------------------
    //! @brief      DiffuseLDテクスチャのGPUディスクリプタハンドルを取得します.
    //!
    //! @return     DiffuseLDテクスチャのGPUディスクリプタハンドルを返却します.
    //-------------------------------------------------------------------------
    D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU_DiffuseLD() const;

    //-------------------------------------------------------------------------
    //! @brief      SpecularLDテクスチャのGPUディスクリプタハンドルを取得します.
    //!
    //! @return     SpecularLDテクスチャのGPUディスクリプタハンドルを返却します.
    //-------------------------------------------------------------------------
    D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPU_SpecularLD() const;

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    ConstantBuffer                              m_BakeCB[MipCount * 6];                 //!< 畳み込み用定数バッファです.
    VertexBuffer                                m_QuadVB;                               //!< 矩形用頂点バッファです.
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_TexDFG;                               //!< DFGテクスチャです.
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_TexDiffuseLD;                         //!< DiffuseLDテクスチャです.
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_TexSpecularLD;                        //!< SpecularLDテクスチャです.
    DescriptorPool*                             m_pPoolRes;                             //!< リソース用ディスクリプタプールです.
    DescriptorPool*                             m_pPoolRTV;                             //!< レンダーターゲット用ディスクリプタプールです.
    DescriptorHandle*                           m_pHandleRTV_DFG;                       //!< DFGレンダーターゲットビューです.
    DescriptorHandle*                           m_pHandleRTV_DiffuseLD[6];              //!< DiffuseLDレンダーターゲットビューです.
    DescriptorHandle*                           m_pHandleRTV_SpecularLD[MipCount * 6];  //!< SpecularLDレンダーターゲットビューです.
    DescriptorHandle*                           m_pHandleSRV_DFG;                       //!< DFGシェーダリソースビューです.
    DescriptorHandle*                           m_pHandleSRV_DiffuseLD;                 //!< DiffuseLDシェーダリソースビューです.
    DescriptorHandle*                           m_pHandleSRV_SpecularLD;                //!< SpecularLDシェーダリソースビューです.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pDFG_PSO;                             //!< DFGパイプラインステートです.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pDiffuseLD_PSO;                       //!< DiffuseLDパイプラインステートです.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pSpecularLD_PSO;                      //!< SpecularLDパイプラインステートです.
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pDFG_RootSig;                         //!< DFGルートシグニチャです.
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pLD_RootSig;                          //!< LDルートシグニチャです.

    //=========================================================================
    // private methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      DiffuseLD項の積分計算を行います.
    //!
    //! @param[in]      pCmd        コマンドリストです.
    //! @param[in]      handle      キューブマップのGPUディスクリプタハンドルです.
    //-------------------------------------------------------------------------
    void IntegrateDiffuseLD(
        ID3D12GraphicsCommandList*  pCmd,
        D3D12_GPU_DESCRIPTOR_HANDLE handle);

    //-------------------------------------------------------------------------
    //! @brief      SpecularLD項の積分計算を行います.
    //!
    //! @param[in]      pCmd        コマンドリストです.
    //! @param[in]      handle      キューブマップのGPUディスクリプタハンドルです.
    //-------------------------------------------------------------------------
    void IntegrateSpecularLD(
        ID3D12GraphicsCommandList*  pCmd,
        D3D12_GPU_DESCRIPTOR_HANDLE handle);
};
