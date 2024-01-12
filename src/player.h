/**
 * Cosmore
 * Copyright (c) 2020-2024 Scott Smitelli and contributors
 *
 * Based on COSMO{1..3}.EXE distributed with "Cosmo's Cosmic Adventure"
 * Copyright (c) 1992 Apogee Software, Ltd.
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

/*****************************************************************************
 *                     COSMORE HEADER for PLAYER SPRITES                     *
 *                                                                           *
 * This file contains a list of every available player sprite frame.         *
 *****************************************************************************/

/*
Player sprite direction bases. To face the player in a certain direction, add
one of the base values here to one of the frame values (0..22) from the next
group.
*/
#define PLAYER_BASE_WEST        0
#define PLAYER_BASE_EAST        23

/*
Player sprite frames.
*/
#define PLAYER_WALK_1           0
#define PLAYER_WALK_2           1
#define PLAYER_WALK_3           2
#define PLAYER_WALK_4           3
#define PLAYER_STAND            4
#define PLAYER_LOOK_NORTH       5
#define PLAYER_LOOK_SOUTH       6
#define PLAYER_JUMP             7
#define PLAYER_FALL             8
#define PLAYER_CLING            9
#define PLAYER_CLING_OPPOSITE   10
#define PLAYER_CLING_NORTH      11
#define PLAYER_CLING_SOUTH      12
#define PLAYER_FALL_LONG        13
#define PLAYER_CROUCH           14
#define PLAYER_PAIN             15
#define PLAYER_FALL_SEVERE      16
#define PLAYER_PUSHED           17
#define PLAYER_STAND_BLINK      18
#define PLAYER_SHAKE_1          19
#define PLAYER_SHAKE_2          20
#define PLAYER_SHAKE_3          21
#define PLAYER_JUMP_LONG        22
#define PLAYER_DEAD_1           46
#define PLAYER_DEAD_2           47
#define PLAYER_HIDDEN           BYTE_MAX
