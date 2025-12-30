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

#include "../res/Compiled/SimpleMS.inc"
#include "../res/Compiled/SimplePS.inc"


namespace /* anonymous */ {

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
const auto ClassName = TEXT("SampleWindowClass");    //!< ウィンドウクラス名です.


///////////////////////////////////////////////////////////////////////////////
// Vertex structure
///////////////////////////////////////////////////////////////////////////////
struct Vertex
{
    DirectX::XMFLOAT3   Position;    // 位置座標です.
    DirectX::XMFLOAT4   Color;       // 頂点カラーです.
};

///////////////////////////////////////////////////////////////////////////////
// StateParam structure
///////////////////////////////////////////////////////////////////////////////
template<typename ValueType, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE ObjectType>
class alignas(void*) StateParam
{
public:
    StateParam()
    : Type(ObjectType)
    , Value(ValueType())
    { /* DO_NOTHING */ }

    StateParam(const ValueType& value)
    : Value(value)
    , Type(ObjectType)
    { /* DO_NOTHING */ }

    StateParam& operator = (const ValueType& value)
    {
        Type  = ObjectType;
        Value = value;
        return *this;
    }

private:
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE     Type;
    ValueType                               Value;
};

// 長いから省略形を作る.
#define PSST(x) D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_##x

using SP_ROOT_SIGNATURE = StateParam<ID3D12RootSignature*,          PSST(ROOT_SIGNATURE)>;
using SP_AS             = StateParam<D3D12_SHADER_BYTECODE,         PSST(AS)>;
using SP_MS             = StateParam<D3D12_SHADER_BYTECODE,         PSST(MS)>;
using SP_PS             = StateParam<D3D12_SHADER_BYTECODE,         PSST(PS)>;
using SP_BLEND          = StateParam<D3D12_BLEND_DESC,              PSST(BLEND)>;
using SP_RASTERIZER     = StateParam<D3D12_RASTERIZER_DESC,         PSST(RASTERIZER)>;
using SP_DEPTH_STENCIL  = StateParam<D3D12_DEPTH_STENCIL_DESC,      PSST(DEPTH_STENCIL)>;
using SP_SAMPLE_MASK    = StateParam<UINT,                          PSST(SAMPLE_MASK)>;
using SP_SAMPLE_DESC    = StateParam<DXGI_SAMPLE_DESC,              PSST(SAMPLE_DESC)>;
using SP_RT_FORMAT      = StateParam<D3D12_RT_FORMAT_ARRAY,         PSST(RENDER_TARGET_FORMATS)>;
using SP_DS_FORMAT      = StateParam<DXGI_FORMAT,                   PSST(DEPTH_STENCIL_FORMAT)>;
using SP_FLAGS          = StateParam<D3D12_PIPELINE_STATE_FLAGS,    PSST(FLAGS)>;

// 宣言し終わったら要らないので無効化.
#undef PSST

///////////////////////////////////////////////////////////////////////////////
// MeshShaderPipelineStateDesc structure
///////////////////////////////////////////////////////////////////////////////
struct MeshShaderPipelineStateDesc
{
    SP_ROOT_SIGNATURE  RootSignature;           // ルートシグニチャ.
    SP_AS              AS;                      // 増幅シェーダ.
    SP_MS              MS;                      // メッシュシェーダ.
    SP_PS              PS;                      // ピクセルシェーダ.
    SP_BLEND           Blend;                   // ブレンドステート.
    SP_RASTERIZER      Rasterizer;              // ラスタライザーステート.
    SP_DEPTH_STENCIL   DepthStencil;            // 深度ステンシルステート.
    SP_SAMPLE_MASK     SampleMask;              // サンプルマスク.
    SP_SAMPLE_DESC     SampleDesc;              // サンプル設定.
    SP_RT_FORMAT       RTFormats;               // レンダーゲットフォーマット.
    SP_DS_FORMAT       DSFormat;                // 深度ステンシルフォーマット.
    SP_FLAGS           Flags;                   // フラグです.
};

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

    if (!OnInit())
    { return false; }

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      終了処理です.
//-----------------------------------------------------------------------------
void App::TermApp()
{
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

    ComPtr<IDXGIFactory6> pFactory;
    {
        uint32_t flags = 0;
    #if defined(DEBUG) || defined(_DEBUG)
        flags |= DXGI_CREATE_FACTORY_DEBUG;
    #endif

        ComPtr<IDXGIFactory2> factory;

        auto hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(factory.GetAddressOf()));
        if (FAILED(hr))
        {
            OutputDebugStringA("Error : CreateDXGIFactory2() Failed.");
            return false;
        }

        hr = factory.As(&pFactory);
        if (FAILED(hr))
        {
            OutputDebugStringA("Error : ComPtr::As() Failed.");
            return false;
        }
    }

    // DXGIアダプター生成.
    ComPtr<IDXGIAdapter1> pAdapter;
    {
        // 高パフォーマンスのものを選択.
        for(auto adapterId=0;
            DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapterByGpuPreference(adapterId, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(pAdapter.ReleaseAndGetAddressOf()));
            adapterId++)
        {
            DXGI_ADAPTER_DESC1 desc;
            auto hr = pAdapter->GetDesc1(&desc);
            if (FAILED(hr))
            { continue; }

            // ソフトウェアは回避する.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            { continue; }

            // 最初に見つかったものをD3D12デバイス生成として利用する.
            hr = D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr);
            if (SUCCEEDED(hr))
            { break; }
        }
    }


    // デバイスの生成.
    auto hr = D3D12CreateDevice(
        pAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(m_pDevice.GetAddressOf()));
    if (FAILED(hr))
    { return false; }

    // シェーダモデルをチェック.
    {
        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_5 };
        auto hr = m_pDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
        if (FAILED(hr) || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_5))
        {
            OutputDebugStringA("Error : Shader Model 6.5 is not supported.");
            return false;
        }
    }

    // メッシュシェーダをサポートしているかどうかチェック.
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
        auto hr = m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features));
        if (FAILED(hr) || (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED))
        {
            OutputDebugStringA("Error : Mesh Shaders aren't supported.");
            return false;
        }
    }

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
//      初期化時の処理です.
//-----------------------------------------------------------------------------
bool App::OnInit()
{
    // 定数バッファ用ディスクリプタヒープの生成.
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 2 + 1 * FrameCount;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask       = 0;

        auto hr = m_pDevice->CreateDescriptorHeap(
            &desc,
            IID_PPV_ARGS(m_pHeapRes.GetAddressOf()));
        if ( FAILED(hr) )
        { return false; }
    }

    auto incrementSize = m_pDevice
        ->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    auto handleCPU = m_pHeapRes->GetCPUDescriptorHandleForHeapStart();
    auto handleGPU = m_pHeapRes->GetGPUDescriptorHandleForHeapStart();

    // 頂点バッファの生成.
    {
        // 頂点データ.
        Vertex vertices[] = {
            { DirectX::XMFLOAT3( -1.0f, -1.0f, 0.0f ), DirectX::XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
            { DirectX::XMFLOAT3(  1.0f, -1.0f, 0.0f ), DirectX::XMFLOAT4( 0.0f, 1.0f, 0.0f, 1.0f ) },
            { DirectX::XMFLOAT3(  0.0f,  1.0f, 0.0f ), DirectX::XMFLOAT4( 1.0f, 0.0f, 0.0f, 1.0f ) },
        };

        // ヒーププロパティ.
        D3D12_HEAP_PROPERTIES prop = {};
        prop.Type                   = D3D12_HEAP_TYPE_UPLOAD;
        prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
        prop.CreationNodeMask       = 1;
        prop.VisibleNodeMask        = 1;

        // リソースの設定.
        D3D12_RESOURCE_DESC desc = {};
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
        auto hr = m_pDevice->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_pVB.GetAddressOf()));
        if ( FAILED(hr) )
        { return false; }

        // マッピングする.
        void* ptr = nullptr;
        hr = m_pVB->Map(0, nullptr, &ptr);
        if (FAILED(hr))
        { return false; }

        // 頂点データをマッピング先に設定.
        memcpy(ptr, vertices, sizeof(vertices));

        // マッピング解除.
        m_pVB->Unmap(0, nullptr);

        D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
        viewDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        viewDesc.Format                     = DXGI_FORMAT_UNKNOWN;
        viewDesc.Buffer.FirstElement        = 0;
        viewDesc.Buffer.NumElements         = 3;
        viewDesc.Buffer.StructureByteStride = sizeof(Vertex);
        viewDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_pDevice->CreateShaderResourceView(m_pVB.Get(), &viewDesc, handleCPU);
        
        handleCPU.ptr += incrementSize;
        handleGPU.ptr += incrementSize;
    }

    // インデックスバッファの生成.
    {
        uint32_t indices[] = { 0, 1, 2 };

        // ヒーププロパティ.
        D3D12_HEAP_PROPERTIES prop = {};
        prop.Type                   = D3D12_HEAP_TYPE_UPLOAD;
        prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
        prop.CreationNodeMask       = 1;
        prop.VisibleNodeMask        = 1;

        // リソースの設定.
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment          = 0;
        desc.Width              = sizeof(indices);
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        // リソースを生成.
        auto hr = m_pDevice->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_pIB.GetAddressOf()));
        if ( FAILED(hr) )
        { return false; }

        // マッピングする.
        void* ptr = nullptr;
        hr = m_pIB->Map(0, nullptr, &ptr);
        if (FAILED(hr))
        { return false; }

        // インデックスデータをマッピング先に設定.
        memcpy(ptr, indices, sizeof(indices));

        // マッピング解除.
        m_pIB->Unmap(0, nullptr);

        D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
        viewDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        viewDesc.Format                     = DXGI_FORMAT_UNKNOWN;
        viewDesc.Buffer.FirstElement        = 0;
        viewDesc.Buffer.NumElements         = 3;
        viewDesc.Buffer.StructureByteStride = sizeof(uint32_t);
        viewDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_pDevice->CreateShaderResourceView(m_pIB.Get(), &viewDesc, handleCPU);
        
        handleCPU.ptr += incrementSize;
        handleGPU.ptr += incrementSize;
    }

    // 定数バッファの生成.
    {
        // ヒーププロパティ.
        D3D12_HEAP_PROPERTIES prop = {};
        prop.Type                   = D3D12_HEAP_TYPE_UPLOAD;
        prop.CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        prop.MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN;
        prop.CreationNodeMask       = 1;
        prop.VisibleNodeMask        = 1;

        // リソースの設定.
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment          = 0;
        desc.Width              = sizeof(Transform);
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.Format             = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        for(auto i=0; i<FrameCount; ++i)
        {
            // リソース生成.
            auto hr = m_pDevice->CreateCommittedResource( 
                &prop,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(m_pCB[i].GetAddressOf()));
            if (FAILED(hr))
            { return false; }

            auto address   = m_pCB[i]->GetGPUVirtualAddress();

            // 定数バッファビューの設定.
            m_CBV[i].HandleCPU           = handleCPU;
            m_CBV[i].HandleGPU           = handleGPU;
            m_CBV[i].Desc.BufferLocation = address;
            m_CBV[i].Desc.SizeInBytes    = sizeof(Transform);

            // 定数バッファビューを生成.
            m_pDevice->CreateConstantBufferView(&m_CBV[i].Desc, handleCPU);

            handleCPU.ptr += incrementSize;
            handleGPU.ptr += incrementSize;

            // マッピング.
            hr = m_pCB[i]->Map(0, nullptr, reinterpret_cast<void**>(&m_CBV[i].pBuffer));
            if (FAILED(hr))
            { return false; }

            auto eyePos     = DirectX::XMVectorSet(0.0f, 0.0f, 5.0f, 0.0f);
            auto targetPos  = DirectX::XMVectorZero();
            auto upward     = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

            auto fovY   = DirectX::XMConvertToRadians(37.5f);
            auto aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

            // 変換行列の設定.
            m_CBV[i].pBuffer->World = DirectX::XMMatrixIdentity();
            m_CBV[i].pBuffer->View  = DirectX::XMMatrixLookAtRH(eyePos, targetPos, upward);
            m_CBV[i].pBuffer->Proj  = DirectX::XMMatrixPerspectiveFovRH(fovY, aspect, 1.0f, 1000.0f);
        }
    }

    // ルートシグニチャの生成.
    {
        auto flag = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
        flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        // ルートパラメータの設定.
        D3D12_ROOT_PARAMETER param[3] = {};
        param[0].ParameterType              = D3D12_ROOT_PARAMETER_TYPE_SRV;
        param[0].Descriptor.ShaderRegister  = 0;
        param[0].Descriptor.RegisterSpace   = 0;
        param[0].ShaderVisibility           = D3D12_SHADER_VISIBILITY_MESH;

        param[1].ParameterType              = D3D12_ROOT_PARAMETER_TYPE_SRV;
        param[1].Descriptor.ShaderRegister  = 1;
        param[1].Descriptor.RegisterSpace   = 0;
        param[1].ShaderVisibility           = D3D12_SHADER_VISIBILITY_MESH;

        param[2].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param[2].Descriptor.ShaderRegister = 0;
        param[2].Descriptor.RegisterSpace  = 0;
        param[2].ShaderVisibility          = D3D12_SHADER_VISIBILITY_MESH;

        // ルートシグニチャの設定.
        D3D12_ROOT_SIGNATURE_DESC desc = {};
        desc.NumParameters      = 3;
        desc.NumStaticSamplers  = 0;
        desc.pParameters        = param;
        desc.pStaticSamplers    = nullptr;
        desc.Flags              = flag;

        ComPtr<ID3DBlob> pBlob;
        ComPtr<ID3DBlob> pErrorBlob;

        // シリアライズ
        auto hr = D3D12SerializeRootSignature(
            &desc,
            D3D_ROOT_SIGNATURE_VERSION_1_0,
            pBlob.GetAddressOf(),
            pErrorBlob.GetAddressOf());
        if (FAILED(hr))
        { return false; }

        // ルートシグニチャを生成.
        hr = m_pDevice->CreateRootSignature(
            0,
            pBlob->GetBufferPointer(),
            pBlob->GetBufferSize(),
            IID_PPV_ARGS(m_pRootSignature.GetAddressOf()));
        if (FAILED(hr))
        { return false; }
    }

    // パイプラインステートの生成.
    {
        // ラスタライザーステートの設定.
        D3D12_RASTERIZER_DESC descRS = {};
        descRS.FillMode                 = D3D12_FILL_MODE_SOLID;
        descRS.CullMode                 = D3D12_CULL_MODE_NONE;
        descRS.FrontCounterClockwise    = FALSE;
        descRS.DepthBias                = D3D12_DEFAULT_DEPTH_BIAS;
        descRS.DepthBiasClamp           = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        descRS.SlopeScaledDepthBias     = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        descRS.DepthClipEnable          = FALSE;
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
        D3D12_BLEND_DESC descBS = {};
        descBS.AlphaToCoverageEnable  = FALSE;
        descBS.IndependentBlendEnable = FALSE;
        for( UINT i=0; i<D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i )
        { descBS.RenderTarget[i] = descRTBS; }

        D3D12_DEPTH_STENCILOP_DESC descStencil = {};
        descStencil.StencilFailOp       = D3D12_STENCIL_OP_KEEP;
        descStencil.StencilDepthFailOp  = D3D12_STENCIL_OP_KEEP;
        descStencil.StencilPassOp       = D3D12_STENCIL_OP_KEEP;
        descStencil.StencilFunc         = D3D12_COMPARISON_FUNC_ALWAYS;

        D3D12_DEPTH_STENCIL_DESC descDSS = {};
        descDSS.DepthEnable        = FALSE;
        descDSS.DepthWriteMask     = D3D12_DEPTH_WRITE_MASK_ALL;
        descDSS.DepthFunc          = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        descDSS.StencilEnable      = FALSE;
        descDSS.StencilReadMask    = D3D12_DEFAULT_STENCIL_READ_MASK;
        descDSS.StencilWriteMask   = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        descDSS.FrontFace          = descStencil;
        descDSS.BackFace           = descStencil;

        D3D12_SHADER_BYTECODE ms;
        ms.BytecodeLength   = sizeof(SimpleMS);
        ms.pShaderBytecode  = SimpleMS;

        D3D12_SHADER_BYTECODE ps;
        ps.BytecodeLength   = sizeof(SimplePS);
        ps.pShaderBytecode  = SimplePS;

        DXGI_SAMPLE_DESC descSample;
        descSample.Count    = 1;
        descSample.Quality  = 0;

        D3D12_RT_FORMAT_ARRAY rtFormat = {};
        rtFormat.NumRenderTargets   = 1;
        rtFormat.RTFormats[0]       = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        ID3D12RootSignature* pRootSig = m_pRootSignature.Get();

        MeshShaderPipelineStateDesc descState = {};
        descState.RootSignature     = pRootSig;
        descState.MS                = ms;
        descState.PS                = ps;
        descState.Rasterizer        = descRS;
        descState.Blend             = descBS;
        descState.DepthStencil      = descDSS;
        descState.SampleMask        = UINT_MAX;
        descState.SampleDesc        = descSample;
        descState.RTFormats         = rtFormat;
        descState.DSFormat          = DXGI_FORMAT_UNKNOWN;
        descState.Flags             = D3D12_PIPELINE_STATE_FLAG_NONE;

        D3D12_PIPELINE_STATE_STREAM_DESC descStream = {};
        descStream.SizeInBytes = sizeof(descState);
        descStream.pPipelineStateSubobjectStream = &descState;

        // パイプラインステートを生成.
        auto hr = m_pDevice->CreatePipelineState(
            &descStream,
            IID_PPV_ARGS(m_pPSO.GetAddressOf()));
        if ( FAILED( hr ) )
        { return false; }
    }

    // ビューポートとシザー矩形の設定.
    {
        m_Viewport.TopLeftX   = 0;
        m_Viewport.TopLeftY   = 0;
        m_Viewport.Width      = static_cast<float>(m_Width);
        m_Viewport.Height     = static_cast<float>(m_Height);
        m_Viewport.MinDepth   = 0.0f;
        m_Viewport.MaxDepth   = 1.0f;

        m_Scissor.left    = 0;
        m_Scissor.right   = m_Width;
        m_Scissor.top     = 0;
        m_Scissor.bottom  = m_Height;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      終了時の処理です.
//-----------------------------------------------------------------------------
void App::OnTerm()
{
    for(auto i=0; i<FrameCount; ++i)
    {
        if (m_pCB[i].Get() != nullptr)
        {
            m_pCB[i]->Unmap(0, nullptr);
            memset(&m_CBV[i], 0, sizeof(m_CBV[i]));
        }
        m_pCB[i].Reset();
    }

    m_pVB.Reset();
    m_pIB.Reset();
    m_pPSO.Reset();
    m_pHeapRes.Reset();
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
            Render();
        }
    }
}

//-----------------------------------------------------------------------------
//      描画処理です.
//-----------------------------------------------------------------------------
void App::Render()
{
    // 更新処理.
    {
        m_RotateAngle += 0.025f;
        m_CBV[m_FrameIndex].pBuffer->World = DirectX::XMMatrixRotationY(m_RotateAngle);
    }

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
        m_pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
        m_pCmdList->SetDescriptorHeaps(1, m_pHeapRes.GetAddressOf());
        m_pCmdList->SetGraphicsRootShaderResourceView(0, m_pVB->GetGPUVirtualAddress());
        m_pCmdList->SetGraphicsRootShaderResourceView(1, m_pIB->GetGPUVirtualAddress());
        m_pCmdList->SetGraphicsRootConstantBufferView(2, m_CBV[m_FrameIndex].Desc.BufferLocation);
        m_pCmdList->SetPipelineState(m_pPSO.Get());

        m_pCmdList->RSSetViewports(1, &m_Viewport);
        m_pCmdList->RSSetScissorRects(1, &m_Scissor);

        m_pCmdList->DispatchMesh(1, 1, 1);
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
    switch( msg )
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
