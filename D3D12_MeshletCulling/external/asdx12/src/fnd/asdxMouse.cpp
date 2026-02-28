//-----------------------------------------------------------------------------
// File : asdxMouse.cpp
// Desc : Mouse Module.
// Copyright(c) Project Ausra. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cassert>
#include <cstring>
#include <Windows.h>
#include <fnd/asdxHid.h>


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// Mouse class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Mouse::Mouse()
: m_CursorX    ( -1 )
, m_CursorY    ( -1 )
, m_PrevCursorX( -1 )
, m_PrevCursorY( -1 )
, m_Index      ( 0 )
{ memset( m_Button, false, sizeof(bool) * NUM_MOUSE_BUTTON * 2 ); }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Mouse::~Mouse()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      マウスステートを更新します.
//-----------------------------------------------------------------------------
void Mouse::UpdateState(void* pHandle)
{
    m_Index       = 1 - m_Index;
    m_PrevCursorX = m_CursorX;
    m_PrevCursorY = m_CursorY;

    POINT pt;
    GetCursorPos( &pt );

    HWND hWnd = nullptr;
    if (pHandle != nullptr)
    {
        hWnd = (HWND)pHandle;
        ScreenToClient( hWnd, &pt );
    }
    //auto hWnd = FindWindowW( L"asdxWindowClass", nullptr );
    //if ( hWnd != nullptr )
    //{ ScreenToClient( hWnd, &pt ); }

    m_CursorX = int( pt.x );
    m_CursorY = int( pt.y );

    // フォーカスがある場合のみ，ボタンステートを更新.
    if ( GetActiveWindow() == hWnd )
    {
        m_Button[ m_Index ][ MOUSE_BUTTON_L ]  = ( GetAsyncKeyState( VK_LBUTTON )  & 0x8000 ) ? true : false;
        m_Button[ m_Index ][ MOUSE_BUTTON_R ]  = ( GetAsyncKeyState( VK_RBUTTON )  & 0x8000 ) ? true : false;
        m_Button[ m_Index ][ MOUSE_BUTTON_M ]  = ( GetAsyncKeyState( VK_MBUTTON )  & 0x8000 ) ? true : false;
        m_Button[ m_Index ][ MOUSE_BUTTON_X1 ] = ( GetAsyncKeyState( VK_XBUTTON1 ) & 0x8000 ) ? true : false;
        m_Button[ m_Index ][ MOUSE_BUTTON_X2 ] = ( GetAsyncKeyState( VK_XBUTTON2 ) & 0x8000 ) ? true : false;
    }
    else
    {
        m_Button[ m_Index ][ MOUSE_BUTTON_L ]  = false;
        m_Button[ m_Index ][ MOUSE_BUTTON_R ]  = false;
        m_Button[ m_Index ][ MOUSE_BUTTON_M ]  = false;
        m_Button[ m_Index ][ MOUSE_BUTTON_X1 ] = false;
        m_Button[ m_Index ][ MOUSE_BUTTON_X2 ] = false;
    }
}

//-----------------------------------------------------------------------------
//      マウスの状態をリセットします.
//-----------------------------------------------------------------------------
void Mouse::ResetState()
{
    m_CursorX     = -1;
    m_CursorY     = -1;
    m_PrevCursorX = -1;
    m_PrevCursorY = -1;
    memset( m_Button, false, sizeof(bool) * NUM_MOUSE_BUTTON * 2 );
}

//-----------------------------------------------------------------------------
//      マウスのX座標を取得します.
//-----------------------------------------------------------------------------
int Mouse::GetCursorX() const
{ return m_CursorX; }

//-----------------------------------------------------------------------------
//      マウスのY座標を取得します.
//-----------------------------------------------------------------------------
int Mouse::GetCursorY() const
{ return m_CursorY; }

//-----------------------------------------------------------------------------
//      前のマウスのX座標を取得します.
//-----------------------------------------------------------------------------
int Mouse::GetPrevCursorX() const
{ return m_PrevCursorX; }

//-----------------------------------------------------------------------------
//      前のマウスのY座標を取得します.
//-----------------------------------------------------------------------------
int Mouse::GetPrevCursorY() const
{ return m_PrevCursorY; }

//-----------------------------------------------------------------------------
//      現在のカーソルと前のカーソルのX座標の差分を取得します.
//-----------------------------------------------------------------------------
int Mouse::GetCursorDiffX() const
{ return m_CursorX - m_PrevCursorX; }

//-----------------------------------------------------------------------------
//      現在のカーソルと前のカーソルのY座標の差分を取得します.
//-----------------------------------------------------------------------------
int Mouse::GetCursorDiffY() const
{ return m_CursorY - m_PrevCursorY; }

//-----------------------------------------------------------------------------
//      キーが押されっぱなしかどうかチェックします.
//-----------------------------------------------------------------------------
bool Mouse::IsHold( const uint32_t button ) const
{
    assert( button < NUM_MOUSE_BUTTON );
    return m_Button[ m_Index ][ button ];
}

//-----------------------------------------------------------------------------
//      キーが押されたかどうかチェックします.
//-----------------------------------------------------------------------------
bool Mouse::IsDown( const uint32_t button ) const
{
    assert( button < NUM_MOUSE_BUTTON );
    return m_Button[ m_Index ][ button ] && ( !m_Button[ 1 - m_Index ][ button ] );
}

//-----------------------------------------------------------------------------
//      ドラッグ中かどうかチェックします.
//-----------------------------------------------------------------------------
bool Mouse::IsDrag( const uint32_t button ) const
{
    assert( button < NUM_MOUSE_BUTTON );
    return m_Button[ m_Index ][ button ] && m_Button[ 1 - m_Index ][ button ];
}

} // namespace asdx
