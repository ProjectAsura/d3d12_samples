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


namespace {

static const char* kText[] = {
    ASDX_U8("かくして、バロン国、飛空艇団"),
    ASDX_U8("「赤い翼」の部隊長であった"),
    ASDX_U8("暗黒騎士セシルは"),
    ASDX_U8("その座を剥奪され"),
    ASDX_U8("竜騎士部隊長カインと共に"),
    ASDX_U8("辺境の村、ミストを目指し"),
    ASDX_U8("切り深く立ち込める谷へと"),
    ASDX_U8("バロン城を後にした…"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("人々の夢であった"),
    ASDX_U8("天かける船、飛空艇"),
    ASDX_U8("…だが、その飛空艇の機動力は"),
    ASDX_U8("夢の実現ばかりか"),
    ASDX_U8("欲望を満たす手段にまでなりえた。"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("飛空艇団「赤い翼」により"),
    ASDX_U8("最強の軍事国家となったバロン国。"),
    ASDX_U8("なぜ、その強大な力をもつバロンが"),
    ASDX_U8("クリスタルを求めたのか…"),
    ASDX_U8("そしてなぜ、あまたの魔物が"),
    ASDX_U8("白日のもとに"),
    ASDX_U8("その姿を現し始めたのか…"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("\n"),
    ASDX_U8("クリスタルはただ静かに"),
    ASDX_U8("その光をたたえていた…"),
};

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
    m_SwapChainFormat    = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

    if (!asdx::TextureManager::Instance().Init())
    {
        ELOGA("Error : TextureManager::Init() Failed.");
        return false;
    }

    if (!m_SpriteRenderer.Init(m_Width, m_Height, UINT16_MAX, UINT16_MAX, m_SwapChainFormat, DXGI_FORMAT_UNKNOWN))
    {
        ELOGA("Error : SpriteRenderer::Init() Failed.");
        return false;
    }

    if (!asdx::FontRenderer::Instance().Init(m_SpriteRenderer))
    {
        ELOGA("Error : FontRenderer::Init() Failed");
        return false;
    }

    {
        asdx::fs::path path;
        if (!asdx::SearchFilePath("../res/font/MintMono35-Regular_32.fnb", path))
        {
            ELOGA("Error : File Not Found.");
            return false;
        }

        std::vector<uint8_t> binary;
        if (!asdx::LoadA(path.string().c_str(), binary))
        {
            ELOGA("Error : File Load Failed. path = %s", path.string().c_str());
            return false;
        }

        if (!m_MintMono.Init(pCmd, std::move(binary)))
        {
            ELOGA("Error : Font::Init() Failed.");
            return false;
        }
    }

    {
        asdx::fs::path path;
        if (!asdx::SearchFilePath("../res/texture/sample_bg.txb", path))
        {
            ELOGA("Error : File Not Found.");
            return false;
        }

        m_TextureBG = asdx::TextureManager::Instance().GetOrCreate(path.string().c_str());
        if (!m_TextureBG.IsValid())
        {
            ELOGA("Error : TextureManager::GetOrCreate() Failed. path = %s", path.string().c_str());
            return false;
        }
    }

    // 初期値設定.
    m_PosY = float(m_Height) - 500.0f;

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
    asdx::FontRenderer::Instance().Term();
    m_SpriteRenderer.Term();
    m_MintMono.Term();

    m_TextureBG.Reset();

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

    m_SpriteRenderer.Reset();
    m_SpriteRenderer.SetScreenSize(m_Width, m_Height);

    m_PosY -= float(args.ElapsedTimeSec) * 20.0f;

    auto count = _countof(kText);
    auto h = m_MintMono.GetBinary().GetLineHeight() * m_MintMono.GetBinary().GetFontSize() * 1.5f;
    auto y = m_PosY + h * count;
    if (y < 200.0f)
    {
        m_PosY = float(m_Height) - 500.0f;
    }
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

    auto handleRTV = m_ColorTarget[idx].GetCpuHandleRTV();
    auto handleDSV = m_DepthTarget.GetCpuHandleDSV();
    pCmd->OMSetRenderTargets(1, &handleRTV, FALSE, &handleDSV);
    pCmd->ClearRenderTargetView(handleRTV, m_ClearColor, 0, nullptr);
    pCmd->ClearDepthStencilView(handleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    pCmd->RSSetViewports(1, &m_Viewport);
    pCmd->RSSetScissorRects(1, &m_ScissorRect);

    {
        m_SpriteRenderer.SetPipelineState(pCmd);
        m_SpriteRenderer.SetTexture(m_TextureBG.GetHandleGPU(), asdx::FontRenderer::Instance().GetSampler().GetHandleGPU());
        m_SpriteRenderer.Add(0, 0, m_Width, m_Height);
        m_SpriteRenderer.Draw(pCmd);

        m_SpriteRenderer.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
        asdx::FontRenderer::Instance().SetState(pCmd, m_SpriteRenderer, m_MintMono);
        asdx::FontRenderer::Instance().SetScale(1.5f);
        asdx::FontRenderer::Instance().SetEnableOuter(true);
        asdx::FontRenderer::Instance().SetOuterColor(0.0f, 0.0f, 0.0f, 1.0f);

        D3D12_RECT rect = { LONG(100), LONG(200), LONG(m_Width - 100), LONG(m_Height - 500) };
        pCmd->RSSetScissorRects(1, &rect);

        auto count = _countof(kText);
        for(auto i=0; i<count; ++i)
        {
            auto w = m_MintMono.CalcWidth(kText[i], 1.5f);
            auto h = m_MintMono.GetBinary().GetLineHeight() * m_MintMono.GetBinary().GetFontSize() * 1.5f;
            asdx::FontRenderer::Instance().Add(m_SpriteRenderer, m_MintMono, (m_Width - w) / 2, int(m_PosY + h * i), kText[i]);
        }
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