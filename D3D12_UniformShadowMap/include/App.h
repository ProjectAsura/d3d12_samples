//-----------------------------------------------------------------------------
// File : App.h
// Desc : Sample Application.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <fnd/asdxMacro.h>
#include <fw/asdxApp.h>
#include <fw/asdxAppCamera.h>
#include <gfx/asdxCommandQueue.h>
#include <gfx/asdxBuffer.h>
#include <gfx/asdxModelManager.h>
#include <gfx/asdxTextureManager.h>
#include <gfx/asdxPipelineState.h>


///////////////////////////////////////////////////////////////////////////////
// SampleApp class
///////////////////////////////////////////////////////////////////////////////
class SampleApp final : public asdx::App
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
    SampleApp();

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    ~SampleApp();

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    asdx::AppCamera                     m_Camera;
    asdx::WaitPoint                     m_FrameWaitPoint;
    asdx::ModelHolder                   m_Model;
    asdx::TextureHolder                 m_DFGMap;
    asdx::TextureHolder                 m_DiffuseLDMap;
    asdx::TextureHolder                 m_SpecularLDMap;
    asdx::RefPtr<ID3D12RootSignature>   m_RootSig;
    asdx::GraphicsPipelineState         m_OpaqueState;
    asdx::GraphicsPipelineState         m_AlphaBlendState;
    asdx::DoubledConstantBuffer         m_SceneBuffer;
    asdx::DoubledConstantBuffer         m_DirLightBuffer;
    asdx::DoubledConstantBuffer         m_ShadowSceneBuffer;
    asdx::GraphicsPipelineState         m_ShadowState;
    asdx::DepthTarget                   m_ShadowMap;
    asdx::Vector3                       m_DirLightForward = asdx::Vector3(0.0f, -1.0f, 1.0f);

    //=========================================================================
    // private methods.
    //=========================================================================

    //-------------------------------------------------------------------------
    //! @brief      初期化時の処理です.
    //! 
    //! @retval true    初期化に成功.
    //! @retval false   初期化に失敗.
    //-------------------------------------------------------------------------
    bool OnInit() override;

    //-------------------------------------------------------------------------
    //! @brief      終了時の処理です.
    //-------------------------------------------------------------------------
    void OnTerm() override;

    //-------------------------------------------------------------------------
    //! @brief      フレーム遷移時の処理です.
    //! 
    //! @param[in]      args        イベント引数.
    //-------------------------------------------------------------------------
    void OnFrameMove(const asdx::App::FrameEventArgs& args) override;

    //-------------------------------------------------------------------------
    //! @brief      フレーム描画時の処理です.
    //! 
    //! @param[in]      args        イベント引数.
    //-------------------------------------------------------------------------
    void OnFrameRender(const asdx::App::FrameEventArgs& args) override;

    //-------------------------------------------------------------------------
    //! @brief      リサイズ時の処理です.
    //! 
    //! @param[in]      args        イベント引数.
    //-------------------------------------------------------------------------
    void OnResize(const asdx::App::ResizeEventArgs& args) override;

    //-------------------------------------------------------------------------
    //! @brief      キー処理です.
    //! 
    //! @param[in]      args        イベント引数.
    //-------------------------------------------------------------------------
    void OnKey(const asdx::App::KeyEventArgs& args) override;

    //-------------------------------------------------------------------------
    //! @brief      マウス処理です.
    //! 
    //! @param[in]      args        イベント引数.
    //-------------------------------------------------------------------------
    void OnMouse(const asdx::App::MouseEventArgs& args) override;

    //-------------------------------------------------------------------------
    //! @brief      タイピング時の処理です.
    //! 
    //! @param[in]      keyCode     キーコードです.
    //-------------------------------------------------------------------------
    void OnTyping(uint32_t keyCode) override;
};


