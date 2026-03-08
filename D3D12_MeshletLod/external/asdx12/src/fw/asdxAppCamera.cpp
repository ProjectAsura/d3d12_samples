//-----------------------------------------------------------------------------
// File : asdxAppCamera.cpp
// Desc : Camera Update Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <fw/asdxAppCamera.h>


namespace asdx {

///////////////////////////////////////////////////////////////////////////////
// AppCamera class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
AppCamera::AppCamera()
: m_Camera      ()
, m_MouseLeft   ()
, m_MouseRight  ()
, m_MouseMiddle ()
, m_Type        (CAMERA_TYPE_TARGET)
{ ResetGain(); }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
AppCamera::~AppCamera()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      初期設定を行います.
//-----------------------------------------------------------------------------
void AppCamera::Init
(
    asdx::Vector3 cameraPos,
    asdx::Vector3 cameraAim,
    asdx::Vector3 cameraUp,
    float        nearClip,
    float        farClip
)
{
    m_Camera.SetPosition( cameraPos );
    m_Camera.SetTarget( cameraAim );
    m_Camera.SetUpward( cameraUp );
    m_Camera.SetRange( nearClip, farClip );

    m_Camera.Preset();
    m_Camera.Update();
    m_MouseLeft.Reset();
    m_MouseRight.Reset();
    m_MouseMiddle.Reset();
}

//-----------------------------------------------------------------------------
//      マウスの処理です.
//-----------------------------------------------------------------------------
void AppCamera::OnMouse
(
    int  x,
    int  y,
    int  wheelDelta,
    bool isLeftButtonDown,
    bool isRightButtonDown,
    bool isMiddleButtonDown,
    bool isSideButton1Down,
    bool isSideButton2Down
)
{
    m_MouseLeft  .Update( x, y, isLeftButtonDown );
    m_MouseRight .Update( x, y, isRightButtonDown );
    m_MouseMiddle.Update( x, y, isMiddleButtonDown );

    CameraEvent e = MakeEventFromMouse( wheelDelta );
    m_Camera.UpdateByEvent( e );
}

//-----------------------------------------------------------------------------
//      キーの処理です.
//-----------------------------------------------------------------------------
void AppCamera::OnKey( uint32_t nChar, bool isKeyDown, bool isAltDown )
{
    CameraEvent e = MakeEventFromKey( nChar, isKeyDown, isAltDown );
    m_Camera.UpdateByEvent( e );
}

//-----------------------------------------------------------------------------
//      ビュー行列を取得します.
//-----------------------------------------------------------------------------
asdx::Matrix AppCamera::GetView() const
{ return m_Camera.GetView(); }

//-----------------------------------------------------------------------------
//      位置座標を取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 AppCamera::GetPosition() const
{ return m_Camera.GetPosition(); }

//-----------------------------------------------------------------------------
//      注視点を取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 AppCamera::GetTarget() const
{ return m_Camera.GetTarget(); }

//-----------------------------------------------------------------------------
//      上向き方向を取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 AppCamera::GetUpward() const
{ return m_Camera.GetUpward(); }

//-----------------------------------------------------------------------------
//      カメラ基底ベクトルのX軸を取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 AppCamera::GetAxisX() const
{ return m_Camera.GetAxisX(); }

//-----------------------------------------------------------------------------
//      カメラ基底ベクトルのY軸を取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 AppCamera::GetAxisY() const
{ return m_Camera.GetAxisY(); }

//-----------------------------------------------------------------------------
//      カメラ基底ベクトルのZ軸を取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 AppCamera::GetAxisZ() const
{ return m_Camera.GetAxisZ(); }

//-----------------------------------------------------------------------------
//      カメラのニアクリップ平面を取得します.
//-----------------------------------------------------------------------------
float AppCamera::GetNearClip() const
{ return m_Camera.GetMinDist(); }

//-----------------------------------------------------------------------------
//      カメラのファークリップ平面を取得します.
//-----------------------------------------------------------------------------
float AppCamera::GetFarClip() const
{ return m_Camera.GetMaxDist(); }

//-----------------------------------------------------------------------------
//      カメライベントによって更新を行います.
//-----------------------------------------------------------------------------
void AppCamera::UpdateByEvent(const CameraEvent& cameraEvent)
{ m_Camera.UpdateByEvent(cameraEvent); }

//-----------------------------------------------------------------------------
//      マウス入力からカメライベントを生成します.
//-----------------------------------------------------------------------------
CameraEvent AppCamera::MakeEventFromMouse( int wheelDelta )
{
    CameraEvent result;
    uint32_t flags = 0;

    // 移動軸.
    auto forward = m_Camera.GetAxisZ();
    auto right   = m_Camera.GetAxisX();
    auto upward  = m_Camera.GetAxisY();

    // 注視点カメラの場合.
    if (m_Type == CAMERA_TYPE_TARGET)
    {
        // ホイールでドリー.
        if ( wheelDelta > 0 )
        {
            flags |= CameraEvent::EVENT_DOLLY;
            result.Dolly = m_Gain[m_Type].Wheel;
        }
        else if ( wheelDelta < 0 )
        {
            flags |= CameraEvent::EVENT_DOLLY;
            result.Dolly = -m_Gain[m_Type].Wheel;
        }

        // 左ボタンドラッグで回転処理.
        if ( m_MouseLeft.isClick && m_MouseLeft.isPrevClick && ( !m_MouseRight.isClick ) && ( !m_MouseMiddle.isClick ) )
        {
            flags |= CameraEvent::EVENT_ROTATE;

            // 動いた差分を回転量とする.
            result.Rotate.x = -( m_MouseLeft.X - m_MouseLeft.prevX ) * m_Gain[m_Type].Rotate;
            result.Rotate.y = -( m_MouseLeft.Y - m_MouseLeft.prevY ) * m_Gain[m_Type].Rotate;
        }

        // 右ボタンドラッグでドリー.
        if ( ( !m_MouseLeft.isClick ) && m_MouseRight.isClick && m_MouseRight.isPrevClick && ( !m_MouseMiddle.isClick ) )
        {
            flags |= CameraEvent::EVENT_DOLLY;

            // 動いた差分をドリー量とする.
            result.Dolly = ( -( m_MouseRight.Y - m_MouseRight.prevY ) +
                             -( m_MouseRight.X - m_MouseRight.prevX ) ) * m_Gain[m_Type].Dolly;
        }

        // 左＋右ボタンボタンドラッグでトラック処理.
        if ( m_MouseLeft.isClick && m_MouseLeft.isPrevClick && m_MouseRight.isClick && m_MouseRight.isPrevClick )
        {
            flags |= CameraEvent::EVENT_TRUCK;

            // 方向ベクトル算出.
            asdx::Vector3 dir = m_Camera.GetTarget() - m_Camera.GetPosition();
            if ( dir.LengthSq() != 0.0f )
            { dir.Normalize(); }

            // 右ベクトル算出.
            asdx::Vector3 right = Vector3::Cross( m_Camera.GetUpward(), dir );
            if ( right.LengthSq() != 0.0f )
            { right.Normalize(); }

            asdx::Vector3 upward = m_Camera.GetUpward();

            // 動いた差分を算出.
            float rightGain  = ( m_MouseMiddle.X - m_MouseMiddle.prevX ) * m_Gain[m_Type].Move;
            float upwardGain = ( m_MouseMiddle.Y - m_MouseMiddle.prevY ) * m_Gain[m_Type].Move;

            // 係数をかける.
            right.x  *= rightGain;
            right.y  *= rightGain;
            right.z  *= rightGain;
            upward.x *= upwardGain;
            upward.y *= upwardGain;
            upward.z *= upwardGain;

            // カメラ空間で平行に動かす.
            result.Truck.x = ( right.x + upward.x );
            result.Truck.y = ( right.y + upward.y );
            result.Truck.z = ( right.z + upward.z );
        }
    }
    // フリーカメラの場合.
    else if (m_Type == CAMERA_TYPE_FREE)
    {
        // ホイールで前後移動.
        if (wheelDelta != 0)
        {
            flags |= CameraEvent::EVENT_TRUCK;

            auto move = forward * m_Gain[m_Type].Wheel * float(wheelDelta);
            move.y = 0.0f;
            result.Truck += move;
        }

        // 右ボタンで上下移動とカニ歩き.
        if ( m_MouseRight.isClick && m_MouseRight.isPrevClick )
        {
            flags |= CameraEvent::EVENT_TRUCK;

            result.Truck += upward * m_Gain[m_Type].Move * float(m_MouseRight.Y - m_MouseRight.prevY);
            result.Truck += right  * m_Gain[m_Type].Move * float(m_MouseRight.X - m_MouseRight.prevX);
        }

        // 左ボタンで首振り.
        if ( m_MouseLeft.isClick && m_MouseLeft.isPrevClick )
        {
            flags |= CameraEvent::EVENT_PANTILT;

            // 動いた差分をパン・チルト角とする.
            result.PanTilt.x = ( m_MouseLeft.X - m_MouseLeft.prevX ) * m_Gain[m_Type].Rotate;
            result.PanTilt.y = ( m_MouseLeft.Y - m_MouseLeft.prevY ) * m_Gain[m_Type].Rotate;

            // 縦の首振りを抑える.
            const auto eps = 0.01f;
            if (result.PanTilt.y > asdx::F_PIDIV2 - eps)
            { result.PanTilt.y = asdx::F_PIDIV2 - eps; }
            else if (result.PanTilt.y < -asdx::F_PIDIV2 + eps)
            { result.PanTilt.y = -asdx::F_PIDIV2 + eps; }
        }
    }

    // フラグを設定.
    result.Flags = flags;

    return result;
}

//-----------------------------------------------------------------------------
//      キー入力からカメライベントを生成する.
//-----------------------------------------------------------------------------
CameraEvent AppCamera::MakeEventFromKey( uint32_t nChar, bool isKeyDown, bool isAltDown )
{
    CameraEvent result;

    switch( nChar )
    {
    // Fキーでリセット.
    case 0x46:
        if ( isKeyDown )
        { result.Flags |= CameraEvent::EVENT_RESET; }
        break;

    //// Tキーでツイスト
    //case 0x54:
    //    if ( isAltDown && isKeyDown )
    //    {
    //        result.Flags |= CameraEvent::EVENT_TWIST;
    //        result.Twist = -0.1f;
    //    }
    //    else if ( !isAltDown && isKeyDown )
    //    {
    //        result.Flags |= CameraEvent::EVENT_TWIST;
    //        result.Twist = 0.1f;
    //    }
    //    break;
    }

    return result;
}

//-----------------------------------------------------------------------------
//      調整係数をリセットします.
//-----------------------------------------------------------------------------
void AppCamera::ResetGain()
{
    m_Gain[0].Dolly  = 0.5f;
    m_Gain[0].Rotate = 0.01f;
    m_Gain[0].Move   = 1.0f;
    m_Gain[0].Wheel  = 20.0f;

    m_Gain[1].Dolly  = 0.5f;
    m_Gain[1].Rotate = 0.01f;
    m_Gain[1].Move   = 1.0f;
    m_Gain[1].Wheel  = 1.0f;
}

//-----------------------------------------------------------------------------
//      カメラパラメータを記憶します.
//-----------------------------------------------------------------------------
void AppCamera::Present()
{ m_Camera.Preset(); }

//-----------------------------------------------------------------------------
//      カメラパラメータをリセットします.
//-----------------------------------------------------------------------------
void AppCamera::Reset()
{ m_Camera.Reset(); }

} // namespace asdx
