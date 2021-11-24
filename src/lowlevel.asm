;**
;* Cosmore
;* Copyright (c) 2020-2021 Scott Smitelli
;*
;* Based on COSMO{1..3}.EXE distributed with "Cosmo's Cosmic Adventure"
;* Copyright (c) 1992 Apogee Software, Ltd.
;*
;* This source code is licensed under the MIT license found in the LICENSE file
;* in the root directory of this source tree.
;*

;******************************************************************************
;*                    COSMORE LOW-LEVEL ASSEMBLY FUNCTIONS                    *
;*                                                                            *
;* The majority of the code here handles EGA setup, page flipping, and the    *
;* low-level drawing for individual 8x8 pixel tiles. There is also a CPU      *
;* detection routine at the end of the file.                                  *
;*                                                                            *
;* References:                                                                *
;* - [ECD]: IBM PC Hardware Reference Library - Enhanced Color Display,       *
;*   August 2, 1984                                                           *
;* - [EGA]: IBM PC Hardware Reference Library - Enhanced Graphics Adapter,    *
;*   August 2, 1984                                                           *
;* - [iAPX]: Intel iAPX 86/88, 186/188 User's Manual / Programmer's           *
;    Reference, 1985                                                          *
;* - [Smith]: CPUID by Bob Smith, PC Tech Journal Vol 4 No 4, April 1986      *
;* - [Tischer]: PC System Programming for Developers by Michael Tischer, 1988 *
;******************************************************************************

; This game uses EGA mode Dh's default palette with a few rare exceptions. This
; palette has several useful properties: Bits 0,1,2,3 in the palette index
; correspond directly to memory planes 0,1,2,3 and to bits 0,1,2,4 in the color
; value. When shown on screen, each bit, regardless of the context, affects the
; visible B,G,R,I channels in a predictable way. (See the documentation for
; the SetPaletteRegister procedure for an explanation of why the color value
; bits are not contiguous.)
;
;   Palette | Plane Number/Channel  || Color |
;    Index  | 3/I | 2/R | 1/G | 0/B || Value | Color Name
;   --------+-----+-----+-----+-----++-------+-----------
;         0 |   0 |   0 |   0 |   0 ||     0 | Black
;         1 |   0 |   0 |   0 |   1 ||     1 | Blue
;         2 |   0 |   0 |   1 |   0 ||     2 | Green
;         3 |   0 |   0 |   1 |   1 ||     3 | Cyan
;         4 |   0 |   1 |   0 |   0 ||     4 | Red
;         5 |   0 |   1 |   0 |   1 ||     5 | Magenta
;         6 |   0 |   1 |   1 |   0 ||     6 | Brown
;         7 |   0 |   1 |   1 |   1 ||     7 | Dark White (Light Gray)
;         8 |   1 |   0 |   0 |   0 ||    16 | Bright Black (Dark Gray)
;         9 |   1 |   0 |   0 |   1 ||    17 | Bright Blue
;        10 |   1 |   0 |   1 |   0 ||    18 | Bright Green
;        11 |   1 |   0 |   1 |   1 ||    19 | Bright Cyan
;        12 |   1 |   1 |   0 |   0 ||    20 | Bright Red
;        13 |   1 |   1 |   0 |   1 ||    21 | Bright Magenta
;        14 |   1 |   1 |   1 |   0 ||    22 | Bright Yellow
;        15 |   1 |   1 |   1 |   1 ||    23 | Bright White

IDEAL
ASSUME cs:_TEXT, ds:DGROUP, es:NOTHING

INCLUDE "lowlevel.equ"

SEGMENT _DATA
;
; Global storage in the data segment.
;
EXTRN _yOffsetTable:WORD:25     ; Maps tile Y coordinate to EGA memory offset
yOffsetTable EQU _yOffsetTable  ; Defined in game2.c
ENDS

SEGMENT _TEXT
;
; Global storage in the code segment.
;
drawPageNumber  dw 0            ; Most recent SelectDrawPage call argument
drawPageSegment dw EGA_SEGMENT  ; EGA memory segment to be written to

; Subsequent instructions can use 80286 opcodes if desired, as that's the
; minimum supported CPU in-game.
P286

;
; Select the Map Mask [EGA, pg. 20] via the Sequencer Address Register [EGA, pg.
; 18].
;
MACRO SELECT_EGA_SEQ_MAP_MASK
        mov   dx,SEQUENCER_ADDR
        mov   al,SEQ_MAP_MASK
        out   dx,al
ENDM

;
; Set the Color Don't Care [EGA, pg. 53] bits for all four color planes via the
; Graphics 1 & 2 Address Register [EGA, pg. 46] using the provided `mask` value.
;
; The bits in `mask` refer to planes 3-2-1-0. Two different byte-sized registers
; are being written with a single word-sized OUT; the high byte goes to the
; GRAPHICS_DATA I/O port.
;
MACRO SET_EGA_COLOR_DONT_CARE mask
        mov   dx,GRAPHICS_1_2_ADDR
        mov   ax,(mask SHL 8) OR GFX_COLOR_DONT_CARE
        out   dx,ax
ENDM

;
; Load a `mask` into the Bit Mask Register [EGA, pg. 54] via the Graphics 1 & 2
; Address Register [EGA, pg. 46].
;
; Each bit in `mask` refers to one screen pixel positon. Two different byte-
; sized registers are being written with a single word-sized OUT; the high byte
; goes to the GRAPHICS_DATA I/O port.
;
MACRO SET_EGA_BIT_MASK mask
        mov   dx,GRAPHICS_1_2_ADDR
        mov   ax,(mask SHL 8) OR GFX_BIT_MASK
        out   dx,ax
ENDM

;
; Load a `mask` into the Map Mask [EGA, pg. 20] via the Sequencer Address
; Register [EGA, pg. 18].
;
; The bits in `mask` refer to planes 3-2-1-0. Two different byte-sized registers
; are being written with a single word-sized OUT; the high byte goes to the
; SEQUENCER_DATA I/O port.
;
MACRO SET_EGA_MAP_MASK mask
        mov   dx,SEQUENCER_ADDR
        mov   ax,(mask SHL 8) OR SEQ_MAP_MASK
        out   dx,ax
ENDM

;
; Load `map` into the Read Map Select register [EGA, pg. 50] via the Graphics 1
; & 2 Address Register [EGA, pg. 46] to select one of the four color planes
; using decimal notation.
;
; Plane numbers range from 0 (blue) to 3 (intensity). Two bytes are sent in one
; word OUT; the high byte goes to the GRAPHICS_DATA I/O port.
;
MACRO SET_EGA_READ_MAP map
        mov   dx,GRAPHICS_1_2_ADDR
        mov   ax,(map SHL 8) OR GFX_READ_MAP_SELECT
        out   dx,ax
ENDM

;
; Load a `mode` byte into the Mode register [EGA, pg. 50] via the Graphics 1 & 2
; Address Register [EGA, pg. 46].
;
; Two different byte- sized registers are being written with a single word-sized
; OUT; the high byte goes to the GRAPHICS_DATA I/O port.
;
MACRO SET_EGA_GC_MODE mode
        mov   dx,GRAPHICS_1_2_ADDR
        mov   ax,(mode SHL 8) OR GFX_MODE
        out   dx,ax
ENDM

;
; Set the video mode to the specified mode number and initialize the EGA card.
;
; mode_num (word): The BIOS video mode to enter. Only the low byte is used.
; Returns: Nothing
; Registers destroyed: AX, DX
;
PROC _SetVideoMode FAR @@mode_num:WORD
        PUBLIC _SetVideoMode
        push  bp
        mov   bp,sp

        ; Change video mode via BIOS video service interrupt [EGA, pg. 104].
        mov   ax,[@@mode_num]
        mov   ah,VSVC_SET_VIDEO_MODE
        int   INT_VIDEO_SERVICE

        ; Turn *on* Color Don't Care for all four color planes. This appears to
        ; be an instance where the IBM documentation is either confusing or
        ; flat-out incorrect -- bit value 1 means the Color Compare register is
        ; used during applicable memory read operations, and value 0 means the
        ; Color Compare register is ignored. This only affects memory read
        ; operations when Read Mode = 1, and the only place these types of reads
        ; occur is in the DrawSpriteTileWhite procedure.
        SET_EGA_COLOR_DONT_CARE 0000b

        ; Pre-select the Map Mask, but don't actually write anything to it yet.
        ; This probably isn't needed -- _usually_ any further changes to the map
        ; mask are accompanied by a re-select of this register.
        SELECT_EGA_SEQ_MAP_MASK

        pop   bp
        ret
ENDP

;
; Load the video border (overscan) register with the specified color value.
;
; See the documentation for the SetPaletteRegister procedure for a detailed
; description of the color value.
;
; color_value (word): The color value to program into the register (0..63). Only
;     the low byte is used.
; Returns: Nothing
; Registers destroyed: AX, BX
;
PROC _SetBorderColorRegister FAR @@color_value:WORD
        PUBLIC _SetBorderColorRegister
        push  bp
        mov   bp,sp

        ; Change border color via BIOS video service interrupt [EGA, pg. 104].
        mov   ah,VSVC_PALETTE_REGISTERS
        mov   al,PALREG_SET_BORDER_COLOR
        mov   bx,[@@color_value]
        mov   bh,bl             ; Value in BL is not used again
        int   INT_VIDEO_SERVICE

        pop   bp
        ret
ENDP

;
; Load one video palette register with the specified color value.
;
; The color value is a byte with the highest two bits unused. The exact meaning
; of the bits changes depending on the video mode and the type of display
; connected to the adapter. IBM had three different display units that used
; similar 9-pin connectors:
; - IBM Monochrome Display
; - IBM Color Display
; - IBM Enhanced Color Display
;
; The three display types were electrically interchangeable, and the EGA
; hardware was capable of driving any of the three displays provided it was set
; to a video mode that was supported by that display. Since the signal at each
; pin could be either on or off (with no intermediate levels), the two signal
; pins on the Monochrome Display supported 4 different shades of mono, the four
; signal pins on the Color Display supported 16 colors, and the six signal pins
; on the Enhanced Color Display supported 64 colors. There is a direct mapping
; between the individual bits in a color value and the signals on the display
; connector pins, as shown on the abbreviated pinouts [ECD, pg. 6]:
;
;   Pin | Monochrome  | Color         | Enhanced Color  | Color Value Bit
;   ----+-------------+---------------+-----------------+----------------
;     5 | --          | Blue          | Blue            | 00.....X
;     4 | --          | Green         | Green           | 00....X.
;     3 | --          | Red           | Red             | 00...X..
;     7 | Mono Signal | -- (Reserved) | Blue Intensity  | 00..X...
;     6 | Intensity   | Intensity     | Green Intensity | 00.X....
;     2 | -- (Ground) | -- (Ground)   | Red Intensity   | 00X.....
;
; NOTE: The base Red/Green/Blue signals contribute 67% of the power of a color
; channel, and the corresponding Intensity signal(s) provide the remaining 33%.
; Some sources refer to the Intensity signals as "least significant" due to the
; amount of output they generate, but this is confusing from the perspective of
; color value bit packing and we will never say it that way here.
;
; The Enhanced Color Display supported two modes [ECD, pg. 3]:
; - Mode 1: 200 lines, 15.75 kHz HSync rate, positive VSync pulse
; - Mode 2: 350 lines, 21.8 kHz HSync rate, negative VSync pulse
;
; The display used the polarity of the Vertical Sync pulse to determine which
; mode to use. In display mode 2, all six color inputs contributed to the
; picture and 64 colors were available for use. In mode 1, the Red Intensity and
; Blue Intensity inputs to the display were ignored, and the Green Intensity
; signal was applied across all three color channels [ECD, pg. 4]. This limited
; the display to 16 colors using the same RGBI pinout the Color Display used.
; Mode 1 also emulated the Color Display's handling of the color brown: When
; RGBI 1100 was received, the display fudged *just* the Green Intensity bit on
; to avoid an unpleasant dark yellow [ECD, pg. 4].
;
; This game uses video mode Dh exclusively -- a 200-line mode -- so there is
; only one intensity bit that can visibly change the screen color and it is not
; contiguous with the R/G/B bits. The 64 color values are really 16 distinct
; display colors, each repeated four times. The effective bit positions are:
;   Bits     | Meaning
;   ---------+--------
;   .......X | Blue
;   ......X. | Green
;   .....X.. | Red
;   ....0... | Not Used
;   ...X.... | Intensity
;   000..... | Not Used
;
; A silver lining to this is that, with the default EGA palette loaded, the bits
; in a palette register index correspond to bits in the color value, which match
; the RGBI signals being shown on the display. This is what allows EGA memory
; planes 0123 to be thought of as BGRI instead. If the sequence were changed or
; custom-mixed colors were used, this reasonable mental model would fall apart.
;
; palette_index (word): The palette register index (0..15). Only the low byte is
;     used.
; color_value (word): The color value to program into the register (0..63). Only
;     the low byte is used.
; Returns: Nothing
; Registers destroyed: AX, BX
;
PROC _SetPaletteRegister FAR @@palette_index:WORD, @@color_value:WORD
        PUBLIC _SetPaletteRegister
        push  bp
        mov   bp,sp

        ; Change one palette register via BIOS video service interrupt [EGA, pg.
        ; 105]. Two byte-sized subfunction numbers are being loaded in one MOV
        ; into AX.
        mov   ax,(VSVC_PALETTE_REGISTERS SHL 8) OR PALREG_SET_ONE_COLOR
        mov   bl,[BYTE PTR @@palette_index]
        mov   bh,[BYTE PTR @@color_value]
        int   INT_VIDEO_SERVICE

        pop   bp
        ret
ENDP

;
; Draw a single 8x8 pixel solid tile to the current draw page.
;
; This procedure draws non-transparent tiles that are part of the game map and
; backdrops. It also draws solid backgrounds for UI elements.
;
; Source data is read from EGA memory at SOLID_TILE_SEGMENT:src_offset.
; The destination address is drawPageSegment:dest_offset.
; The EGA *must* be in latched write mode for this to work correctly.
;
; Each tile is an 8x8 pixel square. Each 8-pixel tile row occupies one byte of
; EGA address space (1 bit per pixel), for a total of 8 bytes per tile. These 8
; bytes are stored sequentially in the source memory, or at 40-byte intervals
; in the destination memory.
;
; Within the EGA, each memory read/write operation is quadrupled across the four
; color planes. Although only 8 bytes of address space are handled during each
; call to this procedure, 32 bytes of physical memory are copied internally.
;
; src_offset (word): Memory offset of the source tile. This value should always
;     be a multiple of 8.
; dst_offset (word): Memory offset to write to, relative to the current draw
;     page segment.
; Returns: Nothing
; Registers destroyed: AL, DX, ES
;
PROC _DrawSolidTile FAR @@src_offset:WORD, @@dst_offset:WORD
        PUBLIC _DrawSolidTile
        push  bp
        mov   bp,sp
        push  ds
        push  si
        push  di

        ; Set up source and destination pointers from the arguments:
        ;   DS:SI <- Source tile data address (in EGA memory)
        ;   ES:DI <- Destination draw page address (in EGA memory)
        mov   dx,[drawPageSegment]
        mov   es,dx
        mov   dx,SOLID_TILE_SEGMENT
        mov   ds,dx
        ASSUME ds:NOTHING
        mov   si,[@@src_offset]
        mov   di,[@@dst_offset]

        ; Draw eight rows of tile pixels. All four color planes are copied, in
        ; parallel, through the EGA's internal latches. The memory read/write
        ; cycles are doing the actual work here -- the value in AL has no effect
        ; on the visual result. Latched write mode must be enabled here!
        srcoff = 0
        dstoff = 0
REPT 8
        mov   al,[si+srcoff]
        mov   [es:di+dstoff],al
        srcoff = srcoff + 1
        dstoff = dstoff + SCREEN_Y_STRIDE
ENDM

        pop   di
        pop   si
        pop   ds
        ASSUME ds:DGROUP
        pop   bp
        ret
ENDP

;
; Recalculate the drawPageSegment address from the current drawPageNumber.
;
; In all typical cases:
;
;   PgNum | Segment
;   ------+--------
;   0     | A000h
;   1     | A200h
;
; Returns: Nothing
; Registers destroyed: AX, BX
;
PROC UpdateDrawPageSegment FAR
        push  bp
        mov   bp,sp
        push  ds

        ; drawPageSegment = EGA_SEGMENT + (drawPageNumber * 200h)
        mov   ax,[drawPageNumber]
        xchg  ah,al
        shl   ah,1
        mov   bx,EGA_SEGMENT
        add   bx,ax
        mov   [drawPageSegment],bx

        pop   ds
        pop   bp
        ret
ENDP

;
; Select the video page where subsequent writes should occur.
;
; Although higher page numbers are accepted and will store a sensible result,
; pages beyond number 1 are used for tile storage and writing to them will
; corrupt the graphics.
;
; page_num (word): Page number to write to (0..1).
; Returns: Nothing
; Registers destroyed: AX, and those destroyed by UpdateDrawPageSegment
;
PROC _SelectDrawPage FAR @@page_num:WORD
        PUBLIC _SelectDrawPage
        push  bp
        mov   bp,sp

        ; Stash the selected draw page number
        mov   ax,[@@page_num]
        mov   [drawPageNumber],ax

        ; Update the segment address that the page number refers to
        call  UpdateDrawPageSegment

        pop   bp
        ret
ENDP

;
; Draw a single 8x8 pixel sprite tile, with translucency, to the draw page.
;
; The translucency effect is created by turning the "intensity" bit on for every
; pixel covered by the sprite, while leaving uncovered pixels alone. Visually
; this causes dark colors to lighten, and light colors to stay the same. The
; other color planes do not change.
;
; NOTE: This color effect fundamentally changes the pixels, meaning that
; "magenta" areas become "bright magenta" and no longer match for palette
; animation purposes.
;
; The tile data contains five color planes -- mask, blue, green, red, and
; intensity -- with 1-bit color depth on each plane. This procedure only deals
; with mask, ignoring the content of the other four planes. Tile image data is
; stored byte-planar in MBGRI order. Transparent areas have their mask bits set
; to 1.
;
; The basic theory of operation is as follows. For each pixel row in the tile:
; 1. Read a mask byte from the source data. Each bit maps to one screen pixel.
; 2. Invert the mask data bits to match the EGA's expectations.
; 3. Load the mask data into the EGA bit mask register. For each 0 bit in the
;    bit mask register, the EGA will ignore the data sent by the processor in
;    subsequent memory write operations and instead use the contents of a latch.
; 4. Load the map mask register with the binary value 1000, indicating that we
;    only want the writes to apply to the intensity plane. All other color
;    planes will remain unaffected.
; 5. Read the EGA memory to freshen the content of the latches, then write
;    binary value 11111111 back to the EGA memory, invoking a refresh of every
;    pixel on the row. Pixels that belong to a transparent area will be masked
;    off by the bit mask register and will not change. Planes that are not
;    intensity (i.e. RGB) will be masked off by the map mask register and will
;    not change either. The net result is that the intensity bit will be turned
;    on for every pixel belonging to an opaque part of the tile, while
;    transparent areas retain their previous value.
; 6. Advance the read position by a 5-byte step, and advance the write position
;    by a 40-byte step, in preparation for the next pixel row.
;
; Intersting tidbit: The written value in step 5 could just as easily be changed
; to all zeros, in which case the intensity bit would be turned *off* for every
; opaque pixel, leading to a darkening of light areas. Try it someday; the
; effect is pleasant.
;
; src (far pointer): Memory address of the first byte of tile data to read.
; x (word): X-position on the screen, in tiles. (0..39, leftmost column is 0)
; y (word): Y-position on the screen, in tiles. (0..24, topmost row is 0)
; Returns: Nothing
; Registers destroyed: AX, BX, CX, DX, ES
;
PROC _DrawSpriteTileTranslucent FAR @@src:FAR PTR, @@x:WORD, @@y:WORD
        PUBLIC _DrawSpriteTileTranslucent
        push  bp
        mov   bp,sp
        push  ds
        push  di
        push  si

        ; Set up source and destination pointers from the arguments:
        ;   DS:SI <- Source tile data address (could be anywhere in memory)
        ;   ES:BX <- Destination draw page address (in EGA memory)
        lds   si,[@@src]
        ASSUME ds:NOTHING
        mov   dx,[drawPageSegment]
        mov   es,dx
        ; NOTE: This procedure eschews the global yOffsetTable in favor of
        ; calculating the destination address directly. That may have been a
        ; missed optimization, as this drawing procedure is seldom used in the
        ; game. The following instructions are effectively doing:
        ;   BX = (y * 320) + x
        ; using shifts/adds instead of multiplication. An alternative:
        ;   mov   di,[@@y]
        ;   shl   di,1
        ;   mov   bx,[yOffsetTable+di]
        ;   add   bx,[@@x]
        ; is equivalent to:
        ;   BX = yOffsetTable[y] + x
        mov   bx,[@@y]
        mov   ax,bx
        shl   ax,1
        shl   ax,1
        add   ax,bx
REPT 6
        shl   ax,1
ENDM
        mov   bx,[@@x]
        add   bx,ax

        ; Pre-select the Map Mask before the loop is entered.
        SELECT_EGA_SEQ_MAP_MASK

        ; Draw eight rows of tile pixels.
        mov   cx,8
@@do_row:
        ; Read one byte of mask data (8 bits for an 8-pixel row) from the source
        ; memory. Write the mask data back into the EGA Bit Mask Register [EGA,
        ; pg. 54] via the Graphics 1 & 2 Address Register [EGA, pg. 46] to
        ; disallow changes to pixels that are not part of the sprite's shape.
        ; Two different byte-sized registers are being written with a single
        ; word-sized OUT; the high byte goes to the GRAPHICS_DATA I/O port.
        mov   dx,GRAPHICS_1_2_ADDR
        lodsb
        not   al                ; Tile data stores mask opposite to how EGA does
        mov   ah,GFX_BIT_MASK
        xchg  ah,al
        out   dx,ax

        ; Program the map mask (which was selected before this loop was entered)
        ; to only operate on plane 3 -- the intensity bit in the default game
        ; palette.
        mov   dx,SEQUENCER_DATA
        mov   al,1000b          ; Intensity -- bits are planes 3210
        out   dx,al

        ; Since the tile data stores plane bytes in MBGRI order, but we only
        ; care about mask, we must advance SI an additional 4 bytes in order for
        ; the next iteration to read the correct plane. In the middle of that,
        ; we also read and then write back a byte of video memory to actually
        ; commit the changes that were previously set up. Each memory bit gets
        ; set to 1 if it satisfies the mask conditions, otherwise the written
        ; bit is ignored and the latched value is used in its place.
        inc   si
        mov   al,11111111b      ; All 8 pixels of the row are candidates
        xchg  al,[es:bx]        ; Result in AL is not used again
        inc   si
        inc   si
        inc   si

        ; Advance the destination write position, then do the next row.
        add   bx,SCREEN_Y_STRIDE
        loop  @@do_row

        pop   si
        pop   di
        pop   ds
        ASSUME ds:DGROUP
        pop   bp
        ret
ENDP

;
; Lighten the right half of a single 8x8 pixel screen tile.
;
; Applies the following pattern to the selected screen tile's contents:
;   .......@
;   ......@@
;   .....@@@
;   ....@@@@
;   ...@@@@@
;   ..@@@@@@
;   .@@@@@@@
;   @@@@@@@@
; where pixels marked `.` do not change and pixels marked `@` have their
; "intensity" bit unconditionally turned on. Visually this causes dark colors to
; lighten, and light colors to stay the same. The other color planes do not
; change. This is usually applied to the "west" (i.e. left) edge of light beams
; in maps.
;
; NOTE: This assumes port 3C4h has been set to 2h (sequencer index = map mask).
; This is the only state that the game ever leaves the register in, but it's an
; unsafe assumption.
;
; NOTE: This color effect fundamentally changes the pixels, meaning that
; "magenta" areas become "bright magenta" and no longer match for palette
; animation purposes.
;
; The basic theory of operation is as follows. For each pixel row in the tile:
; 1. Select a new mask. The mask follows a pattern of 1 bits shifting in from
;    the right while 0 bits shift out of the left.
; 2. Load the mask into the EGA bit mask register. For each 0 bit in the bit
;    mask register, the EGA will ignore the data sent by the processor in
;    subsequent memory write operations and instead use the contents of a latch.
; 3. Read the EGA memory to freshen the content of the latches, then write
;    binary 1's back to the EGA memory. Pixels that were masked off by the bit
;    mask register will not change. Planes that are not intensity (i.e. RGB)
;    will be masked off by the map mask register and will not change either.
;    The net result is that the intensity bit will be turned on for every pixel
;    covered by a "light" region of the mask, while uncovered areas retain their
;    previous value.
; 4. Advance the write position by a 40-byte step in preparation for the next
;    pixel row.
;
; x (word): X-position on the screen, in tiles. (0..39, leftmost column is 0)
; y (word): Y-position on the screen, in tiles. (0..24, topmost row is 0)
; Returns: Nothing
; Registers destroyed: AX, BX, DX, ES
;
PROC _LightenScreenTileWest FAR @@x:WORD, @@y:WORD
        PUBLIC _LightenScreenTileWest
        push  bp
        mov   bp,sp
        push  ds
        push  di
        push  si

        ; Set up destination pointer from the arguments:
        ;   ES:BX <- Destination draw page address (in EGA memory)
        mov   dx,[drawPageSegment]
        mov   es,dx
        mov   di,[@@y]
        shl   di,1
        mov   bx,[yOffsetTable+di]
        add   bx,[@@x]

        ; Program the EGA Map Mask [EGA, pg. 20] to only operate on plane 3 --
        ; the intensity bit in the default game palette. NOTE: This is making an
        ; *unsafe* assumption that the value SEQ_MAP_MASK has been previously
        ; loaded into the SEQUENCER_ADDR I/O port. The assumption seems to not
        ; cause issues in the game.
        mov   dx,SEQUENCER_DATA
        mov   al,1000b          ; Intensity -- bits are planes 3210
        out   dx,al

        ; Redraw eight rows of tile pixels. Each iteration uses a new bit mask
        ; loaded into the Bit Mask Register [EGA, pg. 54] via the Graphics 1 & 2
        ; Address Register [EGA, pg. 46] which progressively lightens more and
        ; more of each row. Two different byte-sized registers are being written
        ; with a single word-sized OUT; the high byte goes to the GRAPHICS_DATA
        ; I/O port.
        mov   dx,GRAPHICS_1_2_ADDR
        dstoff = 0
IRP mask,<00000001b,00000011b,00000111b,00001111b,00011111b,00111111b,01111111b,11111111b>
        mov   ax,(mask SHL 8) OR GFX_BIT_MASK
        out   dx,ax

        ; Read and then write back a byte of video memory to actually commit the
        ; changes that were previously set up. Each memory bit gets set to 1 if
        ; it satisfies the mask conditions, otherwise the written bit is ignored
        ; and the latched value is used in its place. The precise value written
        ; doesn't matter as long as there is a 1 bit set for every pixel that
        ; needs lightening. The mask left in AH conveniently has this property.
        xchg  ah,[es:bx+dstoff]  ; Result in AH is not used again

        ; Advance the destination write position, then the block repeats.
        dstoff = dstoff + SCREEN_Y_STRIDE
ENDM

        pop   si
        pop   di
        pop   ds
        pop   bp
        ret
ENDP

;
; Lighten the full area of a single 8x8 pixel screen tile.
;
; Every pixel in the specified tile has its "intensity" bit unconditionally
; turned on. Visually this causes dark colors to lighten, and light colors to
; stay the same. The other color planes do not change. This is usually applied
; to the main body of light beams in maps.
;
; NOTE: This assumes port 3C4h has been set to 2h (sequencer index = map mask).
; This is the only state that the game ever leaves the register in, but it's an
; unsafe assumption.
;
; NOTE: This color effect fundamentally changes the pixels, meaning that
; "magenta" areas become "bright magenta" and no longer match for palette
; animation purposes.
;
; x (word): X-position on the screen, in tiles. (0..39, leftmost column is 0)
; y (word): Y-position on the screen, in tiles. (0..24, topmost row is 0)
; Returns: Nothing
; Registers destroyed: AX, BX, DX, ES
;
PROC _LightenScreenTile FAR @@x:WORD, @@y:WORD
        PUBLIC _LightenScreenTile
        push  bp
        mov   bp,sp
        push  ds
        push  di
        push  si

        ; Set up destination pointer from the arguments:
        ;   ES:BX <- Destination draw page address (in EGA memory)
        mov   dx,[drawPageSegment]
        mov   es,dx
        mov   di,[@@y]
        shl   di,1
        mov   bx,[yOffsetTable+di]
        add   bx,[@@x]

        ; Load a mask containing all 1 bits into the Bit Mask Register,
        ; permitting writes to every pixel in the row.
        SET_EGA_BIT_MASK 11111111b

        ; Program the EGA Map Mask [EGA, pg. 20] to only operate on plane 3 --
        ; the intensity bit in the default game palette. NOTE: This is making an
        ; *unsafe* assumption that the value SEQ_MAP_MASK has been previously
        ; loaded into the SEQUENCER_ADDR I/O port. The assumption seems to not
        ; cause issues in the game.
        mov   dx,SEQUENCER_DATA
        mov   al,1000b          ; Intensity -- bits are planes 3210
        out   dx,al

        ; Redraw eight rows of tile pixels. Since no part of the row is masked
        ; off, there is no need to set up the latches by reading first. All of
        ; the pixels on each row of the intensity plane are set to a binary 1.
        mov   al,11111111b
        dstoff = 0
REPT 8
        mov   [es:bx+dstoff],al

        ; Advance the destination write position, then the block repeats.
        dstoff = dstoff + SCREEN_Y_STRIDE
ENDM

        pop   si
        pop   di
        pop   ds
        pop   bp
        ret
ENDP

;
; Lighten the left half of a single 8x8 pixel screen tile.
;
; Applies the following pattern to the selected screen tile's contents:
;   @.......
;   @@......
;   @@@.....
;   @@@@....
;   @@@@@...
;   @@@@@@..
;   @@@@@@@.
;   @@@@@@@@
; where pixels marked `.` do not change and pixels marked `@` have their
; "intensity" bit unconditionally turned on. Visually this causes dark colors to
; lighten, and light colors to stay the same. The other color planes do not
; change. This is usually applied to the "east" (i.e. right) edge of light beams
; in maps.
;
; NOTE: This assumes port 3C5h has been set to 1000b (map mask = intensity plane
; only). LightenScreenTile and LightenScreenTileWest both leave the map mask in
; the correct state, but there's a chance that, if this procedure runs first,
; the mask could be incorrectly set.
;
; NOTE: This color effect fundamentally changes the pixels, meaning that
; "magenta" areas become "bright magenta" and no longer match for palette
; animation purposes.
;
; The basic theory of operation is as follows. For each pixel row in the tile:
; 1. Select a new mask. The mask follows a pattern of 1 bits shifting in from
;    the left while 0 bits shift out of the right.
; 2. Load the mask into the EGA bit mask register. For each 0 bit in the bit
;    mask register, the EGA will ignore the data sent by the processor in
;    subsequent memory write operations and instead use the contents of a latch.
; 3. Read the EGA memory to freshen the content of the latches, then write
;    binary 1's back to the EGA memory. Pixels that were masked off by the bit
;    mask register will not change. Planes that are not intensity (i.e. RGB)
;    will be masked off by the map mask register and will not change either.
;    The net result is that the intensity bit will be turned on for every pixel
;    covered by a "light" region of the mask, while uncovered areas retain their
;    previous value.
; 4. Advance the write position by a 40-byte step in preparation for the next
;    pixel row.
;
; x (word): X-position on the screen, in tiles. (0..39, leftmost column is 0)
; y (word): Y-position on the screen, in tiles. (0..24, topmost row is 0)
; Returns: Nothing
; Registers destroyed: AX, BX, DX, ES
;
PROC _LightenScreenTileEast FAR @@x:WORD, @@y:WORD
        PUBLIC _LightenScreenTileEast
        push  bp
        mov   bp,sp
        push  ds
        push  di
        push  si

        ; Set up destination pointer from the arguments:
        ;   ES:BX <- Destination draw page address (in EGA memory)
        mov   dx,[drawPageSegment]
        mov   es,dx
        mov   di,[@@y]
        shl   di,1
        mov   bx,[yOffsetTable+di]
        add   bx,[@@x]

        ; Pre-select the Map Mask before the repeat block is entered.
        SELECT_EGA_SEQ_MAP_MASK

        ; NOTE: Here we are making an *unsafe* assumption that the value 1000b
        ; has been previously loaded into the SEQUENCER_DATA I/O port, thus
        ; limiting memory write operations to the intensity color plane only.
        ; The assumption seems to not cause issues in the game, but if it did
        ; they would appear as lighted areas having an incorrect color along
        ; their east edges.

        ; Redraw eight rows of tile pixels. Each iteration uses a new bit mask
        ; loaded into the Bit Mask Register [EGA, pg. 54] via the Graphics 1 & 2
        ; Address Register [EGA, pg. 46] which progressively lightens more and
        ; more of each row. Two different byte-sized registers are being written
        ; with a single word-sized OUT; the high byte goes to the GRAPHICS_DATA
        ; I/O port.
        mov   dx,GRAPHICS_1_2_ADDR
        dstoff = 0
IRP mask,<10000000b,11000000b,11100000b,11110000b,11111000b,11111100b,11111110b,11111111b>
        mov   ax,(mask SHL 8) OR GFX_BIT_MASK
        out   dx,ax

        ; Read and then write back a byte of video memory to actually commit the
        ; changes that were previously set up. Each memory bit gets set to 1 if
        ; it satisfies the mask conditions, otherwise the written bit is ignored
        ; and the latched value is used in its place. The precise value written
        ; doesn't matter as long as there is a 1 bit set for every pixel that
        ; needs lightening. The mask left in AH conveniently has this property.
        xchg  ah,[es:bx+dstoff]

        ; Advance the destination write position, then the block repeats.
        dstoff = dstoff + SCREEN_Y_STRIDE
ENDM

        pop   si
        pop   di
        pop   ds
        pop   bp
        ret
ENDP

;
; Change the video page that is currently displayed on the screen.
;
; Although higher page numbers are accepted and will behave sensibly, pages
; beyond number 1 are used for tile storage and they will render as garbage if
; sent to the screen.
;
; page_num (word): Page number to show (0..n). Maximum page number varies based
;     on the video mode and installed adapter memory, but common values for the
;     EGA are 0, 1, 3, and 7. Only the low byte is used.
; Returns: Nothing
; Registers destroyed: AX
;
PROC _SelectActivePage FAR @@page_num:WORD
        PUBLIC _SelectActivePage
        push  bp
        mov   bp,sp

        ; Change the active display page via BIOS video service interrupt [EGA,
        ; pg. 104].
        mov   ax,[@@page_num]
        mov   ah,VSVC_SET_ACTIVE_PAGE
        int   INT_VIDEO_SERVICE

        pop   bp
        ret
ENDP

;
; Draw a single 8x8 pixel sprite tile to the draw page.
;
; This procedure draws tiles that are part of actors, decorations, the player,
; and other non-map sprites, with transparency. It is also responsible for
; drawing UI font characters.
;
; The tile data contains five color planes -- mask, blue, green, red, and
; intensity -- with 1-bit color depth on each plane. Tile image data is stored
; byte-planar in MBGRI order. Transparent areas have their mask bits set to 1.
;
; NOTE: Transparent areas in the tile *must* have the bits for all other color
; planes set to 0.
;
; The basic theory of operation is as follows:
; 1. Read the first four rows of mask bits from the source tile data. This
;    starts at the 0th byte of the tile data and repeats at 5-byte intervals.
;    Store these rows in four byte-width registers. (Ideally all eight rows
;    would be read, but the 286 does not have enough registers to do this. Four
;    rows are all that can be stored.)
; 2. For each of the four color planes:
;    a: Set the EGA map mask register to only apply memory write operations to
;       the current color plane being operated on.
;    b: Set the EGA read map select register to only load data from the color
;       plane being operated on during memory read operations.
;    c: For each pixel row in the tile:
;       A: Read one byte (i.e. eight pixels, or one tile row) from the EGA
;          memory to capture what's been written onto the current color plane in
;          the draw page thus far.
;       B: AND this with the mask data for the row. The first four rows are in
;          registers from step 1, the remaining rows must be read from the tile
;          source. This turns off (zeros) the pixels that are covered by an
;          opaque area of the tile.
;       C: OR the result with the color data from the tile source. This data is
;          located at 5-byte intervals, starting at an offset between 1 and 4
;          depending on the current color plane being operated on.
;       D: Write the result back to the original location in the EGA memory.
;          This updates one row of pixels on the current color plane.
;       E. Advance the read position by a 5-byte step, and advance the write
;          position by a 40-byte step, in preparation for the next pixel row.
;
; src (far pointer): Memory address of the first byte of tile data to read.
; x (word): X-position on the screen, in tiles. (0..39, leftmost column is 0)
; y (word): Y-position on the screen, in tiles. (0..24, topmost row is 0)
; Returns: Nothing
; Registers destroyed: AX, BX, CX, DX, ES
;
PROC _DrawSpriteTile FAR @@src:FAR PTR, @@x:WORD, @@y:WORD
        PUBLIC _DrawSpriteTile
        push  bp
        mov   bp,sp
        push  si
        push  di
        push  ds

        ; Set up source and destination pointers from the arguments:
        ;   DS:SI <- Source tile data address (could be anywhere in memory)
        ;   ES:DI <- Destination draw page address (in EGA memory)
        mov   di,[@@y]
        shl   di,1
        mov   di,[yOffsetTable+di]
        add   di,[@@x]
        mov   ax,[drawPageSegment]
        lds   si,[@@src]
        ASSUME ds:NOTHING
        mov   es,ax

        ; Mask data uses eight bytes in each tile (one byte per pixel row), but
        ; there are only four byte-width registers available to hold this data.
        ; Load the first four rows of mask data into registers, starting from
        ; DS:SI and continuing in 5-byte steps, then accept the fact that the
        ; last four rows must be repeatedly re-read from slower memory.
        mov   bl,[si]
        mov   bh,[si+5]
        mov   cl,[si+10]
        mov   ch,[si+15]

        ; The actual data movement is largely the same across the four color
        ; planes, so use a repeating macro to handle it.
IRP plane_num,<0,1,2,3>  ; 0,1,2,3 = blue,green,red,intensity
        ; Configure the Map Mask and enable only one of the four color planes.
        ; This restricts EGA memory writes to only the current color plane.
        SET_EGA_MAP_MASK <1 SHL plane_num>

        ; Configure the Read Map Select register, which makes future EGA memory
        ; reads only fetch data from the current color plane.
        SET_EGA_READ_MAP plane_num

        srcoff = 0
        dstoff = 0

        ; The first four rows of mask data are in byte registers, and the last
        ; four rows are still in memory at DS:SI+20 and 5-byte intervals beyond.
        ; The first row of color data is at DS:SI+plane_num+1, and subsequent
        ; rows are also spaced at 5-byte intervals.
        ;
        ; For each row of pixels in the tile, read the EGA memory contents for
        ; what's already been drawn on the preselected color plane. AND this
        ; with the mask data, so each pixel that is covered by an opaque area
        ; gets set to 0. OR the result with the pixel data for the tile, and
        ; write the result back into the original location in EGA memory.
  IRP maskreg,<bl,bh,cl,ch,[si+20],[si+25],[si+30],[si+35]>
        mov   al,[es:di+dstoff]
        and   al,maskreg
        or    al,[si+(plane_num + 1 + srcoff)]
        mov   [es:di+dstoff],al
        srcoff = srcoff + MASKED_TILE_ROW_STRIDE
        dstoff = dstoff + SCREEN_Y_STRIDE
  ENDM
ENDM

        pop   ds
        ASSUME ds:DGROUP
        pop   di
        pop   si
        pop   bp
        ret
ENDP

;
; Draw a single 8x8 pixel masked tile to the draw page.
;
; This procedure draws tiles that are part of the game map, with transparency.
;
; This works almost identically to DrawSpriteTile, so the comments here aren't
; anywhere near as thorough. Consult the other procedure for a more detailed
; description of what the code here is doing.
;
; These are the differences between this procedure and DrawSpriteTile:
; - The pointer passed in the `src` argument is subtracted by 16,000. This
;   unexpected and frankly dangerous behavior is due to the split between solid
;   and masked tiles in the map data. Very briefly, solid tiles are represented
;   in the map data by values 0..15,999, and masked tiles are represented by
;   values 16,000..55,960. The C code uses 16,000 as a split point to decide
;   which procedure should be used to draw each tile. Unfortunately, it passes
;   the raw map value in `src` without correcting for the masked tile offset.
;   The subtraction of 16,000 normalizes the value and allows all masked tile
;   memory to be read.
; - Before drawing begins, the EGA registers are reprogrammed to ensure that
;   data flows from the processor directly to the EGA memory during write
;   operations, without the internal latches interfering.
; - After drawing is done, the map mask is explicitly set to binary 1111, making
;   future EGA memory reads affect all four color planes in parallel.
; - Before returning, the EGA registers are once again reprogrammed to allow
;   the internal latches to influence the data written by the processor.
;
; src (far pointer): Memory address of the first byte of tile data to read
;     *plus* a constant offset of 16,000. In other words, if the intention is to
;     read the 0th byte in memory, src must be a pointer at offset 16,000.
; x (word): X-position on the screen, in tiles. (0..39, leftmost column is 0)
; y (word): Y-position on the screen, in tiles. (0..24, topmost row is 0)
; Returns: Nothing
; Registers destroyed: AX, BX, CX, DX, ES
;
PROC _DrawMaskedTile FAR @@src:FAR PTR, @@x:WORD, @@y:WORD
        PUBLIC _DrawMaskedTile
        push  bp
        mov   bp,sp
        push  si
        push  di
        push  ds

        ; Set up source and destination pointers from the arguments:
        ;   DS:SI <- Source tile data address (could be anywhere in memory)
        ;   ES:DI <- Destination draw page address (in EGA memory)
        ; NOTE: The source address ends up being 16,000 bytes below what was
        ; passed in the `src` argument.
        mov   di,[@@y]
        shl   di,1
        mov   di,[yOffsetTable+di]
        add   di,[@@x]
        mov   ax,[drawPageSegment]
        lds   si,[@@src]
        ASSUME ds:NOTHING
        sub   si,16000
        mov   es,ax

        ; Configure the Mode register by zeroing everything out.
        ;   Bits     | Meaning
        ;   ---------+--------
        ;   ......00 | Write Mode = Each memory plane is written with the
        ;            |     processor data.
        ;   .....0.. | Test Condition = off
        ;   ....0... | Read Mode = The processor reads data from the memory
        ;            |     plane selected by the read map select register.
        ;   ...0.... | Odd/Even = off
        ;   ..0..... | Shift Register = 0
        ;   00...... | Not Used
        ; The only seemingly important bits are those for Write Mode. This
        ; procedure requires memory to be written with processor data (as
        ; opposed to EGA latch data) and the correct mode must be ensured.
        SET_EGA_GC_MODE 000000b

        ; Store first four rows of mask data.
        mov   bl,[si]
        mov   bh,[si+5]
        mov   cl,[si+10]
        mov   ch,[si+15]

        ; Repeating macro for data movement.
IRP plane_num,<0,1,2,3>  ; 0,1,2,3 = blue,green,red,intensity
        ; Configure the Map Mask and enable only the current color plane.
        SET_EGA_MAP_MASK <1 SHL plane_num>

        ; Configure the Read Map Select register to isolate current color plane.
        SET_EGA_READ_MAP plane_num

        srcoff = 0
        dstoff = 0

        ; Draw eight rows for one color plane.
  IRP maskreg,<bl,bh,cl,ch,[si+20],[si+25],[si+30],[si+35]>
        mov   al,[es:di+dstoff]
        and   al,maskreg
        or    al,[si+(plane_num + 1 + srcoff)]
        mov   [es:di+dstoff],al
        srcoff = srcoff + MASKED_TILE_ROW_STRIDE
        dstoff = dstoff + SCREEN_Y_STRIDE
  ENDM
ENDM

        ; Reset the map mask register to write to all four color planes.
        SET_EGA_MAP_MASK 1111b  ; Bits are planes 3210

        ; Reset the Write Mode value in the Mode register.
        ;   Bits     | Meaning
        ;   ---------+--------
        ;   ......01 | Write Mode = Each memory plane is written with the
        ;            |     contents of the processor latches. These latches are
        ;            |     loaded by a processor read operation.
        ; The other bits retain the same value (and meaning) as above.
        SET_EGA_GC_MODE 000001b

        pop   ds
        ASSUME ds:DGROUP
        pop   di
        pop   si
        pop   bp
        ret
ENDP

;
; Draw a single 8x8 pixel sprite tile, flipped vertically, to the draw page.
;
; This procedure draws tiles upside-down, to represent actors that are being
; destroyed or new items that are spawning into existence. This is a vertical
; flip, *not* a 180-degree rotation.
;
; The tile data contains five color planes -- mask, blue, green, red, and
; intensity -- with 1-bit color depth on each plane. Tile image data is stored
; byte-planar in MBGRI order. Transparent areas have their mask bits set to 1.
;
; NOTE: Transparent areas in the tile *must* have the bits for all other color
; planes set to 0.
;
; The basic theory of operation is as follows. On each of the four color planes:
; 1. Set the EGA map mask register to only apply memory write operations to the
;    current color plane being operated on.
; 2. Set the EGA read map select register to only load data from the color plane
;    being operated on during memory read operations.
; 3. For each pixel row in the tile:
;    a: Decrement the write position by a 40-byte step, since the goal is to
;       draw the tile rows in reverse order.
;    b: Read one byte (i.e. eight pixels, or one tile row) from the EGA
;       memory to capture what's been written onto the current color plane in
;       the draw page thus far.
;    c: AND this with the mask data for the row. The first four rows are in
;       registers from step 1, the remaining rows must be read from the tile
;       source. This turns off (zeros) the pixels that are covered by an
;       opaque area of the tile.
;    d: OR the result with the color data from the tile source. This data is
;       located at 5-byte intervals, starting at an offset between 1 and 4
;       depending on the current color plane being operated on.
;    e: Write the result back to the original location in the EGA memory.
;       This updates one row of pixels on the current color plane.
;    f. Advance the read position by a 5-byte step in preparation for the next
;       pixel row.
;
; src (far pointer): Memory address of the first byte of tile data to read.
; x (word): X-position on the screen, in tiles. (0..39, leftmost column is 0)
; y (word): Y-position on the screen, in tiles. (0..24, topmost row is 0)
; Returns: Nothing
; Registers destroyed: AX, DX, ES
PROC _DrawSpriteTileFlipped FAR @@src:FAR PTR, @@x:WORD, @@y:WORD
        PUBLIC _DrawSpriteTileFlipped
        push  bp
        mov   bp,sp
        push  si
        push  di
        push  ds

        ; Set up source and destination pointers from the arguments:
        ;   DS:SI <- Source tile data address (could be anywhere in memory)
        ;   ES:DI <- Destination draw page address (in EGA memory)
        mov   di,[@@y]
        shl   di,1
        mov   di,[yOffsetTable+di]
        add   di,[@@x]
        mov   ax,[drawPageSegment]
        lds   si,[@@src]
        ASSUME ds:NOTHING
        mov   es,ax

        ; The actual data movement is largely the same across the four color
        ; planes, so use a repeating macro to handle it.
IRP plane_num,<0,1,2,3>  ; 0,1,2,3 = blue,green,red,intensity
        ; Configure the Map Mask and enable only one of the four color planes.
        ; This restricts EGA memory writes to only the current color plane.
        SET_EGA_MAP_MASK <1 SHL plane_num>

        ; Configure the Read Map Select register, which makes future EGA memory
        ; reads only fetch data from the current color plane.
        SET_EGA_READ_MAP plane_num

        srcoff = 0
        dstoff = SCREEN_Y_STRIDE * 8

        ; The mask data is in memory at DS:SI and 5-byte intervals beyond. The
        ; first row of color data is at DS:SI+plane_num+1, and subsequent rows
        ; are also spaced at 5-byte intervals. The destination offset iterates
        ; backwards from the bottom up, while the source offset iterates
        ; forwards. This is what flips the tile vertically. Horizontal pixel
        ; order is unaffected.
        ;
        ; For each row of pixels in the tile, read the EGA memory contents for
        ; what's already been drawn on the preselected color plane. AND this
        ; with the mask data, so each pixel that is covered by an opaque area
        ; gets set to 0. OR the result with the pixel data for the tile, and
        ; write the result back into the original location in EGA memory.
  REPT 8
        dstoff = dstoff - SCREEN_Y_STRIDE
        mov   al,[es:di+dstoff]
        and   al,[si+srcoff]
        or    al,[si+(plane_num + 1 + srcoff)]
        mov   [es:di+dstoff],al
        srcoff = srcoff + MASKED_TILE_ROW_STRIDE
  ENDM
ENDM

        pop   ds
        ASSUME ds:DGROUP
        pop   di
        pop   si
        pop   bp
        ret
ENDP

;
; Draw a single 8x8 pixel sprite tile, as a white shape, to the draw page.
;
; This procedure draws tiles as solid white shapes, to represent actors that are
; taking damage or to get the player's attention.
;
; NOTE: This assumes port 3C4h has been set to 2h (sequencer index = map mask).
; This is the only state that the game ever leaves the register in, but it's an
; unsafe assumption.
;
; The basic theory of operation is as follows.
; 1. Program the EGA registers to accept the upcoming drawing technique. This
;    procedure uses an approach not seen in any other area of the game, so the
;    setup is a little unfamiliar in some places:
;    a. Set the Map Mask register to all 1's. This sets it so that any bits that
;       the processor sets to 1 will apply to all four color planes (red + green
;       + blue + intensity), resulting in white.
;    b. Set the Function Select value to OR any data written by the processor
;       against the EGA latches. This allows the processor to write 1 bits and
;       set the color to white, and write 0 bits to retain the current color,
;       whatever it might be.
;    c. Set the Read Mode value to combine the value of all four color planes
;       during memory reads. Considering the value already in the Color Don't
;       Care register (set in the SetVideoMode procedure), this is a rather
;       convoluted way to ensure the EGA returns FFh for all memory reads.
; 2. For each pixel row in the tile:
;    a. Read one byte (i.e. eight pixels, or one tile row) from the source
;       tile's mask data.
;    b. Invert the bits of the mask data, so each uncovered pixel has a 0 bit
;       while covered/opaque areas are 1 bits.
;    c. AND this value with the contents of the EGA memory. This causes a memory
;       read which sets the EGA's internal latches and returns FFh. (See step
;       1c for the cause of this behavior.) AND against FFh doesn't have any
;       interesting effects, so the result is the mask data being written
;       directly into memory. Written 1 bits become white, and 0 bits retain the
;       latched value and, consequently, the previous color.
;    d. Advance the read position by a 5-byte step, and advance the write
;       position by a 40-byte step, in preparation for the next pixel row.
; 3. Reset the Function Select and Read Mode values to their natural state.
;
; The tile data contains five color planes -- mask, blue, green, red, and
; intensity -- with 1-bit color depth on each plane. Tile image data is stored
; byte-planar in MBGRI order. Transparent areas have their mask bits set to 1.
;
; src (far pointer): Memory address of the first byte of tile data to read.
; x (word): X-position on the screen, in tiles. (0..39, leftmost column is 0)
; y (word): Y-position on the screen, in tiles. (0..24, topmost row is 0)
; Returns: Nothing
; Registers destroyed: AX, DX, ES
;
PROC _DrawSpriteTileWhite FAR @@src:FAR PTR, @@x:WORD, @@y:WORD
        PUBLIC _DrawSpriteTileWhite
        push  bp
        mov   bp,sp
        push  si
        push  di
        push  ds

        ; Set up source and destination pointers from the arguments:
        ;   DS:SI <- Source tile data address (could be anywhere in memory)
        ;   ES:DI <- Destination draw page address (in EGA memory)
        mov   di,[@@y]
        shl   di,1
        mov   di,[yOffsetTable+di]
        add   di,[@@x]
        mov   ax,[drawPageSegment]
        lds   si,[@@src]
        ASSUME ds:NOTHING
        mov   es,ax

        ; Program the EGA Map Mask [EGA, pg. 20] to enable writing to all four
        ; color planes in parallel. NOTE: This is making an *unsafe* assumption
        ; that the value SEQ_MAP_MASK has been previously loaded into the
        ; SEQUENCER_ADDR I/O port. The assumption seems to not cause issues in
        ; the game.
        mov   dx,SEQUENCER_DATA
        mov   al,1111b          ; Bits are planes 3210
        out   dx,al

        ; Select the Data Rotate (Function Select) register [EGA, pg. 49] via
        ; the Graphics 1 & 2 Address Register [EGA, pg. 46] and set the Function
        ; Select bits. Two different byte-sized registers are being written with
        ; a single word-sized OUT; the high byte goes to the GRAPHICS_DATA I/O
        ; port.
        ;   Bits     | Meaning
        ;   ---------+--------
        ;   .....000 | Rotate Count = 0
        ;   ...10... | Function Select = Data written to memory OR'ed with data
        ;            |     already in the latches.
        ;   000..... | Not Used
        ; The only parameter being changed here is Function Select, which is
        ; programmed to OR any data written by the processor with the contents
        ; of the EGA's internal latches. Concretely, this means that the memory
        ; bits will get unconditionally set to 1 where the processor writes a 1,
        ; and the memory bits will retain their latched value (whatever they may
        ; have been) where the processor writes a 0.
        mov   dx,GRAPHICS_1_2_ADDR
        mov   ax,(10000b SHL 8) OR GFX_DATA_ROTATE
        out   dx,ax

        ; Next move to the Mode register [EGA, pg. 50] and set the Read Mode
        ; bit. As before, two bytes are being written with one word OUT.
        ;   Bits     | Meaning
        ;   ---------+--------
        ;   ......00 | Write Mode = Each memory plane is written with the
        ;            |     processor data.
        ;   .....0.. | Test Condition = off
        ;   ....1... | Read Mode = The processor reads the results of the
        ;            |     comparison of the 4 memory planes and the color
        ;            |     compare register.
        ;   ...0.... | Odd/Even = off
        ;   ..0..... | Shift Register = 0
        ;   00...... | Not Used
        ; The only parameter being changed here is Read Mode, which is
        ; programmed to combine bits from all four planes into one result during
        ; processor read operations. Normally this mode is used to test screen
        ; pixels against the Color Compare register, but the global
        ; initialization of Color Don't Care in the SetVideoMode procedure
        ; rendered this test inert. The combination of all these factors means
        ; that all subsequent memory reads will return binary 1 in every
        ; position, regardless of what colors or images are presently in memory.
        mov   ax,(001000b SHL 8) OR GFX_MODE
        out   dx,ax

        srcpos = 0
        dstpos = 0

        ; The source tile's mask data is located in memory at DS:SI and 5-byte
        ; intervals beyond.
        ;
        ; For each row of pixels in the tile, read one row (8 bits) of mask data
        ; from the source and invert the bits to normalize the representation.
        ; The result is a 1 bit in each location where the tile covers the
        ; background. AND this with the EGA memory contents to apply the change.
        ;
        ; The underlying behavior here is unintuitive and perhaps a bit
        ; surprising. Because of the way the EGA Read Mode and Color Don't Care
        ; registers were programmed above, every EGA memory read (including the
        ; one implicit in the AND instruction) returns FFh in every case. AND
        ; against FFh is doing the same thing as MOV would have, with one
        ; important exception: The AND sets the EGA latch state by reading from
        ; memory first. That's important here.
        ;
        ; On the write side, the Funciton Select value specifies that each bit
        ; written should be OR'd with the value presently in the EGA latches.
        ; Wherever the processor writes a 1 bit, the 1 value prevails. Wherever
        ; the processor writes a 0 bit, that value is OR'd with the latch
        ; contents and the latched value remains. The Map Mask register value
        ; specifies that all four color planes are affected during writes, which
        ; causes a white pixel to be created where the processor writes a 1 bit,
        ; and the original color to be maintained at each 0 bit.
REPT 8
        mov   al,[si+srcpos]
        not   al
        and   [es:di+dstpos],al
        srcpos = srcpos + MASKED_TILE_ROW_STRIDE
        dstpos = dstpos + SCREEN_Y_STRIDE
ENDM

        ; Reset the Function Select value in the Data Rotate (Function Select)
        ; register.
        ;   Bits     | Meaning
        ;   ---------+--------
        ;   ...00... | Function Select = Data written to memory is unmodified.
        ; The other bits retain the same value (and meaning) as above.
        mov   ax,(00000b SHL 8) OR GFX_DATA_ROTATE
        out   dx,ax

        ; Reset the Read Mode value in the Mode register.
        ;   Bits     | Meaning
        ;   ---------+--------
        ;   ....0... | Read Mode = The processor reads data from the memory
        ;            |     plane selected by the read map select register.
        ; The other bits retain the same value (and meaning) as above.
        mov   ax,(000000b SHL 8) OR GFX_MODE
        out   dx,ax

        pop   ds
        ASSUME ds:DGROUP
        pop   di
        pop   si
        pop   bp
        ret
ENDP

; The next procedure is used to differentiate between 8086 and 80286 CPUs, and
; must use the lowest common instruction encodings shared by the two.
P8086

;
; Test the system's CPU to try to determine exactly what it is.
;
; There are many similar but subtly different techniques for CPU detection out
; there. It would be a significant undertaking to try to trace the authorship
; of any specific test or group of tests. I'll briefly list some of the names
; and publications I was able to find:
; - Various code examples from Intel processor manuals.
; - Bob Smith with input from Arthur Zachai, in an article in PC Tech Journal,
;   Volume 4 Number 4, April 1986.
; - Robert de Bath, in a utility script provided with the Dev86 project
;   (https://github.com/jbruchon/dev86/blob/master/libc/misc/cputype.c).
; - Unknown author, in an assembly code fragment titled "Chips", where another
;   author named Clif Purkiser is mentioned as creating some techniques.
; - Juan Jimenez, mentioned in passing in files JABHACK.ASM and WL_ASM.ASM from
;   both the Wolfenstein 3D and Catacomb 3-D source releases. Modified, in all
;   likelihood, by Jason Blochowiak.
;
; This code, however, directly matches the structure and implementation choices
; used in "PROCCA.ASM" from a book titled "PC System Programming for Developers"
; by Michael Tischer, pages 664..666. Regardless of who discovered each test,
; this implementation was taken directly from Tischer's book.
;
; The following tests are performed:
; 1. Try to modify certain bits in the FLAGS register to determine if the CPU
;    is an 80286 or higher.
; 2. If step 1 succeeded, continue modifying bits in the FLAGS register to
;    differentiate between the 80286 and the 80386.
; 3. Otherwise, test an unreasonable right-shift instruction to see if it's
;    handled in the way an 80186/88 would.
; 4. Otherwise, test the behavior of a long-running instruction with multiple
;    prefixes to see if it handles interrupts properly like an NEC V30/20.
; 5. Finally, assume 8086/88.
; 6. Test the interplay between the processor's prefetch queue and self-
;    modifying code, to try to determine if the processor has an 8- or 16-bit
;    data bus.
;
; Returns: CPU type (0..7) as a word in AX. More meaningful names for the
;     numeric values are available as CPUTYPE_* equates.
; Registers destroyed: AX, CX, DX, ES
;
PROC _GetProcessorType FAR
        PUBLIC _GetProcessorType
        push  bp
        mov   bp,sp
        push  ds
        push  si
        push  di

        ; Save the current state of the FLAGS register. This value will be
        ; restored right before the procedure returns.
        pushf

        ; Push a zero value to the stack, then immediately pop that zero value
        ; into the FLAGS register. Depending on the CPU, some bits will refuse
        ; this change and remain on.
        xor   ax,ax
        push  ax
        popf

        ; Push the current state of FLAGS to the stack, then immediately pop the
        ; flag state into AX for further analysis.
        pushf
        pop   ax

        ; Consider only flag bits 12..15, and see if they all remained on.
        ;
        ; Bits   | 80286 Meaning              | 80386 Meaning
        ; -------+----------------------------+--------------
        ; 15     | Reserved (Undefined)       | Reserved (0)
        ; 14     | Nested Task (NT)           | Nested Task (NT)
        ; 13..12 | I/O Privilege Level (IOPL) | I/O Privilege Level (IOPL)
        ;
        ; This is testing for an 80286 or better CPU. On lesser CPUs, bits
        ; 12..15 all remain on at all times. If this is occurring, the CPU is
        ; less than an 80286 [Tischer, pg. 655].
        and   ax,0f000h
        cmp   ax,0f000h
        je    @@less_than_286   ; Jump if all of these bits are on.

        ; Here, at least one of the relevant flag bits was off, so the CPU must
        ; be at least an 80286.
        mov   dl,CPUTYPE_80286  ; DL stores the work-in-progress return value.

        ; Push a constant value to the stack, then immediately pop that value
        ; into the FLAGS register. The constant is designed to attempt to turn
        ; flag bits 12..14 (IOPL and NT) on, which an 80286 in real mode will
        ; refuse to do [Tischer, pg. 655].
        mov   ax,7000h
        push  ax
        popf

        ; Push the current state of FLAGS to the stack, then immediately pop the
        ; flag state into AX for further analysis.
        pushf
        pop   ax

        ; Consider only flag bits 12..14, and see if they all remained off.
        and   ax,7000h
        jz    @@done            ; Jump if all of these bits are off.

        ; Here, at least one of the relevant flag bits was on, so the CPU must
        ; be an 80386.
        inc   dl                ; Return value becomes CPUTYPE_80386.
        jmp   @@done

@@less_than_286:
        ; Now we know that the CPU is less than an 80286.
        mov   dl,CPUTYPE_80188

        ; Perform "FFh >> 33" then check for a zero or nonzero result.
        ;
        ; This differentiates an 80186/88 from lesser CPUs. On an 80186/88, only
        ; the low 5 bits of CL are considered, effectively doing "FFh >> 1" and
        ; computing a nonzero result. On lesser CPUs, all bits of CL are
        ; considered and the entire value shifts out, leaving zero [iAPX, pg.
        ; 3-208].
        mov   al,0ffh
        mov   cl,21h
        shr   al,cl
        jnz   @@cpu_class_known ; Jump if result in AL is not zero.

        ; Now we know that the CPU is less than an 80186/88.
        mov   dl,CPUTYPE_V20

        ; Ensure interrupts are enabled, then save SI's value on the stack. The
        ; `push` (and the `pop` below) aren't strictly required since the SI
        ; value is already being saved/restored at the beginning/end of the
        ; procedure.
        sti
        push  si

        ; Here, ES is pointing to some unspecified place in memory. Below is a
        ; busy loop that reads 64 KB of memory from ES:SI, loading each byte
        ; into AL and doing nothing further with it. After each iteration, SI is
        ; incremented and CX is decremented. The loop ends when CX reaches zero.
        ; Or the loop might end after the first hardware interrupt! Here's why:
        ;
        ; The line with `rep lods` assembles to F3h (rep) 26h (ES) ACh (lodsb),
        ; which is a single-byte instruction with two single-byte prefixes. The
        ; original 8086/88 had a bug related to this condition: If a hardware
        ; interrupt occurs while executing an instruction with both a repeat
        ; prefix and a segment override prefix, the repeat prefix will be
        ; dropped when the interrupt returns. The prefixed instruction would
        ; then cease repeating, and CX would not reach zero as expected. Clone
        ; CPUs manufactured by NEC (the V20 and V30) do not suffer from this
        ; bug, and complete the loop fully in all cases [Tischer, pg. 656].
        ;
        ; This code relies on the fact that, in its default configuration, the
        ; hardware timer generates an interrupt approximately once every 55 ms.
        ; `lods` over 64 KB of memory is predicted to take 851,966 cycles on an
        ; 8086/88, which would be 85 ms at a clock rate of 10 MHz [Smith]. The
        ; timer is expected to fire at least once during the whole operation.
        mov   si,0
        mov   cx,0ffffh
        rep lods [BYTE PTR es:si]

        ; Pop the saved value from the stack and restore it to SI.
        pop   si

        ; See if the value in CX made it all the way to zero. If it has, the CPU
        ; is a V30 or V20.
        or    cx,cx
        jz    @@cpu_class_known ; Jump if CX is zero.

        ; Now we know that the CPU is a bottom-rung 8086 or 8088.
        mov   dl,CPUTYPE_8088

@@cpu_class_known:
        ; Now we know which class the CPU belongs to, but we don't yet know if
        ; it has a 8- or 16-bit data bus. The bus width is what differentiates
        ; 8086 from 8088, 80186 from 80188, and V30 from V20. This property
        ; can't be directly measured, but it can be inferred by testing how the
        ; CPU's prefetch queue responds to self-modifying code.

        ; Set ES to point to CS, the segment where this code resides. Turn the
        ; Direction Flag on, which causes pointers to decrement during string
        ; instructions.
        push  cs
        pop   es
        std

        ; Load DI with the offset part (relative to CS) of the memory address at
        ; the end of the prefetch queue test sequence below.
        mov   di,OFFSET @@qEnd

        ; Ensure interrupts are disabled, because any deviation in execution
        ; flow will reset the queue and invalidate the test. Next, enter a short
        ; loop: Write the byte value FBh (which is the opcode for an `sti`
        ; instruction) to the memory pointed to by ES:DI. After each iteration,
        ; decrement both DI and CX. The loop ends after three iterations bring
        ; CX down to zero.
        mov   al,0fbh
        mov   cx,3
        cli
        rep stosb

        ; What actually happens next depends on the type of CPU executing the
        ; code. An 8086 prefetches six bytes ahead of the current instruction,
        ; meaning all instructions up to and including the final `nop` have
        ; already been fetched from memory. An 8088 only prefetches four bytes,
        ; meaning only the third `nop` has been prefetched [Tischer, pg. 657].
        ;
        ; The `rep stosb` loop above is simultaneously operating on this area
        ; of memory, replacing one byte at a time with the opcode for `sti` in
        ; reverse order. Its first iteration overwrites the `sti` at the bottom,
        ; leaving the value unchanged. The second iteration overwrites the final
        ; `nop`. The third and final iteration overwrites `inc dx`.
        ;
        ; Both processors will destroy the `inc dx` instruction in memory
        ; (replacing it with `sti`), but an 8086 will increment as if the
        ; instruction was still there because its prefetch queue doesn't know
        ; about the memory change. An 8088 will read `sti`, and will not
        ; increment [Tischer, pg. 657]. The increment instruction would change
        ; the return value from CPUTYPE_8088 to CPUTYPE_8086, from CPUTYPE_V20
        ; to CPUTYPE_V30, or from CPUTYPE_80188 to CPUTYPE_80186 depending on
        ; what the value already was.
        ;
        ; The `cld` instruction, which turns the Direction Flag back off, is
        ; probably not critical here because DF gets restored by `popf` below.
        cld                     ; Prefetch byte 1
        nop                     ; Prefetch byte 2
        nop                     ; Prefetch byte 3
        nop                     ; Prefetch byte 4; farthest byte for 8088
        inc   dx                ; Prefetch byte 5
        nop                     ; Prefetch byte 6; farthest byte for 8086
@@qEnd: sti

@@done:
        ; Restore the FLAGS register back to the state it was in when the
        ; procedure was first entered.
        popf

        ; Zero the high byte of DX, then copy the result into AX. This is the
        ; procedure's final return value.
        xor   dh,dh
        mov   ax,dx

        pop   di
        pop   si
        pop   ds
        pop   bp
        ret
ENDP
ENDS
