//-----------------------------------------------------------------------------
// File : GameApp.cpp
// Desc : Game Application.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "GameApp.h"
#include <fnd/asdxLogger.h>
#include <fnd/asdxPath.h>
#include <fnd/asdxMisc.h>
#include <fw/asdxSound.h>
#include "Bullet.h"

#if ASDX_DEBUG
#include <edit/asdxGuiMgr.h>
#include "../../external/asdx12/external/imgui/imgui.h"
#endif//ASDX_DEBUG


namespace {

//-----------------------------------------------------------------------------
//      テクスチャをロードします.
//-----------------------------------------------------------------------------
asdx::TextureHolder LoadTexture(const char* path)
{
    asdx::fs::path input = path;
    asdx::fs::path findPath;
    if (!asdx::SearchFilePath(input, findPath))
    {
        ELOGA("Error : File Not Found. path = %s", path);
        return asdx::TextureHolder();
    }

    return asdx::TextureManager::Instance().GetOrCreate(findPath.string().c_str());
}

} // namespace


///////////////////////////////////////////////////////////////////////////////
// GameApp class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
GameApp::GameApp()
: asdx::App(L"STG Sample", 1920, 1080, nullptr, nullptr, nullptr)
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
GameApp::~GameApp()
{
}

//-----------------------------------------------------------------------------
//      初期化時の処理です.
//-----------------------------------------------------------------------------
bool GameApp::OnInit()
{
    auto pDevice = asdx::GetD3D12Device();

    // サウンドマネージャの初期化.
    asdx::InitSoundMgr(reinterpret_cast<uintptr_t>(m_hWnd));

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
    if (!m_SpriteRenderer.Init(m_Width, m_Height, UINT16_MAX, 256, m_SwapChainFormat, DXGI_FORMAT_UNKNOWN))
    {
        ELOGA("Error : SpriteRenderer::Init() Failed.");
        return false;
    }

    // サンプラー初期化.
    if (!m_LinearClamp.Init(&asdx::Sampler::LinearClamp))
    {
        ELOGA("Error : Sampler::Init() Failed.");
        return false;
    }

    // オフスクリーンターゲットを初期化.
    {
        asdx::TargetDesc desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width              = m_Width;
        desc.Height             = m_Height;
        desc.DepthOrArraySize   = 1;
        desc.Format             = m_SwapChainFormat;
        desc.MipLevels          = 1;
        desc.SampleDesc         = { 1, 0 };
        desc.InitState          = D3D12_RESOURCE_STATE_RENDER_TARGET;
        desc.ClearColor[0]      = 0.0f;
        desc.ClearColor[1]      = 0.0f;
        desc.ClearColor[2]      = 0.0f;
        desc.ClearColor[3]      = 1.0f;

        if (!m_OffScreen.Init(&desc))
        {
            ELOGA("Error : ColorTarget::Init() Failed.");
            return false;
        }
    }

    // 背景テクスチャをロード.
    m_TextureBG = LoadTexture("../res/textures/BackGround/back_ground.txb");
    if (!m_TextureBG.IsValid())
    {
        ELOGA("Error : BackGround Texture Initialize Faild.");
        return false;
    }

    // スプライトチップをロード.
    m_SpriteChip = LoadTexture("../res/textures/kenney_space-shooter-remastered/sheet.txb");
    if (!m_SpriteChip.IsValid())
    {
        ELOGA("Error : Sprite Chip Initialized Failed.");
        return false;
    }

    // プレイヤー用弾マネージャの初期化.
    if (!GetPlayerBulletMgr().Init(256))
    {
        ELOGA("Error : Player Bullet Manager Initialize Failed.");
        return false;
    }

    // プレイヤー用弾マネージャの初期アk処理.
    if (!GetEnemyBulletMgr().Init(8192))
    {
        ELOGA("Error : Enemy Bullet Manager Initialize Failed.");
        return false;
    }

    // プレイヤー初期化.
    m_Player.Init(0u, PLAYER_SHIP2_BLUE, m_Width * 0.5f, m_Height * 0.5f);
    m_Player.SetPadLock(false);

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
void GameApp::OnTerm()
{
    m_Player.Term();

    m_SpriteChip.Reset();
    m_TextureBG .Reset();

    m_OffScreen  .Term();
    m_LinearClamp.Term();

    m_SpriteRenderer.Term();

    asdx::TextureManager::Instance().Term();

    GetPlayerBulletMgr().Term();
    GetEnemyBulletMgr ().Term();

    // サウンドマネージャの終了処理.
    asdx::TermSoundMgr();

    #if ASDX_ENABLE_IMGUI
    {
        // GUI終了処理.
        asdx::GuiMgr::Instance().Term();
    }
    #endif
}

//-----------------------------------------------------------------------------
//      フレーム遷移処理です.
//-----------------------------------------------------------------------------
void GameApp::OnFrameMove(const base::FrameEventArgs& args)
{
    #if ASDX_ENABLE_IMGUI
    {
        // ImGuiフレーム開始処理.
        asdx::GuiMgr::Instance().Update(m_Width, m_Height);
    }
    #endif

    // スプライトレンダラーのフレーム初期化処理.
    m_SpriteRenderer.Reset();
    m_SpriteRenderer.SetScreenSize(m_Width, m_Height);


    // スクロールスピード設定.
    const auto kMoveSpeedBG0   = 350.0f;
    const auto kMoveSpeedBG1   = 800.0f;
    const auto kMoveSpeedBG2   = 100.0f;

    // 背景スクロール処理.
    m_OffsetBG0 += int(args.ElapsedTimeSec * kMoveSpeedBG0);
    m_OffsetBG0 = asdx::Wrap(m_OffsetBG0, 0, int(m_Height));

    m_OffsetBG1 += int(args.ElapsedTimeSec * kMoveSpeedBG1);
    m_OffsetBG1 = asdx::Wrap(m_OffsetBG1, 0, int(m_Height));

    m_OffsetBG2 += int(args.ElapsedTimeSec * kMoveSpeedBG2);
    m_OffsetBG2 = asdx::Wrap(m_OffsetBG2, 0, int(m_Height));

    // 背景描画.
    m_SpriteRenderer.SetTexture(m_TextureBG.GetHandleGPU(), m_LinearClamp.GetHandleGPU());
    m_SpriteRenderer.SetColor(1.0f, 1.0f, 1.0f, 0.5f);
    m_SpriteRenderer.Add(0, m_OffsetBG0, m_Width, m_Height);
    m_SpriteRenderer.Add(0, m_OffsetBG0 - m_Height, m_Width, m_Height);

    m_SpriteRenderer.SetColor(1.0f, 1.0f, 1.0f, 0.5f);
    m_SpriteRenderer.Add(0, m_OffsetBG1, m_Width, m_Height);
    m_SpriteRenderer.Add(0, m_OffsetBG1 - m_Height, m_Width, m_Height);

    m_SpriteRenderer.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    m_SpriteRenderer.Add(0, m_OffsetBG2, m_Width, m_Height);
    m_SpriteRenderer.Add(0, m_OffsetBG2 - m_Height, m_Width, m_Height);

    // スプライトチップ設定.
    m_SpriteRenderer.SetTexture(m_SpriteChip.GetHandleGPU(), m_LinearClamp.GetHandleGPU());

    // 弾制御.
    GetEnemyBulletMgr ().Update(m_Width, m_Height);
    GetPlayerBulletMgr().Update(m_Width, m_Height);
    GetEnemyBulletMgr ().Draw(m_SpriteRenderer);
    GetPlayerBulletMgr().Draw(m_SpriteRenderer);

    // プレイヤー制御.
    m_Player.Update(m_Width, m_Height);
    m_Player.Draw(m_SpriteRenderer);
}

//-----------------------------------------------------------------------------
//      フレーム描画処理です.
//-----------------------------------------------------------------------------
void GameApp::OnFrameRender(const base::FrameEventArgs& args)
{
    auto idx = GetCurrentBackBufferIndex();

    // コマンド記録開始.
    m_GfxCmdList.Reset();
    auto pCmd = m_GfxCmdList.GetD3D12CommandList();

    // オフスクリーンに描画.
    {
        m_OffScreen.ChangeState(pCmd, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto handleRTV = m_OffScreen  .GetCpuHandleRTV();
        auto handleDSV = m_DepthTarget.GetCpuHandleDSV();

        float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

        pCmd->OMSetRenderTargets(1, &handleRTV, FALSE, &handleDSV);
        pCmd->ClearRenderTargetView(handleRTV, clearColor, 0, nullptr);
        pCmd->ClearDepthStencilView(handleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        pCmd->RSSetViewports(1, &m_Viewport);
        pCmd->RSSetScissorRects(1, &m_ScissorRect);

        pCmd->SetGraphicsRootSignature(m_SpriteRenderer.GetRootSignature());
        m_SpriteRenderer.Draw(pCmd);

        m_OffScreen.ChangeState(pCmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    // スワップチェインに描画.
    {
        m_ColorTarget[idx].ChangeState(pCmd, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto handleRTV = m_ColorTarget[idx].GetCpuHandleRTV();
        pCmd->OMSetRenderTargets(1, &handleRTV, FALSE, nullptr);
        pCmd->ClearRenderTargetView(handleRTV, m_ClearColor, 0, nullptr);
        pCmd->RSSetViewports(1, &m_Viewport);
        pCmd->RSSetScissorRects(1, &m_ScissorRect);

        pCmd->SetGraphicsRootSignature(m_SpriteRenderer.GetRootSignature());
        m_SpriteRenderer.SetTexture(m_OffScreen.GetGpuHandleSRV(), m_LinearClamp.GetHandleGPU());
        m_SpriteRenderer.Add(0, 0, m_Width, m_Height);
        m_SpriteRenderer.Draw(pCmd);

        #if ASDX_ENABLE_IMGUI
        {
            // ImGui描画処理.
            asdx::GuiMgr::Instance().Draw(pCmd);
        }
        #endif

        m_ColorTarget[idx].ChangeState(pCmd, D3D12_RESOURCE_STATE_PRESENT);
    }

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
void GameApp::OnResize(const base::ResizeEventArgs& args)
{
}

//-----------------------------------------------------------------------------
//      キー処理です.
//-----------------------------------------------------------------------------
void GameApp::OnKey(const base::KeyEventArgs& args)
{
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
void GameApp::OnMouse(const base::MouseEventArgs& args)
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

//-----------------------------------------------------------------------------
//      タイピング処理です.
//-----------------------------------------------------------------------------
void GameApp::OnTyping(uint32_t keyCode)
{
}

//-----------------------------------------------------------------------------
//      メッセージプロシージャ時の処理です.
//-----------------------------------------------------------------------------
void GameApp::OnMsgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    ASDX_UNUSED(hWnd);
    ASDX_UNUSED(wp);
    ASDX_UNUSED(lp);

    switch(msg)
    {
    case MM_MCINOTIFY:
        {
            // サウンドマネージャのコールバック.
            asdx::OnSoundMsg(uint32_t(lp), uint32_t(wp));
        }
        break;

    default:
        break;
    }
}
