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

static const char8_t* kText[] = {
    u8"あのイーハトーヴォのすきとおった風、",
    u8"夏でも底に冷たさをもつ青いそら、",
    u8"うつくしい森で飾られたモリーオ市、",
    u8"郊外のぎらぎらひかる草の波。",
    u8"またそのなかでいっしょになったたくさんのひとたち、",
    u8"ファゼーロとロザーロ、羊飼のミーロや、",
    u8"顔の赤いこどもたち、地主のテーモ、",
    u8"山猫博士のボーガント・デストゥパーゴなど、",
    u8"いまこの暗い巨きな石の建物のなかで考えていると、",
    u8"みんなむかし風のなつかしい青い幻燈のように思われます。",
    u8"では、わたくしはいつかの小さなみだしをつけながら、",
    u8"しずかにあの年のイーハトーヴォの五月から十月までを書きつけましょう。"
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

    // テクスチャマネージャの初期化.
    if (!asdx::TextureManager::Instance().Init())
    {
        ELOGA("Error : TextureManager::Init() Failed.");
        return false;
    }

    // スプライトレンダラーの初期化.
    if (!m_SpriteRenderer.Init(m_Width, m_Height, UINT16_MAX, UINT16_MAX, m_SwapChainFormat, DXGI_FORMAT_UNKNOWN))
    {
        ELOGA("Error : SpriteRenderer::Init() Failed.");
        return false;
    }

    // フォントレンダラーの初期化.
    if (!asdx::FontRenderer::Instance().Init(m_SpriteRenderer))
    {
        ELOGA("Error : FontRenderer::Init() Failed");
        return false;
    }

    // フォントの初期化.
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

    // 背景テクスチャの初期化.
    {
        asdx::fs::path path;
        if (!asdx::SearchFilePath("../res/textures/Test0.txb", path))
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
    // フォントレンダラーの終了処理.
    asdx::FontRenderer::Instance().Term();

    // スプライトレンダラーの終了処理.
    m_SpriteRenderer.Term();

    // フォントの解放処理.
    m_MintMono.Term();

    // 背景テクスチャの解放処理.
    m_TextureBG.Reset();

    // テクスチャマネージャの終了処理.
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

    // スクリーンサイズ設定.
    m_SpriteRenderer.SetScreenSize(m_Width, m_Height);

    // 文字列をスクロール.
    m_PosY -= float(args.ElapsedTimeSec) * 20.0f;

    auto count = _countof(kText);
    auto h = m_MintMono.GetBinary().GetLineHeight() * m_MintMono.GetBinary().GetFontSize() * 1.5f;
    auto y = m_PosY + h * count;

    // 最後まで表示したらリセット.
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
        // 背景描画.
        pCmd->SetGraphicsRootSignature(m_SpriteRenderer.GetRootSignature());
        m_SpriteRenderer.SetTexture(m_TextureBG.GetHandleGPU(), asdx::FontRenderer::Instance().GetSampler().GetHandleGPU());
        m_SpriteRenderer.Add(0, 0, m_Width, m_Height);
        m_SpriteRenderer.Draw(pCmd);

        // 文字列描画設定.
        m_SpriteRenderer.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
        asdx::FontRenderer::Instance().SetState(m_SpriteRenderer, m_MintMono);
        asdx::FontRenderer::Instance().SetScale(1.5f);
        asdx::FontRenderer::Instance().SetEnableOuter(true);
        asdx::FontRenderer::Instance().SetOuterColor(0.0f, 0.0f, 0.0f, 1.0f);

        // シーザー矩形設定.
        D3D12_RECT rect = { LONG(100), LONG(200), LONG(m_Width - 100), LONG(m_Height - 500) };
        pCmd->RSSetScissorRects(1, &rect);

        // テキスト描画.
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