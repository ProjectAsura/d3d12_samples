//-----------------------------------------------------------------------------
// File : ShotBehavior.cpp
// Desc : Shot Behavior.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "ShotBehavior.h"
#include "Enemy.h"
#include "Bullet.h"
#include "Player.h"


///////////////////////////////////////////////////////////////////////////////
// DirShotBehavior class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      方向弾を撃ちます.
//-----------------------------------------------------------------------------
void DirShotBehavior::OnTick(Enemy& entity)
{
    auto pos  = entity.GetCenter();
    auto time = entity.GetTimer();

    time = time % (m_Param.ShotTime + m_Param.WaitTime);
    if (time >= m_Param.ShotTime || (time % m_Param.Interval) != 0)
        return;

    if (m_Param.Count == 1)
    {
        GetEnemyBulletMgr().Spwan(
            m_Param.SpriteKind,
            pos.x,
            pos.y,
            m_Param.Scale.x,
            m_Param.Scale.y,
            m_Param.Angle,
            0.0f,
            m_Param.Speed,
            m_Param.SpeedRate);
        return;
    }

    for(auto i=0; i<m_Param.Count; ++i)
    {
        GetEnemyBulletMgr().Spwan(
            m_Param.SpriteKind,
            pos.x, pos.y,
            m_Param.Scale.x,
            m_Param.Scale.y,
            m_Param.Angle + m_Param.AngleRange * (float(i) / float(m_Param.Count - 1) - 0.5f),
            0.0f,
            m_Param.Speed,
            m_Param.SpeedRate);
    }
}


///////////////////////////////////////////////////////////////////////////////
// SpiralShotBehavior class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      渦巻弾を撃ちます.
//-----------------------------------------------------------------------------
void SpiralShotBehavior::OnTick(Enemy& entity)
{
    auto pos  = entity.GetCenter();
    auto time = entity.GetTimer();

    time = time % (m_Param.ShotTime + m_Param.WaitTime);
    if (time >= m_Param.ShotTime || (time % m_Param.Interval) != 0)
        return;

    asdx::Vector4 userData = entity.GetParam();

    auto step = 1.0f / float(m_Param.Count);
    for(auto i=0; i<m_Param.Count; ++i)
    {
        GetEnemyBulletMgr().Spwan(
            m_Param.SpriteKind,
            pos.x, pos.y,
            m_Param.Scale.x,
            m_Param.Scale.y,
            userData.x + (step * i),
            m_Param.BulletAngleRate,
            m_Param.Speed,
            m_Param.BulletSpeedRate);
    }

    userData.x += m_Param.AngleRate;
    userData.x -= floor(userData.x);

    entity.SetParam(userData);
}


///////////////////////////////////////////////////////////////////////////////
// AimingDirShotBehavior class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      方向弾を狙い撃ちます.
//-----------------------------------------------------------------------------
void AimingDirShotBehavior::OnTick(Enemy& entity)
{
    auto pos  = entity.GetCenter();
    auto time = entity.GetTimer();

    time = time % (m_Param.ShotTime + m_Param.WaitTime);
    if (time >= m_Param.ShotTime || (time % m_Param.Interval) != 0)
        return;

    auto angle = entity.ToAngle(GetPlayer().GetPos());

    if (m_Param.Count == 1)
    {
        GetEnemyBulletMgr().Spwan(
            m_Param.SpriteKind,
            pos.x,
            pos.y,
            m_Param.Scale.x,
            m_Param.Scale.y,
            angle,
            0.0f,
            m_Param.Speed,
            m_Param.SpeedRate);
        return;
    }

    for(auto i=0; i<m_Param.Count; ++i)
    {
        GetEnemyBulletMgr().Spwan(
            m_Param.SpriteKind,
            pos.x, pos.y,
            m_Param.Scale.x,
            m_Param.Scale.y,
            angle + m_Param.AngleRange * (float(i) / float(m_Param.Count - 1) - 0.5f),
            0.0f,
            m_Param.Speed,
            m_Param.SpeedRate);
    }
}
