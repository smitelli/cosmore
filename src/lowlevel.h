/**
 * Copyright (c) 2020-2021 Scott Smitelli
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
Common EGA-related inline functions.
*/
#define EGA_CLEAR_BIT_MASK() { outport(0x03ce, 0xff08); }
#define EGA_MODE_DIRECT()    { outport(0x03ce, 0x0005); }
/* Enables writing to all map masks, sets mode to allow latched reads */
#define EGA_RESET()          { outport(0x03c4, 0x0f02); outport(0x03ce, 0x0105); }

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
