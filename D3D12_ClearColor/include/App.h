//-----------------------------------------------------------------------------
// File : App.h
// Desc : Application Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h> // ★追加

//-----------------------------------------------------------------------------
// Linker
//-----------------------------------------------------------------------------
#pragma comment( lib, "d3d12.lib" )
#pragma comment( lib, "dxgi.lib" )

//-----------------------------------------------------------------------------
// Type definitions.
//-----------------------------------------------------------------------------
template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;   // ★追加.


///////////////////////////////////////////////////////////////////////////////
// App class
///////////////////////////////////////////////////////////////////////////////
class App
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================
    App(uint32_t width, uint32_t height);
    ~App();
    void Run();

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    static const uint32_t FrameCount = 2;   // フレームバッファ数です.

    HINSTANCE   m_hInst;        // インスタンスハンドルです.
    HWND        m_hWnd;         // ウィンドウハンドルです.
    uint32_t    m_Width;        // ウィンドウの横幅です.
    uint32_t    m_Height;       // ウィンドウの縦幅です.

    ComPtr<ID3D12Device>                m_pDevice;                      // ★変更 : デバイスです.
    ComPtr<ID3D12CommandQueue>          m_pQueue;                       // ★変更 : コマンドキューです.
    ComPtr<IDXGISwapChain3>             m_pSwapChain;                   // ★変更 : スワップチェインです.
    ComPtr<ID3D12Resource>              m_pColorBuffer[FrameCount];     // ★変更 : カラーバッファです.
    ComPtr<ID3D12CommandAllocator>      m_pCmdAllocator[FrameCount];    // ★変更 : コマンドアロケータです.
    ComPtr<ID3D12GraphicsCommandList>   m_pCmdList;                     // ★変更 : コマンドリストです.
    ComPtr<ID3D12DescriptorHeap>        m_pHeapRTV;                     // ★変更 : ディスクリプタヒープです(レンダーターゲットビュー).
    ComPtr<ID3D12Fence>                 m_pFence;                       // ★変更 : フェンスです.
    HANDLE                              m_FenceEvent;                   // フェンスイベントです.
    uint64_t                            m_FenceCounter[FrameCount];     // フェンスカウンターです.
    uint32_t                            m_FrameIndex;                   // フレーム番号です.
    D3D12_CPU_DESCRIPTOR_HANDLE         m_HandleRTV[FrameCount];        // CPUディスクリプタ(レンダーターゲットビュー)です.

    //=========================================================================
    // private methods.
    //=========================================================================
    bool InitApp();
    void TermApp();
    bool InitWnd();
    void TermWnd();
    void MainLoop();
    bool InitD3D();
    void TermD3D();
    void Render();
    void WaitGpu();
    void Present(uint32_t interval);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
};