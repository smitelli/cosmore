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
 *                      COSMORE HEADER for LOWLEVEL.ASM                      *
 *                                                                           *
 * Provides constants and function prototypes for all public procedures in   *
 * lowlevel.asm.                                                             *
 *****************************************************************************/

/*
CPU types as returned by GetProcessorType().
*/
#define CPUTYPE_8088            0
#define CPUTYPE_8086            1
#define CPUTYPE_V20             2
#define CPUTYPE_V30             3
#define CPUTYPE_80188           4
#define CPUTYPE_80186           5
#define CPUTYPE_80286           6
#define CPUTYPE_80386           7

/*
Resets the EGA's bit mask to its default state. Allows writes to all eight pixel
positions in each written byte.
*/
#define EGA_BIT_MASK_DEFAULT() { outport(0x03ce, (0xff << 8) | 0x08); }

/*
Resets the EGA's read and write modes to their default state. This allows for
direct (i.e. non-latched) writes from the CPU.
*/
#define EGA_MODE_DEFAULT() { outport(0x03ce, (0x00 << 8) | 0x05); }

/*
Resets the EGA's map mask to its default state (allows writes to all four memory
planes), sets default read mode, and enables latched writes from the CPU.
*/
#define EGA_MODE_LATCHED_WRITE() { \
    outport(0x03c4, (0x0f << 8) | 0x02);  /* map mask: all planes active */ \
    outport(0x03ce, (0x01 << 8) | 0x05);  /* mode: default w/ latched write */ \
}

/*
Define the 16 default colors available in EGA "display mode 1" (aka the 200-line
CGA-compatible mode). Uses the COLORS enum from CONIO.H where possible; see the
comments for SetPaletteRegister() for information about the skip by 8.
*/
enum MODE1_COLORS {
    MODE1_BLACK = BLACK,
    MODE1_BLUE,
    MODE1_GREEN,
    MODE1_CYAN,
    MODE1_RED,
    MODE1_MAGENTA,
    MODE1_BROWN,
    MODE1_LIGHTGRAY,
    MODE1_DARKGRAY = DARKGRAY + 8,
    MODE1_LIGHTBLUE,
    MODE1_LIGHTGREEN,
    MODE1_LIGHTCYAN,
    MODE1_LIGHTRED,
    MODE1_LIGHTMAGENTA,
    MODE1_YELLOW,
    MODE1_WHITE
};

/*
Prototypes for public procedures.
*/
void SetVideoMode(word mode_num);
void SetBorderColorRegister(word color_value);
void SetPaletteRegister(word palette_index, word color_value);
void DrawSolidTile(word src_offset, word dst_offset);
void SelectDrawPage(word page_num);
void DrawSpriteTileTranslucent(byte *src, word x, word y);
void LightenScreenTileWest(word x, word y);
void LightenScreenTile(word x, word y);
void LightenScreenTileEast(word x, word y);
void SelectActivePage(word page_num);
void DrawSpriteTile(byte *src, word x, word y);
void DrawMaskedTile(byte *src, word x, word y);
void DrawSpriteTileFlipped(byte *src, word x, word y);
void DrawSpriteTileWhite(byte *src, word x, word y);
word GetProcessorType(void);
