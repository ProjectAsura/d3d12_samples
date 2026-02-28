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
#include <d3dcompiler.h>
#include <ComPtr.h>
#include <DescriptorPool.h>
#include <ColorTarget.h>
#include <DepthTarget.h>
#include <CommandList.h>
#include <Fence.h>
#include <Mesh.h>
#include <Texture.h>
#include <InlineUtil.h>


//-----------------------------------------------------------------------------
// Linker
//-----------------------------------------------------------------------------
#pragma comment( lib, "d3d12.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "d3dcompiler.lib" )


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

    //-------------------------------------------------------------------------
    //! @brief      コンストラクタです.
    //-------------------------------------------------------------------------
    App(uint32_t width, uint32_t height);

    //-------------------------------------------------------------------------
    //! @brief      デストラクタです.
    //-------------------------------------------------------------------------
    virtual ~App();

    //-------------------------------------------------------------------------
    //! @brief      アプリケーションを実行します.
    //-------------------------------------------------------------------------
    void Run();

protected:
    ///////////////////////////////////////////////////////////////////////////
    // POOL_TYPE enum
    ///////////////////////////////////////////////////////////////////////////
    enum POOL_TYPE
    {
        POOL_TYPE_RES  = 0,     // CBV / SRV / UAV
        POOL_TYPE_SMP  = 1,     // Sampler
        POOL_TYPE_RTV  = 2,     // RTV
        POOL_TYPE_DSV  = 3,     // DSV
        POOL_COUNT     = 4,
    };

    //=========================================================================
    // private variables.
    //=========================================================================
    static const uint32_t FrameCount = 2;   // フレームバッファ数です.

    HINSTANCE   m_hInst;        // インスタンスハンドルです.
    HWND        m_hWnd;         // ウィンドウハンドルです.
    uint32_t    m_Width;        // ウィンドウの横幅です.
    uint32_t    m_Height;       // ウィンドウの縦幅です.

    ComPtr<ID3D12Device>        m_pDevice;                   // デバイスです.
    ComPtr<ID3D12CommandQueue>  m_pQueue;                    // コマンドキューです.
    ComPtr<IDXGISwapChain3>     m_pSwapChain;                // スワップチェインです.
    ColorTarget                 m_ColorTarget[FrameCount];   // カラーターゲットです.
    DepthTarget                 m_DepthTarget;               // 深度ターゲットです.
    DescriptorPool*             m_pPool[POOL_COUNT];         // ディスクリプタプールです.
    CommandList                 m_CommandList;               // コマンドリストです.
    Fence                       m_Fence;                     // フェンスです.
    uint32_t                    m_FrameIndex;                // フレーム番号です.
    D3D12_VIEWPORT              m_Viewport;                  // ビューポートです.
    D3D12_RECT                  m_Scissor;                   // シザー矩形です.

    //=========================================================================
    // protected methods.
    //=========================================================================
    void Present(uint32_t interval);

    virtual bool OnInit()
    { return true; }

    virtual void OnTerm()
    { /* DO_NOTHING */ }

    virtual void OnRender()
    { /* DO_NOTHING */ }

    virtual void OnMsgProc(HWND, UINT, WPARAM, LPARAM)
    { /* DO_NOTHING */ }

private:
    //=========================================================================
    // private methods.
    //=========================================================================
    bool InitApp();
    void TermApp();
    bool InitWnd();
    void TermWnd();
    bool InitD3D();
    void TermD3D();
    void MainLoop();

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
};
