//-----------------------------------------------------------------------------
// File : Camera.cpp
// Desc : Camera Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <Camera.h>


namespace {

float Cos(float rad)
{
    if (abs(rad) < FLT_EPSILON)
    { return 1.0f; }

    return cosf(rad);
}

float Sin(float rad)
{
    if (abs(rad) < FLT_EPSILON)
    { return 0.0f; }

    return sinf(rad);
}

//-----------------------------------------------------------------------------
//      角度を求めます.
//-----------------------------------------------------------------------------
float CalcAngle(float sine, float cosine)
{
    auto result = asinf(sine);
    if (cosine < 0.0f)
    { result = DirectX::XM_PI - result; }

    return result;
}

//-----------------------------------------------------------------------------
//      指定ベクトルから角度と距離を求めます.
//-----------------------------------------------------------------------------
void ToAngle
(
    const DirectX::SimpleMath::Vector3& v,
    float* angleH,
    float* angleV,
    float* dist
)
{
    if (dist != nullptr)
    { *dist = v.Length(); }

    DirectX::SimpleMath::Vector3 src(-v.x, 0.0f, -v.z);
    DirectX::SimpleMath::Vector3 dst = src;

    if (angleH != nullptr)
    {
        // 単位ベクトル化.
        if (fabs(src.x) > FLT_EPSILON || fabs(src.z) > FLT_EPSILON)
        { src.Normalize(dst); }

        *angleH = CalcAngle(dst.x, dst.z);
    }

    if (angleV != nullptr)
    { 
        auto d = src.Length();
        src.x = d;
        src.y = -v.y;
        src.z = 0.0f;

        dst = src;

        // 単位ベクトル化.
        if (fabs(src.x) > FLT_EPSILON || fabs(src.y) > FLT_EPSILON)
        { src.Normalize(dst); }

        *angleV = CalcAngle(dst.y, dst.x);
    }
}

//-----------------------------------------------------------------------------
//      指定角度から視線ベクトルと上向きベクトルを求めます.
//-----------------------------------------------------------------------------
void ToVector
(
    float angleH,
    float angleV,
    DirectX::SimpleMath::Vector3* forward,
    DirectX::SimpleMath::Vector3* upward
)
{
    auto sx = Sin( angleH );
    auto cx = Cos( angleH );
    auto sy = Sin( angleV );
    auto cy = Cos( angleV );

    if (forward != nullptr)
    {
        forward->x = -cy * sx;
        forward->y = -sy;
        forward->z = -cy * cx;
    }

    if (upward != nullptr)
    {
        upward->x = -sy * sx;
        upward->y =  cy;
        upward->z = -sy * cx;
    }
}

} // namespace


///////////////////////////////////////////////////////////////////////////////
// Camera class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Camera::Camera()
{
    m_Current.Position = DirectX::SimpleMath::Vector3(0.0f, 0.0f, -1.0f);
    m_Current.Target   = DirectX::SimpleMath::Vector3::Zero;
    m_Current.Upward   = DirectX::SimpleMath::Vector3::UnitY;
    m_Current.Angle    = DirectX::XMFLOAT2(0.0f, 0.0f);
    m_Current.Forward  = DirectX::SimpleMath::Vector3::UnitZ;
    m_Current.Distance = 1.0f;
    m_DirtyFlag        = DirtyPosition;

    m_Preserve = m_Current;
}

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Camera::~Camera()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      カメラ位置を設定します.
//-----------------------------------------------------------------------------
void Camera::SetPosition(const DirectX::SimpleMath::Vector3& value)
{
    m_Current.Position = value;
    ComputeTarget();
    m_DirtyFlag = DirtyAngle;
    Update();
}

//-----------------------------------------------------------------------------
//      注視点を設定します.
//-----------------------------------------------------------------------------
void Camera::SetTarget(const DirectX::SimpleMath::Vector3& value)
{
    m_Current.Target = value;
    ComputePosition();
    m_DirtyFlag = DirtyAngle;
    Update();
}

//-----------------------------------------------------------------------------
//      カメライベントを基に更新処理を行います.
//-----------------------------------------------------------------------------
void Camera::UpdateByEvent(const Event& value)
{
    if (value.Type & EventRotate)
    { Rotate(value.RotateH, value.RotateV); }

    if (value.Type & EventPanTilt)
    { Pantilt(value.Pan, value.Tilt); }

    if (value.Type & EventDolly)
    { Dolly(value.Dolly); }

    if (value.Type & EventMove)
    { Move(value.MoveX, value.MoveY, value.MoveZ); }

    if (value.Type & EventReset)
    {
        Reset();
        return;
    }

    Update();
}

//-----------------------------------------------------------------------------
//      更新処理を行います.
//-----------------------------------------------------------------------------
void Camera::Update()
{
    if (m_DirtyFlag == DirtyNone)
    { return; }

    if (m_DirtyFlag & DirtyPosition)
    { ComputePosition(); }

    if (m_DirtyFlag & DirtyTarget)
    { ComputeTarget(); }

    if (m_DirtyFlag & DirtyAngle)
    { ComputeAngle(); }

    m_View = DirectX::SimpleMath::Matrix::CreateLookAt(
        m_Current.Position,
        m_Current.Target,
        m_Current.Upward);

    m_DirtyFlag = DirtyNone;
}

//-----------------------------------------------------------------------------
//      現在のカメラパラメータを保存します.
//-----------------------------------------------------------------------------
void Camera::Preserve()
{ m_Preserve = m_Current; }

//-----------------------------------------------------------------------------
//      カメラパラメータをリセットします.
//-----------------------------------------------------------------------------
void Camera::Reset()
{ 
    m_Current   = m_Preserve;
    m_DirtyFlag = DirtyMatrix;
    Update();
}

//-----------------------------------------------------------------------------
//      カメラの水平方向回転角(rad)を取得します.
//-----------------------------------------------------------------------------
const float& Camera::GetAngleH() const
{ return m_Current.Angle.x; }

//-----------------------------------------------------------------------------
//      カメラの垂直方向回転角(rad)を取得します.
//-----------------------------------------------------------------------------
const float& Camera::GetAngleV() const
{ return m_Current.Angle.y; }

//-----------------------------------------------------------------------------
//      カメラから注視点までの距離を取得します.
//-----------------------------------------------------------------------------
const float& Camera::GetDistance() const
{ return m_Current.Distance; }

//-----------------------------------------------------------------------------
//      カメラ位置を取得します.
//-----------------------------------------------------------------------------
const DirectX::SimpleMath::Vector3& Camera::GetPosition() const
{ return m_Current.Position; }

//-----------------------------------------------------------------------------
//      注視点を取得します.
//-----------------------------------------------------------------------------
const DirectX::SimpleMath::Vector3& Camera::GetTarget() const
{ return m_Current.Target; }

//-----------------------------------------------------------------------------
//      上向きベクトルを取得します.
//-----------------------------------------------------------------------------
const DirectX::SimpleMath::Vector3& Camera::GetUpward() const
{ return m_Current.Upward; }

//-----------------------------------------------------------------------------
//      ビュー行列を取得します.
//-----------------------------------------------------------------------------
const DirectX::SimpleMath::Matrix& Camera::GetView() const
{ return m_View; }

//-----------------------------------------------------------------------------
//      注視点を軸として回転します.
//-----------------------------------------------------------------------------
void Camera::Rotate(float angleH, float angleV)
{
    ComputeAngle();
    ComputeTarget();

    m_Current.Angle.x += angleH;
    m_Current.Angle.y += angleV;

    // ジンバルロック対策
    {
        if (m_Current.Angle.y > DirectX::XM_PIDIV2 - FLT_EPSILON)
        { m_Current.Angle.y = DirectX::XM_PIDIV2 - FLT_EPSILON; }

        if (m_Current.Angle.y < -DirectX::XM_PIDIV2 + FLT_EPSILON)
        { m_Current.Angle.y = -DirectX::XM_PIDIV2 + FLT_EPSILON; }
    }

    m_DirtyFlag |= DirtyPosition;
}

//-----------------------------------------------------------------------------
//      首振り処理を行います.
//-----------------------------------------------------------------------------
void Camera::Pantilt(float angleH, float angleV)
{
    ComputeAngle();
    ComputePosition();

    m_Current.Angle.x += angleH;
    m_Current.Angle.y += angleV;

    m_DirtyFlag |= DirtyTarget;
}

//-----------------------------------------------------------------------------
//      ビュー空間の基底ベクトル方向に移動させます.
//-----------------------------------------------------------------------------
void Camera::Move(float moveX, float moveY, float moveZ)
{
    auto translate = m_View.Right() * moveX + m_View.Up() * moveY + m_View.Forward() * moveZ;

    m_Current.Position += translate;
    m_Current.Target   += translate;

    m_DirtyFlag |= DirtyMatrix;
}

//-----------------------------------------------------------------------------
//      ズーム処理を行います.
//-----------------------------------------------------------------------------
void Camera::Dolly(float value)
{
    ComputeAngle();
    ComputeTarget();

    m_Current.Distance += value;
    if (m_Current.Distance < 0.001f)
    { m_Current.Distance = 0.001f; }

    m_DirtyFlag |= DirtyPosition;
}

//-----------------------------------------------------------------------------
//      カメラ位置を計算します.
//-----------------------------------------------------------------------------
void Camera::ComputePosition()
{
    ToVector( m_Current.Angle.x, m_Current.Angle.y, &m_Current.Forward, &m_Current.Upward );
    m_Current.Position = m_Current.Target - m_Current.Distance * m_Current.Forward;
}

//-----------------------------------------------------------------------------
//      注視点を計算します.
//-----------------------------------------------------------------------------
void Camera::ComputeTarget()
{
    ToVector( m_Current.Angle.x, m_Current.Angle.y, &m_Current.Forward, &m_Current.Upward );
    m_Current.Target = m_Current.Position + m_Current.Distance * m_Current.Forward;
}

//-----------------------------------------------------------------------------
//      回転角を計算します.
//-----------------------------------------------------------------------------
void Camera::ComputeAngle()
{
    m_Current.Forward = m_Current.Target - m_Current.Position;
    ToAngle( m_Current.Forward, &m_Current.Angle.x, &m_Current.Angle.y, &m_Current.Distance );
    ToVector( m_Current.Angle.x, m_Current.Angle.y, nullptr, &m_Current.Upward );
}


///////////////////////////////////////////////////////////////////////////////
// Projector class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Projector::Projector()
{
    m_Current.Mode        = Mode::Perspective;
    m_Current.Aspect      = 1.333f;
    m_Current.FieldOfView = DirectX::XM_PIDIV4;
    m_Current.NearClip    = 1.0f;
    m_Current.FarClip     = 1000.0f;
    m_Current.Left        = 0.0f;
    m_Current.Right       = 100.0f;
    m_Current.Top         = 0.0f;
    m_Current.Bottom      = 100.0f;

    m_Preserve = m_Current;
}

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Projector::~Projector()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      現在の射影パラメータを保存します.
//-----------------------------------------------------------------------------
void Projector::Preserve()
{ m_Preserve = m_Current; }

//-----------------------------------------------------------------------------
//      射影パラメータをリセットします.
//-----------------------------------------------------------------------------
void Projector::Reset()
{
    m_Current = m_Preserve;
    switch (m_Current.Mode)
    {
    case Mode::Perspective:
        { 
            m_Proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
                m_Current.FieldOfView,
                m_Current.Aspect,
                m_Current.NearClip,
                m_Current.FarClip);
        }
        break;

    case Mode::Orhographic:
        {
            m_Proj = DirectX::SimpleMath::Matrix::CreateOrthographicOffCenter(
                m_Current.Left,
                m_Current.Right,
                m_Current.Bottom,
                m_Current.Top,
                m_Current.NearClip,
                m_Current.FarClip);
        }
        break;
    }
}

//-----------------------------------------------------------------------------
//      透視投影パラメータを設定します.
//-----------------------------------------------------------------------------
void Projector::SetPerspective(float fov, float aspect, float nearClip, float farClip)
{
    m_Current.Mode        = Mode::Perspective;
    m_Current.FieldOfView = fov;
    m_Current.Aspect      = aspect;
    m_Current.NearClip    = nearClip;
    m_Current.FarClip     = farClip;

    m_Proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(fov, aspect, nearClip, farClip);
}

//-----------------------------------------------------------------------------
//      正射影パラメータを設定します.
//-----------------------------------------------------------------------------
void Projector::SetOrthographic(float left, float right, float top, float bottom, float nearClip, float farClip)
{
    m_Current.Mode   = Mode::Orhographic;
    m_Current.Left   = left;
    m_Current.Right  = right;
    m_Current.Top    = top;
    m_Current.Bottom = bottom;

    m_Proj = DirectX::SimpleMath::Matrix::CreateOrthographicOffCenter(left, right, bottom, top, nearClip, farClip);
}

//-----------------------------------------------------------------------------
//      射影モードを取得します.
//-----------------------------------------------------------------------------
const Projector::Mode& Projector::GetMode() const
{ return m_Current.Mode; }

//-----------------------------------------------------------------------------
//      垂直画角を取得します.
//-----------------------------------------------------------------------------
const float& Projector::GetFieldOfView() const
{ return m_Current.FieldOfView; }

//-----------------------------------------------------------------------------
//      アスペクト比を取得します.
//-----------------------------------------------------------------------------
const float& Projector::GetAspect() const
{ return m_Current.Aspect; }

//-----------------------------------------------------------------------------
//      近クリップ平面までの距離を取得します.
//-----------------------------------------------------------------------------
const float& Projector::GetNearClip() const
{ return m_Current.NearClip; }

//-----------------------------------------------------------------------------
//      遠クリップ平面までの距離を取得します.
//-----------------------------------------------------------------------------
const float& Projector::GetFarClip() const
{ return m_Current.FarClip; }

//-----------------------------------------------------------------------------
//      射影行列を取得します.
//-----------------------------------------------------------------------------
const DirectX::SimpleMath::Matrix& Projector::GetMatrix() const
{ return m_Proj; }
