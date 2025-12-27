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
#include <string>
#include "asdxMath.h"
#include <shlwapi.h>
#include "asdxResTGA.h"
#include "asdxResBMP.h"
#include "asdxResDDS.h"
#include "TextureHelper.h"

//-----------------------------------------------------------------------------
// Linker Settings
//-----------------------------------------------------------------------------
#pragma comment(lib, "shlwapi.lib")


#ifndef WND_CLASSNAME
#define WND_CLASSNAME    L"AppClassWindow"
#endif//WND_CLASSNAME

#ifndef ELOG
#define ELOG(x, ...)    fprintf_s(stderr, "[File:%s, Line:%d]"x"\n", __FILE__, __LINE__, ##__VA_ARGS__ )
#endif//ELOG

#define USE_TGA (0)
#define USE_BMP (1)
#define USE_DDS (2)

#define USE_TYPE (USE_TGA)


namespace {

#include "Compiled/SimpleVS.inc"
#include "Compiled/SimplePS.inc"

///////////////////////////////////////////////////////////////////////////////
// SceneParam structure
///////////////////////////////////////////////////////////////////////////////
struct SceneParam
{
    asdx::Matrix World;     //!< ワールド行列.
    asdx::Matrix View;      //!< ビュー行列.
    asdx::Matrix Proj;      //!< 射影行列.
};

//-------------------------------------------------------------------------------------------------
//      ファイルパスを検索します.
//-------------------------------------------------------------------------------------------------
bool SearchFilePath(const wchar_t* filePath, std::wstring& result)
{
    if ( filePath == nullptr )
    { return false; }

    if ( wcscmp( filePath, L" " ) == 0 || wcscmp( filePath, L"" ) == 0 )
    { return false; }

    wchar_t exePath[ 520 ] = { 0 };
    GetModuleFileNameW( nullptr, exePath, 520  );
    exePath[ 519 ] = 0; // null終端化.
    PathRemoveFileSpecW( exePath );

    wchar_t dstPath[ 520 ] = { 0 };

    wcscpy_s( dstPath, filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"..\\%s", filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"..\\..\\%s", filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"\\res\\%s", filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"%s\\%s", exePath, filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"%s\\..\\%s", exePath, filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"%s\\..\\..\\%s", exePath, filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    swprintf_s( dstPath, L"%s\\res\\%s", exePath, filePath );
    if ( PathFileExistsW( dstPath ) == TRUE )
    {
        result = dstPath;
        return true;
    }

    return false;
}

} // namespace

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

    // RTVディスクリプタヒープの生成
    UINT incrementRTV = 0;
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = 256;
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

    // DSVディスクリプタヒープの生成
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        desc.NumDescriptors = 256;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask       = 0;
        hr = m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_HeapDSV.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateDescriptorHeap() Failed. hr = 0x%x", hr);
            return false;
        }
    }

    // 深度ステンシルバッファの生成.
    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment          = 0;
        desc.Width              = m_Width;
        desc.Height             = m_Height;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = m_DepthStencilFormat;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask      = 0;
        heapProps.VisibleNodeMask       = 0;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format               = m_DepthStencilFormat;
        clearValue.DepthStencil.Depth   = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        hr = m_Device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(m_DepthStencilTexture.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. hr = 0x%x", hr);
            return false;
        }
    }

    // 深度ステンシルビューの生成.
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
        desc.Format               = m_DepthStencilFormat;
        desc.ViewDimension        = D3D12_DSV_DIMENSION_TEXTURE2D;
        desc.Flags                = D3D12_DSV_FLAG_NONE;
        desc.Texture2D.MipSlice   = 0;

        m_HandleDSV = m_HeapDSV->GetCPUDescriptorHandleForHeapStart();

        m_Device->CreateDepthStencilView(
            m_DepthStencilTexture.Get(),
            &desc,
            m_HandleDSV);
    }

    // CBV SRV UAV ディスクリプタヒープの生成
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 8192;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask       = 0;
        hr = m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_HeapRes.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateDescriptorHeap() Failed. hr = 0x%x", hr);
            return false;
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
    m_HandleDSV = {};
    m_DepthStencilTexture.Reset();

    if (m_FenceEvent != nullptr)
    {
        CloseHandle(m_FenceEvent);
        m_FenceEvent = nullptr;
    }
    m_Fence         .Reset();

    m_HeapRTV       .Reset();
    m_HeapDSV       .Reset();
    m_HeapRes       .Reset();
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

        m_DepthStencilTexture.Reset();

        HRESULT hr = S_OK;

        // バッファをリサイズ.
        hr = m_SwapChain->ResizeBuffers(2, m_Width, m_Height, m_SwapChainFormat, 0 );
        if (FAILED(hr))
        { ELOG( "Error : IDXGISwapChain::ResizeBuffer() Failed. errcode = 0x%x", hr ); }

        {
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

        // 深度ステンシルバッファの生成.
        {
            D3D12_RESOURCE_DESC desc = {};
            desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Alignment          = 0;
            desc.Width              = m_Width;
            desc.Height             = m_Height;
            desc.DepthOrArraySize   = 1;
            desc.MipLevels          = 1;
            desc.Format             = m_DepthStencilFormat;
            desc.SampleDesc.Count   = 1;
            desc.SampleDesc.Quality = 0;
            desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;
            heapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
            heapProps.CreationNodeMask      = 0;
            heapProps.VisibleNodeMask       = 0;

            D3D12_CLEAR_VALUE clearValue = {};
            clearValue.Format               = m_DepthStencilFormat;
            clearValue.DepthStencil.Depth   = 1.0f;
            clearValue.DepthStencil.Stencil = 0;

            hr = m_Device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clearValue,
                IID_PPV_ARGS(m_DepthStencilTexture.GetAddressOf()));
            if (SUCCEEDED(hr))
            {
                D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
                desc.Format               = m_DepthStencilFormat;
                desc.ViewDimension        = D3D12_DSV_DIMENSION_TEXTURE2D;
                desc.Flags                = D3D12_DSV_FLAG_NONE;
                desc.Texture2D.MipSlice   = 0;

                m_Device->CreateDepthStencilView(
                    m_DepthStencilTexture.Get(),
                    &desc,
                    m_HandleDSV);
            }
        }

    }

    OnResize(w, h);
}

//-----------------------------------------------------------------------------
//      アプリケーション固有の初期化処理です.
//-----------------------------------------------------------------------------
bool App::OnInit()
{
    // 頂点バッファの生成.
    {
        struct Vertex
        {
            asdx::Vector3 Position;     //!< 位置座標です.
            asdx::Vector3 Normal;       //!< 法線ベクトルです.
            asdx::Vector2 TexCoord;     //!< テクスチャ座標です.
            asdx::Vector4 Color;        //!< 頂点カラーです.
        };

        // 頂点データ.
        Vertex vertices[] = {
            // 右下
            { {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },

            // 左上
            { {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        };

        // ヒーププロパティの設定.
        D3D12_HEAP_PROPERTIES prop;
        prop.Type                 = D3D12_HEAP_TYPE_UPLOAD;
        prop.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        prop.CreationNodeMask     = 1;
        prop.VisibleNodeMask      = 1;

        // リソースの設定.
        D3D12_RESOURCE_DESC desc;
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment          = 0;
        desc.Width              = sizeof(vertices);
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        // リソースを生成.
        auto hr = m_Device->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_VertexBuffer.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG( "Error : ID3D12Device::CreateCommittedResource() Failed." );
            return false;
        }

        // マップする.
        UINT8* pData;
        hr = m_VertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        if (FAILED(hr))
        {
            ELOG( "Error : ID3D12Resource::Map() Failed." );
            return false;
        }

        // 頂点データをコピー.
        memcpy(pData, vertices, sizeof(vertices));

        // アンマップする.
        m_VertexBuffer->Unmap(0, nullptr);

        // 頂点バッファビューの設定.
        m_VBV.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_VBV.StrideInBytes  = sizeof(Vertex);
        m_VBV.SizeInBytes    = sizeof(vertices);
    }

    // 定数バッファの生成.
    {
        auto size = asdx::RoundUp<uint32_t>(sizeof(SceneParam), 256);

        // ヒーププロパティの設定.
        D3D12_HEAP_PROPERTIES prop = {};
        prop.Type                 = D3D12_HEAP_TYPE_UPLOAD;
        prop.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        prop.CreationNodeMask     = 1;
        prop.VisibleNodeMask      = 1;

        // リソースの設定.
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment          = 0;
        desc.Width              = size;
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        // リソースを生成.
        auto hr = m_Device->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_ConstantBuffer.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. hr = 0x%x", hr);
            return false;
        }
    }

    {
        auto size = asdx::RoundUp<uint32_t>(sizeof(SceneParam), 256);

        // 定数バッファビューの設定.
        D3D12_CONSTANT_BUFFER_VIEW_DESC bufferDesc = {};
        bufferDesc.BufferLocation = m_ConstantBuffer->GetGPUVirtualAddress();
        bufferDesc.SizeInBytes    = size;

        m_HandleCBV = m_HeapRes->GetCPUDescriptorHandleForHeapStart();

        // 定数バッファビューを生成.
        m_Device->CreateConstantBufferView(&bufferDesc, m_HandleCBV);

        // マップする. アプリケーション終了まで Unmap しない.
        // "Keeping things mapped for the lifetime of the resource is okay." とのこと。
        uint8_t* ptr = nullptr;
        auto hr = m_ConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&ptr));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Resource::Map() Failed. hr = 0x%x", hr);
            return false;
        }

        // アスペクト比算出.
        auto aspectRatio = static_cast<FLOAT>( m_Width ) / static_cast<FLOAT>( m_Height );

        // 定数バッファデータの設定.
        SceneParam param = {};
        param.World = asdx::Matrix::CreateIdentity();
        param.View  = asdx::Matrix::CreateLookAt(asdx::Vector3(0.0f, 0.0f, 5.0f), asdx::Vector3(0.0f, 0.0f, 0.0f), asdx::Vector3(0.0f, 1.0f, 0.0f));
        param.Proj  = asdx::Matrix::CreatePerspectiveFieldOfView(asdx::F_PIDIV4, aspectRatio, 1.0f, 1000.0f);

        // コピる.
        memcpy(ptr, &param, sizeof(param));

        m_ConstantBuffer->Unmap(0, nullptr);
    }

    // ルートシグニチャ生成.
    {
        D3D12_DESCRIPTOR_RANGE ranges[1] = {};
        ranges[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[0].NumDescriptors                    = 1;
        ranges[0].BaseShaderRegister                = 0;
        ranges[0].RegisterSpace                     = 0;
        ranges[0].OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER params[2] = {};
        params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0;
        params[0].Descriptor.RegisterSpace  = 0;
        params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

        params[1].ParameterType                         = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[1].DescriptorTable.NumDescriptorRanges   = 1;
        params[1].DescriptorTable.pDescriptorRanges     = &ranges[0];
        params[1].ShaderVisibility                      = D3D12_SHADER_VISIBILITY_ALL;

        // 静的サンプラーの設定.
        D3D12_STATIC_SAMPLER_DESC samplers[1] = {};
        samplers[0].Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplers[0].AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[0].AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[0].AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[0].MipLODBias       = 0;
        samplers[0].MaxAnisotropy    = 0;
        samplers[0].ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
        samplers[0].BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        samplers[0].MinLOD           = 0.0f;
        samplers[0].MaxLOD           = D3D12_FLOAT32_MAX;
        samplers[0].ShaderRegister   = 0;
        samplers[0].RegisterSpace    = 0;
        samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters      = _countof(params);
        desc.pParameters        = params;
        desc.NumStaticSamplers  = _countof(samplers);
        desc.pStaticSamplers    = samplers;
        desc.Flags              = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        RefPtr<ID3DBlob> signature;
        RefPtr<ID3DBlob> error;
        auto hr = D3D12SerializeRootSignature(
            &desc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            signature.GetAddressOf(),
            error.GetAddressOf());
        if (FAILED(hr))
        {
            ELOG("Error : D3D12SerializeRootSignature() Failed. hr = 0x%x", hr);
            return false;
        }

        hr = m_Device->CreateRootSignature(
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(m_RootSignature.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG("Error : ID3D12Device::CreateRootSignature() Failed. hr = 0x%x", hr);
            return false;
        }
    }

    // パイプラインステート生成.
    {
        // 入力レイアウトの設定.
        D3D12_INPUT_ELEMENT_DESC inputElements[] = {
            { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "VTX_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // ラスタライザーステートの設定.
        D3D12_RASTERIZER_DESC descRS;
        descRS.FillMode                 = D3D12_FILL_MODE_SOLID;
        descRS.CullMode                 = D3D12_CULL_MODE_NONE;
        descRS.FrontCounterClockwise    = FALSE;
        descRS.DepthBias                = D3D12_DEFAULT_DEPTH_BIAS;
        descRS.DepthBiasClamp           = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        descRS.SlopeScaledDepthBias     = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        descRS.DepthClipEnable          = TRUE;
        descRS.MultisampleEnable        = FALSE;
        descRS.AntialiasedLineEnable    = FALSE;
        descRS.ForcedSampleCount        = 0;
        descRS.ConservativeRaster       = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // レンダーターゲットのブレンド設定.
        D3D12_RENDER_TARGET_BLEND_DESC descRTBS = {
            FALSE, FALSE,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL
        };

        // ブレンドステートの設定.
        D3D12_BLEND_DESC descBS;
        descBS.AlphaToCoverageEnable  = FALSE;
        descBS.IndependentBlendEnable = FALSE;
        for( UINT i=0; i<D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i )
        { descBS.RenderTarget[i] = descRTBS; }

        // パイプラインステートの設定.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout                     = { inputElements, _countof(inputElements) };
        desc.pRootSignature                  = m_RootSignature.Get();
        desc.VS                              = { SimpleVS, sizeof(SimpleVS) };
        desc.PS                              = { SimplePS, sizeof(SimplePS) };
        desc.RasterizerState                 = descRS;
        desc.BlendState                      = descBS;
        desc.DepthStencilState.DepthEnable   = FALSE;
        desc.DepthStencilState.StencilEnable = FALSE;
        desc.SampleMask                      = UINT_MAX;
        desc.PrimitiveTopologyType           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets                = 1;
        desc.RTVFormats[0]                   = m_SwapChainFormat;
        desc.DSVFormat                       = m_DepthStencilFormat;
        desc.SampleDesc.Count                = 1;

        // パイプラインステートを生成.
        auto hr = m_Device->CreateGraphicsPipelineState( &desc, IID_PPV_ARGS(m_PipelineState.GetAddressOf()) );
        if (FAILED(hr))
        {
            ELOG( "Error : ID3D12Device::CreateGraphicsPipelineState() Failed." );
            return false;
        }
    }

    // テクスチャ設定.
    {
    #if USE_TYPE == USE_TGA
        // TGAファイルを検索.
        std::wstring path;
        if (!SearchFilePath(L"res/texture/sample32bitRLE.tga", path))
        {
            ELOG("Error : File Not Found.");
            return false;
        }

        // TGAファイルをロード.
        asdx::ResTGA tga;
        if (!tga.Load(path.c_str()))
        {
            ELOG("Error : Targa File Load Failed.");
            return false;
        }
        uint32_t width  = tga.GetWidth();
        uint32_t height = tga.GetHeight();

    #elif USE_TYPE == USE_BMP
        // BMPファイルを検索.
        std::wstring path;
        if (!SearchFilePath(L"res/texture/sample24Bit.bmp", path))
        {
            ELOG("Error : File Not Found.");
            return false;
        }

        // BMPファイルをロード.
        asdx::ResBMP bmp;
        if (!bmp.Load(path.c_str()))
        {
            ELOG("Error : BMP File Load Failed.");
            return false;
        }
        uint32_t width  = bmp.GetWidth();
        uint32_t height = bmp.GetHeight();

    #elif USE_TYPE == USE_DDS
        // DDSファイルを検索.
        std::wstring path;
        if (!SearchFilePath(L"res/texture/sample_X8R8G8B8.dds", path))
        {
            ELOG("Error : File Not Found.");
            return false;
        }
        // DDSファイルをロード.
        asdx::ResDDS dds;
        if (!dds.Load(path.c_str()))
        {
            ELOG("Error : DDS File Load Failed.");
            return false;
        }
        uint32_t width  = dds.GetWidth();
        uint32_t height = dds.GetHeight();

    #endif

        bool enableGpuUpload = false;
        HRESULT hr = S_OK;

        // GPUアップロードヒープのサポートチェック.
        D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = {};
        hr = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16));
        if (SUCCEEDED(hr))
        { enableGpuUpload = options16.GPUUploadHeapSupported; }

        auto heapType = enableGpuUpload ? D3D12_HEAP_TYPE_GPU_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
        auto resState = enableGpuUpload ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;

        // ヒーププロパティの設定.
        D3D12_HEAP_PROPERTIES prop = {};
        prop.Type                 = heapType;
        prop.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        prop.CreationNodeMask     = 1;
        prop.VisibleNodeMask      = 1;

        // リソースの設定.
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width              = width;
        desc.Height             = height;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        // リソースを生成.
        hr = m_Device->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            resState,
            nullptr,
            IID_PPV_ARGS(m_Texture.GetAddressOf()));
        if (FAILED(hr))
        {
            ELOG( "Error : ID3D12Device::CreateCommittedResource() Failed." );
            return false;
        }

    #if USE_TYPE == USE_TGA
        auto pixels     = tga.GetPixels();
        UINT rowPitch   = tga.GetWidth() * tga.GetBitPerPixel() / 8;
        UINT depthPitch = rowPitch * tga.GetHeight();

    #elif USE_TYPE == USE_BMP
        auto pixels     = bmp.GetPixels();
        UINT rowPitch   = bmp.GetWidth() * 4;
        UINT depthPitch = rowPitch * bmp.GetHeight();

    #elif USE_TYPE == USE_DDS
        auto surfaces = dds.GetSurfaces();
        assert(surfaces != nullptr);

        auto pixels     = surfaces[0].pPixels;
        UINT rowPitch   = surfaces[0].Pitch;
        UINT depthPitch = surfaces[0].Pitch * surfaces[0].Height;
    #endif

        // GPUアップロードヒープがサポートされている場合は直接書き込み.
        if (enableGpuUpload)
        {
            hr = m_Texture->WriteToSubresource(
                0,
                nullptr,
                pixels,
                rowPitch,
                depthPitch);
            if (FAILED(hr))
            {
                ELOG("Error : ID3D12Resource::WriteToSubresource() Failed. hr = 0x%x", hr);
                return false;
            }
        }
        // GPUアップロードヒープがサポートされていない場合はアップロードバッファ経由で転送.
        else
        {
            m_CommandList->Reset(m_CommandAllocator[0].Get(), nullptr);

            // サブリソースデータ設定.
            D3D12_SUBRESOURCE_DATA subRes;
            subRes.pData      = pixels;
            subRes.RowPitch   = rowPitch;
            subRes.SlicePitch = depthPitch;

            // サブリソースの更新.
            auto ret = UpdateSubresources(
                m_CommandList.Get(),
                m_GraphicsQueue.Get(),
                m_Fence.Get(),
                m_FenceEvent,
                m_Texture.Get(),
                0,
                1,
                &subRes);
            if (!ret)
            {
                ELOG("Error : UdpateSubresources() Failed. hr = 0x%x", hr);
                return false;
            }
        }

        // シェーダリソースビューの設定.
        D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
        viewDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        viewDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
        viewDesc.Texture2D.MipLevels       = 1;
        viewDesc.Texture2D.MostDetailedMip = 0;

        // シェーダリソースビューを生成.
        D3D12_CPU_DESCRIPTOR_HANDLE handle = m_HeapRes->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_Device->CreateShaderResourceView(m_Texture.Get(), &viewDesc, handle);

        m_HandleSRV = m_HeapRes->GetGPUDescriptorHandleForHeapStart();
        m_HandleSRV.ptr += m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    return true;
}

//-----------------------------------------------------------------------------
//      アプリケーション固有の終了処理です.
//-----------------------------------------------------------------------------
void App::OnTerm()
{
    if (m_ConstantBuffer.Get() != nullptr)
    {
        m_ConstantBuffer->Unmap(0, nullptr);
    }

    m_ConstantBuffer.Reset();
    m_VertexBuffer.Reset();
    m_HandleCBV = {};

    m_PipelineState.Reset();
    m_RootSignature.Reset();
}

//-----------------------------------------------------------------------------
//      フレーム遷移処理です
//-----------------------------------------------------------------------------
void App::OnFrameMove()
{
    // 回転角を増やす.
    m_RotateAngle += 0.05f;

    SceneParam* param = nullptr;
    auto hr = m_ConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&param));
    if (SUCCEEDED(hr))
    {
        // ワールド行列を更新.
        param->World = asdx::Matrix::CreateRotationY(m_RotateAngle);
    }
    m_ConstantBuffer->Unmap(0, nullptr);
}

//-----------------------------------------------------------------------------
//      フレーム描画処理です
//-----------------------------------------------------------------------------
void App::OnFrameRender()
{
    auto idx = m_SwapChain->GetCurrentBackBufferIndex();

    m_CommandList->Reset(m_CommandAllocator[idx].Get(), nullptr);

    // ディスクリプタヒープ設定.
    m_CommandList->SetDescriptorHeaps(1, m_HeapRes.GetAddressOf());

    // ビューポートとシザー矩形の設定.
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

    // レンダーターゲットの設定.
    m_CommandList->OMSetRenderTargets(1, &m_HandleRTV[idx], FALSE, &m_HandleDSV);

    // カラーバッファをクリア.
    float clearColor[] = { 0.39f, 0.58f, 0.92f, 1.0f };
    m_CommandList->ClearRenderTargetView(m_HandleRTV[idx], clearColor, 0, nullptr );
    m_CommandList->ClearDepthStencilView(m_HandleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    {
        m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
        m_CommandList->SetPipelineState(m_PipelineState.Get());

        // 定数バッファ設定.
        m_CommandList->SetGraphicsRootConstantBufferView(0, m_ConstantBuffer->GetGPUVirtualAddress());

        // テクスチャ設定.
        m_CommandList->SetGraphicsRootDescriptorTable(1, m_HandleSRV);

        // プリミティブトポロジーの設定.
        m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 頂点バッファビューを設定.
        m_CommandList->IASetVertexBuffers(0, 1, &m_VBV);

        // 描画コマンドを生成.
        m_CommandList->DrawInstanced(6, 1, 0, 0);
    }

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