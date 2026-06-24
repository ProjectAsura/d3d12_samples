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
#include <gfx/asdxTextureManager.h>
#include <gfx/asdxFont.h>
#include <edit/asdxGuiMgr.h>

#if ASDX_ENABLE_SOUND
#include <snd/asdxSoundEngine.h>
#endif//ASDX_ENABLE_SOUND

namespace {

//-----------------------------------------------------------------------------
//      テクスチャをロードします.
//-----------------------------------------------------------------------------
bool LoadTexture(const char* path, asdx::TextureHolder& holder)
{
    asdx::fs::path input = path;
    asdx::fs::path findPath;
    if (!asdx::SearchFilePath(input, findPath))
    {
        ELOGA("Error : SearchFilePath() Failed. path = %s", path);
        return false;
    }

    holder = asdx::TextureManager::Instance().GetOrCreate(findPath.string().c_str());
    if (!holder.IsValid())
    {
        ELOGA("Error : Texture Load Faild. path = %s", findPath.string().c_str());
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      フォントをロードします.
//-----------------------------------------------------------------------------
bool LoadFont(ID3D12GraphicsCommandList* pCmd, const char* path, asdx::Font& font)
{
    asdx::fs::path input = path;
    asdx::fs::path findPath;
    if (!asdx::SearchFilePath(input, findPath))
    {
        ELOGA("Error : SearchFilePath() Failed. path = %s", path);
        return false;
    }

    std::vector<uint8_t> bin;
    if (!asdx::LoadA(findPath.string().c_str(), bin))
    {
        ELOGA("Error : File Load Failed. path = %s", findPath.string().c_str());
        return false;
    }

    if (!font.Init(std::move(bin)))
    {
        ELOGA("Error : Font::Init() Failed. path = %s", path);
        return false;
    }

    return true;
}

#if ASDX_ENABLE_SOUND
//-----------------------------------------------------------------------------
//      WAVファイルをロードします.
//-----------------------------------------------------------------------------
bool LoadWav(const char* path, asdx::SoundResource& resource, bool loop = false)
{
    asdx::fs::path input = path;
    asdx::fs::path findPath;
    if (!asdx::SearchFilePath(input, findPath))
    {
        ELOGA("Error : SearchFilePath() Failed. path = %s", path);
        return false;
    }

    asdx::SoundData data;
    if (!asdx::LoadFromWav(findPath.string().c_str(), data))
    {
        ELOGA("Error : File Load Failed. path = %s", findPath.string().c_str());
        return false;
    }

    data.Loop = loop;

    if (!resource.Init(data))
    {
        ELOGA("Error : SoundResource::Init() Failed.");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      Ogg Vorbis をロードします.
//-----------------------------------------------------------------------------
bool LoadOgg(const char* path, asdx::SoundResource& resource, bool loop = false)
{
    asdx::fs::path input = path;
    asdx::fs::path findPath;
    if (!asdx::SearchFilePath(input, findPath))
    {
        ELOGA("Error : SearchFilePath() Failed. path = %s", path);
        return false;
    }

    asdx::SoundData data;
    if (!asdx::LoadFromWav(findPath.string().c_str(), data))
    {
        ELOGA("Error : File Load Failed. path = %s", findPath.string().c_str());
        return false;
    }

    data.Loop = loop;

    if (!resource.Init(data))
    {
        ELOGA("Error : SoundResource::Init() Failed.");
        return false;
    }

    return true;
}

#endif

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
        // サウンドエンジンの初期化.
        if (!asdx::InitSound())
        {
            ELOGA("Error : InitSound() Failed.");
            return false;
        }
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
    // TODO : Implementation.
    {
    }

    // テクスチャマネージャ終了処理.
    asdx::TextureManager::Instance().Term();

    #if ASDX_ENABLE_IMGUI
    {
        // GUI終了処理.
        asdx::GuiMgr::Instance().Term();
    }
    #endif

    #if ASDX_ENABLE_SOUND
    {
        // サウンドエンジンの終了処理.
        asdx::TermSound();
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

    // TODO : Implementation.
    {
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

    // TODO : 描画処理.
    {
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
    ASDX_UNUSED(msg);
    ASDX_UNUSED(wp);
    ASDX_UNUSED(lp);

    // TODO : Implementation.
    {
    }
}