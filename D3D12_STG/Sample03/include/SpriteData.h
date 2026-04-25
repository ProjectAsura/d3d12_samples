//-----------------------------------------------------------------------------
// File : SpriteData.h
// Desc : Sprite Data.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <fnd/asdxMath.h>


///////////////////////////////////////////////////////////////////////////////
// SpriteKind enum
///////////////////////////////////////////////////////////////////////////////
enum SpriteKind
{
    BEAM0,
    BEAM1,
    BEAM2,
    BEAM3,
    BEAM4,
    BEAM5,
    BEAM6,

    BEAM_LONG1,
    BEAM_LONG2,

    BOLT_SILVER,
    BOLT_BRONZE,
    BOLT_GOLD,

    BUTTON_BLUE,
    BUTTON_GREEN,
    BUTTON_RED,
    BUTTON_YELLOW,

    COCKPIT_BLUE0,
    COCKPIT_BLUE1,
    COCKPIT_BLUE2,
    COCKPIT_BLUE3,
    COCKPIT_BLUE4,
    COCKPIT_BLUE5,
    COCKPIT_BLUT6,
    COCKPIT_BLUT7,

    COCKPIT_GREEN0,
    COCKPIT_GREEN1,
    COCKPIT_GREEN2,
    COCKPIT_GREEN3,
    COCKPIT_GREEN4,
    COCKPIT_GREEN5,
    COCKPIT_GREEN6,
    COCKPIT_GREEN7,

    COCKPIT_RED0,
    COCKPIT_RED1,
    COCKPIT_RED2,
    COCKPIT_RED3,
    COCKPIT_RED4,
    COCKPIT_RED5,
    COCKPIT_RED6,
    COCKPIT_RED7,

    COCKPIT_YELLOW0,
    COCKPIT_YELLOW1,
    COCKPIT_YELLOW2,
    COCKPIT_YELLOW3,
    COCKPIT_YELLOW4,
    COCKPIT_YELLOW5,
    COCKPIT_YELLOW6,
    COCKPIT_YELLOW7,

    CURSOR,

    ENEMY_BLACK1,
    ENEMY_BLACK2,
    ENEMY_BLACK3,
    ENEMY_BLACK4,
    ENEMY_BLACK5,

    ENEMY_BLUE1,
    ENEMY_BLUE2,
    ENEMY_BLUE3,
    ENEMY_BLUE4,
    ENEMY_BLUE5,

    ENEMY_GREEN1,
    ENEMY_GREEN2,
    ENEMY_GREEN3,
    ENEMY_GREEN4,
    ENEMY_GREEN5,

    ENEMY_RED1,
    ENEMY_RED2,
    ENEMY_RED3,
    ENEMY_RED4,
    ENEMY_RED5,

    ENGINE1,
    ENGINE2,
    ENGINE3,
    ENGINE4,
    ENGINE5,

    FIRE00,
    FIRE01,
    FIRE02,
    FIRE03,
    FIRE04,
    FIRE05,
    FIRE06,
    FIRE07,
    FIRE08,
    FIRE09,
    FIRE10,
    FIRE11,
    FIRE12,
    FIRE13,
    FIRE14,
    FIRE15,
    FIRE16,
    FIRE17,
    FIRE18,
    FIRE19,

    GUN00,
    GUN01,
    GUN02,
    GUN03,
    GUN04,
    GUN05,
    GUN06,
    GUN07,
    GUN08,
    GUN09,
    GUN10,

    LASER_BLUE01,
    LASER_BLUE02,
    LASER_BLUE03,
    LASER_BLUE04,
    LASER_BLUE05,
    LASER_BLUE06,
    LASER_BLUE07,
    LASER_BLUE08,
    LASER_BLUE09,
    LASER_BLUE10,
    LASER_BLUE11,
    LASER_BLUE12,
    LASER_BLUE13,
    LASER_BLUE14,
    LASER_BLUE15,
    LASER_BLUE16,

    LASER_GREEN01,
    LASER_GREEN02,
    LASER_GREEN03,
    LASER_GREEN04,
    LASER_GREEN05,
    LASER_GREEN06,
    LASER_GREEN07,
    LASER_GREEN08,
    LASER_GREEN09,
    LASER_GREEN10,
    LASER_GREEN11,
    LASER_GREEN12,
    LASER_GREEN13,
    LASER_GREEN14,
    LASER_GREEN15,
    LASER_GREEN16,

    LASER_RED01,
    LASER_RED02,
    LASER_RED03,
    LASER_RED04,
    LASER_RED05,
    LASER_RED06,
    LASER_RED07,
    LASER_RED08,
    LASER_RED09,
    LASER_RED10,
    LASER_RED11,
    LASER_RED12,
    LASER_RED13,
    LASER_RED14,
    LASER_RED15,
    LASER_RED16,

    METEOR_BROWN_BIG1,
    METEOR_BROWN_BIG2,
    METEOR_BROWN_BIG3,
    METEOR_BROWN_BIG4,
    METEOR_BROWN_MED1,
    METEOR_BROWN_MED2,
    METEOR_BROWN_SMALL1,
    METEOR_BROWN_SMALL2,
    METEOR_BROWN_TINY1,
    METEOR_BROWN_TINY2,

    METEOR_GREY_BIG1,
    METEOR_GREY_BIG2,
    METEOR_GREY_BIG3,
    METEOR_GREY_BIG4,
    METEOR_GREY_MED1,
    METEOR_GREY_MED2,
    METEOR_GREY_SMALL1,
    METEOR_GREY_SMALL2,
    METEOR_GREY_TINY1,
    METEOR_GREY_TINY2,

    NUMERAL0,
    NUMERAL1,
    NUMERAL2,
    NUMERAL3,
    NUMERAL4,
    NUMERAL5,
    NUMERAL6,
    NUMERAL7,
    NUMERAL8,
    NUMERAL9,
    NUMERALX,

    PILL_BLUE,
    PILL_GREEN,
    PILL_RED,
    PILL_YELLOW,

    PLAYER_LIFE1_BLUE,
    PLAYER_LIFE1_GREEN,
    PLAYER_LIFE1_ORANGE,
    PLAYER_LIFE1_RED,

    PLAYER_LIFE2_BLUE,
    PLAYER_LIFE2_GREEN,
    PLAYER_LIFE2_ORANGE,
    PLAYER_LIFE2_RED,

    PLAYER_LIFE3_BLUE,
    PLAYER_LIFE3_GREEN,
    PLAYER_LIFE3_ORANGE,
    PLAYER_LIFE3_RED,

    PLAYER_SHIP1_BLUE,
    PLAYER_SHIP1_GREEN,
    PLAYER_SHIP1_ORANGE,
    PLAYER_SHIP1_RED,
    PLAYER_SHIP1_DAMAGE1,
    PLAYER_SHIP1_DAMAGE2,
    PLAYER_SHIP1_DAMAGE3,

    PLAYER_SHIP2_BLUE,
    PLAYER_SHIP2_GREEN,
    PLAYER_SHIP2_ORANGE,
    PLAYER_SHIP2_RED,
    PLAYER_SHIP2_DAMAGE1,
    PLAYER_SHIP2_DAMAGE2,
    PLAYER_SHIP2_DAMAGE3,

    PLAYER_SHIP3_BLUE,
    PLAYER_SHIP3_GREEN,
    PLAYER_SHIP3_ORANGE,
    PLAYER_SHIP3_RED,
    PLAYER_SHIP3_DAMAGE1,
    PLAYER_SHIP3_DAMAGE2,
    PLAYER_SHIP3_DAMAGE3,

    POWER_UP_BLUE,
    POWER_UP_BLUE_BOLT,
    POWER_UP_BLUE_SHIELD,
    POWER_UP_BLUE_STAR,

    POWER_UP_GREEN,
    POWER_UP_GREEN_BOLT,
    POWER_UP_GREEN_SHIELD,
    POWER_UP_GREEN_STAR,

    POWER_UP_RED,
    POWER_UP_RED_BOLT,
    POWER_UP_RED_SHIELD,
    POWER_UP_RED_STAR,

    POWER_UP_YELLOW,
    POWER_UP_YELLOW_BOLT,
    POWER_UP_YELLOW_SHIELD,
    POWER_UP_YELLOW_STAR,

    SCRATCH1,
    SCRATCH2,
    SCRATCH3,

    SHIELD1,
    SHIELD2,
    SHIELD3,

    SHIELD_BRONZE,
    SHIELD_GOLD,
    SHIELD_SILVER,

    SPEED,

    STAR1,
    STAR2,
    STAR3,

    STAR_BRONZE,
    STAR_GOLD,
    STAR_SILVER,

    THINGS_BRONZE,
    THINGS_GOLD,
    THINGS_SILVER,

    TURRET_BASE_BIG,
    TURRET_BASE_SMALL,

    UFO_BLUE,
    UFO_GREEN,
    UFO_RED,
    UFO_YELLOW,

    WING_BLUE0,
    WING_BLUE1,
    WING_BLUE2,
    WING_BLUE3,
    WING_BLUE4,
    WING_BLUE5,
    WING_BLUE6,
    WING_BLUE7,

    WING_GREEN0,
    WING_GREEN1,
    WING_GREEN2,
    WING_GREEN3,
    WING_GREEN4,
    WING_GREEN5,
    WING_GREEN6,
    WING_GREEN7,

    WING_RED0,
    WING_RED1,
    WING_RED2,
    WING_RED3,
    WING_RED4,
    WING_RED5,
    WING_RED6,
    WING_RED7,

    WING_YELLOW0,
    WING_YELLOW1,
    WING_YELLOW2,
    WING_YELLOW3,
    WING_YELLOW4,
    WING_YELLOW5,
    WING_YELLOW6,
    WING_YELLOW7,
};

///////////////////////////////////////////////////////////////////////////////
// SpriteData structure
///////////////////////////////////////////////////////////////////////////////
struct SpriteData
{
    int             X;
    int             Y;
    int             W;
    int             H;
    asdx::Vector2   uv0;
    asdx::Vector2   uv1;
};

//-----------------------------------------------------------------------------
//! @brief      スプライトデータを取得します.
//! 
//! @param[in]      kind        スプライト種別.
//-----------------------------------------------------------------------------
const SpriteData& GetSpriteData(SpriteKind kind);

