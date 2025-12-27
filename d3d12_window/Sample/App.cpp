//---------------------------------------------------------------------------
// File : App.cpp
// Desc : Application Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "App.h"
#include <cstdio>


#ifndef WND_CLASSNAME
#define WND_CLASSNAME    L"AppClassWindow"
#endif//WND_CLASSNAME

#ifndef ELOG
#define ELOG(x, ...)    fprintf_s(stderr, "[File:%s, Line:%d]"x"\n", __FILE__, __LINE__, ##__VA_ARGS__ )
#endif//ELOG

///////////////////////////////////////////////////////////////////////////////
// App class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
App::App()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
App::~App()
{ Term(); }

//-----------------------------------------------------------------------------
//      メインループ処理です.
//-----------------------------------------------------------------------------
int App::Run()
{
    auto ret = -1;

    // 初期化に成功したらメインループを実行.
    if (Init())
    { ret = MainLoop(); }

    // 終了処理.
    Term();

    // 結果を返す.
    return ret;
}

//-----------------------------------------------------------------------------
//      初期化処理です.
//-----------------------------------------------------------------------------
bool App::Init()
{
    // COMライブラリの初期化.
    {
        HRESULT hr = CoInitialize(nullptr);
        if (FAILED(hr))
        {
            return false;
        }
    }

    // ウィンドウの初期化.
    if (!InitWnd())
    {
        return false;
    }

    // D3Dの初期化.
    if (!InitD3D())
    {
        return false;
    }

    // アプリケーション固有の初期化.
    if (!OnInit())
    {
        return false;
    }

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      終了処理です.
//-----------------------------------------------------------------------------
void App::Term()
{
    // アプリケーション固有の終了処理.
    OnTerm();

    // D3Dの終了処理.
    TermD3D();

    // ウィンドウの終了処理.
    TermWnd();

    // COMライブラリの終了処理.
    CoUninitialize();
}

//-----------------------------------------------------------------------------
//      ウィンドウの初期化処理です.
//-----------------------------------------------------------------------------
bool App::InitWnd()
{
    // インスタンスハンドルを取得.
    HINSTANCE hInst = GetModuleHandle( nullptr );
    if ( !hInst )
    {
        ELOG( "Error : GetModuleHandle() Failed." );
        return false;
    }

    // 拡張ウィンドウクラスの設定.
    WNDCLASSEXW wc;
    wc.cbSize           = sizeof(WNDCLASSEXW);
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInst;
    wc.hIcon            = LoadIcon(hInst, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName     = nullptr;
    wc.lpszClassName    = WND_CLASSNAME;
    wc.hIconSm          = LoadIcon(hInst, IDI_APPLICATION);

    // 拡張ウィンドウクラスを登録.
    if (!RegisterClassExW(&wc))
    {
        ELOG("Error : RegisterClassEx() Failed.");
        return false;
    }

    // インスタンスハンドルを設定.
    m_hInstance = hInst;

    RECT rc = { 0, 0, LONG(m_Width), LONG(m_Height) };

    // ウィンドウの矩形を調整.
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
    AdjustWindowRect( &rc, style, FALSE );

    // ウィンドウを生成.
    m_hWnd = CreateWindowW(
        WND_CLASSNAME,
        TEXT("SimpleSample"),   // ウィンドウタイトル名.
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        (rc.right - rc.left),
        (rc.bottom - rc.top),
        nullptr,
        nullptr,
        m_hInstance,
        nullptr );

    // エラーチェック.
    if (!m_hWnd)
    {
        ELOG("Error : CreateWindowW() Failed.");
        return false;
    }

    // ウィンドウを表示
    UpdateWindow(m_hWnd);
    ShowWindow(m_hWnd, SW_SHOWNORMAL);

    // フォーカス設定.
    SetFocus(m_hWnd);

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      ウィンドウの終了処理です.
//-----------------------------------------------------------------------------
void App::TermWnd()
{
    if (m_hInstance != nullptr)
    {
        UnregisterClassW(WND_CLASSNAME, m_hInstance);
        m_hInstance = nullptr;
    }

    m_hWnd = nullptr;
}

//-----------------------------------------------------------------------------
//      D3D12の初期化処理です.
//-----------------------------------------------------------------------------
bool App::InitD3D()
{
    HRESULT hr = S_OK;

#if defined(_DEBUG) || defined(DEBUG)
    RefPtr<ID3D12Debug> debug;
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()));
    if (SUCCEEDED(hr))
    {
        RefPtr<ID3D12Debug1> debug1;
        hr = debug->QueryInterface(IID_PPV_ARGS(debug1.GetAddressOf()));
        if (SUCCEEDED(hr))
        {
            debug1->EnableDebugLayer();
            debug1->SetEnableGPUBasedValidation(TRUE);
        }

        RefPtr<ID3D12Debug5> pDebug5;
        hr = debug->QueryInterface(IID_PPV_ARGS(pDebug5.GetAddressOf()));
        if (SUCCEEDED(hr))
        {
            pDebug5->SetEnableAutoName(TRUE);
        }
    }
#endif

    // デバイスを生成.
    hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_Device.GetAddressOf()));
    if (FAILED(hr))
    {
        ELOG("Error : D3D12CreateDevice() Failed. hr = 0x%x", hr);
        return false;
    }

    // コマンドキューの生成.
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type       = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority   = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags      = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask   = 0;

        hr = m_Device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_GraphicsQueue.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateCommandQueue() Failed. hr = 0x%x", hr);
            return false;
        }
    }

    // コマンドアロケータを生成.
    for(auto i=0; i<2; ++i)
    {
        hr = m_Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(m_CommandAllocator[i].GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateCommandAllocator() Failed. hr = 0x%x", hr);
            return false;
        }
    }

    // コマンドリストを生成.
    {
        hr = m_Device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_CommandAllocator[0].Get(),
            nullptr,
            IID_PPV_ARGS(m_CommandList.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateCommandList() Failed. hr = 0x%x", hr);
            return false;
        }
        // コマンドリストは生成直後は記録状態になっているため、閉じておく.
        m_CommandList->Close();
    }

    // スワップチェインを生成.
    {
        // DXGIファクトリを生成.
        RefPtr<IDXGIFactory4> pFactory;
        hr = CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG( "Error : CreateDXGIFactory() Failed." );
            return false;
        }

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width              = m_Width;
        desc.Height             = m_Height;
        desc.Format             = m_SwapChainFormat;
        desc.Stereo             = FALSE;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferCount        = 2;
        desc.Scaling            = DXGI_SCALING_STRETCH;
        desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Flags              = 0;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC screenDesc = {};
        screenDesc.RefreshRate      = { 60, 1 };
        screenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        screenDesc.Scaling          = DXGI_MODE_SCALING_STRETCHED;
        screenDesc.Windowed         = TRUE;

        RefPtr<IDXGISwapChain1> swapChain;
        hr = pFactory->CreateSwapChainForHwnd(
            m_GraphicsQueue.Get(),
            m_hWnd,
            &desc,
            &screenDesc,
            nullptr,
            reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : IDXGIFactory::CreateSwapChainForHwnd() Failed. hr = 0x%x", hr);
            return false;
        }

        hr = swapChain.As(&m_SwapChain);
        if (FAILED(hr))
        {
            ELOG("Error : IDXGISwapChain1::As() Failed. hr = 0x%x", hr);
            return false;
        }
    }

    // ディスクリプタヒープの生成
    UINT incrementRTV = 0;
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = 2;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask       = 0;
        hr = m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_HeapRTV.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateDescriptorHeap() Failed. hr = 0x%x", hr);
            return false;
        }
        incrementRTV = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // レンダーターゲットの生成.
    {
        for(auto i=0; i<2; ++i)
        {
            hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_RenderTargets[i].GetAddressOf()));
            if (FAILED(hr))
            {
                ELOG("Error : IDXGISwapChain::GetBuffer() Failed. hr = 0x%x", hr);
                return false;
            }
        }

        D3D12_RENDER_TARGET_VIEW_DESC desc = {};
        desc.Format                 = m_SwapChainFormat;
        desc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice     = 0;
        desc.Texture2D.PlaneSlice   = 0;

        m_HandleRTV[0]      = m_HeapRTV->GetCPUDescriptorHandleForHeapStart();
        m_HandleRTV[1].ptr  = m_HandleRTV[0].ptr + incrementRTV;

        for(auto i=0; i<2; ++i)
        {
            m_Device->CreateRenderTargetView(
                m_RenderTargets[i].Get(),
                &desc,
                m_HandleRTV[i]);
        }
    }

    // フェンスの生成.
    {
        hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateFence() Failed. hr = 0x%x", hr);
            return false;
        }
        m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_FenceEvent == nullptr)
        {
            ELOG("Error : CreateEvent() Failed.");
            return false;
        }
    }

    // ビューポートの設定.
    m_Viewport.Width    = FLOAT(m_Width);
    m_Viewport.Height   = FLOAT(m_Height);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;

    // シザー矩形の設定.
    m_ScissorRect.left   = 0;
    m_ScissorRect.right  = m_Width;
    m_ScissorRect.top    = 0;
    m_ScissorRect.bottom = m_Height;

    return true;
}

//-----------------------------------------------------------------------------
//      D3D12の終了処理です.
//-----------------------------------------------------------------------------
void App::TermD3D()
{
    for(auto i=0; i<2; ++i)
    {
        m_HandleRTV[i] = {};
        m_RenderTargets[i].Reset();
        m_CommandAllocator[i].Reset();
    }

    if (m_FenceEvent != nullptr)
    {
        CloseHandle(m_FenceEvent);
        m_FenceEvent = nullptr;
    }
    m_Fence         .Reset();
    m_HeapRTV       .Reset();
    m_CommandList   .Reset();
    m_GraphicsQueue .Reset();
    m_SwapChain     .Reset();
    m_Device        .Reset();
}

//-----------------------------------------------------------------------------
//      リサイズ処理です.
//-----------------------------------------------------------------------------
void App::Resize(uint32_t w, uint32_t h)
{
    m_Width  = w;
    m_Height = h;

    if (m_SwapChain.Get() == nullptr)
    { return; }

    // ビューポートの設定.
    m_Viewport.Width    = FLOAT(m_Width);
    m_Viewport.Height   = FLOAT(m_Height);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;

    // シザー矩形の設定.
    m_ScissorRect.left   = 0;
    m_ScissorRect.right  = m_Width;
    m_ScissorRect.top    = 0;
    m_ScissorRect.bottom = m_Height;

    if (m_SwapChain != nullptr)
    {
        WaitForGPU();

        // 描画ターゲットを解放.
        for(auto i=0; i<2; ++i)
        { m_RenderTargets[i].Reset(); }

        HRESULT hr = S_OK;

        // バッファをリサイズ.
        hr = m_SwapChain->ResizeBuffers(2, m_Width, m_Height, m_SwapChainFormat, 0 );
        if (FAILED(hr))
        { ELOG( "Error : IDXGISwapChain::ResizeBuffer() Failed. errcode = 0x%x", hr ); }

        D3D12_RENDER_TARGET_VIEW_DESC desc = {};
        desc.Format                 = m_SwapChainFormat;
        desc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice     = 0;
        desc.Texture2D.PlaneSlice   = 0;

        for(auto i=0; i<2; ++i)
        {
            m_Device->CreateRenderTargetView(
                m_RenderTargets[i].Get(),
                &desc,
                m_HandleRTV[i]);
        }
    }

    OnResize(w, h);
}

//-----------------------------------------------------------------------------
//      アプリケーション固有の初期化処理です.
//-----------------------------------------------------------------------------
bool App::OnInit()
{
    /* DO_NOTHING */
    return true;
}

//-----------------------------------------------------------------------------
//      アプリケーション固有の終了処理です.
//-----------------------------------------------------------------------------
void App::OnTerm()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      フレーム遷移処理です
//-----------------------------------------------------------------------------
void App::OnFrameMove()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      フレーム描画処理です
//-----------------------------------------------------------------------------
void App::OnFrameRender()
{
    auto idx = m_SwapChain->GetCurrentBackBufferIndex();

    m_CommandList->Reset(m_CommandAllocator[idx].Get(), nullptr);
    m_CommandList->RSSetViewports(1, &m_Viewport);
    m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                   = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource    = m_RenderTargets[idx].Get();
    barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_RENDER_TARGET;

    m_CommandList->ResourceBarrier(1, &barrier);

    // カラーバッファをクリア.
    float clearColor[] = { 0.39f, 0.58f, 0.92f, 1.0f };
    m_CommandList->ClearRenderTargetView(m_HandleRTV[idx], clearColor, 0, nullptr );

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    m_CommandList->ResourceBarrier(1, &barrier);

    m_CommandList->Close();
    ID3D12CommandList* ppCommandLists[] = { m_CommandList.Get() };
    m_GraphicsQueue->ExecuteCommandLists(1, ppCommandLists);

    // 画面に表示.
    Present(0);

    WaitForGPU();
}

//-----------------------------------------------------------------------------
//      リサイズ時の処理です.
//-----------------------------------------------------------------------------
void App::OnResize(uint32_t w, uint32_t h)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      スワップチェインに表示します.
//-----------------------------------------------------------------------------
void App::Present(uint32_t syncInterval)
{
    if (m_SwapChain.Get() == nullptr)
    { return; }

    HRESULT hr = S_OK;

    // スタンバイモードかどうかチェック.
    if (m_IsStandbyMode)
    {
        // テストする.
        hr = m_SwapChain->Present(syncInterval, DXGI_PRESENT_TEST);

        // スタンバイモードが解除されたかをチェック.
        if ( hr == S_OK )
        { m_IsStandbyMode = false; }

        // 処理を中断.
        return;
    }

    // 画面更新する.
    hr = m_SwapChain->Present(syncInterval, 0);

    switch( hr )
    {
    // デバイスがリセットされた場合(=コマンドが正しくない場合)
    case DXGI_ERROR_DEVICE_RESET:
        {
            // エラーログ出力.
            ELOG("Fatal Error : IDXGISwapChain::Present() Failed. ErrorCode = DXGI_ERROR_DEVICE_RESET.");

            // 続行できないのでダイアログを表示.
            MessageBoxW(m_hWnd, L"A Fatal Error Occured. Shutting down.", L"FATAL ERROR", MB_OK | MB_ICONERROR);

            // 終了メッセージを送る.
            PostQuitMessage(1);
        }
        break;

    // デバイスが削除された場合(=GPUがぶっこ抜かれた場合かドライバーアップデート中，またはGPUクラッシュ時.)
    case DXGI_ERROR_DEVICE_REMOVED:
        {
            // エラーログ出力.
            ELOG("Fatal Error : IDXGISwapChain::Present() Failed. ErrorCode = DXGI_ERROR_DEVICE_REMOVED.");

            // 続行できないのでダイアログを表示.
            MessageBoxW(m_hWnd, L"A Fatal Error Occured. Shutting down.", L"FATAL ERROR", MB_OK | MB_ICONERROR);

            // 終了メッセージを送る.
            PostQuitMessage(2);
        }
        break;

    // 表示領域がなければスタンバイモードに入る.
    case DXGI_STATUS_OCCLUDED:
        { m_IsStandbyMode = true; }
        break;

    // 現在のフレームバッファを表示する場合.
    case S_OK:
        { /* DO_NOTHING */ }
        break;
    }
}

//-----------------------------------------------------------------------------
//      GPUの完了を待機します.
//-----------------------------------------------------------------------------
void App::WaitForGPU()
{
    // シグナル状態にして，フェンス値を増加.
    const UINT64 fence = m_FenceValue;
    auto hr = m_GraphicsQueue->Signal(m_Fence.Get(), fence);
    if (FAILED(hr))
    {
        ELOG("Error : ID3D12CommandQueue::Signal() Failed.");
        return;
    }
    m_FenceValue++;

    // 完了を待機.
    if (m_Fence->GetCompletedValue() < fence)
    {
        hr = m_Fence->SetEventOnCompletion(fence, m_FenceEvent);
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Fence::SetEventOnCompletaion() Failed.");
            return;
        }

        WaitForSingleObject(m_FenceEvent, INFINITE);
    }
}

//-----------------------------------------------------------------------------
//      メインループ処理です.
//-----------------------------------------------------------------------------
int App::MainLoop()
{
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            OnFrameMove();
            OnFrameRender();
        }
    }
    return static_cast<int>(msg.wParam);
}

//-----------------------------------------------------------------------------
//      ウィンドウプロシージャです.
//-----------------------------------------------------------------------------
LRESULT CALLBACK App::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto pInstance = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    LRESULT result = 0;
    switch (msg)
    {
    case WM_CREATE:
        {
            auto pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lp);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        }
        break;
    case WM_DESTROY:
        { PostQuitMessage(0); }
        break;
    default:
        { result = DefWindowProc(hWnd, msg, wp, lp); }
        break;
    }
    return result;
}