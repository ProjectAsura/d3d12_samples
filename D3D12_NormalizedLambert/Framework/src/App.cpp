//-----------------------------------------------------------------------------
// File : App.cpp
// Desc : Application Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "App.h"


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
: m_hInst           (nullptr)
, m_hWnd            (nullptr)
, m_Width           (width)
, m_Height          (height)
, m_FrameIndex      (0)
{ /* DO_NOTHING */ }

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

    // アプリケーション固有の初期化.
    if (!OnInit())
    { return false; }

    // ウィンドウを表示.
    ShowWindow( m_hWnd, SW_SHOWNORMAL );

    // ウィンドウを更新.
    UpdateWindow( m_hWnd );

    // ウィンドウにフォーカスを設定.
    SetFocus( m_hWnd );

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      終了処理です.
//-----------------------------------------------------------------------------
void App::TermApp()
{
    // アプリケーション固有の終了処理.
    OnTerm();

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
        this);

    if (m_hWnd == nullptr)
    { return false; }

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      ウィンドウの終了処理です.
//-----------------------------------------------------------------------------
void App::TermWnd()
{
    // ウィンドウの登録を解除.
    if ( m_hInst != nullptr )
    { UnregisterClass( ClassName, m_hInst ); }

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
        ComPtr<ID3D12Debug> pDebug;
        auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(pDebug.GetAddressOf()));
        if (SUCCEEDED(hr))
        { pDebug->EnableDebugLayer(); }
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
        ComPtr<IDXGIFactory4> pFactory;
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
        pFactory.Reset();
        pSwapChain.Reset();
    }

    // ディスクリプタプールの生成.
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};

        desc.NodeMask       = 1;
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 512;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_RES]))
        { return false; }

        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        desc.NumDescriptors = 256;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_SMP]))
        { return false; }

        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = 512;
        desc.Flags          =  D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_RTV]))
        { return false; }

        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        desc.NumDescriptors = 512;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (!DescriptorPool::Create(m_pDevice.Get(), &desc, &m_pPool[POOL_TYPE_DSV]))
        { return false; }
    }

    // コマンドリストの生成.
    {
        if (!m_CommandList.Init(
            m_pDevice.Get(),
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            FrameCount))
        { return false; }
    }

    // レンダーターゲットビューの生成.
    {
        for (auto i=0u; i<FrameCount; ++i)
        {
            if (!m_ColorTarget[i].InitFromBackBuffer(
                m_pDevice.Get(),
                m_pPool[POOL_TYPE_RTV],
                true,
                i,
                m_pSwapChain.Get()))
            { return false; }
        }
    }

    // 深度ステンシルバッファの生成
    {
        if (!m_DepthTarget.Init(
            m_pDevice.Get(),
            m_pPool[POOL_TYPE_DSV],
            m_Width,
            m_Height,
            DXGI_FORMAT_D32_FLOAT))
        { return false; }
    }

    // フェンスの生成.
    if (!m_Fence.Init(m_pDevice.Get()))
    { return false; }

    // ビューポートの設定.
    {
        m_Viewport.TopLeftX = 0.0f;
        m_Viewport.TopLeftY = 0.0f;
        m_Viewport.Width    = float(m_Width);
        m_Viewport.Height   = float(m_Height);
        m_Viewport.MinDepth = 0.0f;
        m_Viewport.MaxDepth = 1.0f;
    }

    // シザー矩形の設定.
    {
        m_Scissor.left   = 0;
        m_Scissor.right  = m_Width;
        m_Scissor.top    = 0;
        m_Scissor.bottom = m_Height;
    }

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      Direct3Dの終了処理です.
//-----------------------------------------------------------------------------
void App::TermD3D()
{
    // GPU処理の完了を待機.
    m_Fence.Sync(m_pQueue.Get());

    // フェンス破棄.
    m_Fence.Term();

    // レンダーターゲットビューの破棄.
    for(auto i=0u; i<FrameCount; ++i)
    { m_ColorTarget[i].Term(); }

    // 深度ステンシルビューの破棄.
    m_DepthTarget.Term();

    // コマンドリストの破棄.
    m_CommandList.Term();

    for(auto i=0; i<POOL_COUNT; ++i)
    {
        if (m_pPool[i] != nullptr)
        {
            m_pPool[i]->Release();
            m_pPool[i] = nullptr;
        }
    }

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
        if ( PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == TRUE)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            OnRender();
        }
    }
}

//-----------------------------------------------------------------------------
//      画面に表示し，次のフレームの準備を行います.
//-----------------------------------------------------------------------------
void App::Present(uint32_t interval)
{
    // 画面に表示.
    m_pSwapChain->Present(interval, 0);

    // 完了待ち.
    m_Fence.Wait( m_pQueue.Get(), INFINITE );

    // フレーム番号を更新.
    m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}

//-----------------------------------------------------------------------------
//      ウィンドウプロシージャです.
//-----------------------------------------------------------------------------
LRESULT CALLBACK App::WndProc( HWND hWnd, UINT msg, WPARAM wp, LPARAM lp )
{
    auto instance = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch(msg)
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
            { /* DO_NOTHING */ }
            break;
    }

    if (instance != nullptr)
    { instance->OnMsgProc(hWnd, msg, wp, lp); }

    return DefWindowProc(hWnd, msg, wp, lp);
}
