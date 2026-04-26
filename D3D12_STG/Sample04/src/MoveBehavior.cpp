//-----------------------------------------------------------------------------
// File : MoveBehavior.cpp
// Desc : Move Behavior.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MoveBehavior.h"
#include "Player.h"


///////////////////////////////////////////////////////////////////////////////
// OneWayMoveBehaivor class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      直線的な移動処理です.
//-----------------------------------------------------------------------------
void OneWayMoveBehavior::OnTick(Enemy& entity)
{
    auto pos = entity.GetCenter();
    pos += m_Param.Velocity;
    entity.SetCenter(pos);
}


///////////////////////////////////////////////////////////////////////////////
// WaveMoveBehavior class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      波状の移動処理です.
//-----------------------------------------------------------------------------
void WaveMoveBehavior::OnTick(Enemy& entity)
{
    auto pos = entity.GetCenter();
    float cx;
    if (entity.GetTimer() == 0)
    {
        asdx::Vector4 param = entity.GetParam();
        param.z = pos.x;
        entity.SetParam(param);
        cx = pos.x;
    }
    else
    {
        const auto& param = entity.GetParam();
        cx = param.z;
    }

    auto angle = entity.GetTimer() * m_Param.AngleScale;
    angle += m_Param.AngleOffset;
    angle = asdx::Wrap(angle, 0.0f, 360.0f);

    pos.x = m_Param.Amplitude * sinf(asdx::ToRadian(angle)) + cx;
    pos.y += m_Param.Speed;

    entity.SetCenter(pos);
}


///////////////////////////////////////////////////////////////////////////////
// AinminMoveBehavior class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      追跡移動します.
//-----------------------------------------------------------------------------
void AimingMoveBehavior::OnTick(Enemy& entity)
{
    auto pos = entity.GetCenter();

    auto timer = entity.GetTimer();
    if ((timer % m_Param.Interval) == 0)
    {
        auto playerPos = GetPlayer().GetCenter();
        auto dir = playerPos - pos;
        dir.Normalize();

        asdx::Vector4 param = entity.GetParam();
        param.z = dir.x * m_Param.Speed;
        param.w = dir.y * m_Param.Speed;
        pos.x += param.z;
        pos.y += param.w;
        entity.SetParam(param);
        entity.SetCenter(pos);
    }
    else
    {
        const auto& param = entity.GetParam();
        pos.x += param.z;
        pos.y += param.w;
        entity.SetCenter(pos);
    }
}
