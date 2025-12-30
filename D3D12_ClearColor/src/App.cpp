//-----------------------------------------------------------------------------
// File : App.cpp
// Desc : Application Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <App.h>
#include <cassert>


namespace /* anonymous */ {

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
const auto ClassName = TEXT("SampleWindowClass");    //!< ウィンドウクラス名です.

} // namespace /* anonymous */


///////////////////////////////////////////////////////////////////////////////
// App class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
App::App(uint32_t width, uint32_t height)
: m_hInst       (nullptr)
, m_hWnd        (nullptr)
, m_Width       (width)
, m_Height      (height)
, m_pDevice     (nullptr)
, m_pQueue      (nullptr)
, m_pSwapChain  (nullptr)
, m_pCmdList    (nullptr)
, m_pHeapRTV    (nullptr)
, m_pFence      (nullptr)
, m_FrameIndex  (0)
{
    for(auto i=0u; i<FrameCount; ++i)
    {
        m_pColorBuffer [i] = nullptr;
        m_pCmdAllocator[i] = nullptr;
        m_FenceCounter [i] = 0;
    }
}

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
App::~App()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      実行します.
//-----------------------------------------------------------------------------
void App::Run()
{
    if (InitApp())
    { MainLoop(); }

    TermApp();
}

//-----------------------------------------------------------------------------
//      初期化処理です.
//-----------------------------------------------------------------------------
bool App::InitApp()
{
    // ウィンドウの初期化.
    if (!InitWnd())
    { return false; }

    // Direct3D 12の初期化.
    if (!InitD3D())
    { return false; }

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      終了処理です.
//-----------------------------------------------------------------------------
void App::TermApp()
{
    // Direct3D 12の終了処理.
    TermD3D();

    // ウィンドウの終了処理.
    TermWnd();
}

//-----------------------------------------------------------------------------
//      ウィンドウの初期化処理です.
//-----------------------------------------------------------------------------
bool App::InitWnd()
{
    // インスタンスハンドルを取得.
    auto hInst = GetModuleHandle(nullptr);
    if (hInst == nullptr)
    { return false; }

    // ウィンドウの設定.
    WNDCLASSEX wc = {};
    wc.cbSize           = sizeof(WNDCLASSEX);
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = WndProc;
    wc.hIcon            = LoadIcon(hInst, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(hInst, IDC_ARROW);
    wc.hbrBackground    = GetSysColorBrush(COLOR_BACKGROUND);
    wc.lpszMenuName     = nullptr;
    wc.lpszClassName    = ClassName;
    wc.hIconSm          = LoadIcon(hInst, IDI_APPLICATION);

    // ウィンドウの登録.
    if (!RegisterClassEx(&wc))
    { return false; }

    // インスタンスハンドル設定.
    m_hInst = hInst;

    // ウィンドウのサイズを設定.
    RECT rc = {};
    rc.right  = static_cast<LONG>(m_Width);
    rc.bottom = static_cast<LONG>(m_Height);

    // ウィンドウサイズを調整.
    auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    AdjustWindowRect(&rc, style, FALSE);

    // ウィンドウを生成.
    m_hWnd = CreateWindowEx(
        0,
        ClassName,
        TEXT("Sample"),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rc.right  - rc.left,
        rc.bottom - rc.top,
        nullptr,
        nullptr,
        m_hInst,
        nullptr);

    if (m_hWnd == nullptr)
    { return false; }
 
    // ウィンドウを表示.
    ShowWindow(m_hWnd, SW_SHOWNORMAL);

    // ウィンドウを更新.
    UpdateWindow(m_hWnd);

    // ウィンドウにフォーカスを設定.
    SetFocus(m_hWnd);

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      ウィンドウの終了処理です.
//-----------------------------------------------------------------------------
void App::TermWnd()
{
    // ウィンドウの登録を解除.
    if (m_hInst != nullptr)
    { UnregisterClass(ClassName, m_hInst); }

    m_hInst = nullptr;
    m_hWnd  = nullptr;
}

//-----------------------------------------------------------------------------
//      Direct3Dの初期化処理です.
//-----------------------------------------------------------------------------
bool App::InitD3D()
{
    #if defined(DEBUG) || defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debug;
        auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()));

        // デバッグレイヤーを有効化.
        if (SUCCEEDED(hr))
        { debug->EnableDebugLayer(); }
    }
    #endif

    // デバイスの生成.
    auto hr = D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(m_pDevice.GetAddressOf()));
    if (FAILED(hr))
    { return false; }

    // コマンドキューの生成.
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        hr = m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(m_pQueue.GetAddressOf()));
        if (FAILED(hr))
        { return false; }
    }

    // スワップチェインの生成.
    {
        // DXGIファクトリーの生成.
        ComPtr<IDXGIFactory4> pFactory = nullptr;
        hr = CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf()));
        if (FAILED(hr))
        { return false; }

        // スワップチェインの設定.
        DXGI_SWAP_CHAIN_DESC desc = {};
        desc.BufferDesc.Width                   = m_Width;
        desc.BufferDesc.Height                  = m_Height;
        desc.BufferDesc.RefreshRate.Numerator   = 60;
        desc.BufferDesc.RefreshRate.Denominator = 1;
        desc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        desc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
        desc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count                   = 1;
        desc.SampleDesc.Quality                 = 0;
        desc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount                        = FrameCount;
        desc.OutputWindow                       = m_hWnd;
        desc.Windowed                           = TRUE;
        desc.SwapEffect                         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        // スワップチェインの生成.
        ComPtr<IDXGISwapChain> pSwapChain;
        hr = pFactory->CreateSwapChain(m_pQueue.Get(), &desc, pSwapChain.GetAddressOf());
        if (FAILED(hr))
        { return false; }

        // IDXGISwapChain3 を取得.
        hr = pSwapChain.As(&m_pSwapChain);
        if (FAILED(hr))
        { return false; }

        // バックバッファ番号を取得.
        m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

        // 不要になったので解放.
        pFactory  .Reset();
        pSwapChain.Reset();
    }

    // コマンドアロケータの生成.
    {
        for(auto i=0u; i<FrameCount; ++i)
        {
            hr = m_pDevice->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(m_pCmdAllocator[i].GetAddressOf()));
            if (FAILED(hr))
            { return false; }
        }
    }

    // コマンドリストの生成.
    {
        hr = m_pDevice->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_pCmdAllocator[m_FrameIndex].Get(),
            nullptr,
            IID_PPV_ARGS(m_pCmdList.GetAddressOf()));
        if (FAILED(hr))
        { return false; }
    }

    // レンダーターゲットビューの生成.
    {
        // ディスクリプタヒープの設定.
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = FrameCount;
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask       = 0;

        // ディスクリプタヒープを生成.
        hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pHeapRTV.GetAddressOf()));
        if (FAILED(hr))
        { return false; }

        auto handle = m_pHeapRTV->GetCPUDescriptorHandleForHeapStart();
        auto incrementSize = m_pDevice
            ->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        for (auto i=0u; i<FrameCount; ++i)
        {
            hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pColorBuffer[i].GetAddressOf()));
            if (FAILED(hr))
            { return false; }

            D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
            viewDesc.Format                 = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            viewDesc.ViewDimension          = D3D12_RTV_DIMENSION_TEXTURE2D;
            viewDesc.Texture2D.MipSlice     = 0;
            viewDesc.Texture2D.PlaneSlice   = 0;

            // レンダーターゲットビューの生成.
            m_pDevice->CreateRenderTargetView(m_pColorBuffer[i].Get(), &viewDesc, handle);

            m_HandleRTV[i] = handle;
            handle.ptr += incrementSize;
        }
    }

    // フェンスの生成.
    {
        // フェンスカウンターをリセット.
        for(auto i=0u; i<FrameCount; ++i)
        { m_FenceCounter[i] = 0; }

        // フェンスの生成.
        hr = m_pDevice->CreateFence(
            m_FenceCounter[m_FrameIndex],
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(m_pFence.GetAddressOf()));
        if (FAILED(hr))
        { return false; }

        m_FenceCounter[m_FrameIndex]++;

        // イベントの生成.
        m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_FenceEvent == nullptr)
        { return false; }
    }

    // コマンドリストを閉じる.
    m_pCmdList->Close();

    return true;
}

//-----------------------------------------------------------------------------
//      Direct3Dの終了処理です.
//-----------------------------------------------------------------------------
void App::TermD3D()
{
    // GPU処理の完了を待機.
    WaitGpu();

    // イベント破棄.
    if (m_FenceEvent != nullptr)
    {
        CloseHandle(m_FenceEvent);
        m_FenceEvent = nullptr;
    }

    // フェンス破棄.
    m_pFence.Reset();

    // レンダーターゲットビューの破棄.
    m_pHeapRTV.Reset();
    for(auto i=0u; i<FrameCount; ++i)
    { m_pColorBuffer[i].Reset(); }

    // コマンドリストの破棄.
    m_pCmdList.Reset();

    // コマンドアロケータの破棄.
    for(auto i=0u; i<FrameCount; ++i)
    { m_pCmdAllocator[i].Reset(); }

    // スワップチェインの破棄.
    m_pSwapChain.Reset();

    // コマンドキューの破棄.
    m_pQueue.Reset();

    // デバイスの破棄.
    m_pDevice.Reset();
}

//-----------------------------------------------------------------------------
//      メインループです.
//-----------------------------------------------------------------------------
void App::MainLoop()
{
    MSG msg = {};

    while(WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == TRUE)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }
}

//-----------------------------------------------------------------------------
//      描画処理です.
//-----------------------------------------------------------------------------
void App::Render()
{
    // コマンドの記録を開始.
    m_pCmdAllocator[m_FrameIndex]->Reset();
    m_pCmdList->Reset(m_pCmdAllocator[m_FrameIndex].Get(), nullptr);

    // リソースバリアの設定.
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = m_pColorBuffer[m_FrameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    // リソースバリア.
    m_pCmdList->ResourceBarrier(1, &barrier);

    // レンダーゲットの設定.
    m_pCmdList->OMSetRenderTargets(1, &m_HandleRTV[m_FrameIndex], FALSE, nullptr);

    // クリアカラーの設定.
    float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };

    // レンダーターゲットビューをクリア.
    m_pCmdList->ClearRenderTargetView(m_HandleRTV[m_FrameIndex], clearColor, 0, nullptr);

    // 描画処理.
    {
       // TODO : ポリゴン描画用の処理を追加
    }

    // リソースバリアの設定.
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = m_pColorBuffer[m_FrameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    // リソースバリア.
    m_pCmdList->ResourceBarrier(1, &barrier);

    // コマンドの記録を終了.
    m_pCmdList->Close();

    // コマンドを実行.
    ID3D12CommandList* ppCmdLists[] = { m_pCmdList.Get() };
    m_pQueue->ExecuteCommandLists(1, ppCmdLists);

    // 画面に表示.
    Present(1);
}

//-----------------------------------------------------------------------------
//      画面に表示し，次のフレームの準備を行います.
//-----------------------------------------------------------------------------
void App::Present(uint32_t interval)
{
    // 画面に表示.
    m_pSwapChain->Present(interval, 0);

    // シグナル処理.
    const auto currentValue = m_FenceCounter[m_FrameIndex];
    m_pQueue->Signal(m_pFence.Get(), currentValue);

    // バックバッファ番号を更新.
    m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    // 次のフレームの描画準備がまだであれば待機する
    if (m_pFence->GetCompletedValue() < m_FenceCounter[m_FrameIndex])
    {
        m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);
        WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
    }

    // 次のフレームのフェンスカウンターを増やす.
    m_FenceCounter[m_FrameIndex] = currentValue + 1;
}

//-----------------------------------------------------------------------------
//      GPUの処理完了を待機します.
//-----------------------------------------------------------------------------
void App::WaitGpu()
{
    assert(m_pQueue     != nullptr);
    assert(m_pFence     != nullptr);
    assert(m_FenceEvent != nullptr);

    // シグナル処理.
    m_pQueue->Signal(m_pFence.Get(), m_FenceCounter[m_FrameIndex]);

    // 完了時にイベントを設定する..
    m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);

    // 待機処理.
    WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);

    // カウンターを増やす.
    m_FenceCounter[m_FrameIndex]++;
}

//-----------------------------------------------------------------------------
//      ウィンドウプロシージャです.
//-----------------------------------------------------------------------------
LRESULT CALLBACK App::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_DESTROY:
            { PostQuitMessage(0); }
            break;

        default:
            { /* DO_NOTHING */ }
            break;
    }

    return DefWindowProc(hWnd, msg, wp, lp);
}
