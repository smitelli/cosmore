/**
 * Cosmore
 * Copyright (c) 2020-2021 Scott Smitelli
 *
 * Based on COSMO{1..3}.EXE distributed with "Cosmo's Cosmic Adventure"
 * Copyright (c) 1992 Apogee Software, Ltd.
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

/*****************************************************************************
 *                COSMORE HEADER for PC SPEAKER SOUND EFFECTS                *
 *                                                                           *
 * This file contains a list of every sound effect number available in the   *
 * game along with relevant constants.                                       *
 *****************************************************************************/

/*
Constant indicating the end of a piece of sound effect data.
*/
#define END_SOUND               WORD_MAX

/*
PC speaker sound effect types.
*/
#define SND_BIG_PRIZE           1
#define SND_PLAYER_JUMP         2
#define SND_PLAYER_LAND         3
#define SND_PLAYER_CLING        4
#define SND_PLAYER_HIT_HEAD     5
#define SND_PLAYER_POUNCE       6
#define SND_PLAYER_DEATH        7
#define SND_DOOR_UNLOCK         8
#define SND_SPIKES_MOVE         9
#define SND_EXPLOSION           10
#define SND_WIN_LEVEL           11
#define SND_BARREL_DESTROY_1    12
#define SND_PRIZE               13
#define SND_PLAYER_HURT         14
#define SND_FOOT_SWITCH_MOVE    15
#define SND_FOOT_SWITCH_ON      16
#define SND_ROAMER_GIFT         17
#define SND_DESTROY_SATELLITE   18
#define SND_PLAYER_FOOTSTEP     19
#define SND_PUSH_PLAYER         20
#define SND_DRIP                21
#define SND_PIPE_CORNER_HIT     22
#define SND_TRANSPORTER_ON      23
#define SND_SCOOTER_PUTT        24
#define SND_DESTROY_SOLID       25
#define SND_PROJECTILE_LAUNCH   26
#define SND_BIG_OBJECT_HIT      27
#define SND_NO_BOMBS            28
#define SND_PLACE_BOMB          29
#define SND_HINT_DIALOG_ALERT   30
#define SND_RED_JUMPER_JUMP     31
#define SND_RED_JUMPER_LAND     32
#define SND_BGHOST_EGG_CRACK    33
#define SND_BGHOST_EGG_HATCH    34
#define SND_SAW_BLADE_MOVE      35
#define SND_FIREBALL_LAUNCH     36
#define SND_OBJECT_HIT          37
#define SND_EXIT_MONSTER_OPEN   38
#define SND_EXIT_MONSTER_INGEST 39
#define SND_BEAR_TRAP_CLOSE     40
#define SND_PAUSE_GAME          41
#define SND_WEEEEEEEE           42
#define SND_JUMP_PAD_ROBOT      43
#define SND_TEXT_TYPEWRITER     44
#define SND_BONUS_STAGE         45
#define SND_SHARD_BOUNCE        46
#define SND_TULIP_LAUNCH        47
#define SND_NEW_GAME            48
#define SND_ROCKET_BURN         49
#define SND_SMASH               50
#define SND_HIGH_SCORE_DISPLAY  51
#define SND_HIGH_SCORE_SET      52
#define SND_IVY_PLANT_RISE      53
#define SND_FLAME_PULSE         54
#define SND_BOSS_DAMAGE         55
#define SND_BOSS_MOVE           56
#define SND_SPEECH_BUBBLE       57
#define SND_BABY_GHOST_JUMP     58
#define SND_BABY_GHOST_LAND     59
#define SND_THUNDER             60
#define SND_BARREL_DESTROY_2    61
#define SND_TULIP_INGEST        62
#define SND_PLANT_MOUTH_OPEN    63
#define SND_ENTERING_LEVEL_NUM  64
#define SND_BOSS_LAUNCH         65
