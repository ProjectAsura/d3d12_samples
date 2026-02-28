//-----------------------------------------------------------------------------
// File : asdxCommandList.cpp
// Desc : Command List Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cassert>
#include <vector>
#include <gfx/asdxCommandList.h>
#include <gfx/asdxDevice.h>
#include <fnd/asdxLogger.h>
#include <fnd/asdxMisc.h>


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// GraphicsCommandList class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
CommandList::CommandList()
: m_Allocator()
, m_CmdList  ()
, m_Index    (0)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
CommandList::~CommandList()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool CommandList::Init(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
{
    // 引数チェック.
    if (pDevice == nullptr)
    {
        ELOG( "Error : Invalid Argument." );
        return false;
    }

    for(auto i=0; i<2; ++i)
    {
        // コマンドアロケータを生成.
        auto hr = pDevice->CreateCommandAllocator( type, IID_PPV_ARGS( m_Allocator[i].GetAddress() ) );
        if ( FAILED( hr ) )
        {
            ELOG( "Error : ID3D12Device::CreateCommandAllocator() Failed. errcode = 0x%x", hr );
            return false;
        }
    }

    // コマンドリストを生成.
    auto hr = pDevice->CreateCommandList(
        0,
        type,
        m_Allocator[0].GetPtr(),
        nullptr,
        IID_PPV_ARGS( m_CmdList.GetAddress() ) );
    if ( FAILED( hr ) )
    {
        ELOG( "Error : ID3D12Device::CreateCommandList() Failed. errcode = 0x%x", hr );
        return false;
    }

    // 生成直後は開きっぱなしの扱いになっているので閉じておく.
    m_CmdList->Close();

    m_Index = 0;

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void CommandList::Term()
{
    m_CmdList.Reset();

    for(auto i=0; i<2; ++i)
    { m_Allocator[i].Reset(); }
}

//-----------------------------------------------------------------------------
//      コマンドリストをリセットします.
//-----------------------------------------------------------------------------
ID3D12GraphicsCommandList6* CommandList::Reset()
{
    // ダブルバッファリング.
    m_Index = (m_Index + 1) & 0x1;

    // コマンドアロケータをリセット.
    m_Allocator[m_Index]->Reset();

    // コマンドリストをリセット.
    m_CmdList->Reset( m_Allocator[m_Index].GetPtr(), nullptr );

    // ディスクリプターヒープを設定しおく.
    SetDescriptorHeaps(m_CmdList.GetPtr());

    return m_CmdList.GetPtr();
}

//-----------------------------------------------------------------------------
//      コマンドリストアロケータを取得します.
//-----------------------------------------------------------------------------
ID3D12CommandAllocator* CommandList::GetAllocator(uint8_t index) const
{
    assert(index < 2);
    return m_Allocator[index].GetPtr();
}

//-----------------------------------------------------------------------------
//      グラフィックスコマンドリストを取得します.
//-----------------------------------------------------------------------------
ID3D12GraphicsCommandList6* CommandList::GetCommandList() const
{ return m_CmdList.GetPtr(); }

//-----------------------------------------------------------------------------
//      現在のバッファ番号を返却します.
//-----------------------------------------------------------------------------
uint8_t CommandList::GetIndex() const
{ return m_Index; }

//-----------------------------------------------------------------------------
//      デバッグ名を設定します.
//-----------------------------------------------------------------------------
void CommandList::SetName(LPCWSTR name)
{ m_CmdList->SetName(name); }


///////////////////////////////////////////////////////////////////////////////
// ScopedMarker class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
ScopedMarker::ScopedMarker(ID3D12GraphicsCommandList* pCmd, const char* text)
: m_pCmd(pCmd)
{
    assert(text != nullptr);
    static const UINT PIX_EVENT_ANSI_VERSION = 1;
    auto size = UINT((strlen(text) + 1) * sizeof(char));
    m_pCmd->BeginEvent(PIX_EVENT_ANSI_VERSION, text, size);
}

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
ScopedMarker::~ScopedMarker()
{ m_pCmd->EndEvent(); }


} // namespace asdx
