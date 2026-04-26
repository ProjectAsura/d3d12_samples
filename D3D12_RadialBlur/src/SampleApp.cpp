//-----------------------------------------------------------------------------
// File : SampleApp.cpp
// Desc : Sample Application.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <SampleApp.h>
#include <fnd/asdxPath.h>
#include <fnd/asdxLogger.h>
#include <fnd/asdxFileIO.h>
#include <fnd/asdxMisc.h>
#include <edit/asdxGuiMgr.h>
#include <gfx/asdxFade.h>

#include "../external/asdx12/external/imgui/imgui.h"


namespace {


} // namespace

///////////////////////////////////////////////////////////////////////////////
// SampleApp class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
SampleApp::SampleApp()
: asdx::App(L"Sample", 1920, 1080, nullptr, nullptr, nullptr)
{
    m_SwapChainFormat    = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;

    m_DeviceDesc.MaxShaderResourceCount = 8192;
    m_DeviceDesc.MaxSamplerCount        = 128;
    m_DeviceDesc.MaxColorTargetCount    = 256;
    m_DeviceDesc.MaxDepthTargetCount    = 256;

#if ASDX_DEBUG
    m_DeviceDesc.EnableDebug   = true;
    m_DeviceDesc.EnableCapture = true;
#endif
}

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
SampleApp::~SampleApp()
{
}

//-----------------------------------------------------------------------------
//      初期化処理です.
//-----------------------------------------------------------------------------
bool SampleApp::OnInit()
{
    auto pDevice = asdx::GetD3D12Device();

    #if ASDX_ENABLE_SOUND
    {
        // サウンドマネージャの初期化.
        asdx::InitSoundMgr(reinterpret_cast<uintptr_t>(m_hWnd));
    }
    #endif

    // コマンドリストをリセット.
    m_GfxCmdList.Reset();
    auto pCmd = m_GfxCmdList.GetD3D12CommandList();

    #if ASDX_ENABLE_IMGUI
    // GUI初期化.
    {
        if (!asdx::GuiMgr::Instance().Init(pCmd, m_hWnd, m_Width, m_Height, m_SwapChainFormat))
        {
            ELOGA("Error : GuiMgr::Init() Failed.");
            return false;
        }
    }
    #endif

    // テクスチャマネージャ初期化.
    if (!asdx::TextureManager::Instance().Init())
    {
        ELOGA("Error : TextureManager::Init() Failed.");
        return false;
    }

    // スプライトレンダラー初期化.
    if (!m_SpriteRenderer.Init(m_Width, m_Height, 512, 32, m_SwapChainFormat, DXGI_FORMAT_UNKNOWN))
    {
        ELOGA("Error : SpriteRenderer::Init() Failed.");
        return false;
    }

    // テクスチャ初期化.
    {
        asdx::fs::path input = "../res/textures/Test0.txb";
        asdx::fs::path path;
        if (!asdx::SearchFilePath(input, path))
        {
            ELOGA("Error : File Not Found. path= %s", input.string().c_str());
            return false;
        }

        m_TextureBG = asdx::TextureManager::Instance().GetOrCreate(path.string().c_str());
        if (!m_TextureBG.IsValid())
        {
            ELOGA("Error : Texture Load Failed.");
            return false;
        }
    }

    // サンプラー初期化.
    if (!m_LinearClamp.Init(&asdx::Sampler::LinearClamp))
    {
        ELOGA("Error : Sampler::Init() Failed.");
        return false;
    }

    // 放射ブラー初期化.
    if (!m_RadialBlur.Init(m_SwapChainFormat))
    {
        ELOGA("Error : RadialBlurEffect::Init() Failed.");
        return false;
    }

    // ブラーターゲット初期化.
    {
        asdx::TargetDesc desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width              = m_Width;
        desc.Height             = m_Height;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc         = { 1, 0 };
        desc.InitState          = D3D12_RESOURCE_STATE_COMMON;
        desc.ClearColor[0]      = 0.0f;
        desc.ClearColor[1]      = 0.0f;
        desc.ClearColor[2]      = 0.0f;
        desc.ClearColor[3]      = 0.0f;

        if (!m_BlurTarget.Init(&desc))
        {
            ELOGA("Error : BlurTarget Init Failed.");
            return false;
        }
    }

    // コマンドの記録を終了.
    pCmd->Close();

    // セットアップコマンド実行.
    {
        ID3D12CommandList* pCmds[] = { pCmd };

        // グラフィックスキューを取得.
        auto pGraphicsQueue = asdx::GetGraphicsQueue();

        // コマンドを実行.
        pGraphicsQueue->Execute(_countof(pCmds), pCmds);

        // 待機点を発行.
        m_FrameWaitPoint = pGraphicsQueue->Signal();

        // 完了を待機.
        pGraphicsQueue->Sync(m_FrameWaitPoint);
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了時の処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnTerm()
{
    // テクスチャ解放.
    m_TextureBG.Reset();

    // テクスチャマネージャ終了処理.
    asdx::TextureManager::Instance().Term();

    // スプライトレンダラー初期化.
    m_SpriteRenderer.Term();

    // サンプラー解放.
    m_LinearClamp.Term();

    // 放射ブラー解放.
    m_RadialBlur.Term();

    // ブラーターゲット解放.
    m_BlurTarget.Term();

    #if ASDX_ENABLE_IMGUI
    {
        // GUI終了処理.
        asdx::GuiMgr::Instance().Term();
    }
    #endif

    #if ASDX_ENABLE_SOUND
    {
        // サウンドマネージャの終了処理.
        asdx::TermSoundMgr();
    }
    #endif
}

//-----------------------------------------------------------------------------
//      フレーム遷移時の処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnFrameMove(const asdx::App::FrameEventArgs& args)
{
    #if ASDX_ENABLE_IMGUI
    {
        // ImGuiフレーム開始処理.
        asdx::GuiMgr::Instance().Update(m_Width, m_Height);

        ImGui::SetNextWindowSize(ImVec2(250, 110), ImGuiCond_Once);
        if (ImGui::Begin(ASDX_U8("放射ブラー")))
        {
            auto strength = m_RadialBlur.GetStrength();
            if (ImGui::DragFloat(ASDX_U8("ブラー強度"), &strength, 0.01f))
            {
                m_RadialBlur.SetStrength(strength);
            }

            auto center = m_RadialBlur.GetCenter();
            if (ImGui::DragFloat2(ASDX_U8("ブラー中心"), &center.x, 0.1f, 0.0f, 1.0f))
            {
                m_RadialBlur.SetCenter(center);
            }

            int count = m_RadialBlur.GetSampleCount();
            if (ImGui::DragInt(ASDX_U8("サンプル数"), &count, 1, 1, 255))
            {
                m_RadialBlur.SetSampleCount(uint8_t(count));
            }

            ImGui::End();
        }
    }
    #endif

    // スプライトレンダラーのフレームリセット処理.
    m_SpriteRenderer.Reset();
    m_SpriteRenderer.SetScreenSize(m_Width, m_Height);
}

//-----------------------------------------------------------------------------
//      フレーム描画時の処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnFrameRender(const asdx::App::FrameEventArgs& args)
{
    auto idx = GetCurrentBackBufferIndex();

    // コマンド記録開始.
    m_GfxCmdList.Reset();
    auto pCmd = m_GfxCmdList.GetD3D12CommandList();

    // カラーフィルタ描画.
    {
    #if USE_COMPUTE_SHADER
        auto inputDesc  = m_TextureBG.GetDesc();
        auto outputDesc = m_BlurTarget.GetDesc();

        m_BlurTarget.ChangeState(pCmd, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        m_RadialBlur.Dispatch(pCmd, 
            outputDesc.Width, outputDesc.Height, m_BlurTarget.GetGpuHandleUAV(),
            uint32_t(inputDesc.Width), inputDesc.Height, m_TextureBG.GetHandleGPU());

        m_BlurTarget.ChangeState(pCmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    #else
        float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        auto handleRTV = m_BlurTarget.GetCpuHandleRTV();

        m_BlurTarget.ChangeState(pCmd, D3D12_RESOURCE_STATE_RENDER_TARGET);

        pCmd->OMSetRenderTargets(1, &handleRTV, FALSE, nullptr);
        pCmd->ClearRenderTargetView(handleRTV, clearColor, 0, nullptr);
        pCmd->RSSetViewports(1, &m_Viewport);
        pCmd->RSSetScissorRects(1, &m_ScissorRect);

        auto desc = m_TextureBG.GetDesc();
        m_RadialBlur.Draw(pCmd, uint32_t(desc.Width), desc.Height, m_TextureBG.GetHandleGPU());

        m_BlurTarget.ChangeState(pCmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    #endif
    }

    m_ColorTarget[idx].ChangeState(pCmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
    auto handleRTV = m_ColorTarget[idx].GetCpuHandleRTV();
    auto handleDSV = m_DepthTarget.GetCpuHandleDSV();
    pCmd->OMSetRenderTargets(1, &handleRTV, FALSE, &handleDSV);
    pCmd->ClearRenderTargetView(handleRTV, m_ClearColor, 0, nullptr);
    pCmd->ClearDepthStencilView(handleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    pCmd->RSSetViewports(1, &m_Viewport);
    pCmd->RSSetScissorRects(1, &m_ScissorRect);

    // スプライト描画.
    {
        pCmd->SetGraphicsRootSignature(m_SpriteRenderer.GetRootSignature());
        m_SpriteRenderer.SetTexture(m_BlurTarget.GetGpuHandleSRV(), m_LinearClamp.GetHandleGPU());
        m_SpriteRenderer.Add(0, 0, m_Width, m_Height);
        m_SpriteRenderer.Draw(pCmd);
    }

    #if ASDX_ENABLE_IMGUI
    {
        // ImGui描画処理.
        asdx::GuiMgr::Instance().Draw(pCmd);
    }
    #endif

    m_ColorTarget[idx].ChangeState(pCmd, D3D12_RESOURCE_STATE_PRESENT);

    // コマンド記録終了.
    pCmd->Close();

    // コマンドを実行.
    {
        ID3D12CommandList* pCmds[] = { pCmd };

        auto pGraphicsQueue = asdx::GetGraphicsQueue();

        // 前フレームの描画の完了を待機.
        if (m_FrameWaitPoint.IsValid())
        { pGraphicsQueue->Sync(m_FrameWaitPoint); }

        // コマンドを実行.
        pGraphicsQueue->Execute(_countof(pCmds), pCmds);

        // 待機点を発行.
        m_FrameWaitPoint = pGraphicsQueue->Signal();
    }

    // 画面に表示.
    Present(1);

    // フレーム同期.
    asdx::FrameSync();
}

//-----------------------------------------------------------------------------
//      リサイズ時の処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnResize(const asdx::App::ResizeEventArgs& args)
{
    // ブラーターゲットリサイズ.
    m_BlurTarget.Resize(args.Width, args.Height);
}

//-----------------------------------------------------------------------------
//      キー処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnKey(const asdx::App::KeyEventArgs& args)
{
    m_Camera.OnKey(args.KeyCode, args.IsKeyDown, args.IsAltDown);
    #if ASDX_ENABLE_IMGUI
    {
        // ImGuiのキー処理.
        asdx::GuiMgr::Instance().OnKey(args.KeyCode, args.IsKeyDown, args.IsAltDown);
    }
    #endif
}

//-----------------------------------------------------------------------------
//      マウス処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnMouse(const asdx::App::MouseEventArgs& args)
{
    if(args.IsAltDown)
    {
        // カメラのマウス処理.
        m_Camera.OnMouse(
            args.X,
            args.Y,
            args.WheelDelta,
            args.IsDownL,
            args.IsDownR,
            args.IsDownM,
            args.IsDownX1,
            args.IsDownX2);
    }
    else
    {
        #if ASDX_ENABLE_IMGUI
        {
            // ImGuiのマウス処理.
            asdx::GuiMgr::Instance().OnMouse(
                args.X,
                args.Y,
                args.WheelDelta,
                args.IsDownL,
                args.IsDownM,
                args.IsDownR);
        }
        #endif
    }
}

//-----------------------------------------------------------------------------
//      タイピング処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnTyping(uint32_t keyCode)
{
    #if ASDX_ENABLE_IMGUI
    {
        // タイピング時の処理です.
        asdx::GuiMgr::Instance().OnTyping(keyCode);
    }
    #endif
}

//-----------------------------------------------------------------------------
//      メッセージプロシージャ時の処理です.
//-----------------------------------------------------------------------------
void SampleApp::OnMsgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    ASDX_UNUSED(hWnd);
    ASDX_UNUSED(wp);
    ASDX_UNUSED(lp);

    switch(msg)
    {
    case MM_MCINOTIFY:
        {
        #if ASDX_ENABLE_SOUND
            // サウンドマネージャのコールバック.
            asdx::OnSoundMsg(uint32_t(lp), uint32_t(wp));
        #endif//ASDX_ENABLE_SOUND
        }
        break;

    default:
        break;
    }
}