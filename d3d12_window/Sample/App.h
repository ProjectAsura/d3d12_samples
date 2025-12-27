//-----------------------------------------------------------------------------
// File : App.h
// Desc : Application Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h> // For ComPtr

//-----------------------------------------------------------------------------
// Linker Settings
//-----------------------------------------------------------------------------
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------
template<typename T>
using RefPtr = Microsoft::WRL::ComPtr<T>;


///////////////////////////////////////////////////////////////////////////////
// App class
///////////////////////////////////////////////////////////////////////////////
class App
{
    //========================================================================
    // list of friend classes and methods.
    //========================================================================
    /* NOTHING */

public:
    //========================================================================
    // public variables.
    //========================================================================
    /* NOTHING */

    //========================================================================
    // public methods.
    //========================================================================
    App();
    ~App();
    int Run();

protected:
    //========================================================================
    // protected variables.
    //========================================================================
    HINSTANCE       m_hInstance         = nullptr;
    HWND            m_hWnd              = nullptr;
    DXGI_FORMAT     m_SwapChainFormat   = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_VIEWPORT  m_Viewport          = {};
    D3D12_RECT      m_ScissorRect       = {};
    uint32_t        m_Width             = 1920;
    uint32_t        m_Height            = 1080;

    RefPtr<ID3D12Device>                m_Device;
    RefPtr<IDXGISwapChain4>             m_SwapChain;
    RefPtr<ID3D12CommandQueue>          m_GraphicsQueue;
    RefPtr<ID3D12CommandAllocator>      m_CommandAllocator[2];
    RefPtr<ID3D12GraphicsCommandList>   m_CommandList;
    RefPtr<ID3D12DescriptorHeap>        m_HeapRTV;
    RefPtr<ID3D12Resource>              m_RenderTargets[2];
    RefPtr<ID3D12Fence>                 m_Fence;
    D3D12_CPU_DESCRIPTOR_HANDLE         m_HandleRTV[2]  = {};
    HANDLE                              m_FenceEvent    = nullptr;
    bool                                m_IsStandbyMode = false;
    UINT64                              m_FenceValue    = 0;

    //========================================================================
    // protected methods.
    //========================================================================
    virtual bool OnInit();
    virtual void OnTerm();
    virtual void OnFrameMove();
    virtual void OnFrameRender();
    virtual void OnResize(uint32_t w, uint32_t h);
    void Present(uint32_t syncInterval);
    void WaitForGPU();

private:
    //========================================================================
    // private variables.
    //========================================================================
    /* NOTHING */

    //========================================================================
    // private methods.
    //========================================================================
    bool Init();
    void Term();
    bool InitWnd();
    void TermWnd();
    bool InitD3D();
    void TermD3D();
    void Resize(uint32_t w, uint32_t h);
    int  MainLoop();

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};