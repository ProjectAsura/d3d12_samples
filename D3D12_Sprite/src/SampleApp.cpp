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
#include <edit/asdxGuiMgr.h>


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
    //m_DeviceDesc.EnableCapture = true;
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

    // テクスチャマネージャーを初期化.
    if (!asdx::TextureManager::Instance().Init())
    {
        ELOGA("Error : TextureManager::Init() Failed.");
        return false;
    }

    // スプライトレンダラー初期化.
    if (!m_SpriteRenderer.Init(m_Width, m_Height, 4096, 4096, m_SwapChainFormat, m_DepthStencilFormat))
    {
        ELOGA("Error : SpriteRenderer::Init() Failed.");
        return false;
    }

    {
        asdx::fs::path input = "../res/textures/BackGround/back_ground.txb";
        asdx::fs::path path;
        if (!asdx::SearchFilePath(input, path))
        {
            ELOGA("Error : File Not Found.");
            return false;
        }

        m_TextureBG = asdx::TextureManager::Instance().GetOrCreate(path.string().c_str());
        if (!m_TextureBG.IsValid())
        {
            ELOGA("Error : TextureManager::GetOrCreate() Failed.");
            return false;
        }
    }

    {
        asdx::fs::path input = "../res/textures/kenney_space-shooter-remastered/playerShip3_green.txb";
        asdx::fs::path path;
        if (!asdx::SearchFilePath(input, path))
        {
            ELOGA("Error : File Not Found.");
            return false;
        }

        m_TexturePL = asdx::TextureManager::Instance().GetOrCreate(path.string().c_str());
        if (!m_TexturePL.IsValid())
        {
            ELOGA("Error : TextureManager::GetOrCreate() Failed.");
            return false;
        }
    }

    {
        asdx::fs::path input = "../res/textures/kenney_space-shooter-remastered/laserGreen11.txb";
        asdx::fs::path path;
        if (!asdx::SearchFilePath(input, path))
        {
            ELOGA("Error : TextureManager::GetOrCreate() Failed.");
            return false;
        }

        m_TextureLaser = asdx::TextureManager::Instance().GetOrCreate(path.string().c_str());
        if (!m_TextureLaser.IsValid())
        {
            ELOGA("Error : TextureManager::GetOrCreate() Failed.");
            return false;
        }
    }

    {
        if (!m_LinearClamp.Init(&asdx::Sampler::LinearClamp))
        {
            ELOGA("Error : Sampler::Init() Failed.");
            return false;
        }
    }

    m_Pad.SetPlayerIndex(0);

    m_PlayerPos.x = float(m_Width  / 2) - 46.0f;
    m_PlayerPos.y = float(m_Height / 2) - 37.0f;

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
    m_TextureBG.Reset();
    m_TexturePL.Reset();
    m_TextureLaser.Reset();
    m_LinearClamp.Term();

    m_SpriteRenderer.Term();

    asdx::TextureManager::Instance().Term();

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
    }
    #endif

    // フレームリセット.
    m_SpriteRenderer.Reset();
    m_SpriteRenderer.SetScreenSize(m_Width, m_Height);

    // HID更新.
    m_Pad     .UpdateState();
    m_Keyboard.UpdateState();

    const asdx::Vector2 kPlayerSize(98.0f, 75.0f);
    const asdx::Vector2 kLaserSize(9.0 * 2.0f, 54.0f * 2.0f);

    const auto kMoveSpeedPL    = 500.0f;
    const auto kMoveSpeedBG0   = 350.0f;
    const auto kMoveSpeedBG1   = 800.0f;
    const auto kMoveSpeedBG2   = 100.0f;
    const auto kMoveSpeedLaser = 2400.0f;

    m_OffsetBG0 += int(args.ElapsedTimeSec * kMoveSpeedBG0);
    m_OffsetBG0 = asdx::Wrap(m_OffsetBG0, 0, int(m_Height));

    m_OffsetBG1 += int(args.ElapsedTimeSec * kMoveSpeedBG1);
    m_OffsetBG1 = asdx::Wrap(m_OffsetBG1, 0, int(m_Height));

    m_OffsetBG2 += int(args.ElapsedTimeSec * kMoveSpeedBG2);
    m_OffsetBG2 = asdx::Wrap(m_OffsetBG2, 0, int(m_Height));


    // プレイヤー移動制御.
    {
        asdx::Vector2 dir = {};

        auto isHoldL = m_Pad.IsHold(asdx::PAD_LEFT)  || m_Keyboard.IsHold('A');
        auto isHoldR = m_Pad.IsHold(asdx::PAD_RIGHT) || m_Keyboard.IsHold('D');
        auto isHoldU = m_Pad.IsHold(asdx::PAD_UP)    || m_Keyboard.IsHold('W');
        auto isHoldD = m_Pad.IsHold(asdx::PAD_DOWN)  || m_Keyboard.IsHold('S');

        if (isHoldL)
            dir.x -= 1.0f;
        else if (isHoldR)
            dir.x += 1.0f;
        else
            dir.x += m_Pad.GetNormalizedThumbLX();

        if (isHoldU)
            dir.y -= 1.0f;
        else if (isHoldD)
            dir.y += 1.0f;
        else
            dir.y -= m_Pad.GetNormalizedThumbLY();


        m_PlayerPos += dir * float(args.ElapsedTimeSec) * kMoveSpeedPL;
        m_PlayerPos = asdx::Vector2::Clamp(m_PlayerPos, asdx::Vector2(0.0f, 0.0f), asdx::Vector2(m_Width - kPlayerSize.x, m_Height - kPlayerSize.y));
    }

    // レーザー移動制御.
    for(auto& laser : m_Layers)
    {
        if (!laser.Shoot)
            continue;

        laser.Pos.y -= float(args.ElapsedTimeSec) * kMoveSpeedLaser;

        // レーザー描画.
        m_SpriteRenderer.SetTexture(m_TextureLaser.GetHandleGPU(), m_LinearClamp.GetHandleGPU());
        m_SpriteRenderer.Add(int(laser.Pos.x), int(laser.Pos.y), int(kLaserSize.x), int(kLaserSize.y));

        if ((laser.Pos.y + kLaserSize.y) < 0.0f)
        {
            laser.Shoot = false;
            laser.Pos   = asdx::Vector2(0.0f, 0.0f);
        }
    }

    // レーザー発射判定.
    auto isShot = m_Pad.IsDown(asdx::PAD_B) || m_Keyboard.IsDown('J');
    if (isShot)
    {
        auto& laser = m_Layers[m_LaserIndex];
        if (!laser.Shoot)
        {
            laser.Pos   = m_PlayerPos + asdx::Vector2((kPlayerSize.x - kLaserSize.x) * 0.5f , -kLaserSize.y);
            laser.Shoot = true;

            m_LaserIndex = (m_LaserIndex + 1) % int(m_Layers.size());

            m_SpriteRenderer.SetTexture(m_TextureLaser.GetHandleGPU(), m_LinearClamp.GetHandleGPU());
            m_SpriteRenderer.Add(int(laser.Pos.x), int(laser.Pos.y), int(kLaserSize.x), int(kLaserSize.y));
        }
    }

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

    // プレイヤー描画.
    m_SpriteRenderer.SetTexture(m_TexturePL.GetHandleGPU(), m_LinearClamp.GetHandleGPU());
    m_SpriteRenderer.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    m_SpriteRenderer.Add(int(m_PlayerPos.x), int(m_PlayerPos.y), int(kPlayerSize.x), int(kPlayerSize.y));

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

    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_ColorTarget[idx].GetResource();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_RENDER_TARGET;
        pCmd->ResourceBarrier(1, &barrier);
    }

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    auto handleRTV = m_ColorTarget[idx].GetCpuHandleRTV();
    auto handleDSV = m_DepthTarget.GetCpuHandleDSV();
    pCmd->OMSetRenderTargets(1, &handleRTV, FALSE, &handleDSV);
    pCmd->ClearRenderTargetView(handleRTV, clearColor, 0, nullptr);
    pCmd->ClearDepthStencilView(handleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    pCmd->RSSetViewports(1, &m_Viewport);
    pCmd->RSSetScissorRects(1, &m_ScissorRect);

    // スプライト描画.
    {
        m_SpriteRenderer.SetPipelineState(pCmd);
        m_SpriteRenderer.Draw(pCmd);
    }

    #if ASDX_ENABLE_IMGUI
    {
        // ImGui描画処理.
        asdx::GuiMgr::Instance().Draw(pCmd);
    }
    #endif

    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_ColorTarget[idx].GetResource();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_PRESENT;
        pCmd->ResourceBarrier(1, &barrier);
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
void SampleApp::OnResize(const asdx::App::ResizeEventArgs& args)
{
    // TODO : Implementation.
    {
    }
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