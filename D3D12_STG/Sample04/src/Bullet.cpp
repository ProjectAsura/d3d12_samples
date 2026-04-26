//-----------------------------------------------------------------------------
// File : Bullet.cpp
// Desc : Bullet.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "Bullet.h"
#include <fnd/asdxLogger.h>


///////////////////////////////////////////////////////////////////////////////
// Bullet class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Bullet::Bullet()
: Entity()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      引数付きコンストラクタです.
//-----------------------------------------------------------------------------
Bullet::Bullet
(
    uint16_t    kind,
    float       px,
    float       py,
    float       angle,
    float       angleRate,
    float       speed,
    float       speedRate
)
: Entity(kind, px, py, true)
, m_Angle       (angle)
, m_AngleRate   (angleRate)
, m_Speed       (speed)
, m_SpeedRate   (speedRate)
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      更新処理を行います.
//-----------------------------------------------------------------------------
void Bullet::Update()
{
    asdx::Vector2 pos = GetPos();

    // 角度をラジアンに変換.
    auto rad = asdx::ToRadian(m_Angle);

    // 角度と速さを使って、座標を更新.
    pos.x += m_Speed * cosf(rad);
    pos.y += m_Speed * sinf(rad);
    SetPos(pos);

    // 角度に角速度を加算する.
    m_Angle += m_AngleRate;

    // 速度に加速度を加算する.
    m_Speed += m_SpeedRate;
}


///////////////////////////////////////////////////////////////////////////////
// BulletManager class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
BulletManager::BulletManager()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
BulletManager::~BulletManager()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化を行います.
//-----------------------------------------------------------------------------
bool BulletManager::Init(uint32_t count)
{
    Term();

    m_Bullets = new(std::nothrow) Bullet[count];
    if (m_Bullets == nullptr)
    {
        ELOGA("Error : Out of Memory.");
        return false;
    }

    m_MaxCount = count;
    m_UsedCount = 0;

    for(auto i=0u; i<count; ++i)
    { m_FreeList.push_back(&m_Bullets[i]); }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void BulletManager::Term()
{
    m_FreeList.clear();
    m_UsedList.clear();
    m_MaxCount  = 0;
    m_UsedCount = 0;

    if (m_Bullets)
    {
        delete[] m_Bullets;
        m_Bullets = nullptr;
    }
}

//-----------------------------------------------------------------------------
//      弾を生成します.
//-----------------------------------------------------------------------------
bool BulletManager::Spwan
(
    uint16_t    kind,
    float       px,
    float       py,
    float       sx,
    float       sy,
    float       angle,
    float       angleRate,
    float       speed,
    float       speedRate
)
{
    if (m_UsedCount + 1 >= m_MaxCount)
        return false;

    auto itr = &(*(m_FreeList.begin()));
    m_FreeList.pop_front();

    itr->SetKind(kind);
    itr->SetCenter(px, py);
    itr->SetScale(sx, sy);
    itr->SetAngle(angle);
    itr->SetAngleRate(angleRate);
    itr->SetSpeed(speed);
    itr->SetSpeedRate(speedRate);

    m_UsedList.push_back(itr);
    m_UsedCount++;

    return true;
}

//-----------------------------------------------------------------------------
//      更新処理を行います.
//-----------------------------------------------------------------------------
void BulletManager::Update(uint32_t w, uint32_t h)
{
    for(auto& itr : m_UsedList)
    { itr.Update(); }

    // 画面外に出たものは未使用リストに戻す.
    {
        auto itr = m_UsedList.begin();
        while(itr != m_UsedList.end())
        {
            if (itr->IsOutOfScreen(w, h))
            {
                auto item = &(*itr);
                itr = m_UsedList.erase(itr);
                m_FreeList.push_back(item);
                m_UsedCount--;
            }
            else
            {
                itr++;
            }
        }
    }
}

//-----------------------------------------------------------------------------
//      スプライトを描画します.
//-----------------------------------------------------------------------------
void BulletManager::Draw(asdx::SpriteRenderer& renderer)
{
    for(auto& itr : m_UsedList)
    { itr.Draw(renderer); }
}

//-----------------------------------------------------------------------------
//      交差判定を行います.
//-----------------------------------------------------------------------------
bool BulletManager::IsHit(const Entity& entity)
{
    bool hit = false;

    auto itr = m_UsedList.begin();
    while(itr != m_UsedList.end())
    {
        if (itr->IsHit(entity))
        {
            auto item = &(*itr);
            itr = m_UsedList.erase(itr);
            m_FreeList.push_back(item);
            m_UsedCount--;

            hit = true;
        }
        else
        {
            itr++;
        }
    }

    return hit;
}

namespace {

//-----------------------------------------------------------------------------
// Global Variables.
//-----------------------------------------------------------------------------
static BulletManager g_PlayerBulletMgr;     //!< プレイヤー用弾マネージャ.
static BulletManager g_EnemyBulletMgr;      //!< エネミー用弾マネージャ.

} // namespace

//-----------------------------------------------------------------------------
//      プレイヤー用弾マネージャを取得します.
//-----------------------------------------------------------------------------
BulletManager& GetPlayerBulletMgr()
{ return g_PlayerBulletMgr; }

//-----------------------------------------------------------------------------
//      エネミー用弾マネージャを取得します.
//-----------------------------------------------------------------------------
BulletManager& GetEnemyBulletMgr()
{ return g_EnemyBulletMgr; }
