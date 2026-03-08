//-----------------------------------------------------------------------------
// File : asdxGamePad.cpp
// Desc : Game Pad Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cassert>
#include <cmath>
#include <Windows.h>
#include <Xinput.h>
#include <fnd/asdxHid.h>



namespace /* anonymous */ {

//-----------------------------------------------------------------------------
//! @brief      2つの値のうち，大きい方を返却します.
//!
//! @param [in]     a       判定する値.
//! @param [in]     b       判定する値.
//! @return     2つの値のうち，大きい方を返却します.
//-----------------------------------------------------------------------------
template<typename T> inline
T Max( T a, T b )
{ return ( a > b ) ? a : b; }


} // namespace /* anonymous */


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// GamePad class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
GamePad::GamePad( )
: m_PlayerIndex( 0 )
{ Reset(); }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
GamePad::~GamePad()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      ボタンの値をリセットします/
//-----------------------------------------------------------------------------
void GamePad::Reset()
{
    m_Buttons  = 0;
    m_TriggerL = 0;
    m_TriggerR = 0;

    m_ThumbLX = 0;
    m_ThumbLY = 0;
    m_ThumbRX = 0;
    m_ThumbRY = 0;

    m_NormThumbLX = 0.0f;
    m_NormThumbLY = 0.0f;
    m_NormThumbRX = 0.0f;
    m_NormThumbRY = 0.0f;

    m_PressedButtons = 0;
    m_LastButtons    = 0;
}

//-----------------------------------------------------------------------------
//      コントローラーが接続されているかチェックします.
//-----------------------------------------------------------------------------
bool GamePad::IsConnected() const
{ return m_IsConnected; }

//-----------------------------------------------------------------------------
//      コントローラーの状態を更新します.
//-----------------------------------------------------------------------------
void GamePad::UpdateState()
{
    XINPUT_STATE state;
    ZeroMemory( &state, sizeof(state) );

    const auto result = XInputGetState( m_PlayerIndex, &state );
    if ( result == ERROR_SUCCESS )
    {
        m_IsConnected = true;
    }
    else
    {
        m_IsConnected = false;
        Reset();
        return;
    }

    // XINPUT_STATEのデータを設定.
    m_Buttons  = state.Gamepad.wButtons;
    m_TriggerL = state.Gamepad.bLeftTrigger;
    m_TriggerR = state.Gamepad.bRightTrigger;
    m_ThumbLX  = state.Gamepad.sThumbLX;
    m_ThumbLY  = state.Gamepad.sThumbLY;
    m_ThumbRX  = state.Gamepad.sThumbRX;
    m_ThumbRY  = state.Gamepad.sThumbRY;


    if ( m_ThumbLX <  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
         m_ThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE )
    { m_ThumbLX = 0; }

    if ( m_ThumbLY <  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
         m_ThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE )
    { m_ThumbLY = 0; }

    if ( m_ThumbRX <  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
         m_ThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE )
    { m_ThumbRX = 0; }

    if ( m_ThumbRY <  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
         m_ThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE )
    { m_ThumbRY = 0; }

    // 正規化.
    m_NormThumbLX = Max<float>( -1.0f, static_cast<float>(m_ThumbLX) / 32767.0f );
    m_NormThumbLY = Max<float>( -1.0f, static_cast<float>(m_ThumbLY) / 32767.0f );
    m_NormThumbRX = Max<float>( -1.0f, static_cast<float>(m_ThumbRX) / 32767.0f );
    m_NormThumbRY = Max<float>( -1.0f, static_cast<float>(m_ThumbRY) / 32767.0f );

    if ( m_TriggerL > XINPUT_GAMEPAD_TRIGGER_THRESHOLD )
    { m_Buttons |= PAD_TRIGGER_L; }

    if ( m_TriggerR > XINPUT_GAMEPAD_TRIGGER_THRESHOLD )
    { m_Buttons |= PAD_TRIGGER_R; }

    m_PressedButtons = ( m_LastButtons ^ m_Buttons ) & m_Buttons;
    m_LastButtons    = m_Buttons;
}

//-----------------------------------------------------------------------------
//      バイブレーションさせます.
//-----------------------------------------------------------------------------
void GamePad::Vibrate( float leftMoter, float rightMoter ) const
{
    XINPUT_VIBRATION vibrate;
    ZeroMemory( &vibrate, sizeof(vibrate) );

    vibrate.wLeftMotorSpeed  = static_cast<WORD>(leftMoter * 65535.0f);
    vibrate.wRightMotorSpeed = static_cast<WORD>(rightMoter * 65535.0f);

    XInputSetState( m_PlayerIndex, &vibrate );
}

//-----------------------------------------------------------------------------
//      プレイヤーインデックスを設定します.
//-----------------------------------------------------------------------------
void GamePad::SetPlayerIndex( const uint32_t index )
{
    assert( index < 4 );
    m_PlayerIndex = index;
}

//-----------------------------------------------------------------------------
//      プレイヤーインデックスを取得します.
//-----------------------------------------------------------------------------
uint32_t GamePad::GetPlayerIndex() const
{ return m_PlayerIndex; }

//-----------------------------------------------------------------------------
//      ボタンが押されたかチェックします.
//-----------------------------------------------------------------------------
bool GamePad::IsDown( PAD_BUTTON type ) const
{ return ( m_PressedButtons & type ) > 0; }

//-----------------------------------------------------------------------------
//      ボタンが押されているかチェックします.
//-----------------------------------------------------------------------------
bool GamePad::IsHold( PAD_BUTTON type ) const
{ return ( m_Buttons & type ) > 0; }

//-----------------------------------------------------------------------------
//      左サムスティックのX成分を取得します.
//-----------------------------------------------------------------------------
int16_t GamePad::GetThumbLX() const
{ return m_ThumbLX; }

//-----------------------------------------------------------------------------
//      左サムスティックのY成分を取得します.
//-----------------------------------------------------------------------------
int16_t GamePad::GetThumbLY() const
{ return m_ThumbLY; }

//-----------------------------------------------------------------------------
//      右サムスティックのX成分を取得します.
//-----------------------------------------------------------------------------
int16_t GamePad::GetThumbRX() const
{ return m_ThumbRX; }

//-----------------------------------------------------------------------------
//      右サムスティックのY成分を取得します.
//-----------------------------------------------------------------------------
int16_t GamePad::GetThumbRY() const
{ return m_ThumbRY; }

//-----------------------------------------------------------------------------
//      正規化された左サムスティックのX成分を取得します.
//-----------------------------------------------------------------------------
float GamePad::GetNormalizedThumbLX() const
{ return m_NormThumbLX; }

//-----------------------------------------------------------------------------
//      正規化された左サムスティックのY成分を取得します.
//-----------------------------------------------------------------------------
float GamePad::GetNormalizedThumbLY() const
{ return m_NormThumbLY; }

//-----------------------------------------------------------------------------
//      正規化された右サムスティックのX成分を取得します.
//-----------------------------------------------------------------------------
float GamePad::GetNormalizedThumbRX() const
{ return m_NormThumbRX; }

//-----------------------------------------------------------------------------
//      正規化された右サムスティックのY成分を取得します.
//-----------------------------------------------------------------------------
float GamePad::GetNormalizedThumbRY() const
{ return m_NormThumbRY; }

} // namespace asdx
