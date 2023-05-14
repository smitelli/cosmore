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
 *                        COSMORE HEADER for GRAPHICS                        *
 *                                                                           *
 * This file contains a list of every image/tile number/offset used by the   *
 * game. There are hundreds of graphics tiles spread across different group  *
 * entry files, but only ones directly referenced in the game code are       *
 * included here.                                                            *
 *****************************************************************************/

/*
Useful constants pertaining to some of the magic numbers surrounding backdrops.
*/
#define BACKDROP_SIZE           23040
#define BACKDROP_SIZE_EGA_MEM   (BACKDROP_SIZE / 4)
#define BACKDROP_WIDTH          40
#define BACKDROP_HEIGHT         18

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
0xA000  | 0XA300 | 0x1680 | Backdrop tile images
0xA000  | 0XB980 | 0x1680 | Backdrop tile images (shifted for odd X)
0xA000  | 0XD000 | 0x1680 | Backdrop tile images (shifted for odd Y)
0xA000  | 0XE680 | 0x1680 | Backdrop tile images (shifted for odd X *and* Y)

Note that due to the planar nature of the EGA's memory, each byte of address
space here represents four physical bytes of memory on the card. This game
*requires* a 256K EGA card to hold everything!
*/
#define EGA_OFFSET_SOLID_TILES  0x4000
#define EGA_OFFSET_STATUS_TILES 0x8000
#define EGA_OFFSET_BDROP_EVEN   0xa300
#define EGA_OFFSET_BDROP_ODD_X  0xb980
#define EGA_OFFSET_BDROP_ODD_Y  0xd000
#define EGA_OFFSET_BDROP_ODD_XY 0xe680

/*
Font characters. Only ones directly referenced in the game code are included
here. The rest are calculated relative to these.
*/
#define FONT_UP_ARROW           0x0050
#define FONT_LOWER_BAR_0        0x0140
#define FONT_UPPER_BAR_0        0x0168
#define FONT_0                  0x0410
#define FONT_LOWER_A            0x0ac8
#define FONT_UPPER_BAR_1        0x0ed8
#define FONT_LOWER_BAR_1        0x0f00
#define FONT_BACKGROUND_GRAY    0x0f28

/*
Full-screen image types.
*/
#define IMAGE_PRETITLE          0
#define IMAGE_TITLE             1
#define IMAGE_CREDITS           2
#define IMAGE_BONUS             3
#define IMAGE_END               4
#define IMAGE_ONE_MOMENT        5
/* The next three are not images, but they're treated that way for caching. */
#define IMAGE_TILEATTR          0x1111
#define IMAGE_DEMO              0xfff1
#define IMAGE_NONE              0xffff

/*
Constant indicating the end of a palette animation array.
*/
#define END_ANIMATION           BYTE_MAX

/*
Defines the EGA palette index used as the "key" color for animations. This must
be one of the COLORS members, *not* MODE1_COLORS!
*/
#define PALETTE_KEY_INDEX       MAGENTA

/*
Palette animation types.
*/
#define PAL_ANIM_NONE           0
#define PAL_ANIM_LIGHTNING      1
#define PAL_ANIM_R_Y_W          2
#define PAL_ANIM_R_G_B          3
#define PAL_ANIM_MONO           4
#define PAL_ANIM_W_R_M          5
#define PAL_ANIM_EXPLOSIONS     6

/*
Map tile values. Values 0..15,999 are spaced in increments of 8 and reference
solid tiles. Tiles not mentioned directly in the game code are excluded. For
tiles that are meant to appear as part of a map, the name offers some hint about
how their tile attributes make them behave:
- Empty/Free: Tile does not impede player/actor movement in any way. It is
  essentially background decoration.
- Platform: Tile blocks southern movement, but all other directions are free.
  These tiles behave like conventional platforms.
- Block: Tile prevents all movement and, in some instances, can be clung to.
*/
#define TILE_EMPTY              0x0000  /* for lack of a better term, "air" */
#define TILE_INVISIBLE_PLATFORM 0x0048
#define TILE_STRIPED_PLATFORM   0x0050
#define TILE_SWITCH_FREE_1N     0x3d68  /* has no ("N") line at top */
#define TILE_SWITCH_BLOCK_1     0x3d88
#define TILE_SWITCH_BLOCK_2     0x3d90
#define TILE_SWITCH_BLOCK_3     0x3d98
#define TILE_SWITCH_BLOCK_4     0x3da0
#define TILE_SWITCH_FREE_1L     0x3da8  /* has light ("L") line at top */
#define TILE_DOOR_BLOCK         0x3dc8
#define TILE_BLUE_PLATFORM      0x3dd0
#define TILE_MYSTERY_BLOCK_NW   0x3df8
#define TILE_MYSTERY_BLOCK_NE   0x3e00
#define TILE_MYSTERY_BLOCK_SW   0x3e08
#define TILE_MYSTERY_BLOCK_SE   0x3e10
/* Subsequent tiles are for UI elements; tile attributes aren't relevant. */
#define TILE_WAIT_SPINNER_1     0x3e18
#define TILE_TXTFRAME_NORTHWEST 0x3e38
#define TILE_TXTFRAME_NORTHEAST 0x3e40
#define TILE_TXTFRAME_SOUTHWEST 0x3e48
#define TILE_TXTFRAME_SOUTHEAST 0x3e50
#define TILE_TXTFRAME_NORTH     0x3e58
#define TILE_TXTFRAME_SOUTH     0x3e60
#define TILE_TXTFRAME_WEST      0x3e68
#define TILE_TXTFRAME_EAST      0x3e70
#define TILE_DARK_GRAY          0x3e78
/* First masked tile value. */
#define TILE_MASKED_0           0x3e80  /* aka 16,000 */
