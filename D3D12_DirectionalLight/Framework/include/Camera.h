//-----------------------------------------------------------------------------
// File : Camera.h
// Desc : Camera Module.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>
#include <d3d12.h>
#include <SimpleMath.h>


///////////////////////////////////////////////////////////////////////////////
// Camera class
///////////////////////////////////////////////////////////////////////////////
class Camera
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    ///////////////////////////////////////////////////////////////////////////
    // EventType enum
    ///////////////////////////////////////////////////////////////////////////
    enum EventType
    {
        EventRotate   = 0x1 << 0,    //!< 回転.
        EventDolly    = 0x1 << 1,    //!< ズーム.
        EventMove     = 0x1 << 2,    //!< 移動.
        EventPanTilt  = 0x1 << 3,    //!< パン・チルト
        EventReset    = 0x1 << 4,    //!< リセット.
    };

    ///////////////////////////////////////////////////////////////////////////
    // Event structure
    ///////////////////////////////////////////////////////////////////////////
    struct Event
    {
        uint32_t    Type        = 0;        //!< イベントタイプ.
        float       RotateH     = 0.0f;     //!< 水平方向の回転角(rad).
        float       RotateV     = 0.0f;     //!< 垂直方向の回転角(rad).
        float       Pan         = 0.0f;     //!< 左右の首振り角(rad).
        float       Tilt        = 0.0f;     //!< 上下の首振り角(rad).
        float       Dolly       = 0.0f;     //!< ズーム量.
        float       MoveX       = 0.0f;     //!< X軸方向への移動量(ビュー空間).
        float       MoveY       = 0.0f;     //!< Y軸方向への移動量(ビュー空間).
        float       MoveZ       = 0.0f;     //!< Z軸方向への移動量(ビュー空間).
    };

    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================
    Camera();
    ~Camera();

    void SetPosition    (const DirectX::SimpleMath::Vector3& value);
    void SetTarget      (const DirectX::SimpleMath::Vector3& value);
    void UpdateByEvent  (const Event& value);
    void Update         ();
    void Preserve       ();
    void Reset          ();

    const float& GetAngleV  () const;
    const float& GetAngleH  () const;
    const float& GetDistance() const;

    const DirectX::SimpleMath::Vector3& GetPosition () const;
    const DirectX::SimpleMath::Vector3& GetTarget   () const;
    const DirectX::SimpleMath::Vector3& GetUpward   () const;
    const DirectX::SimpleMath::Matrix&  GetView     () const;

private:
    ///////////////////////////////////////////////////////////////////////////
    // DirtyFlag enum
    ///////////////////////////////////////////////////////////////////////////
    enum DirtyFlag
    {
        DirtyNone       = 0x0,       //!< 再計算なし.
        DirtyPosition   = 0x1 << 0,  //!< 位置を再計算.
        DirtyTarget     = 0x1 << 1,  //!< 注視点を再計算.
        DirtyAngle      = 0x1 << 2,  //!< 回転角を再計算.
        DirtyMatrix     = 0x1 << 3,  //!< 行列を再計算.
    };

    ///////////////////////////////////////////////////////////////////////////
    // Param structure
    ///////////////////////////////////////////////////////////////////////////
    struct Param
    {
        DirectX::SimpleMath::Vector3    Position;
        DirectX::SimpleMath::Vector3    Target;
        DirectX::SimpleMath::Vector3    Upward;
        DirectX::SimpleMath::Vector3    Forward;
        DirectX::XMFLOAT2               Angle;
        float                           Distance;
    };

    //=========================================================================
    // private variables.
    //=========================================================================
    Param                           m_Current   = {};
    Param                           m_Preserve  = {};
    DirectX::SimpleMath::Matrix     m_View      = DirectX::SimpleMath::Matrix::Identity;
    uint32_t                        m_DirtyFlag = 0;

    //=========================================================================
    // private methods.
    //=========================================================================
    void Rotate (float angleH, float angleV);
    void Pantilt(float angleH, float angleV);
    void Move   (float moveX,  float moveY, float moveZ);
    void Dolly  (float value);

    void ComputePosition();
    void ComputeTarget  ();
    void ComputeAngle   ();
};


///////////////////////////////////////////////////////////////////////////////
// Projector class
///////////////////////////////////////////////////////////////////////////////
class Projector
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    ///////////////////////////////////////////////////////////////////////////
    // Mode enum
    ///////////////////////////////////////////////////////////////////////////
    enum Mode
    {
        Perspective,        //!< 透視投影.
        Orhographic,        //!< 正射影.
    };

    //=========================================================================
    // public variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // public methods.
    //=========================================================================
    Projector();
    ~Projector();

    void Preserve();
    void Reset   ();

    void SetPerspective (float fov,  float aspect, float nearClip, float farClip);
    void SetOrthographic(float left, float right, float top, float bottom, float nearClip, float farClip);

    const Mode&  GetMode        () const; 
    const float& GetFieldOfView () const;
    const float& GetAspect      () const;
    const float& GetNearClip    () const;
    const float& GetFarClip     () const;


    const DirectX::SimpleMath::Matrix& GetMatrix() const;

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    struct Param
    {
        Mode    Mode;             //!< 射影モード.
        float   Aspect;           //!< アスペクト比.
        float   FieldOfView;      //!< 垂直画角.
        float   Left;             //!< 左端.
        float   Right;            //!< 右端.
        float   Top;              //!< 上端.
        float   Bottom;           //!< 下端.
        float   NearClip;         //!< 近クリップ平面までの距離.
        float   FarClip;          //!< 遠クリップ平面までの距離.
    };
    Param m_Current;
    Param m_Preserve;

    DirectX::SimpleMath::Matrix m_Proj; //!< 射影行列.
};