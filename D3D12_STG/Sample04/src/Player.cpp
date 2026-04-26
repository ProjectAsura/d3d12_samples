//-----------------------------------------------------------------------------
// File : Player.cpp
// Desc : Player Entity.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "Player.h"
#include "SpriteData.h"
#include "Bullet.h"


namespace {

//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------
constexpr uint8_t   kDefaultLife      = 3;        //!< 残機数のデフォルト値.
constexpr float     kDefaultMoveSpeed = 5.0f;     //!< プレイヤー移動速度のデフォルト値.
constexpr float     kShotSpeed        = 25.0f;    //!< 弾の速さ.
constexpr uint32_t  kMaxPlayerCount   = 3;        //!< 最大プレイヤー数.

constexpr asdx::PAD_BUTTON kShotButton = asdx::PAD_B;
SpriteKind kPlayerShip[kMaxPlayerCount] = {
    PLAYER_SHIP2_BLUE,
    PLAYER_SHIP2_GREEN,
    PLAYER_SHIP2_RED,
};

SpriteKind kLaser[kMaxPlayerCount] = {
    LASER_BLUE01,
    LASER_GREEN01,
    LASER_RED01,
};

} // namespace


///////////////////////////////////////////////////////////////////////////////
// Player class
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//      コンストラクタです.
//-----------------------------------------------------------------------------
Player::Player()
{ /* DO_NOTHING */ }

//-----------------------------------------------------------------------------
//      デストラクタです.
//-----------------------------------------------------------------------------
Player::~Player()
{ Term(); }

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
void Player::Init(float px, float py, float sx, float sy, bool center)
{
    m_Pad.SetPlayerIndex(0);

    SetKind(kPlayerShip[0]);

    if (center)
    { SetCenter(px, py); }
    else
    { SetPos(px, py); }

    SetScale(sx, sy);

    m_Life      = kDefaultLife;
    m_MoveSpeed = kDefaultMoveSpeed;
    m_PadLock   = true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void Player::Term()
{
    m_PadLock = true;
    m_Life    = 0;
}

//-----------------------------------------------------------------------------
//      更新処理を行います.
//-----------------------------------------------------------------------------
void Player::Update(uint32_t w, uint32_t h)
{
    // プレイヤー番号取得.
    auto idx = m_Pad.GetPlayerIndex();
    assert(idx < kMaxPlayerCount);

    asdx::Vector2 pos = GetPos();
    const auto& shipData = GetSpriteData(kPlayerShip[idx]);
    auto center = asdx::Vector2(pos.x + shipData.W * 0.5f, pos.y + shipData.H * 0.5f);

    // パッド更新.
    m_Pad.UpdateState();

    if (!m_PadLock)
    {
        // パッド操作による移動.
        pos.x += m_MoveSpeed * m_Pad.GetNormalizedThumbLX();
        pos.y -= m_MoveSpeed * m_Pad.GetNormalizedThumbLY();

        // 中心位置更新.
        center = asdx::Vector2(pos.x + shipData.W * 0.5f, pos.y + shipData.H * 0.5f);

        if (m_Pad.IsDown(kShotButton))
        {
            auto x = center.x;
            auto y = center.y;

            // 真上方向に撃つ.
            GetPlayerBulletMgr().Spwan(kLaser[idx], x, y, 1.0f, 1.0f, -90.0f, 0.0f, kShotSpeed, 0.0f);
        }

        // 操作時のみクランプ処理を入れる.
        // ※ 演出上で画面外にも出せるようにするため.
        pos = asdx::Vector2::Clamp(
            pos,
            asdx::Vector2(0.0f, 0.0f),
            asdx::Vector2(float(w - shipData.W), float(h - shipData.H)));
    }

    SetPos(pos);
}

//-----------------------------------------------------------------------------
//      ゲームパッドを取得します.
//-----------------------------------------------------------------------------
const asdx::GamePad& Player::GetPad() const
{ return m_Pad; }

//-----------------------------------------------------------------------------
//      パッド操作をロックします.
//-----------------------------------------------------------------------------
void Player::SetPadLock(bool value)
{ m_PadLock = value; }

//-----------------------------------------------------------------------------
//      パッド操作がロックされているかどうかチェックします.
//-----------------------------------------------------------------------------
bool Player::IsPadLock() const
{ return m_PadLock; }

//-----------------------------------------------------------------------------
//      残機数を設定します.
//-----------------------------------------------------------------------------
void Player::SetLife(uint8_t value)
{ m_Life = value; }

//-----------------------------------------------------------------------------
//      残機数を取得します.
//-----------------------------------------------------------------------------
uint8_t Player::GetLife() const
{ return m_Life; }


namespace {

//-----------------------------------------------------------------------------
// Global Variables.
//-----------------------------------------------------------------------------
static Player g_Player = {};

} // namespace

//----------------------------------------------------------------------------
//      プレイヤーを取得します.
//----------------------------------------------------------------------------
Player& GetPlayer()
{ return g_Player; }
