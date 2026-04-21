//-----------------------------------------------------------------------------
// File : Enemy.cpp
// Desc : Enemy Class
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "Enemy.h"
#include <fnd/asdxLogger.h>


///////////////////////////////////////////////////////////////////////////////
// Enemy class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Enemy::Enemy()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Enemy::~Enemy()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      セットアップ処理を行います.
//-----------------------------------------------------------------------------
void Enemy::Setup
(
    uint16_t            kind,
    float               x,
    float               y,
    const ShotParam&    shotParam,
    const MoveParam&    moveParam,
    EnemyBehavior       shotBehvaior,
    EnemyBehavior       moveBehavior
)
{
    SetKind(kind);
    SetCenter(x, y);
    m_ShotParam    = shotParam;
    m_MoveParam    = moveParam;
    m_ShotBehavior = shotBehvaior;
    m_MoveBehavior = moveBehavior;
}

//-----------------------------------------------------------------------------
//      発弾パラメータを設定します.
//-----------------------------------------------------------------------------
void Enemy::SetShotParam(const ShotParam& value)
{ m_ShotParam = value; }

//-----------------------------------------------------------------------------
//      移動パラメータを設定します.
//-----------------------------------------------------------------------------
void Enemy::SetMoveParam(const MoveParam& value)
{ m_MoveParam = value; }

//-----------------------------------------------------------------------------
//      発弾パラメータを取得します.
//-----------------------------------------------------------------------------
const Enemy::ShotParam& Enemy::GetShotParam() const
{ return m_ShotParam; }

//-----------------------------------------------------------------------------
//      移動パラメータを取得します.
//-----------------------------------------------------------------------------
const Enemy::MoveParam& Enemy::GetMoveParam() const
{ return m_MoveParam; }

//-----------------------------------------------------------------------------
//      発弾挙動を設定します.
//-----------------------------------------------------------------------------
void Enemy::SetShotBehavior(EnemyBehavior value)
{ m_ShotBehavior = value; }

//-----------------------------------------------------------------------------
//      移動挙動を設定します.
//-----------------------------------------------------------------------------
void Enemy::SetMoveBehavior(EnemyBehavior value)
{ m_MoveBehavior = value; }

//-----------------------------------------------------------------------------
//      更新処理を行います.
//-----------------------------------------------------------------------------
void Enemy::Update()
{
    // 移動処理.
    if (m_MoveBehavior != nullptr)
    { m_MoveBehavior(*this); }

    // 発弾処理.
    if (m_ShotBehavior != nullptr)
    { m_ShotBehavior(*this); }
}


///////////////////////////////////////////////////////////////////////////////
// EnemyManager class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
EnemyManager::EnemyManager()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
EnemyManager::~EnemyManager()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化を行います.
//-----------------------------------------------------------------------------
bool EnemyManager::Init(uint32_t count)
{
    Term();

    m_Enemies = new(std::nothrow) Enemy[count];
    if (m_Enemies == nullptr)
    {
        ELOGA("Error : Out of Memory.");
        return false;
    }

    m_MaxCount = count;
    m_UsedCount = 0;

    for(auto i=0u; i<count; ++i)
    { m_FreeList.push_back(&m_Enemies[i]); }

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void EnemyManager::Term()
{
    m_FreeList.clear();
    m_UsedList.clear();
    m_MaxCount  = 0;
    m_UsedCount = 0;

    if (m_Enemies)
    {
        delete[] m_Enemies;
        m_Enemies = nullptr;
    }

    m_Types.clear();
}

//-----------------------------------------------------------------------------
//      生成タイプを登録します.
//-----------------------------------------------------------------------------
void EnemyManager::AddType(uint32_t type, SpawnParam param)
{ m_Types[type] = param; }

//-----------------------------------------------------------------------------
//      生成タイプを登録解除します.
//-----------------------------------------------------------------------------
void EnemyManager::DelType(uint32_t type)
{ m_Types.erase(type); }

//-----------------------------------------------------------------------------
//      生成タイプをクリアします.
//-----------------------------------------------------------------------------
void EnemyManager::ClearTypes()
{ m_Types.clear(); }

//-----------------------------------------------------------------------------
//      弾を生成します.
//-----------------------------------------------------------------------------
bool EnemyManager::Spwan(uint32_t type, float x, float y)
{
    if (m_UsedCount + 1 >= m_MaxCount)
        return false;

    auto item = m_Types.find(type);
    if (item == m_Types.end())
        return false;

    auto itr = &(*(m_FreeList.begin()));
    m_FreeList.pop_front();

    itr->Setup(
        item->second.ShipKind,
        x,
        y,
        item->second.InitShotParam,
        item->second.InitMoveParam,
        item->second.ShotBehavior,
        item->second.MoveBehavior);

    m_UsedList.push_back(itr);
    m_UsedCount++;

    return true;
}

//-----------------------------------------------------------------------------
//      更新処理を行います.
//-----------------------------------------------------------------------------
void EnemyManager::Update(uint32_t w, uint32_t h)
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
void EnemyManager::Draw(asdx::SpriteRenderer& renderer)
{
    for(auto& itr : m_UsedList)
    { itr.Draw(renderer); }
}

//-----------------------------------------------------------------------------
//      交差判定を行います.
//-----------------------------------------------------------------------------
bool EnemyManager::IsHit(const Entity& entity)
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
static EnemyManager g_EnemyManager;

} // namespace

//-----------------------------------------------------------------------------
//      敵マネージャを取得します.
//-----------------------------------------------------------------------------
EnemyManager& GetEnemyMgr()
{ return g_EnemyManager; }
