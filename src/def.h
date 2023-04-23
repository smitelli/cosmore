/**
 * Cosmore
 * Copyright (c) 2020-2023 Scott Smitelli and contributors
 *
 * Based on COSMO{1..3}.EXE distributed with "Cosmo's Cosmic Adventure"
 * Copyright (c) 1992 Apogee Software, Ltd.
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

/*****************************************************************************
 *               COSMORE HEADER for MISCELLANEOUS DEFINITIONS                *
 *                                                                           *
 * This file is a general catch-all for constants that don't fit anywhere    *
 * else. Most of these would be well-suited for conversion to enumerations,  *
 * but those use up more compiler memory which cannot be spared here.        *
 *****************************************************************************/

/*
Demo states. Indicates if a demo is being recorded, played back, or neither.
*/
#define DEMOSTATE_NONE          0
#define DEMOSTATE_RECORD        1
#define DEMOSTATE_PLAY          2

/*
Two-way direction systems. For actors that only move in one dimension, these
values can be negated (d = !d) to ping-pong back and forth.
*/
#define DIR2_SOUTH              0
#define DIR2_NORTH              1
#define DIR2_WEST               0
#define DIR2_EAST               1

/*
Four-way direction system. DIR4_NONE is defined in an attempt to make it a
little clearer that something is "directionless" as opposed to north.
*/
#define DIR4_NONE               0  /* Used by player bomb/cling */
#define DIR4_NORTH              0
#define DIR4_SOUTH              1
#define DIR4_WEST               2
#define DIR4_EAST               3

/*
Eight-way direction system.
*/
#define DIR8_STATIONARY         0
#define DIR8_NORTH              1
#define DIR8_NORTHEAST          2
#define DIR8_EAST               3
#define DIR8_SOUTHEAST          4
#define DIR8_SOUTH              5
#define DIR8_SOUTHWEST          6
#define DIR8_WEST               7
#define DIR8_NORTHWEST          8

/*
Partial eight-way direction system used only by projectile actors.
*/
#define DIRP_WEST               0
#define DIRP_SOUTHWEST          1
#define DIRP_SOUTH              2
#define DIRP_SOUTHEAST          3
#define DIRP_EAST               4

/*
Draw modes for sprite drawing functions. Unless otherwise stated, all of these
modes consider the x,y position to be somewhere on the map, and subject to the
viewable scroll position.
*/
#define DRAWMODE_NORMAL         0  /* draw unmodified, but in-front tiles can cover the sprite */
#define DRAWMODE_HIDDEN         1  /* don't draw under any circumstance */
#define DRAWMODE_WHITE          2  /* same as _NORMAL, but all non-transparent pixels are white */
#define DRAWMODE_TRANSLUCENT    3  /* same as _WHITE, but drawn semi-transparent */
#define DRAWMODE_FLIPPED        4  /* same as _NORMAL, but flipped vertically */
#define DRAWMODE_IN_FRONT       5  /* draw unmodified, covering _all_ tiles */
#define DRAWMODE_ABSOLUTE       6  /* draw unmodified, but x,y are screen coordinates */

/*
All memory offsets that reference data sored in the EGA's memory. These are all
relative to the EGA's segment address: 0xA000. This is the typical memory layout
while the game is running:

Segment | Offset | Size   | Content
--------|--------|--------|--------
0xA000  | 0x0000 | 0x1F40 | Display page 0
0xA000  | 0x1F40 | 0xC0   | (unused)
0xA000  | 0x2000 | 0x1F40 | Display page 1
0xA000  | 0x3F40 | 0xC0   | (unused)
0xA000  | 0x4000 | 0x3E80 | Solid tile images
0xA000  | 0x7E80 | 0x180  | (unused)
0xA000  | 0x8000 | 0x720  | Status bar tile images
0xA000  | 0x8720 | 0x1BE0 | (unused)
0xA000  | 0XA300 | 0x1680 | Backdrop 0_0 tile images
0xA000  | 0XB980 | 0x1680 | Backdrop 4_0 tile images
0xA000  | 0XD000 | 0x1680 | Backdrop 0_4 tile images
0xA000  | 0XE680 | 0x1680 | Backdrop 4_4 tile images

Note that due to the planar nature of the EGA's memory, each byte of address
space here represents four physical bytes of memory on the card. This game
*requires* a 256K EGA card to hold everything!
*/
#define EGA_OFFSET_SOLID_TILES  0x4000
#define EGA_OFFSET_STATUS_TILES 0x8000
#define EGA_OFFSET_BACKDROP_0_0 0xa300
#define EGA_OFFSET_BACKDROP_4_0 0xb980
#define EGA_OFFSET_BACKDROP_0_4 0xd000
#define EGA_OFFSET_BACKDROP_4_4 0xe680

/*
Game input return states. Usually continues the game loop, but also permits
quitting the game or restarting the game loop on a new level.
*/
#define GAME_INPUT_CONTINUE     0
#define GAME_INPUT_QUIT         1
#define GAME_INPUT_RESTART      2

/*
In-game help menu return states. Same idea as GAME_INPUT_*, except the values
don't match up.
*/
#define HELP_MENU_CONTINUE      0
#define HELP_MENU_RESTART       1
#define HELP_MENU_QUIT          2

/*
Joystick identifiers. This game doesn't use joystick B, but some functionality
for it is present.
*/
#define JOYSTICK_A              1
#define JOYSTICK_B              2

/*
Player/actor move states. Indicates if an attempted move is free to take,
blocked by map elements, or in need of special processing for sloped areas.
*/
#define MOVE_FREE               0
#define MOVE_BLOCKED            1
#define MOVE_SLOPED             2

/*
Pounce hint queueing state.
*/
#define POUNCE_HINT_UNSEEN      0
#define POUNCE_HINT_QUEUED      1
#define POUNCE_HINT_SEEN        2

/*
"Restore game" menu return states. Differentiates between successful loads,
failures, and user aborts.
*/
#define RESTORE_GAME_NOT_FOUND  0
#define RESTORE_GAME_SUCCESS    1
#define RESTORE_GAME_ABORT      2

/*
Size of the game window, not including the status bar and the one-tile black
border at the top/left/right edges of the screen.
*/
#define SCROLLW                 38
#define SCROLLH                 18
