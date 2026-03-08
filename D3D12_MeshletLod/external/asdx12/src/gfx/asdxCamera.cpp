//-----------------------------------------------------------------------------
// File : asdxCamera.cpp
// Desc : Camera Module.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <gfx/asdxCamera.h>
#include <fnd/asdxLogger.h>


namespace /* anonymos */ {

//-----------------------------------------------------------------------------
//      角度を求めます.
//-----------------------------------------------------------------------------
static inline 
float GetAngle( float sin, float cos )
{
    float angle = asinf( sin );

    if ( cos < FLT_EPSILON )
    { angle = asdx::F_PI - angle; }

    return angle;
}

//-----------------------------------------------------------------------------
//      指定された位置とターゲットから角度に変換します.
//-----------------------------------------------------------------------------
static inline 
void ToAngle
( 
    const asdx::Vector3& dir,
    asdx::Vector2&       angle
)
{
    asdx::Vector3 v1;
    v1.x = -dir.x;
    v1.y = 0.0f;
    v1.z = -dir.z;

    asdx::Vector3 v2 = v1;
    v2.Normalize();

    angle.x = GetAngle( v2.x, v2.z );

    auto dist = v1.Length();
    v1.x = dist;
    v1.y = -dir.y;
    v1.z = 0.0f;

    v2 = v1;
    v2.Normalize();

    angle.y = GetAngle( v2.y, v2.x );
}

//-----------------------------------------------------------------------------
//      指定された角度からベクトルを求めます.
//-----------------------------------------------------------------------------
static inline
void ToVector
(
    const asdx::Vector2& angle,
    asdx::Vector3* pLookDir,
    asdx::Vector3* pUpward
)
{
    auto sinH = sinf( angle.x );
    auto cosH = cosf( angle.x );

    auto sinV = sinf( angle.y );
    auto cosV = cosf( angle.y );

   if ( pLookDir )
   {
       (*pLookDir).x = -cosV * sinH;
       (*pLookDir).y = -sinV;
       (*pLookDir).z = -cosV * cosH;
   }

   if  ( pUpward )
   {
       (*pUpward).x = -sinV * sinH;
       (*pUpward).y = cosV;
       (*pUpward).z = -sinV * cosH;
   }
}

} // namespace /* anonymos */ 


namespace asdx {

///////////////////////////////////////////////////////////////////////////////////////////////////
// Camera class
///////////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Camera::Camera()
: m_Param()
, m_Preset()
, m_View( Matrix::CreateIdentity() )
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Camera::~Camera()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      位置を設定します.
//-----------------------------------------------------------------------------
void Camera::SetPosition( const asdx::Vector3& position )
{
    m_Param.Position = position;

    ClampDist();
    ComputeAngle();
}

//-----------------------------------------------------------------------------
//      注視点を設定します.
//-----------------------------------------------------------------------------
void Camera::SetTarget( const asdx::Vector3& target )
{
    m_Param.Target = target;

    ClampDist();
    ComputeAngle();
}

//-----------------------------------------------------------------------------
//      上向きベクトルを設定します.
//-----------------------------------------------------------------------------
void Camera::SetUpward( const asdx::Vector3& upward )
{ m_Param.Upward = upward; }

//-----------------------------------------------------------------------------
//      ツイスト角を設定します.
//-----------------------------------------------------------------------------
void Camera::SetTwist( const float twist )
{ m_Param.Twist = twist; }

//-----------------------------------------------------------------------------
//      可動距離範囲を設定します.
//-----------------------------------------------------------------------------
void Camera::SetRange( const float minDist, const float maxDist )
{
    m_Param.MinDist = minDist;
    m_Param.MaxDist = maxDist;

    ClampDist();
    ComputeAngle();
}

//-----------------------------------------------------------------------------
//      パラメータを保存します.
//-----------------------------------------------------------------------------
void Camera::Preset()
{ m_Preset = m_Param; }

//-----------------------------------------------------------------------------
//      パラメータをリセットします.
//-----------------------------------------------------------------------------
void Camera::Reset()
{
    m_Param = m_Preset;

    ClampDist();
    ComputeAngle();
}

//-----------------------------------------------------------------------------
//      ビュー行列を取得します.
//-----------------------------------------------------------------------------
asdx::Matrix Camera::GetView() const
{ return m_View; }

//-----------------------------------------------------------------------------
//      位置を取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 Camera::GetPosition() const
{ return m_Param.Position; }

//-----------------------------------------------------------------------------
//      注視点を取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 Camera::GetTarget() const
{ return m_Param.Target; }

//-----------------------------------------------------------------------------
//      上向きベクトルを取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 Camera::GetUpward() const
{ return m_Param.Upward; }

//-----------------------------------------------------------------------------
//      ツイスト角を取得します.
//-----------------------------------------------------------------------------
float Camera::GetTwist() const
{ return m_Param.Twist; }

//-----------------------------------------------------------------------------
//      可動最小距離を取得します.
//-----------------------------------------------------------------------------
float Camera::GetMinDist() const
{ return m_Param.MinDist; }

//-----------------------------------------------------------------------------
//      可動最大距離を取得します.
//-----------------------------------------------------------------------------
float Camera::GetMaxDist() const
{ return m_Param.MaxDist; }

//-----------------------------------------------------------------------------
//      角度を計算します.
//-----------------------------------------------------------------------------
void Camera::ComputeAngle()
{
    Vector3 dir = m_Param.Position - m_Param.Target;
    dir.Normalize();

    ToAngle( dir, m_Param.Rotate );
    ToAngle( dir, m_Param.PanTilt );

    Vector3 lookDir;
    Vector3 upward;
    ToVector( m_Param.Rotate, &lookDir, &upward );

    m_Param.Upward = upward;

    ComputePosition();
    ComputeTarget();
}

//-----------------------------------------------------------------------------
//      位置座標を計算します.
//-----------------------------------------------------------------------------
void Camera::ComputePosition()
{
    // ターゲットまでの距離を算出.
    float dist = Vector3::Distance( m_Param.Position, m_Param.Target );

    Vector3 lookDir;
    Vector3 upward;

    ToVector( m_Param.Rotate, &lookDir, &upward );

    m_Param.Position.x = m_Param.Target.x + dist * lookDir.x;
    m_Param.Position.y = m_Param.Target.y + dist * lookDir.y;
    m_Param.Position.z = m_Param.Target.z + dist * lookDir.z;

    m_Param.Upward = upward;
}

//-----------------------------------------------------------------------------
//      注視点を計算します.
//-----------------------------------------------------------------------------
void Camera::ComputeTarget()
{
    // ターゲットとの距離を算出.
    float dist = Vector3::Distance( m_Param.Target, m_Param.Position );

    Vector3 lookDir;
    Vector3 upward;

    ToVector( m_Param.PanTilt, &lookDir, &upward );

    m_Param.Target.x = m_Param.Position.x - dist * lookDir.x;
    m_Param.Target.y = m_Param.Position.y - dist * lookDir.y;
    m_Param.Target.z = m_Param.Position.z - dist * lookDir.z;

    m_Param.Upward = upward;
}

//--------------------------------------------------------------------------------------------
//      可動距離範囲内に制限します.
//--------------------------------------------------------------------------------------------
void Camera::ClampDist()
{
    // 距離を算出.
    float dist = Vector3::Distance( m_Param.Position, m_Param.Target );

    // 方向ベクトル
    asdx::Vector3 dir = m_Param.Position - m_Param.Target;

    // 最大距離を超えないように制限.
    if ( dist > m_Param.MaxDist )
    {
        // ゼロ除算対策.
        if ( dir.LengthSq() > FLT_EPSILON )
        { dir.Normalize(); }

        m_Param.Position = m_Param.Target + dir * m_Param.MaxDist;
    }

    // 最小距離を下回らないように制限.
    if ( dist < m_Param.MinDist )
    {
        // ゼロ除算対策.
        if ( dir.LengthSq() > FLT_EPSILON )
        { dir.Normalize(); }

        m_Param.Position = m_Param.Target + dir * m_Param.MinDist;
    }
}

//-----------------------------------------------------------------------------
//      カメラ基底ベクトルのX軸を取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 Camera::GetAxisX() const
{ return asdx::Vector3(m_View._11, m_View._21, m_View._31); }

//-----------------------------------------------------------------------------
//      カメラ基底ベクトルのY軸を取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 Camera::GetAxisY() const
{ return asdx::Vector3(m_View._12, m_View._22, m_View._32); }

//-----------------------------------------------------------------------------
//      カメラ基底ベクトルのZ軸を取得します.
//-----------------------------------------------------------------------------
asdx::Vector3 Camera::GetAxisZ() const
{ return asdx::Vector3(m_View._13, m_View._23, m_View._33); }

//-----------------------------------------------------------------------------
//      パラメータを取得します.
//-----------------------------------------------------------------------------
const Camera::Param& Camera::GetParam() const
{ return m_Param; }

//-----------------------------------------------------------------------------
//      パラメータを設定します.
//-----------------------------------------------------------------------------
void Camera::SetParam(const Param& param)
{ m_Param = param; }

//-----------------------------------------------------------------------------
//      ビュー行列を更新します.
//-----------------------------------------------------------------------------
void Camera::Update()
{
    asdx::Vector3 upward = m_Param.Upward;

    // ツイスト角がゼロでない場合.
    if ( fabs( m_Param.Twist ) > FLT_EPSILON )
    {
        // 視線ベクトルを作成.
        asdx::Vector3 dir = m_Param.Target - m_Param.Position;
        if ( dir.LengthSq() > FLT_EPSILON )
        { dir.Normalize(); }

        // 視線ベクトル軸とした回転行列を作成.
        asdx::Matrix rotate = Matrix::CreateFromAxisAngle( dir, m_Param.Twist );

        // アップベクトルを回転.
        upward = Vector3::Transform( upward, rotate );
    }

    // ビュー行列を更新.
    m_View = Matrix::CreateLookAt( m_Param.Position, m_Param.Target, upward );
}

//-----------------------------------------------------------------------------
//      カメライベントを基にビュー行列を更新します.
//-----------------------------------------------------------------------------
void Camera::UpdateByEvent( const CameraEvent& camEvent )
{
    auto isProcess = false;

    // 回転処理.
    if ( camEvent.Flags & CameraEvent::EVENT_ROTATE )
    {
        // 回転角を加算.
        m_Param.Rotate += camEvent.Rotate;

        // 90度制限.
        if ( m_Param.Rotate.y > 1.564f )
        { m_Param.Rotate.y = 1.564f; }
        if ( m_Param.Rotate.y < -1.564f )
        { m_Param.Rotate.y = -1.564f; }

        ComputePosition();

        Vector3 dir = m_Param.Position - m_Param.Target;
        dir.Normalize();
        ToAngle( dir, m_Param.Rotate );
        ToAngle( dir, m_Param.PanTilt );

        isProcess = true;
    }

    // ドリー処理.
    if ( camEvent.Flags & CameraEvent::EVENT_DOLLY )
    {
        // 視線ベクトルを作成.
        asdx::Vector3 dir = m_Param.Position - m_Param.Target;
        float dist = Vector3::Distance( m_Param.Position, m_Param.Target );

        // 正規化.
        if ( dist > FLT_EPSILON )
        {
            float invDist = 1.0f / dist;
            dir.x *= invDist;
            dir.y *= invDist;
            dir.z *= invDist;
        }

        // ドリー量を加算.
        dist += camEvent.Dolly;

        // 可動距離範囲内に制限.
        if ( m_Param.MinDist > dist )
        { dist = m_Param.MinDist; }
        if ( m_Param.MaxDist < dist )
        { dist = m_Param.MaxDist; }

        // 位置ベクトルを更新.
        m_Param.Position = m_Param.Target + ( dir * dist );

        isProcess = true;
    }

    // トラック処理.
    if ( camEvent.Flags & CameraEvent::EVENT_TRUCK )
    {
        m_Param.Position += camEvent.Truck;
        m_Param.Target   += camEvent.Truck;

        isProcess = true;
    }

    // パン・チルト処理.
    if ( camEvent.Flags & CameraEvent::EVENT_PANTILT )
    {
        // パン・チルト角を加算.
        m_Param.PanTilt += camEvent.PanTilt;

        // 縦90度制限.
        if ( m_Param.PanTilt.y > 1.564f )
        { m_Param.PanTilt.y = 1.564f; }
        if ( m_Param.PanTilt.y < -1.564f )
        { m_Param.PanTilt.y = -1.564f; }

        ComputeTarget();

        Vector3 dir = m_Param.Position - m_Param.Target;
        dir.Normalize();
        ToAngle( dir, m_Param.Rotate );
        ToAngle( dir, m_Param.PanTilt );

        isProcess = true;
    }

    // ツイスト処理.
    if ( camEvent.Flags & CameraEvent::EVENT_TWIST )
    { 
        m_Param.Twist += camEvent.Twist;
        isProcess = true;
    }

    // リセット処理.
    if ( camEvent.Flags & CameraEvent::EVENT_RESET )
    {
        Reset();
        isProcess = true;
    }

    // ビュー行列を更新.
    if ( isProcess )
    {
        Update();
    }
}


} // namespace asdx

