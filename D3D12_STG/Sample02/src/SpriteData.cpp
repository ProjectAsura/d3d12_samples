//-----------------------------------------------------------------------------
// File : SpriteData.cpp
// Desc : Sprite Data.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "SpriteData.h"


namespace {

constexpr int kSpriteChipW = 1024;  //!< スプライトチップの横幅.
constexpr int kSpriteChipH = 1024;  //!< スプライトチップの縦幅.

// テクスチャ座標を計算.
#define UV(x, y) asdx::Vector2(float(x)/float(kSpriteChipW), float(y)/float(kSpriteChipH))

// スプライトデータ
#define SPRITE_ENTRY(tag, x, y, w, h) { x, y, w, h, UV(x, y+h), UV(x+w, y) }

// 各スプライトデータの定義.
static const SpriteData kSpriteData[] = {
    SPRITE_ENTRY(beam0 ,143 ,377 ,43 ,31), // BEAM0
    SPRITE_ENTRY(beam1 ,327 ,644 ,40 ,20), // BEAM1
    SPRITE_ENTRY(beam2 ,262 ,907 ,38 ,31), // BEAM2
    SPRITE_ENTRY(beam3 ,396 ,384 ,29 ,29), // BEAM3
    SPRITE_ENTRY(beam4 ,177 ,496 ,41 ,17), // BEAM4
    SPRITE_ENTRY(beam5 ,186 ,377 ,40 ,25), // BEAM5
    SPRITE_ENTRY(beam6 ,120 ,688 ,43 ,23), // BEAM6

    SPRITE_ENTRY(beamLong1 ,828 ,943 ,15 ,67),  // BEAM_LONG1
    SPRITE_ENTRY(beamLong2 ,307 ,309 ,25 ,64),  // BEAM_LONG2

    SPRITE_ENTRY(bold_silver ,810 ,837 ,19 ,30),    // BOLT_SILVER
    SPRITE_ENTRY(bolt_bronze ,810 ,467 ,19 ,30),    // BOLT_BRONZE
    SPRITE_ENTRY(bolt_gold   ,809 ,437 ,19 ,30),    // BOLT_GOLD

    SPRITE_ENTRY(buttonBlue   ,0 ,78  ,222 ,39),    // BUTTON_BLUE
    SPRITE_ENTRY(buttonGreen  ,0 ,117 ,222 ,39),    // BUTTON_GREEN
    SPRITE_ENTRY(buttonRed    ,0 ,0   ,222 ,39),    // BUTTON_RED
    SPRITE_ENTRY(buttonYellow ,0 ,39  ,222 ,39),    // BUTTON_YELLOW

    SPRITE_ENTRY(cockpitBlue_0 ,586 ,0   ,51 ,75),      // COCKPIT_BLUE0
    SPRITE_ENTRY(cockpitBlue_1 ,736 ,862 ,40 ,40),      // COCKPIT_BLUE1
    SPRITE_ENTRY(cockpitBlue_2 ,684 ,67  ,42 ,56),      // COCKPIT_BLUE2
    SPRITE_ENTRY(cockpitBlue_3 ,336 ,384 ,60 ,61),      // COCKPIT_BLUE3
    SPRITE_ENTRY(cockpitBlue_4 ,637 ,0   ,47 ,67),      // COCKPIT_BLUE4
    SPRITE_ENTRY(cockpitBlue_5 ,627 ,144 ,48 ,75),      // COCKPIT_BLUE5
    SPRITE_ENTRY(cockpitBlue_6 ,684 ,0   ,42 ,67),      // COCKPIT_BLUE6
    SPRITE_ENTRY(cockpitBlue_7 ,737 ,542 ,41 ,71),      // COCKPIT_BLUE7

    SPRITE_ENTRY(cockpitGreen_0 ,576 ,225 ,51 ,75),     // COCKPIT_GREEN0
    SPRITE_ENTRY(cockpitGreen_1 ,734 ,977 ,40 ,40),     // COCKPIT_GREEN1
    SPRITE_ENTRY(cockpitGreen_2 ,696 ,659 ,42 ,56),     // COCKPIT_GREEN2
    SPRITE_ENTRY(cockpitGreen_3 ,346 ,234 ,60 ,61),     // COCKPIT_GREEN3
    SPRITE_ENTRY(cockpitGreen_4 ,627 ,219 ,47 ,67),     // COCKPIT_GREEN4
    SPRITE_ENTRY(cockpitGreen_5 ,694 ,364 ,42 ,67),     // COCKPIT_GREEN5
    SPRITE_ENTRY(cockpitGreen_6 ,737 ,471 ,41 ,71),     // COCKPIT_GREEN6
    SPRITE_ENTRY(cockpitGreen_7 ,602 ,525 ,48 ,75),     // COCKPIT_GREEN7

    SPRITE_ENTRY(cockpitRed_0 ,535 ,75  ,51 ,75),        // COCKPIT_RED0
    SPRITE_ENTRY(cockpitRed_1 ,351 ,982 ,40 ,40),       // COCKPIT_RED1
    SPRITE_ENTRY(cockpitRed_2 ,718 ,197 ,42 ,56),       // COCKPIT_RED2
    SPRITE_ENTRY(cockpitRed_3 ,520 ,661 ,60 ,61),       // COCKPIT_RED3
    SPRITE_ENTRY(cockpitRed_4 ,647 ,857 ,47 ,67),       // COCKPIT_RED4
    SPRITE_ENTRY(cockpitRed_5 ,605 ,707 ,48 ,75),       // COCKPIT_RED5
    SPRITE_ENTRY(cockpitRed_6 ,736 ,795 ,42 ,67),       // COCKPIT_RED6
    SPRITE_ENTRY(cockpitRed_7 ,736 ,329 ,41 ,71),       // COCKPIT_RED7

    SPRITE_ENTRY(cockpitYellow_0 ,726 ,80  ,40 ,40),    // COCKPIT_YELLOW0
    SPRITE_ENTRY(cockpitYellow_1 ,247 ,309 ,60 ,61),    // COCKPIT_YELLOW1
    SPRITE_ENTRY(cockpitYellow_2 ,637 ,67  ,47 ,67),    // COCKPIT_YELLOW2
    SPRITE_ENTRY(cockpitYellow_3 ,607 ,782 ,48 ,75),    // COCKPIT_YELLOW3
    SPRITE_ENTRY(cockpitYellow_4 ,696 ,262 ,42 ,67),    // COCKPIT_YELLOW4
    SPRITE_ENTRY(cockpitYellow_5 ,736 ,400 ,41 ,71),    // COCKPIT_YELLOW5
    SPRITE_ENTRY(cockpitYellow_6 ,734 ,921 ,42 ,56),    // COCKPIT_YELLOW6
    SPRITE_ENTRY(cockpitYellow_7 ,600 ,375 ,51 ,75),    // COCKPIT_YELLOW7

    SPRITE_ENTRY(cursor ,797 ,173 ,30 ,33), // CURSOR
 
    SPRITE_ENTRY(enemyBlack1 ,423 ,728 ,93  ,84),   // ENEMY_BLACK1
    SPRITE_ENTRY(enemyBlack2 ,120 ,604 ,104 ,84),   // ENEMY_BLACK2
    SPRITE_ENTRY(enemyBlack3 ,144 ,156 ,103 ,84),   // ENEMY_BLACK3
    SPRITE_ENTRY(enemyBlack4 ,518 ,325 ,82  ,84),   // ENEMY_BLACK4
    SPRITE_ENTRY(enemyBlack5 ,346 ,150 ,97  ,84),   // ENEMY_BLACK5

    SPRITE_ENTRY(enemyBlue1 ,425 ,468 ,93  ,84),    // ENEMY_BLUE1
    SPRITE_ENTRY(enemyBlue2 ,143 ,293 ,104 ,84),    // ENEMY_BLUE2
    SPRITE_ENTRY(enemyBlue3 ,222 ,0   ,103 ,84),    // ENEMY_BLUE3
    SPRITE_ENTRY(enemyBlue4 ,518 ,409 ,82  ,84),    // ENEMY_BLUE4
    SPRITE_ENTRY(enemyBlue5 ,421 ,814 ,97  ,84),    // ENEMY_BLUE5

    SPRITE_ENTRY(enemyGreen1 ,425 ,552 ,93 ,84),    // ENEMY_GREEN1
    SPRITE_ENTRY(enemyGreen2 ,133 ,412 ,104 ,84),   // ENEMY_GREEN2
    SPRITE_ENTRY(enemyGreen3 ,224 ,496 ,103 ,84),   // ENEMY_GREEN3
    SPRITE_ENTRY(enemyGreen4 ,518 ,493 ,82 ,84),    // ENEMY_GREEN4
    SPRITE_ENTRY(enemyGreen5 ,408 ,907 ,97 ,84),    // ENEMY_GREEN5

    SPRITE_ENTRY(enemyRed1 ,425 ,384 ,93 ,84),      // ENEMY_RED1
    SPRITE_ENTRY(enemyRed2 ,120 ,520 ,104 ,84),     // ENEMY_RED2
    SPRITE_ENTRY(enemyRed3 ,224 ,580 ,103 ,84),     // ENEMY_RED3
    SPRITE_ENTRY(enemyRed4 ,520 ,577 ,82 ,84),      // ENEMY_RED4
    SPRITE_ENTRY(enemyRed5 ,423 ,644 ,97 ,84),      // ENEMY_RED5

    SPRITE_ENTRY(engine1 ,224 ,907  ,38 ,23),       // ENGINE1
    SPRITE_ENTRY(engine2 ,163 ,688  ,42 ,28),       // ENGINE2
    SPRITE_ENTRY(engine3 ,644 ,1002 ,27 ,22),       // ENGINE3
    SPRITE_ENTRY(engine4 ,144 ,240  ,49 ,45),       // ENGINE4
    SPRITE_ENTRY(engine5 ,133 ,496  ,44 ,24),       // ENGINE5

    SPRITE_ENTRY(fire00 ,827 ,125 ,16 ,40),     // FIRE00
    SPRITE_ENTRY(fire01 ,828 ,206 ,14 ,31),     // FIRE01
    SPRITE_ENTRY(fire02 ,827 ,663 ,14 ,32),     // FIRE02
    SPRITE_ENTRY(fire03 ,829 ,437 ,14 ,34),     // FIRE03
    SPRITE_ENTRY(fire04 ,831 ,0   ,14 ,31),     // FIRE04
    SPRITE_ENTRY(fire05 ,834 ,299 ,14 ,31),     // FIRE05
    SPRITE_ENTRY(fire06 ,835 ,502 ,14 ,31),     // FIRE06
    SPRITE_ENTRY(fire07 ,835 ,330 ,14 ,31),     // FIRE07
    SPRITE_ENTRY(fire08 ,827 ,867 ,16 ,40),     // FIRE08
    SPRITE_ENTRY(fire09 ,811 ,663 ,16 ,40),     // FIRE09
    SPRITE_ENTRY(fire10 ,812 ,206 ,16 ,40),     // FIRE10
    SPRITE_ENTRY(fire11 ,835 ,395 ,14 ,31),     // FIRE11
    SPRITE_ENTRY(fire12 ,835 ,533 ,14 ,32),     // FIRE12
    SPRITE_ENTRY(fire13 ,835 ,361 ,14 ,34),     // FIRE13
    SPRITE_ENTRY(fire14 ,831 ,31  ,14 ,31),      // FIRE14
    SPRITE_ENTRY(fire15 ,829 ,471 ,14 ,31),     // FIRE15
    SPRITE_ENTRY(fire16 ,828 ,268 ,14 ,31),     // FIRE16
    SPRITE_ENTRY(fire17 ,828 ,237 ,14 ,31),     // FIRE17
    SPRITE_ENTRY(fire18 ,827 ,165 ,16 ,41),     // FIRE18
    SPRITE_ENTRY(fire19 ,812 ,246 ,16 ,41),     // FIRE19
    
    SPRITE_ENTRY(gun00 ,827 ,907 ,16 ,36),      // GUN00
    SPRITE_ENTRY(gun01 ,810 ,867 ,17 ,33),      // GUN01
    SPRITE_ENTRY(gun02 ,829 ,611 ,14 ,36),      // GUN02
    SPRITE_ENTRY(gun03 ,809 ,796 ,20 ,41),      // GUN03
    SPRITE_ENTRY(gun04 ,827 ,84  ,16 ,41),      // GUN04
    SPRITE_ENTRY(gun05 ,423 ,0   ,21 ,41),      // GUN05
    SPRITE_ENTRY(gun06 ,810 ,900 ,17 ,38),      // GUN06
    SPRITE_ENTRY(gun07 ,829 ,796 ,14 ,41),      // GUN07
    SPRITE_ENTRY(gun08 ,848 ,263 ,10 ,47),      // GUN08
    SPRITE_ENTRY(gun09 ,809 ,611 ,20 ,52),      // GUN09
    SPRITE_ENTRY(gun10 ,808 ,961 ,20 ,52),      // GUN10
    
    SPRITE_ENTRY(laserBlue01 ,856 ,421 ,9  ,54),    // LASER_BLUE01
    SPRITE_ENTRY(laserBlue02 ,841 ,647 ,13 ,37),    // LASER_BLUE02
    SPRITE_ENTRY(laserBlue03 ,856 ,57  ,9  ,37),    // LASER_BLUE03
    SPRITE_ENTRY(laserBlue04 ,835 ,565 ,13 ,37),    // LASER_BLUE04
    SPRITE_ENTRY(laserBlue05 ,858 ,475 ,9  ,37),    // LASER_BLUE05
    SPRITE_ENTRY(laserBlue06 ,835 ,752 ,13 ,37),    // LASER_BLUE06
    SPRITE_ENTRY(laserBlue07 ,856 ,775 ,9  ,37),    // LASER_BLUE07
    SPRITE_ENTRY(laserBlue08 ,596 ,961 ,48 ,46),    // LASER_BLUE08
    SPRITE_ENTRY(laserBlue09 ,434 ,325 ,48 ,46),    // LASER_BLUE09
    SPRITE_ENTRY(laserBlue10 ,740 ,724 ,37 ,37),    // LASER_BLUE10
    SPRITE_ENTRY(laserBlue11 ,698 ,795 ,38 ,37),    // LASER_BLUE11
    SPRITE_ENTRY(laserBlue12 ,835 ,695 ,13 ,57),    // LASER_BLUE12
    SPRITE_ENTRY(laserBlue13 ,856 ,869 ,9  ,57),    // LASER_BLUE13
    SPRITE_ENTRY(laserBlue14 ,842 ,206 ,13 ,57),    // LASER_BLUE14
    SPRITE_ENTRY(laserBlue15 ,849 ,480 ,9  ,57),    // LASER_BLUE15
    SPRITE_ENTRY(laserBlue16 ,843 ,62  ,13 ,54),    // LASER_BLUE16
    
    SPRITE_ENTRY(laserGreen01 ,740 ,686 ,37 ,38),   // LASER_GREEN01
    SPRITE_ENTRY(laserGreen02 ,843 ,116 ,13 ,57),   // LASER_GREEN02
    SPRITE_ENTRY(laserGreen03 ,855 ,173 ,9  ,57),   // LASER_GREEN03
    SPRITE_ENTRY(laserGreen04 ,848 ,565 ,13 ,37),   // LASER_GREEN04
    SPRITE_ENTRY(laserGreen05 ,854 ,639 ,9  ,37),   // LASER_GREEN05
    SPRITE_ENTRY(laserGreen06 ,845 ,0   ,13 ,57),   // LASER_GREEN06
    SPRITE_ENTRY(laserGreen07 ,849 ,364 ,9  ,57),   // LASER_GREEN07
    SPRITE_ENTRY(laserGreen08 ,848 ,738 ,13 ,37),   // LASER_GREEN08
    SPRITE_ENTRY(laserGreen09 ,856 ,94  ,9  ,37),   // LASER_GREEN09
    SPRITE_ENTRY(laserGreen10 ,843 ,426 ,13 ,54),   // LASER_GREEN10
    SPRITE_ENTRY(laserGreen11 ,849 ,310 ,9  ,54),   // LASER_GREEN11
    SPRITE_ENTRY(laserGreen12 ,843 ,602 ,13 ,37),   // LASER_GREEN12
    SPRITE_ENTRY(laserGreen13 ,858 ,0   ,9  ,37),   // LASER_GREEN13
    SPRITE_ENTRY(laserGreen14 ,193 ,240 ,48 ,46),   // LASER_GREEN14
    SPRITE_ENTRY(laserGreen15 ,443 ,182 ,48 ,46),   // LASER_GREEN15
    SPRITE_ENTRY(laserGreen16 ,760 ,192 ,37 ,37),   // LASER_GREEN16

    SPRITE_ENTRY(laserRed01 ,858 ,230 ,9  ,54),     // LASER_RED01
    SPRITE_ENTRY(laserRed02 ,843 ,977 ,13 ,37),     // LASER_RED02
    SPRITE_ENTRY(laserRed03 ,856 ,602 ,9  ,37),     // LASER_RED03
    SPRITE_ENTRY(laserRed04 ,843 ,940 ,13 ,37),     // LASER_RED04
    SPRITE_ENTRY(laserRed05 ,856 ,983 ,9  ,37),     // LASER_RED05
    SPRITE_ENTRY(laserRed06 ,843 ,903 ,13 ,37),     // LASER_RED06
    SPRITE_ENTRY(laserRed07 ,856 ,131 ,9  ,37),     // LASER_RED07
    SPRITE_ENTRY(laserRed08 ,580 ,661 ,48 ,46),     // LASER_RED08
    SPRITE_ENTRY(laserRed09 ,602 ,600 ,48 ,46),     // LASER_RED09
    SPRITE_ENTRY(laserRed10 ,738 ,650 ,37 ,36),     // LASER_RED10
    SPRITE_ENTRY(laserRed11 ,737 ,613 ,37 ,37),     // LASER_RED11
    SPRITE_ENTRY(laserRed12 ,843 ,846 ,13 ,57),     // LASER_RED12
    SPRITE_ENTRY(laserRed13 ,856 ,812 ,9  ,57),     // LASER_RED13
    SPRITE_ENTRY(laserRed14 ,843 ,789 ,13 ,57),     // LASER_RED14
    SPRITE_ENTRY(laserRed15 ,856 ,926 ,9  ,57),     // LASER_RED15
    SPRITE_ENTRY(laserRed16 ,848 ,684 ,13 ,54),     // LASER_RED16

    SPRITE_ENTRY(meteorBrown_big1   ,224 ,664 ,101 ,84),    // METEOR_BROWN_BIG1
    SPRITE_ENTRY(meteorBrown_big2   ,0   ,520 ,120 ,98),    // METEOR_BROWN_BIG2
    SPRITE_ENTRY(meteorBrown_big3   ,518 ,810 ,89  ,82),    // METEOR_BROWN_BIG3
    SPRITE_ENTRY(meteorBrown_big4   ,327 ,452 ,98  ,96),    // METEOR_BROWN_BIG4
    SPRITE_ENTRY(meteorBrown_med1   ,651 ,447 ,43  ,43),    // METEOR_BROWN_MED1
    SPRITE_ENTRY(meteorBrown_med3   ,237 ,452 ,45  ,40),    // METEOR_BROWN_MED2
    SPRITE_ENTRY(meteorBrown_small1 ,406 ,234 ,28  ,28),    // METEOR_BROWN_SMALL1
    SPRITE_ENTRY(meteorBrown_small2 ,778 ,587 ,29  ,26),    // METEOR_BROWN_SMALL2
    SPRITE_ENTRY(meteorBrown_tiny1  ,346 ,814 ,18  ,18),    // METEOR_BROWN_TINY1
    SPRITE_ENTRY(meteorBrown_tiny2  ,399 ,814 ,16  ,15),    // METEOR_BROWN_TINY2
 
    SPRITE_ENTRY(meteorGrey_big1   ,224 ,748 ,101 ,84),     // METEOR_GREY_BIG1
    SPRITE_ENTRY(meteorGrey_big2   ,0   ,618 ,120 ,98),     // METEOR_GREY_BIG2
    SPRITE_ENTRY(meteorGrey_big3   ,516 ,728 ,89  ,82),     // METEOR_GREY_BIG3
    SPRITE_ENTRY(meteorGrey_big4   ,327 ,548 ,98  ,96),     // METEOR_GREY_BIG4
    SPRITE_ENTRY(meteorGrey_med1   ,674 ,219 ,43  ,43),     // METEOR_GREY_MED1
    SPRITE_ENTRY(meteorGrey_med2   ,282 ,452 ,45  ,40),     // METEOR_GREY_MED2
    SPRITE_ENTRY(meteorGrey_small1 ,406 ,262 ,28  ,28),     // METEOR_GREY_SMALL1
    SPRITE_ENTRY(meteorGrey_small2 ,396 ,413 ,29  ,26),     // METEOR_GREY_SMALL2
    SPRITE_ENTRY(meteorGrey_tiny1  ,364 ,814 ,18  ,18),     // METEOR_GREY_TINY1
    SPRITE_ENTRY(meteorGrey_tiny2  ,602 ,646 ,16  ,15),     // METEOR_GREY_TINY2
    
    SPRITE_ENTRY(numeral0 ,367 ,644  ,19 ,19),      // NUMERAL0
    SPRITE_ENTRY(numeral1 ,205 ,688  ,19 ,19),      // NUMERAL1
    SPRITE_ENTRY(numeral2 ,406 ,290  ,19 ,19),      // NUMERAL2
    SPRITE_ENTRY(numeral3 ,580 ,707  ,19 ,19),      // NUMERAL3
    SPRITE_ENTRY(numeral4 ,386 ,644  ,19 ,19),      // NUMERAL4
    SPRITE_ENTRY(numeral5 ,628 ,646  ,19 ,19),      // NUMERAL5
    SPRITE_ENTRY(numeral6 ,671 ,1002 ,19 ,19),      // NUMERAL6
    SPRITE_ENTRY(numeral7 ,690 ,1004 ,19 ,19),      // NUMERAL7
    SPRITE_ENTRY(numeral8 ,709 ,1004 ,19 ,19),      // NUMERAL8
    SPRITE_ENTRY(numeral9 ,491 ,215  ,19 ,19),      // NUMERAL9
    SPRITE_ENTRY(numeralX ,382 ,814  ,17 ,17),      // NUMERALX

    SPRITE_ENTRY(pill_blue   ,674 ,262 ,22 ,21),    // PILL_BLUE
    SPRITE_ENTRY(pill_green  ,573 ,989 ,22 ,21),    // PILL_GREEN
    SPRITE_ENTRY(pill_red    ,222 ,108 ,22 ,21),    // PILL_RED
    SPRITE_ENTRY(pill_yellow ,222 ,129 ,22 ,21),    // PILL_YEWLLOW
    
    SPRITE_ENTRY(playerLife1_blue   ,482 ,358 ,33 ,26), // PLAYER_LIFE1_BLUE
    SPRITE_ENTRY(playerLife1_green  ,535 ,150 ,33 ,26), // PLAYER_LIEF1_GREEN
    SPRITE_ENTRY(playerLife1_orange ,777 ,327 ,33 ,26), // PLAYER_LIFE1_ORANGE
    SPRITE_ENTRY(playerLife1_red    ,775 ,301 ,33 ,26), // PLAYER_LIFE1_RED,

    SPRITE_ENTRY(playerLife2_blue   ,465 ,991 ,37 ,26), // PLAYER_LIFE2_BLUE
    SPRITE_ENTRY(playerLife2_green  ,391 ,991 ,37 ,26), // PLAYER_LIFE2_GREEN
    SPRITE_ENTRY(playerLife2_orange ,428 ,991 ,37 ,26), // PLAYER_LIFE2_ORANGE
    SPRITE_ENTRY(playerLife2_red    ,502 ,991 ,37 ,26), // PLAYER_LIFE2_RED
    
    SPRITE_ENTRY(playerLife3_blue   ,777 ,385 ,32 ,26), // PLAYER_LIFE3_BLUE
    SPRITE_ENTRY(playerLife3_green  ,778 ,469 ,32 ,26), // PLAYER_LIFE3_GREEN
    SPRITE_ENTRY(playerLife3_orange ,777 ,712 ,32 ,26), // PLAYER_LIFE3_ORANGE
    SPRITE_ENTRY(playerLife3_red    ,777 ,443 ,32 ,26), // PLAYER_LIFE3_RED
    
    SPRITE_ENTRY(playerShip1_blue    ,211 ,941 ,99 ,75),    // PLAYER_SHIP1_BLUE
    SPRITE_ENTRY(playerShip1_green   ,237 ,377 ,99 ,75),    // PLAYER_SHIP1_GREEN
    SPRITE_ENTRY(playerShip1_orange  ,247 ,84  ,99 ,75),    // PLAYER_SHIP1_ORANGE
    SPRITE_ENTRY(playerShip1_red     ,224 ,832 ,99 ,75),    // PLAYER_SHIP1_RED
    SPRITE_ENTRY(playerShip1_damage1 ,112 ,941 ,99 ,75),    // PLAYER_SHIP1_DAMAGE1
    SPRITE_ENTRY(playerShip1_damage2 ,247 ,234 ,99 ,75),    // PALYER_SHIP1_DAMAGE2
    SPRITE_ENTRY(playerShip1_damage3 ,247 ,159 ,99 ,75),    // PLAYER_SHIP1_DAMAGE3

    SPRITE_ENTRY(playerShip2_blue    ,112 ,791 ,112 ,75),   // PLAYER_SHIP2_BLUE
    SPRITE_ENTRY(playerShip2_green   ,112 ,866 ,112 ,75),   // PLAYER_SHIP2_GREEN
    SPRITE_ENTRY(playerShip2_orange  ,112 ,716 ,112 ,75),   // PLAYER_SHIP2_ORANGE
    SPRITE_ENTRY(playerShip2_red     ,0   ,941 ,112 ,75),   // PLAYER_SHIP2_RED
    SPRITE_ENTRY(playerShip2_damage1 ,0   ,866 ,112 ,75),   // PLAYER_SHIP2_DAMAGE1
    SPRITE_ENTRY(playerShip2_damage2 ,0   ,791 ,112 ,75),   // PLAYER_SHIP2_DAMAGE2
    SPRITE_ENTRY(playerShip2_damage3 ,0   ,716 ,112 ,75),   // PLAYER_SHIP2_DAMAGE3

    SPRITE_ENTRY(playerShip3_blue    ,325 ,739 ,98 ,75),    // PLAYER_SHIP3_BLUE
    SPRITE_ENTRY(playerShip3_green   ,346 ,75  ,98 ,75),    // PLAYER_SHIP3_GREEN
    SPRITE_ENTRY(playerShip3_orange  ,336 ,309 ,98 ,75),    // PLAYER_SHIP3_ORANGE
    SPRITE_ENTRY(playerShip3_red     ,325 ,0   ,98 ,75),    // PLAYER_SHIP3_RED
    SPRITE_ENTRY(playerShip3_damage1 ,323 ,832 ,98 ,75),    // PLAYER_SHIP3_DAMAGE1
    SPRITE_ENTRY(playerShip3_damage2 ,310 ,907 ,98 ,75),    // PLAYER_SHIP3_DAMAGE2
    SPRITE_ENTRY(playerShip3_damage3 ,325 ,664 ,98 ,75),    // PLAYER_SHIP3_DAMAGE3

    SPRITE_ENTRY(powerupBlue        ,696 ,329 ,34 ,33),     // POWER_UP_BLUE
    SPRITE_ENTRY(powerupBlue_bolt   ,539 ,989 ,34 ,33),     // POWER_UP_BOLT
    SPRITE_ENTRY(powerupBlue_shield ,777 ,679 ,34 ,33),     // POWER_UP_SHIELD
    SPRITE_ENTRY(powerupBlue_star   ,776 ,895 ,34 ,33),     // POWER_UP_STAR

    SPRITE_ENTRY(powerupGreen        ,774 ,613 ,34 ,33),    // POWER_UP_GREEN
    SPRITE_ENTRY(powerupGreen_bolt   ,766 ,80  ,34 ,33),    // POWER_UP_GREEN_BOLT
    SPRITE_ENTRY(powerupGreen_shield ,776 ,862 ,34 ,33),    // POWER_UP_GREEN_SHIELD
    SPRITE_ENTRY(powerupGreen_star   ,651 ,490 ,34 ,33),    // POWER_UP_GREEN_STAR
    
    SPRITE_ENTRY(powerupRed         ,491 ,182 ,34 ,33),     // POWER_UP_RED
    SPRITE_ENTRY(powerupRed_bolt    ,775 ,646 ,34 ,33),     // POWER_UP_RED_BOLT
    SPRITE_ENTRY(powerupRed_shield  ,776 ,928 ,34 ,33),     // POWER_UP_RED_SHIELD
    SPRITE_ENTRY(powerupRed_star    ,774 ,977 ,34 ,33),     // POWER_UP_RED_STAR
    
    SPRITE_ENTRY(powerupYellow          ,774 ,761 ,34 ,33), // POWER_UP_YELLOW
    SPRITE_ENTRY(powerupYellow_bolt     ,740 ,761 ,34 ,33), // POWER_UP_YELLOW_BOLT
    SPRITE_ENTRY(powerupYellow_shield   ,482 ,325 ,34 ,33), // POWER_UP_YELLOW_SHIELD
    SPRITE_ENTRY(powerupYellow_star     ,607 ,857 ,34 ,33), // POWER_UP_YWLLOW_STAR
    
    SPRITE_ENTRY(scratch1 ,325 ,814 ,21 ,16),   // SCRATCH1
    SPRITE_ENTRY(scratch2 ,423 ,41  ,21 ,16),   // SCRATCH2
    SPRITE_ENTRY(scratch3 ,346 ,295 ,16 ,12),   // SCRATCH3
    
    SPRITE_ENTRY(shield1 ,0 ,412 ,133 ,108),    // SHIELD1
    SPRITE_ENTRY(shield2 ,0 ,293 ,143 ,119),    // SHIELD2
    SPRITE_ENTRY(shield3 ,0 ,156 ,144 ,137),    // SHIELD3
    
    SPRITE_ENTRY(shield_bronze ,797 ,143 ,30 ,30),  // SHIELD_BRONZE
    SPRITE_ENTRY(shield_gold   ,797 ,113 ,30 ,30),  // SHIELD_GOLD
    SPRITE_ENTRY(shield_silver ,778 ,824 ,30 ,30),  // SHIELD_SILVER

    SPRITE_ENTRY(speed ,858 ,284 ,7 ,108),  // SPEED

    SPRITE_ENTRY(star1 ,628 ,681 ,25 ,24),  // STAR1
    SPRITE_ENTRY(star2 ,222 ,84  ,25 ,24),  // STAR2
    SPRITE_ENTRY(star3 ,576 ,300 ,24 ,24),  // STAR3

    SPRITE_ENTRY(star_bronze ,778 ,794 ,31 ,30),    // STAR_BRONZE
    SPRITE_ENTRY(star_gold   ,778 ,557 ,31 ,30),    // STAR_GOLD
    SPRITE_ENTRY(star_silver ,778 ,527 ,31 ,30),    // STAR_SILVER

    SPRITE_ENTRY(things_bronze ,778 ,495 ,32 ,32),  // THINGS_BRONZE
    SPRITE_ENTRY(things_gold   ,777 ,411 ,32 ,32),  // THINGS_GOLD
    SPRITE_ENTRY(things_silver ,777 ,353 ,32 ,32),  // THINGS_SILVER

    SPRITE_ENTRY(turretBase_big   ,310 ,982 ,41 ,41),   // TURRET_BASE_BIG
    SPRITE_ENTRY(turretBase_small ,808 ,301 ,26 ,26),   // TURRET_BASE_SMALL

    SPRITE_ENTRY(ufoBlue   ,444 ,91  ,91 ,91),  // UFO_BLUE
    SPRITE_ENTRY(ufoGreen  ,434 ,234 ,91 ,91),  // UFO_GREEN
    SPRITE_ENTRY(ufoRed    ,444 ,0   ,91 ,91),  // UFO_RED
    SPRITE_ENTRY(ufoYellow ,505 ,898 ,91 ,91),  // UFO_YELLOW

    SPRITE_ENTRY(wingBlue_0 ,647 ,924 ,45 ,78),     // WING_BLUE0
    SPRITE_ENTRY(wingBlue_1 ,738 ,253 ,37 ,72),     // WING_BLUE1
    SPRITE_ENTRY(wingBlue_2 ,805 ,0   ,26 ,84),     // WING_BLUE2
    SPRITE_ENTRY(wingBlue_3 ,600 ,450 ,51 ,75),     // WING_BLUE3
    SPRITE_ENTRY(wingBlue_4 ,692 ,924 ,42 ,80),     // WING_BLUE4
    SPRITE_ENTRY(wingBlue_5 ,596 ,892 ,51 ,69),     // WING_BLUE5
    SPRITE_ENTRY(wingBlue_6 ,694 ,847 ,42 ,74),     // WING_BLUE6
    SPRITE_ENTRY(wingBlue_7 ,675 ,134 ,43 ,83),     // WING_BLUE7

    SPRITE_ENTRY(wingGreen_0 ,650 ,525 ,45 ,78),    // WING_GREEN0
    SPRITE_ENTRY(wingGreen_1 ,775 ,229 ,37 ,72),    // WING_GREEN1
    SPRITE_ENTRY(wingGreen_2 ,809 ,527 ,26 ,84),    // WING_GREEN2
    SPRITE_ENTRY(wingGreen_3 ,535 ,0   ,51 ,75),    // WING_GREEN3
    SPRITE_ENTRY(wingGreen_4 ,694 ,431 ,42 ,80),    // WING_GREEN4
    SPRITE_ENTRY(wingGreen_5 ,525 ,251 ,51 ,69),    // WING_GREEN5
    SPRITE_ENTRY(wingGreen_6 ,695 ,511 ,42 ,74),    // WING_GREEN6
    SPRITE_ENTRY(wingGreen_7 ,655 ,764 ,43 ,83),    // WING_GREEN7

    SPRITE_ENTRY(wingRed_0 ,809 ,712 ,26 ,84),      // WING_RED0
    SPRITE_ENTRY(wingRed_1 ,768 ,0   ,37 ,72),      // WING_RED1
    SPRITE_ENTRY(wingRed_2 ,600 ,300 ,51 ,75),      // WING_RED2
    SPRITE_ENTRY(wingRed_3 ,698 ,715 ,42 ,80),      // WING_RED3
    SPRITE_ENTRY(wingRed_4 ,586 ,75  ,51 ,69),      // WING_RED4
    SPRITE_ENTRY(wingRed_5 ,718 ,123 ,42 ,74),      // WING_RED5
    SPRITE_ENTRY(wingRed_6 ,653 ,681 ,43 ,83),      // WING_RED6
    SPRITE_ENTRY(wingRed_7 ,651 ,286 ,45 ,78),      // WING_RED7

    SPRITE_ENTRY(wingYellow_0 ,650 ,603 ,45 ,78),   // WING_YELLOW0
    SPRITE_ENTRY(wingYellow_1 ,760 ,120 ,37 ,72),   // WING_YELLOW1
    SPRITE_ENTRY(wingYellow_2 ,809 ,353 ,26 ,84),   // WING_YELLOW2
    SPRITE_ENTRY(wingYellow_3 ,576 ,150 ,51 ,75),   // WING_YELLOW3
    SPRITE_ENTRY(wingYellow_4 ,726 ,0   ,42 ,80),   // WING_YELLOW4
    SPRITE_ENTRY(wingYellow_5 ,525 ,182 ,51 ,69),   // WING_YELLOW5
    SPRITE_ENTRY(wingYellow_6 ,695 ,585 ,42 ,74),   // WING_YELLOW6
    SPRITE_ENTRY(wingYellow_7 ,651 ,364 ,43 ,83),   // WING_YELLOW7
};

#undef UV
#undef SPRITE_ENTRY

} // namespace

//-----------------------------------------------------------------------------
//      スプライトデータを取得します.
//-----------------------------------------------------------------------------
const SpriteData& GetSpriteData(SpriteKind kind)
{ return kSpriteData[kind]; }

