/**
 * Copyright (c) 2020 Scott Smitelli
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

/*****************************************************************************
 *                           COSMORE "GAME2" UNIT                            *
 *                                                                           *
 * This file is primarily a bag of utility functions used by GAME1. There is *
 * a lot of junk in this file -- variables and functions whose name includes *
 * the literal word "junk." These are maintained to keep the reconstruction  *
 * faithful to the original, but none of them serve any useful purpose and   *
 * they could be safely removed without impacting gameplay.                  *
 *                                                                           *
 * The first 17 functions in this file all pertain to music playback through *
 * an AdLib or compatible sound card. That code appears to have been copied  *
 * with slight modification from id Software's "Sound Manager" v1.1d1 by     *
 * Jason Blochowiak, as it existed around the development of Catacomb 3-D.   *
 * They are the only functions in the game that use inline assembly.         *
 *                                                                           *
 * Three of the joystick related functions also appear to have a connection  *
 * to id Software. These bear some similarity to functions in their "C       *
 * Library" by either John Romero or John Carmack, as released with          *
 * Hovertank 3-D.                                                            *
 *                                                                           *
 * References:                                                               *
 * - [ID_SD]: ID_SD.C from the source release of Catacomb 3-D.               *
 *   https://github.com/CatacombGames/Catacomb3D/blob/master/ID_SD.C         *
 * - [IDLIB]: IDLIBC.C from the source release of Hovertank 3-D.             *
 *   https://github.com/FlatRockSoft/Hovertank3D/blob/master/IDLIBC.C        *
 *****************************************************************************/

#include "glue.h"

/*
Table of multiples of 320. Used in the low-level assembly procedures to map a
screen tile Y coordinate to an equivalent EGA memory offset using these
properties:
  - 1 byte of EGA memory == 8 horizontal screen pixels (disregarding planes)
  - 40 bytes of EGA memory == 320 horizontal screen pixels
  - Each screen tile is 8 vertical pixels high
  - Therefore, each tile Y coordinate step == 320 EGA memory bytes
Since each tile is 8 screen pixels wide, there is a 1:1 relationship between a
tile X coordinate and its EGA memory offset. The X coordinate can simply be
added to one of the values from this table.
*/
word yOffsetTable[] = {
    0, 320, 640, 960, 1280, 1600, 1920, 2240, 2560, 2880, 3200, 3520, 3840,
    4160, 4480, 4800, 5120, 5440, 5760, 6080, 6400, 6720, 7040, 7360, 7680
};

/*
Table of human-readable names for each keyboard scancode. The game font does not
have characters for square/curly braces, grave accent, tilde, backslash, or
vertical bar. None of those keys have a visible representation. For the rest of
the scancodes, *only one* of the shifted/unshifted states for each key is
represented.
*/
static KeyName keyNames[] = {
    "NULL", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=",
    "BKSP", "TAB", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", " ", " ",
    "ENTR", "CTRL", "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "\"",
    " ", "LSHFT", " ", "Z", "X", "C", "V", "B", "N", "M", ",", ".", "?",
    "RSHFT", "PRTSC", "ALT", "SPACE", "CAPLK", "F1", "F2", "F3", "F4", "F5",
    "F6", "F7", "F8", "F9", "F10", "NUMLK", "SCRLK", "HOME", "\x18", "PGUP",
    "-", "\x1B", "5", "\x1C", "+", "END", "\x19", "PGDN", "INS", "DEL",
    "SYSRQ", "", "", "F11", "F12", ""
};

/*
Both of these are true if AdLib hardware is installed, and false otherwise. Not
impacted by the state of any game configuration options. The author decided to
detect the AdLib twice into two distinct variables for different scopes.
*/
bbool isAdLibPresent;
static bool isAdLibPresent2;

/*
Remaining AdLib state flags.
*/
static bool skipDetectAdLib, isAdLibStarted, isAdLibEnabled, isAdLibPlaying;

/*
Low-level values for profiling and managing interrupts for the Programmable
Interval Timer, along with processor speed measurement.
*/
static InterruptFunction savedInt8;
static dword pit0Value, timerTickCount;
static word profCountCPU, profCountPIT;

/*
Computed ratios for WaitWallclock(). 10 and 25ms are never used.
*/
static word wallclock10ms, wallclock25ms, wallclock100ms;

/*
Pointers and offsets to music data. "Length/Head" are fixed values used when the
music reaches the end and needs to loop. "Left" decrements during playback, and
"Ptr" advances.
*/
static word musicDataLength, musicDataLeft;
static word *musicDataHead, *musicDataPtr;

/*
Music timing variables. A counter for the number of ticks elapsed so far, and
a "next due" tick value when the next piece of music data needs to be written.
*/
static dword musicTickCount, musicNextDue;

/*
Joystick calibration/button options. Apparently designed with the idea that
there would be support for 3(!) different joysticks.
*/
static int joystickXLow[3], joystickXHigh[3];
static int joystickYLow[3], joystickYHigh[3];
static bool joystickBtn1Bombs;

/*
Junk variables. Some are read/written, others are padding, none are important.
The ones declared non-static are to avoid "never used" warnings.
*/
word junkPadding[7], junkStatic = 2;
static word junk1, junk7;
static long junk2;
static dword junk3, junk6;
static bool junk4, junk5;

/*
Prototypes for "private" functions where strictly required.
*/
void StopAdLibPlayback(void);
void FadeOutAdLibPlayback(void);

/*
Write `value` to the counter on channel 0 of the programmable interval timer.
[ID_SD, SDL_SetTimer0()]
*/
void SetPIT0Value(word value)
{
    /* 0011 0110 = counter 0 select, r/w little-endian, square wave, 16-bit binary */
    outportb(0x0043, 0x36);

    /* PIT counter 0 divisor (low, high byte) */
    outportb(0x0040, value);
    outportb(0x0040, value >> 8);

    pit0Value = value;
}

/*
Program channel 0 of the PIT in terms of interrupts per second (Hz).
[ID_SD, SDL_SetIntsPerSec()]

This controls how many times interrupt vector 8 will fire each second.

The different compnents of the IBM PC ran at integral multiples of 315/88 MHz.
The PIT clock was one-third of that, or 1,193,181.81... Hz. The chosen constant
is about 0.1% off for an unknown reason.
*/
void SetInterruptRate(word ints_second)
{
    SetPIT0Value((word)(1192030L / ints_second));
}

/*
Define a simple test counter to benchmark the CPU against the timer.
[ID_SD, SDL_TimingService()]
*/
void interrupt ProfilePITService(void)
{
    profCountCPU = _CX;
    profCountPIT++;

    /* Send end-of-interrupt to the 8259 PIC...? */
    outportb(0x0020, 0x20);
}

/*
Perform 10 trials to determine the timing of the PIT relative to the speed of
the processor. [ID_SD, SDL_InitDelay()]
*/
void ProfilePIT(void)
{
    int trial;
    word ratio;

    setvect(8, ProfilePITService);
    SetInterruptRate(1000);

    for (trial = 0, ratio = 0; trial < 10; trial++) {
        _DX = 0;
        _CX = 0xffff;
        profCountPIT = _CX;  /* conceptually -1 */

wait4zero:
        asm or    [profCountPIT],0
        asm jnz   wait4zero

wait4one:
        /*
        CX decrements each iteration of this loop. When the profile service runs
        again, it will copy a snapshot of CX to profCountCPU and increment
        profCountPIT, which will stop this loop.
        */
        asm test  [profCountPIT],1
        asm jnz   done
        asm loop  wait4one
done:

        /* Hang onto the worst value */
        if (0xffff - profCountCPU > ratio) {
            ratio = 0xffff - profCountCPU;
        }
    }

    ratio += ratio / 2;
    wallclock10ms  = ratio / (1000 / 10);
    wallclock25ms  = ratio / (1000 / 25);
    wallclock100ms = ratio / (1000 / 100);

    SetPIT0Value(0);

    setvect(8, savedInt8);
}

/*
Wait for a period of time equal to `loops`. This appears to be based on how many
CPU clocks it takes to perform a pointless test/jnz pair. The loop structure
here closely matches that in ProfilePIT(). [ID_SD, SDL_Delay()]
*/
void WaitWallclock(word loops)
{
    if (loops == 0) return;

    _CX = loops;

wait:
    asm test  [profCountPIT],0  /* ZF is always 1 */
    asm jnz   done              /* jump is never taken */
    asm loop  wait
done:
    ;  /* VOID */
}

/*
Write `data` to the AdLib register `reg`. The long string of `in`s after each
`out` is for timing purposes -- that's how long it takes the hardware to process
the write and become ready for another (potential) write. [ID_SD, alOut()]
*/
void SetAdLibRegister(byte reg, byte data)
{
    asm pushf

    disable();

    asm mov   dx,0x0388
    asm mov   al,[reg]
    asm out   dx,al
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm mov   dx,0x0389
    asm mov   al,[data]
    asm out   dx,al

    asm popf

    asm mov   dx,0x0388
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
    asm in    al,dx
}

/*
Update the AdLib with the next music chunk. [ID_SD, SDL_ALService()]
*/
void AdLibService(void)
{
    if (!isAdLibPlaying) return;

    while (musicDataLeft != 0 && musicNextDue <= musicTickCount) {
        byte datalow, datahigh;
        word data = *musicDataPtr++;

        musicNextDue = musicTickCount + *musicDataPtr++;

        asm mov   dx,[data]
        asm mov   [datalow],dl
        asm mov   [datahigh],dh

        SetAdLibRegister(datalow, datahigh);

        musicDataLeft -= 4;
    }

    musicTickCount++;

    if (musicDataLeft == 0) {
        musicDataPtr = musicDataHead;
        musicDataLeft = musicDataLength;
        musicTickCount = musicNextDue = 0;
    }
}

/*
Detect and reset the AdLib hardware. Return true if an AdLib or compatible card
is installed, otherwise return false. [ID_SD, SDL_DetectAdLib()]
*/
bool DetectAdLib(void)
{
    byte status1, status2;
    int reg;

    SetAdLibRegister(0x04, 0x60);  /* timer control, mask timer 1/2 */
    SetAdLibRegister(0x04, 0x80);  /* timer control, IRQ reset, timers 1/2 */

    status1 = inportb(0x0388);  /* read adlib status */

    SetAdLibRegister(0x02, 0xff);  /* timer 1 counter to 0xff */
    SetAdLibRegister(0x04, 0x21);  /* timer control, mask timer 2 */

    WaitWallclock(wallclock100ms);

    status2 = inportb(0x0388);  /* read adlib status */

    SetAdLibRegister(0x04, 0x60);  /* timer control, mask timer 1/2 */
    SetAdLibRegister(0x04, 0x80);  /* timer control, IRQ reset, timers 1/2 */

    /* look at timer1/timer2/both */
    if ((status1 & 0xe0) == 0 && (status2 & 0xe0) == 0xc0) {
        /* zero out every register in the adlib */
        for (reg = 0x01; reg <= 0xf5; reg++) {
            SetAdLibRegister(reg, 0);
        }

        SetAdLibRegister(0x01, 0x20);  /* allows the FM chips to control the waveform of each operator */
        SetAdLibRegister(0x08, 0);  /* zero the CSM Mode/Keyboard Split register */

        return true;
    }

    return false;
}

/*
Respond to timer interrupts and call both the AdLib and PC speaker services at
the appropriate rates. The timer runs 4x faster when the AdLib is being used,
which is the reason for the count division. [ID_SD, SDL_t0Service()]
*/
void interrupt TimerInterruptService(void)
{
    static word count = 1;

    junk1++;

    if (isAdLibEnabled == true) {  /* explicit compare against 1 */
        AdLibService();

        if (count % 4 == 0) {
            PCSpeakerService();
        }

        if (++count % 8 == 0) {
            junk2++;
            junk3++;
        }
    } else {
        PCSpeakerService();

        if (++count % 2 == 0) {
            junk2++;
            junk3++;
        }
    }

    asm mov   ax,[WORD PTR timerTickCount]
    asm add   ax,[WORD PTR pit0Value]
    asm mov   [WORD PTR timerTickCount],ax
    asm jnc   acknowledge
    savedInt8();
    asm jmp   done
acknowledge:
    outportb(0x0020, 0x20);
done:
    ;  /* VOID */
}

/*
Set the timer frequency based on whether or not the AdLib is enabled.
[ID_SD, SDL_SetTimerSpeed()]
*/
void InitializeInterruptRate(void)
{
    word rate;

    if (isAdLibEnabled == true) {  /* explicit compare against 1 */
        rate = 560;
    } else {
        rate = 140;
    }

    SetInterruptRate(rate);
}

/*
Handle global changes to the music state. [ID_SD, SD_SetMusicMode()]
*/
bool SetMusic(bool state)
{
    volatile bool found;  /* Force compiler *not* to use a register var */

    FadeOutAdLibPlayback();

    switch (state) {
    case false:
        junk4 = false;
        found = true;
        break;

    case true:
        if (isAdLibPresent2) {
            junk4 = true;
            found = true;
        }
        /* Possible BUG: `found` will hold garbage here w/ !isAdLibPresent2 */
        break;

    default:
        found = false;
        break;
    }

    if (found) {
        isAdLibEnabled = state;
    }

    InitializeInterruptRate();

    return found;
}

/*
Perform all startup tasks for the AdLib. [ID_SD, SD_Startup()]
*/
void StartAdLib(void)
{
    if (isAdLibStarted) return;

    junk5 = false;
    skipDetectAdLib = false;
    junk6 = 0;

    savedInt8 = getvect(8);

    ProfilePIT();

    setvect(8, TimerInterruptService);

    junk2 = junk3 = musicTickCount = 0;

    SetMusic(false);

    /* Useless check; this is always false here */
    if (!skipDetectAdLib) {
        isAdLibPresent2 = DetectAdLib();
    }

    isAdLibStarted = true;

    /* Rather inelegant, but this is how it was done. */
    isAdLibPresent = DetectAdLib();
}

/*
Perform all shutdown tasks for the AdLib. [ID_SD, SD_Shutdown()]
*/
void StopAdLib(void)
{
    if (!isAdLibStarted) return;

    StopAdLibPlayback();

    asm pushf

    disable();

    SetPIT0Value(0);
    setvect(8, savedInt8);

    asm popf

    isAdLibStarted = false;
}

/*
Activate the AdLib hardware for music playback. [ID_SD, SD_MusicOn()]
*/
void StartAdLibPlayback(void)
{
    isAdLibPlaying = true;
}

/*
Stop all note playback, then deactivate the AdLib hardware.
[ID_SD, SD_MusicOff()]
*/
void StopAdLibPlayback(void)
{
    word reg;

    switch (isAdLibEnabled) {
    case true:
        junk7 = 0;

        /* zero AM/Vibrato Depth/Rhythm */
        SetAdLibRegister(0xbd, 0);

        /* zero all Key On registers */
        for (reg = 0; reg < 10; reg++) {
            SetAdLibRegister(reg + 0xb1, 0);
        }

        break;
    }

    isAdLibPlaying = false;
}

/*
Start playback of a new piece of music. [ID_SD, SD_StartMusic()]
*/
void SwitchMusic(Music *music)
{
    StopAdLibPlayback();

    asm pushf

    disable();

    if (isAdLibEnabled == true) {  /* explicit compare against 1 */
        musicDataPtr = musicDataHead = &music->datahead;
        musicDataLength = musicDataLeft = music->length;

        musicNextDue = 0;
        musicTickCount = 0;

        StartAdLibPlayback();
    }

    asm popf
}

/*
Fade out the music (by switching it off abruptly). [ID_SD, SD_FadeOutMusic()]
*/
void FadeOutAdLibPlayback(void)
{
    switch (isAdLibEnabled) {
    case true:
        StopAdLibPlayback();
        break;
    }
}

/*
Wait until `delay` timer ticks have passed, then return.
*/
void WaitHard(word delay)
{
    gameTickCount = 0;

    while (gameTickCount < delay)
        ;  /* VOID */
}

/*
Wait until `delay` timer ticks have passed, or a key is pressed, then return.
*/
void WaitSoft(word delay)
{
    gameTickCount = 0;

    do {
        if (gameTickCount >= delay) break;
    } while ((inportb(0x0060) & 0x80) != 0);
}

/*
Fade the screen in (from all black to full color), one palette register at a
time. Wait `delay` timer ticks between each step. See the comments for
SetPaletteRegister() for an explanation of why a skip of 8 is applied.
*/
void FadeInCustom(word delay)
{
    word i;
    word skip = 0;

    for (i = 0; i < 16; i++) {
        if (i == 8) {
            skip = 8;
        }

        SetPaletteRegister(i, i + skip);
        WaitHard(delay);
    }
}

/*
Fill the screen with white, one palette register at a time. Wait `delay` timer
ticks between each step. See the comments for SetPaletteRegister() for an
explanation of why 8 is added to the WHITE constant.
*/
void FadeToWhite(word delay)
{
    word i;

    for (i = 0; i < 16; i++) {
        SetPaletteRegister(i, WHITE + 8);
        WaitHard(delay);
    }
}

/*
Draw a solid gray screen tile, to overwrite a former wait spinner location.
*/
void EraseWaitSpinner(word x, word y)
{
    EGA_RESET();

    DrawSolidTile(TILE_GRAY, x + (y * 320));
}

/*
Fade the screen out (from full color to all black), one palette register at a
time. Wait `delay` timer ticks between each step.
*/
void FadeOutCustom(word delay)
{
    int i;

    for (i = 0; i < 16; i++) {
        WaitHard(delay);
        SetPaletteRegister(i, BLACK);
    }
}

/*
Fade in with a fixed delay of 3.
*/
void FadeIn(void)
{
    FadeInCustom(3);
}

/*
Fade out with a fixed delay of 3.
*/
void FadeOut(void)
{
    FadeOutCustom(3);
}

/*
Draw one frame of the wait spinner on the screen at x,y and return the last
scancode read by the keyboard service. Does not wait.
*/
byte StepWaitSpinner(word x, word y)
{
    static word frameoff = 0;
    byte scancode = 0;

    EGA_RESET();

    if (gameTickCount > 5) {
        frameoff += 8;
        gameTickCount = 0;
    }

    if (frameoff == 32) frameoff = 0;

    DrawSolidTile(frameoff + TILE_WAIT_SPINNER_1, x + (y * 320));

    scancode = lastScancode;

    return scancode;
}

/*
Draw an animated wait spinner on the screen at x,y and wait indefinitely for a
key to become pressed. When that happens, return the scancode of the key that is
now down.
*/
byte WaitSpinner(word x, word y)
{
    byte scancode;

    /* Loop until key released (most likely already happened during whatever
    called the function we're now waiting in). */
    do {
        scancode = StepWaitSpinner(x, y);
    } while ((scancode & 0x80) == 0);

    /* Loop until key pressed */
    do {
        scancode = StepWaitSpinner(x, y);
    } while ((scancode & 0x80) != 0);

    scancode = lastScancode;
    isKeyDown[scancode] = false;

    EraseWaitSpinner(x, y);

    return scancode & ~0x80;
}

/*
Clear the screen by drawing a 40x25 page of black tiles.
*/
void ClearScreen(void)
{
    word x, ybase;

    EGA_RESET();

    for (ybase = 0; ybase < 8000; ybase += 320) {
        for (x = 0; x < 40; x++) {
            DrawSolidTile(TILE_EMPTY, ybase + x);  /* renders as solid black */
        }
    }
}

/*
Poll for the X,Y position of the specified joystick, and store the result into
the two provided timer pointers. If either timer exceeds 500 polls, abort.
[IDLIB, ReadJoystick()]
*/
void ReadJoystickTimes(word stick, int *x_time, int *y_time)
{
    word xmask, ymask;

    if (stick == JOYSTICK_A) {
        xmask = 0x0001;
        ymask = 0x0002;
    } else {  /* JOYSTICK_B */
        xmask = 0x0004;
        ymask = 0x0008;
    }

    *x_time = 0;
    *y_time = 0;

    outportb(0x0201, inportb(0x0201));

    do {
        word data = inportb(0x0201);
        int xwaiting = (data & xmask) != 0;
        int ywaiting = (data & ymask) != 0;

        *x_time += xwaiting;
        *y_time += ywaiting;

        if (xwaiting + ywaiting == 0) break;
    } while (*x_time < 500 && *y_time < 500);
}

/*
Translate raw timer data from the joystick into movement commands, taking into
account the current joystick calibration values and button swap configuration.
Returns the current state of *just* the joystick buttons -- movement commands
are changed globally. [IDLIB, ControlJoystick()]
*/
JoystickState ReadJoystickState(word stick)
{
    int xtime = 0, ytime = 0;
    int xmove = 0, ymove = 0;
    word buttons;
    JoystickState state;

    ReadJoystickTimes(stick, &xtime, &ytime);

    /*
    This is really, really a bitwise OR instead of logical. Whether this was
    intentional or not, the result is never true because ReadJoystickTimes()
    aborts before either value gets high enough.
    */
    if ((xtime > 500) | (ytime > 500)) {
        xtime = joystickXLow[stick] + 1;
        ytime = joystickYLow[stick] + 1;
    }

    if (xtime > joystickXHigh[stick]) {
        xmove = 1;
    } else if (xtime < joystickXLow[stick]) {
        xmove = -1;
    }

    if (ytime > joystickYHigh[stick]) {
        ymove = 1;
    } else if (ytime < joystickYLow[stick]) {
        ymove = -1;
    }

    cmdWest = false;
    cmdEast = false;
    cmdNorth = false;
    cmdSouth = false;

    switch ((ymove * 3) + xmove) {
    case -4:
        cmdNorth = true;
        cmdWest = true;
        break;
    case -3:
        cmdNorth = true;
        break;
    case -2:
        cmdNorth = true;
        cmdEast = true;
        break;
    case -1:
        cmdWest = true;
        break;
    case 1:
        cmdEast = true;
        break;
    case 2:
        cmdWest = true;
        cmdSouth = true;
        break;
    case 3:
        cmdSouth = true;
        break;
    case 4:
        cmdEast = true;
        cmdSouth = true;
        break;
    }

    buttons = inportb(0x0201);

    if (stick == JOYSTICK_A) {
        cmdJump = state.button1 = ((buttons & 0x0010) == 0);
        cmdBomb = state.button2 = ((buttons & 0x0020) == 0);
    }  /* JOYSTICK_B not used or implemented */

    if (joystickBtn1Bombs) {
        /* Swap button state, using `buttons` as a temp var */
        buttons = state.button1;
        cmdJump = state.button1 = state.button2;
        cmdBomb = state.button2 = buttons;
    }

    return state;
}

/*
Draw an empty UI frame, erasing whatever was previously in its place. The frame
may be arbitrarily positioned at any left,top coordinate, and the frame may have
any width,height size. There is technically no need for it to be centered on the
screen.

Accepts two lines of text, one to be written inside the top edge of the frame,
and one to be written inside the bottom edge. If `centered` is true, these text
lines are centered horizontally relative to the *screen*. If `centered` is
false, these text lines are left-aligned relative to the inside of the frame.

Returns the X coordinate of the inside-left edge of the frame.
*/
word DrawTextFrame(
    word left, word top, int height, int width, char *top_text,
    char *bottom_text, bbool centered
) {
    register int x, y;

    EGA_RESET();

    /* Draw background, implicitly erasing anything behind the frame */
    for (y = 1; height - 1 > y; y++) {
        for (x = 1; width - 1 > x; x++) {
            DrawSolidTile(TILE_GRAY, x + left + ((y + top) * 320));
        }
    }

    /* Draw vertical borders together */
    for (y = 0; y < height; y++) {
        DrawSolidTile(TILE_TXTFRAME_WEST, left + ((y + top) * 320));
        DrawSolidTile(TILE_TXTFRAME_EAST, left + width - 1 + ((y + top) * 320));
    }

    /* Deaw horizontal borders together */
    for (x = 0; x < width; x++) {
        DrawSolidTile(TILE_TXTFRAME_NORTH, x + left + (top * 320));
        DrawSolidTile(TILE_TXTFRAME_SOUTH, x + left + ((top + height - 1) * 320));
    }

    /* Draw four corners */
    DrawSolidTile(TILE_TXTFRAME_NORTHWEST, left + (top * 320));
    DrawSolidTile(TILE_TXTFRAME_NORTHEAST, width + left - 1 + (top * 320));
    DrawSolidTile(TILE_TXTFRAME_SOUTHWEST, left + ((top + height - 1) * 320));
    DrawSolidTile(TILE_TXTFRAME_SOUTHEAST, width + left - 1 + ((top + height - 1) * 320));

    /* Draw lines of text at the very top and bottom of the frame */
    if (centered) {
        /* No consideration given to `left` or `width` here! */
        DrawTextLine(20 - (strlen(top_text) / 2), top + 1, top_text);
        DrawTextLine(20 - (strlen(bottom_text) / 2), top + height - 2, bottom_text);
    } else {
        DrawTextLine(left + 1, top + 1, top_text);
        DrawTextLine(left + 1, top + height - 2, bottom_text);
    }

    return left + 1;
}

/*
Animate several successive calls to DrawTextFrame() to create an "unfolding"
effect. A tiny frame, centered horizontally on the screen, expands horizontally
until it reaches the final width, then it expands vertically until reaching the
final height. Arguments work the same as DrawTextFrame(), except `left` is
determined automatically (to center the frame horizontally) and `centered` is
forced true. Returns the X coordinate of the inside-left edge of the frame.
*/
word UnfoldTextFrame(
    int top, int height, int width, char *top_text, char *bottom_text
) {
    register int i;
    register int left = 20 - (width >> 1);
    word xcenter = 19;
    word ycenter = top + (height >> 1);
    word size = 1;

    for (i = xcenter; i > left; i--) {
        DrawTextFrame(i, ycenter, 2, size += 2, "", "", false);
        WaitHard(1);
    }

    size = 0;

    for (i = ycenter; top + !(height & 1) < i; i--) {
        DrawTextFrame(left, i, size += 2, width, "", "", false);
        WaitHard(1);
    }

    return DrawTextFrame(left, top, height, width, top_text, bottom_text, true);
}

/*
Present a wait spinner at x,y that accepts text input from the keyboard, echoing
characters to the screen as they are typed. The input is stored in the location
pointed to by `dest`, and no more than `max_length` characters of input are
allowed. `dest` should be large enough to hold `max_length + 1` characters.

NOTE: This draws everything 1 tile right of the location specified in `x`.
*/
void ReadAndEchoText(word x, word y, char *dest, word max_length)
{
    int pos = 0;

    for (;;) {
        /* Wait spinner erases itself upon return */
        byte scancode = WaitSpinner(x + pos + 1, y);

        if (scancode == SCANCODE_ENTER) {
            *(dest + pos) = '\0';
            break;
        } else if (scancode == SCANCODE_ESCAPE) {
            *dest = '\0';
            break;
        } else if (scancode == SCANCODE_BACKSPACE) {
            if (pos > 0) pos--;
        } else if (pos < max_length) {
            if (
                (scancode >= SCANCODE_1 && scancode <= SCANCODE_EQUALS) ||
                (scancode >= SCANCODE_Q && scancode <= SCANCODE_P) ||
                (scancode >= SCANCODE_A && scancode <= SCANCODE_APOSTROPHE) ||
                (scancode >= SCANCODE_Z && scancode <= SCANCODE_SLASH)
            ) {
                *(dest + pos++) = keyNames[scancode][0];
                DrawScancodeCharacter(x + pos, y, scancode);
            } else if (scancode == SCANCODE_SPACE) {
                *(dest + pos++) = ' ';
            }
        }
    }
}

/*
Measure the timing characteristics of the joystick, and store them in global
calibration variables. At any point during the inner loops, a keypress will
cancel the process. The function must run through to completion before the
joystick can be used in the game. [IDLIB, CalibrateJoy()]
*/
void ShowJoystickConfiguration(word stick)
{
    word xframe;
    word junk;
    word xthird, ythird;
    int lefttime, toptime, righttime, bottomtime;
    byte scancode = 0;
    JoystickState state;

    xframe = UnfoldTextFrame(3, 16, 30, "Joystick Config.", "Press ANY key.");

    /* Wait for all keys to be released */
    while ((lastScancode & 0x80) == 0)
        ;  /* VOID */

    /* Wait for all joystick buttons to be released */
    do {
        state = ReadJoystickState(stick);
    } while (state.button1 == true || state.button2 == true);

    DrawTextLine(xframe, 6, " Hold the joystick in the");
    DrawTextLine(xframe, 7, " UPPER LEFT and press a");
    DrawTextLine(xframe, 8, " button.");

    /* Capture left/top at the moment the button is pressed */
    junk = 15;
    do {
        if (++junk == 23) junk = 15;

        ReadJoystickTimes(stick, &lefttime, &toptime);
        state = ReadJoystickState(stick);
        scancode = StepWaitSpinner(xframe + 8, 8);

        if ((scancode & 0x80) == 0) return;
    } while (state.button1 != true && state.button2 != true);

    EraseWaitSpinner(xframe + 8, 8);
    WaitHard(160);

    /* Wait for all buttons to be released */
    do {
        state = ReadJoystickState(stick);
    } while (state.button1 == true || state.button2 == true);

    DrawTextLine(xframe, 10, " Hold the joystick in the");
    DrawTextLine(xframe, 11, " BOTTOM RIGHT and press a");
    DrawTextLine(xframe, 12, " button.");

    /* Capture right/bottom at the moment the button is pressed */
    do {
        if (++junk == 23) junk = 15;

        ReadJoystickTimes(stick, &righttime, &bottomtime);
        state = ReadJoystickState(stick);
        scancode = StepWaitSpinner(xframe + 8, 12);

        if ((scancode & 0x80) == 0) return;
    } while (state.button1 != true && state.button2 != true);

    EraseWaitSpinner(xframe + 8, 12);

    /* Wait for all buttons to be released */
    do {
        state = ReadJoystickState(stick);
    } while (state.button1 == true || state.button2 == true);

    /* The joystick must be 2/3 of the way to an edge to trigger a move */
    xthird = (righttime - lefttime) / 6;
    ythird = (bottomtime - toptime) / 6;
    joystickXLow[stick] = lefttime + xthird;
    joystickXHigh[stick] = righttime - xthird;
    joystickYLow[stick] = toptime + ythird;
    joystickYHigh[stick] = bottomtime - ythird;

    DrawTextLine(xframe, 14, " Should button 1 (D)rop");
    DrawTextLine(xframe, 15, " a bomb or (J)ump?");
    scancode = WaitSpinner(xframe + 19, 15);

    /*
    BUG: The joystick button flag is inverted in the original game. It's
    slightly more likely the mistake was here as opposed to ReadJoystickState().
    */
    if (scancode == SCANCODE_ESCAPE) {
        return;
    } else if (scancode == SCANCODE_J) {
        joystickBtn1Bombs = true;
    } else if (scancode == SCANCODE_D) {
        joystickBtn1Bombs = false;
    } else {
        return;
    }

    isJoystickReady = true;
}

/*
Draw a 32-bit unsigned long value using the game font. The rightmost digit is
anchored at {x,y}_origin, and the leftmost digit ends up "wherever."
*/
void DrawNumberFlushRight(word x_origin, word y_origin, dword value)
{
    int x, len;
    byte buf[16];  /* probably 5 bytes longer than it needs to be */

    EGA_MODE_DIRECT();

    ultoa(value, buf, 10);
    len = strlen(buf);

    /* Draw digits left-to-right */
    for (x = len - 1; x >= 0; x--) {
        DrawSpriteTile(fontTileData + FONT_0 + ((buf[len - x - 1] - '0') * 40), x_origin - x, y_origin);
    }
}

/*
Add `points` to the player's score, then redraw that area of the status bar.
This function does not know where that x/y position on the screen is, so those
need to be passed in.
*/
void DrawStatusBarScore(dword add_points, word x, word y)
{
    gameScore += add_points;

#ifdef DEBUG_BAR
    return;
#endif  /* DEBUG_BAR */

    SelectDrawPage(activePage);
    DrawNumberFlushRight(x, y, gameScore);

    SelectDrawPage(!activePage);
    DrawNumberFlushRight(x, y, gameScore);

    EGA_RESET();
}

/*
Add `points` to the player's score, then redraw that area of the status bar.
*/
void AddScore(dword points)
{
    DrawStatusBarScore(points, 9, 22);
}

/*
Redraw the "Stars" area of the status bar with the current global value. This
function does not know where that x/y position on the screen is, so those need
to be passed in.
*/
void DrawStatusBarStars(word x, word y)
{
#ifdef DEBUG_BAR
    return;
#endif  /* DEBUG_BAR */

    SelectDrawPage(activePage);
    DrawNumberFlushRight(x, y, (word)gameStars);

    SelectDrawPage(!activePage);
    DrawNumberFlushRight(x, y, (word)gameStars);

    EGA_RESET();
}

/*
Redraw the "Stars" area of the status bar with the current global value.
*/
void UpdateStars(void)
{
    DrawStatusBarStars(35, 22);
}

/*
Redraw the "Bombs" area of the status bar with the current global value. This
function does not know where that x/y position on the screen is, so those need
to be passed in.
*/
void DrawStatusBarBombs(word x, word y)
{
#ifdef DEBUG_BAR
    return;
#endif  /* DEBUG_BAR */

    EGA_MODE_DIRECT();

    /*
    Draw a blank tile, then overdraw the number. The blanking is not strictly
    necessary since the bomb count is fixed-width and the font digits have no
    transparent areas.
    */
    SelectDrawPage(activePage);
    DrawSpriteTile(fontTileData + FONT_BACKGROUND_GRAY, x, y);
    DrawNumberFlushRight(x, y, playerBombs);

    SelectDrawPage(!activePage);
    DrawSpriteTile(fontTileData + FONT_BACKGROUND_GRAY, x, y);
    DrawNumberFlushRight(x, y, playerBombs);

    EGA_RESET();
}

/*
Redraw the "Bombs" area of the status bar with the current global value.
*/
void UpdateBombs(void)
{
    DrawStatusBarBombs(24, 23);
}

/*
Redraw the "Health" area of the status bar with the current global value. This
function does not know where that x/y position on the screen is, so those need
to be passed in. It also does not flip the draw page, so this function must be
called twice -- once for each page.
*/
void DrawStatusBarHealth(word x, word y)
{
    word bar;

#ifdef DEBUG_BAR
    return;
#endif  /* DEBUG_BAR */

    for (bar = 0; bar < playerMaxHealth; bar++) {
        /* Why 8 if there are only 5 health bar spaces? Go replay DUKE1. */
        if (bar >= 8) continue;

        if (playerHealth - 1 > bar) {
            /* Each bar is made of 2 font characters stacked vertically */
            DrawSpriteTile(fontTileData + FONT_UPPER_BAR_1, x - bar, y);
            DrawSpriteTile(fontTileData + FONT_LOWER_BAR_1, x - bar, y + 1);
        } else {
            DrawSpriteTile(fontTileData + FONT_UPPER_BAR_0, x - bar, y);
            DrawSpriteTile(fontTileData + FONT_LOWER_BAR_0, x - bar, y + 1);
        }
    }
}

/*
Redraw the "Health" area of the status bar with the current global value, on the
active draw page only.
*/
void DrawSBarHealthHelper(void)
{
    DrawStatusBarHealth(17, 22);
}

/*
Redraw the "Health" area of the status bar with the current global value.
*/
void UpdateHealth(void)
{
    EGA_MODE_DIRECT();

    SelectDrawPage(activePage);
    DrawSBarHealthHelper();

    SelectDrawPage(!activePage);
    DrawSBarHealthHelper();
}

/*
Display the high score table, with an option to zero out all the scores by
pressing F10. This does not restore the Simpsons names like deleting the config
file would -- they get totally blanked.
*/
void ShowHighScoreTable(void)
{
    for (;;) {
        word x, i;
        byte scancode;

        x = UnfoldTextFrame(2, 17, 30, "Hall of Fame", "any other key to exit.");
        for (i = 0; i < 10; i++) {
            /* _1. ___1234 NAME */
            DrawNumberFlushRight(x + 2, i + 5, i + 1);
            DrawTextLine(x + 3, i + 5, ".");
            DrawNumberFlushRight(x + 11, i + 5, highScoreValues[i]);
            DrawTextLine(x + 13, i + 5, highScoreNames[i]);
        }
        DrawTextLine(x + 3, 16, "Press 'F10' to erase or");

        if (!isInGame) {
            FadeIn();
        }

        scancode = WaitSpinner(x + 27, 17);
        if (scancode != SCANCODE_F10) break;

        x = UnfoldTextFrame(5, 4, 28, "Are you sure you want to", "ERASE High Scores?");

        scancode = WaitSpinner(x + 22, 7);
        if (scancode == SCANCODE_Y) {
            for (i = 0; i < 10; i++) {
                highScoreValues[i] = 0;
                highScoreNames[i][0] = '\0';
            }
        }

        if (!isInGame) {
            FadeOut();
            ClearScreen();
        }
    }
}

/*
Check if the player's current score qualifies for insertion into the high score
table, and if so, prompt for a name to go along with it. Display the high score
table at the end whether the prompt was displayed or not.
*/
void CheckHighScore(void)
{
    int i;

    FadeOut();
    SelectDrawPage(0);
    SelectActivePage(0);
    ClearScreen();

    for (i = 0; i < 10; i++) {
        int inferior;
        word x;

        if (highScoreValues[i] >= gameScore) continue;

        /* Shift all inferior scores down one position */
        for (inferior = 10; inferior > i; inferior--) {
            highScoreValues[inferior] = highScoreValues[inferior - 1];
            strcpy(highScoreNames[inferior], highScoreNames[inferior - 1]);
        }

        highScoreNames[i][0] = '\0';
        highScoreValues[i] = gameScore;

        x = UnfoldTextFrame(5, 7, 36, "You made it into the hall of fame!", "Press ESC to quit.");
        DrawTextLine(x, 8, "Enter your name:");
        FadeIn();
        StartSound(SND_HIGH_SCORE_SET);

        ReadAndEchoText(x + 16, 8, highScoreNames[i], 14);

        break;
    }

    FadeOut();
    ClearScreen();
    StartSound(SND_HIGH_SCORE_DISPLAY);
    ShowHighScoreTable();
}

/*
Return a file pointer matching the passed group entry name and update
lastGroupEntryLength with the size of the entry's data. This function tries, in
order: entry inside the STN file, entry inside the VOL file, file in the current
working directory with a name matching the passed entry name. If nothing is
found, assuming the program doesn't crash due to performing a bunch of
operations on a null pointer, return NULL.

NOTE: This uses a 20-byte buffer and an 11-byte comparison on a file format
whose entry names are each 12 bytes long. Fun ensues.

NOTE: The STN/VOL format is not formally specified anywhere, so it's not clear
if the offsets/lengths read from it are supposed to be interpreted as signed or
unsigned. This function uses a bit of a mismash of the two.
*/
FILE *GroupEntryFp(char *entry_name)
{
    byte header[1000];
    byte name[20];
    FILE *fp;
    dword offset;
    int i;
    bool found;

    /* Make an uppercased copy of the entry name as recklessly as possible */
    for (i = 0; i < 19; i++) {
        name[i] = *(entry_name + i);
    }
    name[19] = '\0';
    strupr(name);

    /*
    Attempt to read from the STN file
    */
    fp = fopen(stnGroupFilename, "rb");
    found = false;
    fread(header, 1, 960, fp);

    for (i = 0; i < 980; i += 20) {
        if (header[i] == '\0') break;  /* no more entries */

        if (strncmp(header + i, name, 11) == 0) {
            offset = i + 12;
            found = true;
        }
    }

    if (!found) {
        fclose(fp);

        /*
        Attempt to read from the VOL file
        */
        fp = fopen(volGroupFilename, "rb");
        fread(header, 1, 960, fp);

        for (i = 0; i < 980; i += 20) {
            if (header[i] == '\0') break;  /* no more entries */

            if (strncmp(header + i, name, 11) == 0) {
                offset = i + 12;
                found = true;
            }
        }

        if (!found) {
            fclose(fp);

            /*
            Attempt to read from the current working directory
            */
            fp = fopen(entry_name, "rb");
            i = fileno(fp);
            lastGroupEntryLength = filelength(i);

            return fp;
        }
    }

    /* Here `offset` points to the entry's header data */
    fseek(fp, offset, SEEK_SET);
    fread(&offset, 4, 1, fp);
    fread(&lastGroupEntryLength, 4, 1, fp);

    /* Now `offset` points to the first byte of the entry's data */
    fseek(fp, offset, SEEK_SET);

    return fp;
}

/*
Return true if there is no AdLib hardware installed, and false otherwise.
*/
bbool IsAdLibAbsent(void)
{
    return !isAdLibPresent2;
}

/*
Read music data from the group entry referred to by the music number, and store
it into the passed Music pointer.
*/
Music *LoadMusicData(word music_num, Music *dest)
{
    FILE *fp;
    Music *localdest = dest;  /* not clear why this copy is needed */

    miscDataContents = IMAGE_NONE;

    fp = GroupEntryFp(musicNames[music_num]);
    fread(&dest->datahead, 1, (word)lastGroupEntryLength + 2, fp);
    localdest->length = (word)lastGroupEntryLength;

    SetMusic(true);

    fclose(fp);

    return localdest;
}

#if EPISODE == 1
/*
Maintain parity with the original game.
*/
void Junk1(void) {}

/*
Maintain parity with the original game.
*/
void Junk2(void) {}
#endif  /* EPISODE == 1 */

/*
Display several pages of ordering information if playing the shareware version,
or one page thanking the player for purchasing the registered game.
*/
void ShowOrderingInformation(void)
{
    word x;

    FadeOut();
    ClearScreen();

#ifdef SHAREWARE
    x = UnfoldTextFrame(0, 24, 38, "Ordering Information", "Press ANY key.");
    DrawTextLine(x, 2,  "  \xFE""223000                              \xFE""223000");
    DrawTextLine(x, 4,  "      COSMO'S COSMIC ADVENTURE");
    DrawTextLine(x, 5,  "    consists of three adventures.");
    DrawTextLine(x, 7,  "    Only the first adventure is");
    DrawTextLine(x, 8,  " available as shareware.  The final");
    DrawTextLine(x, 9,  "   two amazing adventures must be");
    DrawTextLine(x, 10, "    purchased from Apogee, or an");
    DrawTextLine(x, 11, "          authorized dealer.");
    DrawTextLine(x, 13, "  The last two adventures of Cosmo");
    DrawTextLine(x, 14, "   feature exciting new graphics,");
    DrawTextLine(x, 15, "  new creatures, new puzzles, new");
    DrawTextLine(x, 16, "   music and all-new challenges!");
    DrawTextLine(x, 18, "    The next few screens provide");
    DrawTextLine(x, 19, "       ordering instructions.");
    DrawTextLine(x, 22, "  \xFE""155000                              \xFE""154001");
    FadeInCustom(1);
    WaitSpinner(x + 35, 22);

    FadeOutCustom(1);
    ClearScreen();

    x = UnfoldTextFrame(1, 22, 38, "Ordering Information", "Press ANY key.");
    DrawTextLine(x, 4,  "       Order now and receive:");
    DrawTextLine(x, 6,  "   * All three exciting adventures");
    DrawTextLine(x, 7,  "   * The hints and tricks sheet");
    DrawTextLine(x, 8,  "   * The Secret Cheat password");
    DrawTextLine(x, 9,  "   * Exciting new bonus games");
    DrawTextLine(x, 11, "      To order, call toll free:");
    DrawTextLine(x, 12, "           1-800-426-3123");
    DrawTextLine(x, 13, "   (Visa and MasterCard Welcome)");
    DrawTextLine(x, 15, "   Order all three adventures for");
    DrawTextLine(x, 16, "     only $35, plus $4 shipping.");
    DrawTextLine(x, 19, "              \xFE""129002");
    DrawTextLine(x, 20, "  \xFB""014                          \xFB""015");
    FadeInCustom(1);
    WaitSpinner(x + 35, 21);

    FadeOutCustom(1);
    ClearScreen();

    x = UnfoldTextFrame(1, 22, 38, "Ordering Information", "Press ANY key.");
    DrawTextLine(x, 4,  "      Please specify disk size:");
    DrawTextLine(x, 5,  "           5.25\"  or  3.5\"");
    DrawTextLine(x, 7,  "     To order send $35, plus $4");
    DrawTextLine(x, 8,  "      shipping, USA funds, to:");
    DrawTextLine(x, 10, "           Apogee Software");
    DrawTextLine(x, 11, "           P.O. Box 476389");
    DrawTextLine(x, 12, "       Garland, TX 75047  (USA)");
    DrawTextLine(x, 14, "\xFE""101003       Or CALL NOW toll free:  \xFE""101000");
    DrawTextLine(x, 15, "           1-800-426-3123");
    DrawTextLine(x, 18, "         ORDER COSMO TODAY!");
    DrawTextLine(x, 19, "           All 3 for $39!");
    DrawTextLine(x, 20, "  \xFB""014                          \xFB""015");
    FadeInCustom(1);
    WaitSpinner(x + 35, 21);

    FadeOutCustom(1);
    ClearScreen();

    x = UnfoldTextFrame(4, 15, 38, "USE YOUR FAX MACHINE TO ORDER!", "Press ANY key.");
    DrawTextLine(x, 7,  "  You can now use your FAX machine");
    DrawTextLine(x, 8,  "   to order your favorite Apogee");
    DrawTextLine(x, 9,  "     games quickly and easily.");
    DrawTextLine(x, 11, "   Simply print out the ORDER.FRM");
    DrawTextLine(x, 12, "    file, fill it out and FAX it");
    DrawTextLine(x, 13, "    to us for prompt processing.");
    DrawTextLine(x, 15, "     FAX Orders: (214) 278-4670");
    FadeInCustom(1);
    WaitSpinner(x + 35, 17);

    FadeOutCustom(1);
    ClearScreen();

    x = UnfoldTextFrame(1, 20, 38, "About Apogee Software", "Press ANY key.");
    x += 2;
    DrawTextLine(x, 4,  "Our goal is to establish Apogee");
    DrawTextLine(x, 5,  "  as the leader in commercial");
    DrawTextLine(x, 6,  " quality shareware games. With");
    DrawTextLine(x, 7,  " enthusiasm and dedication we");
    DrawTextLine(x, 8,  "think our goal can be achieved.");
    DrawTextLine(x, 10, "However,  we need your support.");
    DrawTextLine(x, 11, "Shareware is not free software.");
    DrawTextLine(x, 13, "  We thank you in advance for");
    DrawTextLine(x, 14, "   your contribution to the");
    DrawTextLine(x, 15, "  growing shareware community.");
    DrawTextLine(x - 2, 17, "\xFD""010        Your honesty pays...     \xFD""033");
    FadeInCustom(1);
    WaitSpinner(x + 33, 19);
#else
    x = UnfoldTextFrame(0, 24, 38, "Ordering Information", "Press ANY key.");
    DrawTextLine(x,  4, "      COSMO'S COSMIC ADVENTURE");
    DrawTextLine(x,  6, "  This game IS commercial software.");
    DrawTextLine(x,  8, "    This episode of Cosmo is NOT");
    DrawTextLine(x,  9, " available as shareware.  It is not");
    DrawTextLine(x, 10, "  freeware, nor public domain.  It");
    DrawTextLine(x, 11, "  is only available from Apogee or");
    DrawTextLine(x, 12, "        authorized dealers.");
    DrawTextLine(x, 14, " If you are a registered player, we");
    DrawTextLine(x, 15, "    thank you for your patronage.");
    DrawTextLine(x, 17, "  Please report any illegal selling");
    DrawTextLine(x, 18, "  and distribution of this game to");
    DrawTextLine(x, 19, "  Apogee by calling 1-800-GAME123.");
    FadeInCustom(1);
    WaitSpinner(x + 35, 22);
#endif  /* SHAREWARE */
}

#if EPISODE != 1
/*
Display the final end-game congratulations screen, or no-op depending on ifdef.
*/
void ShowCongratulations(void)
{
#   ifdef END_GAME_CONGRATS
    word x;

    FadeOut();
    ClearScreen();

    x = UnfoldTextFrame(0, 23, 38, "CONGRATULATIONS!", "Press ANY key.");
    x += 2;
    DrawTextLine(x, 3,  "You saved Cosmo's parents and");
    DrawTextLine(x, 4,  "landed at Disney World for the");
    DrawTextLine(x, 5,  "best birthday of your life.");
    DrawTextLine(x, 7,  "After a great birthday on Earth,");
    DrawTextLine(x, 8,  "you headed home and told all of");
    DrawTextLine(x, 9,  "your friends about your amazing");
    DrawTextLine(x, 10, "adventure--no one believed you!");
    DrawTextLine(x, 12, "Maybe on your next adventure you");
    DrawTextLine(x, 13, "can take pictures!");
    /* Fun fact, DN2 slipped this advertised release date by about 12 months. */
    DrawTextLine(x, 15, "Coming Dec. 92: Duke Nukum II --");
    DrawTextLine(x, 16, "The amazing sequel to the first");
    DrawTextLine(x, 17, "Nukum trilogy, in which Duke is");
    DrawTextLine(x, 18, "kidnapped by an alien race to");
    DrawTextLine(x, 19, "save them from termination...");
    FadeInCustom(1);
    WaitSpinner(x + 33, 21);
#   endif  /* END_GAME_CONGRATS */
}

/*
Maintain parity with the original game.
*/
void Junk3(void) {}
#endif  /* EPISODE != 1 */

/*
Show the game story across several pages of text.
*/
void ShowStory(void)
{
    word x;

    FadeOut();
    ClearScreen();

    x = UnfoldTextFrame(1, 23, 38, "STORY", "Press ANY key.");
    DrawTextLine(x + 1,  8,  "\xFB""000");
    DrawTextLine(x + 1,  20, "\xFB""002");
    DrawTextLine(x + 16, 5,  "Tomorrow is Cosmo's");
    DrawTextLine(x + 16, 7,  "birthday, and his");
    DrawTextLine(x + 16, 9,  "parents are taking");
    DrawTextLine(x + 16, 11, "him to the one place");
    DrawTextLine(x + 16, 13, "in the Milky Way");
    DrawTextLine(x + 16, 15, "galaxy that all kids");
    DrawTextLine(x + 16, 17, "would love to go to:");
    DrawTextLine(x + 16, 19, "   Disney World!");
    FadeIn();
    WaitSpinner(x + 35, 22);

    FadeOut();
    ClearScreen();

    x = UnfoldTextFrame(1, 23, 38, "STORY", "Press ANY key.");
    DrawTextLine(x + 3,  12, "\xFB""003");
    DrawTextLine(x + 25, 12, "\xFB""004");
    DrawTextLine(x + 3,  5,  "Suddenly a blazing comet zooms");
    DrawTextLine(x + 4,  7,  "toward their ship--leaving no");
    DrawTextLine(x + 16, 10, "time");
    DrawTextLine(x + 17, 12, "to");
    DrawTextLine(x + 10, 15, "change course...");
    FadeIn();
    WaitSpinner(x + 35, 22);

    FadeOut();
    ClearScreen();

    x = UnfoldTextFrame(1, 23, 38, "STORY", "Press ANY key.");
    DrawTextLine(x + 2,  7,  "\xFB""005");
    DrawTextLine(x + 25, 20, "\xFB""006");
    DrawTextLine(x + 15, 7,  "The comet slams into");
    DrawTextLine(x + 1,  10, "the ship and forces Cosmo's");
    DrawTextLine(x + 1,  13, "dad to make an");
    DrawTextLine(x + 1,  15, "emergency landing");
    DrawTextLine(x + 1,  17, "on an uncharted");
    DrawTextLine(x + 1,  19, "planet.");
    FadeIn();
    WaitSpinner(x + 35, 22);

    FadeOut();
    ClearScreen();

    x = UnfoldTextFrame(1, 23, 38, "STORY", "Press ANY key.");
    DrawTextLine(x + 17, 9,  "\xFB""007");
    DrawTextLine(x + 1,  20, "\xFB""008");
    DrawTextLine(x + 2,  5,  "While Cosmo's");
    DrawTextLine(x + 2,  7,  "dad repairs");
    DrawTextLine(x + 2,  9,  "the ship,");
    DrawTextLine(x + 11, 15, "Cosmo heads off to");
    DrawTextLine(x + 11, 17, "explore and have");
    DrawTextLine(x + 11, 19, "some fun.");
    FadeIn();
    WaitSpinner(x + 35, 22);

    FadeOut();
    ClearScreen();

    x = UnfoldTextFrame(1, 23, 38, "STORY", "Press ANY key.");
    DrawTextLine(x + 3,  15, "\xFB""009");
    DrawTextLine(x + 6,  7,  "Returning an hour later,");
    DrawTextLine(x + 17, 11, "Cosmo cannot find");
    DrawTextLine(x + 17, 13, "his Mom or Dad.");
    DrawTextLine(x + 17, 15, "Instead, he finds");
    DrawTextLine(x + 8,  18, "strange foot prints...");
    FadeIn();
    WaitSpinner(x + 35, 22);

    FadeOut();
    ClearScreen();

    x = UnfoldTextFrame(1, 23, 38, "STORY", "Press ANY key.");
    DrawTextLine(x + 21, 19, "\xFB""010");
    DrawTextLine(x + 2,  5,  "...oh no!  Has his");
    DrawTextLine(x + 2,  7,  "family been taken");
    DrawTextLine(x + 2,  9,  "away by a hungry");
    DrawTextLine(x + 2,  11, "alien creature to");
    DrawTextLine(x + 2,  13, "be eaten?  Cosmo");
    DrawTextLine(x + 2,  15, "must rescue his");
    DrawTextLine(x + 2,  17, "parents before");
    DrawTextLine(x + 2,  19, "it's too late...!");
    FadeIn();
    WaitSpinner(x + 35, 22);
}

/*
Start playing the music referred to by the passed music number. This uses the
tail end of the demo data area for storage. This is the necessary function to
use when playing music inside the game loop.
*/
void StartGameMusic(word music_num)
{
    if (IsAdLibAbsent()) return;

    activeMusic = LoadMusicData(music_num, (Music *) (miscData + 5000));

    if (isMusicEnabled) {
        SwitchMusic(activeMusic);
    }
}

/*
Start playing the music referred to by the passed music number. This uses the
masked tile data area for storage, and cannot be used while the game loop is
running.
*/
void StartMenuMusic(word music_num)
{
    if (IsAdLibAbsent()) return;

    activeMusic = LoadMusicData(music_num, (Music *) maskedTileData);

    if (isMusicEnabled) {
        SwitchMusic(activeMusic);
    }
}

/*
Stop playing all music.
*/
void StopMusic(void)
{
    if (IsAdLibAbsent()) return;

    StopAdLibPlayback();
}

/*
Print one character at x,y reflecting the passed scancode.

NOTE: Since this uses the global "key names" array to map scancodes to
characters, this won't always behave correctly for non-printable characters --
the tab key will render as "T", the left shift key will render as "L", etc.
*/
void DrawScancodeCharacter(word x, word y, byte scancode)
{
    byte buf[2];

    buf[0] = keyNames[scancode][0];
    buf[1] = '\0';

    DrawTextLine(x, y, buf);
}

/*
Draw UI text (without frame) instructing the user to enter a new key to bind to
the action described in the `feedback` text. `x` is used to position the text
horizontally. The typed scancode is written into `target_var` and *false* is
returned. If the user cancels by pressing Esc, *true* is returned with no change
to the target variable.
*/
bbool PromptKeyBind(byte *target_var, word x, char *feedback)
{
    byte scancode;

    DrawTextLine(x + 4, 12, feedback);
    DrawTextLine(x + 4, 13, "Enter new key:");
    scancode = WaitSpinner(x + 18, 13);

    if (scancode == SCANCODE_ESCAPE) {
        return true;
    }

    *target_var = scancode;

    return false;
}

/*
Present the keyboard configuration menu. Any of the six command keys can be set
to different scancodes. There is no sanity checking performed here -- the user
is free to bind multiple commands to the same key, and it's possible to bind
in-game keys like F1 and M to commands, with predictably wacky results. The only
disallowed key is Esc, which exits this menu unconditionally.

NOTE: Entering this menu disables the joystick, requiring a recalibration to get
it back.
*/
void ShowKeyboardConfiguration(void)
{
    word x;

    isJoystickReady = false;

    /* The size, position, and empty space within this frame is calibrated to
    the positioning in KeyboardConfigPrompt(). */
    x = UnfoldTextFrame(3, 15, 27, "Keyboard Config.", "Press ESC to quit.");

    for (;;) {
        byte scancode;

        DrawTextLine(x,      6,  " #1) Up key is:");
        DrawTextLine(x + 19, 6,  keyNames[scancodeNorth]);
        DrawTextLine(x,      7,  " #2) Down key is:");
        DrawTextLine(x + 19, 7,  keyNames[scancodeSouth]);
        DrawTextLine(x,      8,  " #3) Left key is:");
        DrawTextLine(x + 19, 8,  keyNames[scancodeWest]);
        DrawTextLine(x,      9,  " #4) Right key is:");
        DrawTextLine(x + 19, 9,  keyNames[scancodeEast]);
        DrawTextLine(x,      10, " #5) Jump key is:");
        DrawTextLine(x + 19, 10, keyNames[scancodeJump]);
        DrawTextLine(x,      11, " #6) Bomb key is:");
        DrawTextLine(x + 19, 11, keyNames[scancodeBomb]);
        DrawTextLine(x,      15, "Select key # to change or");
        scancode = WaitSpinner(x + 21, 16);

        switch (scancode) {
        case SCANCODE_ESCAPE:
            return;
        case SCANCODE_1:
            if (!PromptKeyBind(&scancodeNorth, x, "Modifying UP.")) break;
            return;
        case SCANCODE_2:
            if (!PromptKeyBind(&scancodeSouth, x, "Modifying DOWN.")) break;
            return;
        case SCANCODE_3:
            if (!PromptKeyBind(&scancodeWest, x, "Modifying LEFT.")) break;
            return;
        case SCANCODE_4:
            if (!PromptKeyBind(&scancodeEast, x, "Modifying RIGHT.")) break;
            return;
        case SCANCODE_5:
            if (!PromptKeyBind(&scancodeJump, x, "Modifying JUMP.")) break;
            return;
        case SCANCODE_6:
            if (!PromptKeyBind(&scancodeBomb, x, "Modifying BOMB.")) break;
            return;
        }

        /* Quickly redraw the same frame as before, without animation, to clear
        the config prompt text. */
        DrawTextFrame(7, 3, 15, 27, "Keyboard Config.", "Press ESC to quit.", true);
    }
}

/*
Shuffle around a bunch of memory contents to prepare a vertical-scrolling
backdrop. How? Dunno.

Arguments might be: source, destination, scratch
*/
void PrepareBackdropVScroll(byte *p1, byte *p2, byte *p3)
{
    register word x, A;
    word B, C, y;

    /* Backdrops are 40x18 tiles, 0x5A00 bytes. */

    B = 0;
    C = 0;
    for (x = 0; x < 40; x++) {
        for (A = 0; A < 16; A++) {  /* 16 times */
            *(p3 + B++) = *(p1 + C++);
        }
        C += 16;
    }

    for (y = 0; y < 18; y++) {
        C = 0;
        for (x = 0; x < 40; x++) {
            for (A = 0; A < 16; A++) {  /* 16 times */
                /* C in lvalue has the PRE-incremented value */
                *(p2 + C + (y * 0x0500)) = *(p1 + (y * 0x0500) + C++ + 16);
            }
            C += 16;
        }

        C = 0;
        for (x = 0; x < 40; x++) {
            for (A = 0; A < 16; A++) {  /* 16 times */
                /* C in lvalue has the PRE-incremented value */
                *(p2 + C + (y * 0x0500) + 16) = *(p1 + (y * 0x0500) + C++ + 0x0500);
            }
            C += 16;
        }
    }

    C = 0;
    B = 0;
    for (x = 0; x < 40; x++) {
        for (A = 0; A < 16; A++) {  /* 16 times */
            *(p2 + C++ + 0x5510) = *(p3 + B++);
        }
        C += 16;
    }
}

/*
Shuffle around a bunch of memory contents to prepare a horizontal-scrolling
backdrop. How? Dunno.

Arguments might be: source, destination
*/
void PrepareBackdropHScroll(byte *p1, byte *p2)
{
    register word y, x;
    word A, B;
    byte buf[4];

    /* Backdrops are 40x18 tiles, 0x5A00 bytes. */

    for (y = 0; y < 0x5a00; y += 0x0500) {  /* 18 times */
        for (B = 0; B < 32; B += 4) {  /* 8 times */
            for (A = 0; A < 4; A++) {  /* 4 times */
                buf[A] = *(p1 + A + B + y) >> 4;
            }

            for (x = 0; x < 0x0500; x += 32) {  /* 40 times */
                for (A = 0; A < 4; A++) {  /* 4 times */
                    *(p2 + x + A + B + y) = *(p1 + x + A + B + y) << 4;
                }

                if (x != 0x04e0) {  /* not the last iteration */
                    for (A = 0; A < 4; A++) {  /* 4 times */
                        *(p2 + x + A + B + y) = *(p2 + A + B + y + x) | (*(p1 + A + B + y + x + 32) >> 4);
                    }
                }
            }

            for (A = 0; A < 4; A++) {  /* 4 times */
                *(p2 + A + y + B + 0x04e0) = *(p2 + A + y + B + 0x04e0) | buf[A];
            }
        }
    }
}

/*
Display a few pages of helpful hints. This can be accessed either through the
main menu or through the in-game menu. Each caller provides its own y value to
change the vertical position on the screen. The in-game menu shows hints higher
up so they clear the status bar, saving a redraw of that area when the hints are
dismissed.
*/
void ShowHintsAndKeys(word y)
{
    word x;
    word y1 = y - 1;

    x = UnfoldTextFrame(y, 18, 38, "Cosmic Hints", "Press ANY key.");
    DrawTextLine(x, y1 + 4,  " * Usually jumping in the paths of");
    DrawTextLine(x, y1 + 5,  "   bonus objects will lead you in");
    DrawTextLine(x, y1 + 6,  "   the right direction.");
    DrawTextLine(x, y1 + 8,  " * There are many secret bonuses in");
    DrawTextLine(x, y1 + 9,  "   this game, such as bombing 15 of");
    DrawTextLine(x, y1 + 10, "   the Eye Plants.  (Registered");
    DrawTextLine(x, y1 + 11, "   players will get the full list.)");
    DrawTextLine(x, y1 + 13, " * When clinging to a wall, tap the");
    DrawTextLine(x, y1 + 14, "   jump key to let go and fall.  To");
    DrawTextLine(x, y1 + 15, "   re-cling to the wall, push");
    DrawTextLine(x, y1 + 16, "   yourself into the wall again.");
    WaitSpinner(x + 35, y1 + 17);

    x = UnfoldTextFrame(y, 18, 38, "Key Definition Screen", "");
    DrawTextLine(x,      y1 + 4,  "                     Look");
    DrawTextLine(x,      y1 + 5,  "                      UP");
    DrawTextLine(x,      y1 + 7,  "              Walk            Walk");
    DrawTextLine(x,      y1 + 8,  "  Jump  Drop  LEFT            RIGHT");
    DrawTextLine(x,      y1 + 9,  "   UP   BOMB");
    DrawTextLine(x,      y1 + 10, "                     \xFD""028");
    DrawTextLine(x,      y1 + 11, "                     Look");
    DrawTextLine(x,      y1 + 12, "                     DOWN");
    DrawTextLine(x,      y1 + 13, "              \xFD""001                 \xFD""023");
    DrawTextLine(x,      y1 + 14, "  \xFD""030      \xFD""037   \xFE""024000");
    DrawTextLine(x,      y1 + 17, "                     \xFD""029");
    DrawTextLine(x + 24, y1 + 7,  keyNames[scancodeNorth]);
    DrawTextLine(x + 24, y1 + 14, keyNames[scancodeSouth]);
    DrawTextLine(x + 14, y1 + 14, keyNames[scancodeWest]);
    DrawTextLine(x + 30, y1 + 14, keyNames[scancodeEast]);
    DrawTextLine(x + 2,  y1 + 15, keyNames[scancodeJump]);
    DrawTextLine(x + 8,  y1 + 15, keyNames[scancodeBomb]);
    WaitSpinner(x + 35, y1 + 17);

    x = UnfoldTextFrame(4, 11, 34, "During the game, you can...", "Press ANY key.");
    DrawTextLine(x, 7,  " Press 'P' to PAUSE GAME");
    DrawTextLine(x, 8,  " Press 'ESC' or 'Q' to QUIT game");
    DrawTextLine(x, 9,  " Press 'S' to toggle SOUND");
    DrawTextLine(x, 10, " Press 'M' to toggle MUSIC");
    DrawTextLine(x, 11, " Press 'F1' to show HELP");
    WaitSpinner(x + 31, 13);
}

/*
Display five pages of instruction text, followed by hints. The instructions can
be paged through by using the up/down or PgUp/PgDn keys, or exited with Esc.
(Actually, pressing any key will advance to the next page.)
*/
void ShowInstructions(void)
{
    word x;
    byte scancode;

    FadeOut();
    ClearScreen();

    /* In this function, you'll see SCANCODE_NUM_8 and _9 tested. On the PC
    keyboard, numpad 8 (with Num Lock off) functions as Up, and numpad 9
    functions as Pg Up. The scancodes mirror this arrangement. */

page1:
    FadeOutCustom(1);

    x = UnfoldTextFrame(0, 24, 38, "Instructions  Page One of Five", "Press PgDn for next.  ESC to Exit.");
    DrawTextLine(x, 4,  " OBJECT OF GAME:");
    DrawTextLine(x, 6,  " On a strange and dangerous planet,");
    DrawTextLine(x, 8,  " Cosmo must find and rescue his");
    DrawTextLine(x, 10, " parents.");
    DrawTextLine(x, 13, " Cosmo, having seen big scary alien");
    DrawTextLine(x, 15, " footprints, believes his parents");
    DrawTextLine(x, 17, " have been captured and taken away");
    DrawTextLine(x, 19, " to be eaten!");
    FadeInCustom(1);

    /* "Previous page" keys are no-ops here */
    do {
        scancode = WaitSpinner(x + 35, 22);
    } while (scancode == SCANCODE_NUM_9 || scancode == SCANCODE_NUM_8);
    if (scancode == SCANCODE_ESCAPE) return;

page2:
    FadeOutCustom(1);

    x = UnfoldTextFrame(0, 24, 38, "Instructions  Page Two of Five", "Press PgUp or PgDn.  Esc to Exit.");
    DrawTextLine(x, 4,  " Cosmo has a very special ability:");
    DrawTextLine(x, 6,  " He can use his suction hands to");
    DrawTextLine(x, 8,  " climb up walls.");
    DrawTextLine(x, 11, " Warning:  Some surfaces, such as");
    DrawTextLine(x, 13, " ice, might be too slippery for");
    DrawTextLine(x, 15, " Cosmo to cling on firmly.");
    DrawTextLine(x, 20, "\xFD""011                                 \xFD""034");
    FadeInCustom(1);

    scancode = WaitSpinner(x + 35, 22);
    if (scancode == SCANCODE_ESCAPE) return;
    if (scancode == SCANCODE_NUM_8 || scancode == SCANCODE_NUM_9) goto page1;

page3:
    FadeOutCustom(1);

    x = UnfoldTextFrame(0, 24, 38, "Instructions  Page Three of Five", "Press PgUp or PgDn.  Esc to Exit.");
    DrawTextLine(x,   4,  " Cosmo can jump onto attacking");
    DrawTextLine(x,   6,  " creatures without being harmed.");
    DrawTextLine(x,   8,  " This is also Cosmo's way of");
    DrawTextLine(x,   10, " defending himself.");
    DrawTextLine(x,   13, " Cosmo can also find and use bombs.");
    DrawTextLine(x + 5, 18, "   \xFD""036");
    DrawTextLine(x + 5, 20, "         \xFD""024          \xFD""037");
    DrawTextLine(x + 5, 20, "   \xFE""118000         \xFE""057000         \xFE""024000");
    FadeInCustom(1);

    scancode = WaitSpinner(x + 35, 22);
    if (scancode == SCANCODE_ESCAPE) return;
    if (scancode == SCANCODE_NUM_8 || scancode == SCANCODE_NUM_9) goto page2;

page4:
    FadeOutCustom(1);

    x = UnfoldTextFrame(0, 24, 38, "Instructions  Page Four of Five", "Press PgUp or PgDn.  Esc to Exit.");
    DrawTextLine(x,   5,  " Use the up and down arrow keys to");
    DrawTextLine(x,   7,  " make Cosmo look up and down,");
    DrawTextLine(x,   9,  " enabling him to see areas that");
    DrawTextLine(x,   11, " might be off the screen.");
    DrawTextLine(x + 4, 18, "   \xFD""028                  \xFD""029");
    DrawTextLine(x,   19, "      Up Key           Down Key");
    FadeInCustom(1);

    scancode = WaitSpinner(x + 35, 22);
    if (scancode == SCANCODE_ESCAPE) return;
    if (scancode == SCANCODE_NUM_8 || scancode == SCANCODE_NUM_9) goto page3;

    FadeOutCustom(1);

    x = UnfoldTextFrame(0, 24, 38, "Instructions  Page Five of Five", "Press PgUp.  Esc to Exit.");
    DrawTextLine(x, 5,  " In Cosmo's Cosmic Adventure, it's");
    DrawTextLine(x, 7,  " up to you to discover the use of");
    DrawTextLine(x, 9,  " all the neat and strange objects");
    DrawTextLine(x, 11, " you'll encounter on your journey.");
    DrawTextLine(x, 13, " Secret Hint Globes will help");
    DrawTextLine(x, 15, " you along the way.");
    DrawTextLine(x, 18, "                 \xFE""125000");
    DrawTextLine(x, 20, "              \xFD""027   \xFE""125002");
    FadeInCustom(1);

    scancode = WaitSpinner(x + 35, 22);
    if (scancode == SCANCODE_ESCAPE) return;
    if (scancode == SCANCODE_NUM_8 || scancode == SCANCODE_NUM_9) goto page4;

    ClearScreen();

    ShowHintsAndKeys(3);
}

/*
Display two pages of text about the various ways to engage with the publisher
online. Bear in mind that the World Wide Web was literally a few months old when
this game was originally released.
*/
void ShowPublisherBBS(void)
{
    word x;

    FadeOut();
    ClearScreen();

    x = UnfoldTextFrame(1, 22, 38, "THE OFFICIAL APOGEE BBS", "Press ANY key.");
    x += 3;
    DrawTextLine(x, 3,  "    -----------------------");
    DrawTextLine(x, 5,  "The SOFTWARE CREATIONS BBS is");
    DrawTextLine(x, 6,  " the home BBS for the latest");
    DrawTextLine(x, 7,  " Apogee games.  Check out our");
    DrawTextLine(x, 8,  "FREE 'Apogee' file section for");
    DrawTextLine(x, 9,  "  new releases and updates.");
    DrawTextLine(x, 11, "       BBS phone lines:");
    DrawTextLine(x, 13, "(508) 365-2359  2400 baud");
    DrawTextLine(x, 14, "(508) 365-9825  9600 baud");
    DrawTextLine(x, 15, "(508) 365-9668  14.4k dual HST");
    DrawTextLine(x, 17, "Home of the Apogee BBS Network!");
    DrawTextLine(x, 19, "    A Major Multi-Line BBS.");
    FadeIn();
    WaitSpinner(x + 32, 21);

    FadeOut();
    ClearScreen();

    x = UnfoldTextFrame(0, 25, 40, "APOGEE ON AMERICA ONLINE! ", "Press ANY key.");
    DrawTextLine(x, 2,  "      -------------------------");
    DrawTextLine(x, 4,  "   America Online (AOL) is host of");
    DrawTextLine(x, 5,  " the Apogee Forum, where you can get");
    DrawTextLine(x, 6,  "   new Apogee games. Use the Apogee");
    DrawTextLine(x, 7,  "  message areas to talk and exchange");
    DrawTextLine(x, 8,  "   ideas, comments and secrets with");
    DrawTextLine(x, 9,  "   our designers and other players.");
    DrawTextLine(x, 11, "  If you are already a member, after");
    DrawTextLine(x, 12, " you log on, use the keyword \"Apogee\"");
    DrawTextLine(x, 13, " (Ctrl-K) to jump to the Apogee area.");
    DrawTextLine(x, 15, "  If you'd like to know how to join,");
    DrawTextLine(x, 16, "        please call toll free:");
    DrawTextLine(x, 18, "            1-800-827-6364");
    DrawTextLine(x, 19, "    Please ask for extension 5703.");
    DrawTextLine(x, 21, "   You'll get the FREE startup kit.");
    FadeIn();
    WaitSpinner(x + 37, 23);
}

#ifdef FOREIGN_ORDERS
/*
Display five pages of text informing a shareware user how to order the
registered version of the game from various international distributors.
*/
void ShowForeignOrders(void)
{
    word x;

    FadeOut();
    ClearScreen();

    x = UnfoldTextFrame(1, 19, 38, "FOREIGN CUSTOMERS", "Press ANY key.");
    x += 2;
    DrawTextLine(x, 3,  "        -----------------");
    DrawTextLine(x, 5,  " The following screens list our");
    DrawTextLine(x, 6,  "   dealers outside the United");
    DrawTextLine(x, 7,  " States, for Australia, Germany,");
    DrawTextLine(x, 8,  " Canada and the United Kingdom.");
    DrawTextLine(x, 10, "   These are official Apogee");
    DrawTextLine(x, 11, "    dealers with the latest");
    DrawTextLine(x, 12, "\xFE""153000       games and updates.    \xFE""153001");
    DrawTextLine(x, 14, " If your country is not listed,");
    DrawTextLine(x, 15, "  you may order directly from");
    DrawTextLine(x, 16, "Apogee by phone: (214) 278-5655.");
    FadeInCustom(1);
    WaitSpinner(x + 33, 18);

    FadeOutCustom(1);
    ClearScreen();

    x = UnfoldTextFrame(1, 19, 38, "AUSTRALIAN CUSTOMERS", "Press ANY key.");
    x += 3;
    DrawTextLine(x, 4,  "PRICE: $45 + $5 shipping.");
    DrawTextLine(x, 6,  "BudgetWare");
    DrawTextLine(x, 7,  "P.O. Box 496");
    DrawTextLine(x, 8,  "Newtown, NSW  2042        \xFE""113000");
    DrawTextLine(x, 10, "Phone:      (02) 519-4233");
    DrawTextLine(x, 11, "Toll free:  (008) 022-064");
    DrawTextLine(x, 12, "Fax:        (02) 516-4236");
    DrawTextLine(x, 13, "CompuServe: 71520,1475");
    DrawTextLine(x, 15, "Use MasterCard, Visa, Bankcard,");
    DrawTextLine(x, 16, "cheques.");
    FadeInCustom(1);
    WaitSpinner(x + 32, 18);

    FadeOutCustom(1);
    ClearScreen();

    x = UnfoldTextFrame(1, 20, 38, "CANADIAN CUSTOMERS", "Press ANY key.");
    x += 3;
    DrawTextLine(x, 4,  "PRICE: $42 Canadian.       \xFE""146000");
    DrawTextLine(x, 6,  "Distant Markets");
    DrawTextLine(x, 7,  "Box 1149");
    DrawTextLine(x, 8,  "194 - 3803 Calgary Trail S.");
    DrawTextLine(x, 9,  "Edmondton, Alb.  T6J 5M8");
    DrawTextLine(x, 10, "CANADA");
    DrawTextLine(x, 12, "Orders:    1-800-661-7383");
    DrawTextLine(x, 13, "Inquiries: (403) 436-3009");
    DrawTextLine(x, 14, "Fax:       (403) 435-0928  \xFE""086002");
    DrawTextLine(x, 16, "Use MasterCard, Visa or");
    DrawTextLine(x, 17, "money orders.");
    FadeInCustom(1);
    WaitSpinner(x + 32, 19);

    FadeOutCustom(1);
    ClearScreen();

    x = UnfoldTextFrame(1, 20, 38, "GERMAN CUSTOMERS", "Press ANY key.");
    x += 3;
    DrawTextLine(x, 4,  "Price: 49,-- DM plus 10,-- DM");
    DrawTextLine(x, 5,  "Total: 59,-- DM (Deutsche Mark)");
    DrawTextLine(x, 7,  "CDV-Software");
    DrawTextLine(x, 8,  "Ettlingerstr. 5");
    DrawTextLine(x, 9,  "7500 Karlsruhe 1  GERMANY");
    DrawTextLine(x, 11, "Phone: 0721-22295");
    DrawTextLine(x, 12, "Fax:   0721-21314            \xFE""127004");
    DrawTextLine(x, 13, "Compuserve: 1000022,274");
    DrawTextLine(x, 15, "Use Visa, MasterCard, EuroCard,");
    DrawTextLine(x, 16, "American Express, cheque, money");
    DrawTextLine(x, 17, "order, or C.O.D.");
    FadeInCustom(1);
    WaitSpinner(x + 32, 19);

    FadeOutCustom(1);
    ClearScreen();

    x = UnfoldTextFrame(1, 20, 38, "UNITED KINGDOM CUSTOMERS", "Press ANY key.");
    x += 3;
    /* The game font renders the symbol for pound sterling instead of `/`. */
    DrawTextLine(x, 4,  "Price: /29 + VAT + 2 P&P     \xFE""085000");
    DrawTextLine(x, 6,  "Precision Software Applications");
    DrawTextLine(x, 7,  "Unit 3, Valley Court Offices");
    DrawTextLine(x, 8,  "Lower Rd");
    DrawTextLine(x, 9,  "Croydon, Near Royston");
    DrawTextLine(x, 10, "Herts. SG8 0HF, United Kingdom");
    DrawTextLine(x, 12, "Phone: +44 (0) 223 208 288");
    DrawTextLine(x, 13, "FAX:   +44 (0) 223 208 089");
    DrawTextLine(x, 15, "Credit cards, Access, cheques,");
    DrawTextLine(x, 16, "postal & Bankers orders.");
    DrawTextLine(x, 17, "Make cheques payable to PSA.");
    FadeInCustom(1);
    WaitSpinner(x + 32, 19);
}
#endif  /* FOREIGN_ORDERS */

/*
Show a simple "saved game not found" error and wait for any key to be pressed.
*/
void ShowRestoreGameError(void)
{
    word x = UnfoldTextFrame(5, 4, 20, "Can't find that", "game to restore! ");

    WaitSpinner(x + 17, 7);
}

/*
Show the copyright year, version number, and abbreviated credits during startup.
*/
void ShowCopyright(void)
{
    word x = UnfoldTextFrame(4, 13, 26, "A game by", "Copyright (c) 1992");

    DrawTextLine(x,      7,  "     Todd J Replogle");
    DrawTextLine(x + 11, 9,  "and");
    DrawTextLine(x,      11, "\xFD""027   Stephen A Hornback\xFD""004");
#ifdef VANITY
    DrawTextLine(x,      13, "  Cosmore version 1.20");
#else
    DrawTextLine(x,      13, "      Version 1.20");
#endif  /* VANITY */

    WaitSoft(700);
    FadeOut();
}

/*
Show a simple "altered file error" and wait for any key to be pressed.
*/
void ShowAlteredFileError(void)
{
    word x = UnfoldTextFrame(2, 4, 28, "Altered file error!!", "Now exiting game!");

    WaitSpinner(x + 25, 4);
}

/*
Draw an empty dialog frame with the player's sprite on the right. Return the
position of the left-inside edge of the frame.
*/
word UnfoldPlayerFrame(void)
{
    register word x;

#ifdef HAS_ACT_FROZEN_DN
    x = UnfoldTextFrame(2, 8, 34, "", "Press a key to continue.");
    DrawTextLine(x + 29, 7, "\xFD""004");

    return x + 1;
#else
#   pragma warn -rvl
#   pragma warn -use
#endif  /* ACT_FROZEN_DN */
}
#pragma warn .rvl
#pragma warn .use

/*
Draw an empty dialog frame with D.N.'s sprite on the left. Return the leftmost
position that clears the drawn sprite.
*/
word UnfoldDNFrame(void)
{
    register word x;

#ifdef HAS_ACT_FROZEN_DN
    x = UnfoldTextFrame(2, 8, 34, "", "Press a key to continue.");
    DrawTextLine(x + 1, 7, "\xFE""221003");

    return x + 4;
#else
#   pragma warn -rvl
#   pragma warn -use
#endif  /* ACT_FROZEN_DN */
}
#pragma warn .rvl
#pragma warn .use

/*
Show a two-sided conversation between the player and D.N.

INCONSISTENCY: The wait spinner placement is different depending on the speaker.
*/
void ShowRescuedDNMessage(void)
{
    register word x;

#ifdef HAS_ACT_FROZEN_DN
    SelectDrawPage(activePage);

    x = UnfoldPlayerFrame();
    DrawTextLine(x, 5, "\xFC""003  Yikes, who are you?");
    WaitSpinner(x + 27, 8);

    x = UnfoldDNFrame();
    DrawTextLine(x, 4, "\xFC""003 I'm Duke Nukum, green");
    DrawTextLine(x, 5, "\xFC""003 alien dude.              ");
    WaitSpinner(x + 27, 8);

    x = UnfoldDNFrame();
    DrawTextLine(x, 4, "\xFC""003 Until you rescued me, I");
    DrawTextLine(x, 5, "\xFC""003 was stopped cold by an");
    DrawTextLine(x, 6, "\xFC""003 alien invasion force!");
    WaitSpinner(x + 27, 8);

    x = UnfoldPlayerFrame();
    DrawTextLine(x, 4, "\xFC""003 Wow!  Can you help rescue ");
    DrawTextLine(x, 5, "\xFC""003 my parents?");
    WaitSpinner(x + 27, 8);

    x = UnfoldDNFrame();
    DrawTextLine(x, 4, "\xFC""003 Sorry, kid, I've got to");
    DrawTextLine(x, 5, "\xFC""003 save the galaxy...");
    WaitSpinner(x + 27, 8);

    x = UnfoldDNFrame();
    DrawTextLine(x, 4, "\xFC""003 ...but, I can give you");
    DrawTextLine(x, 5, "\xFC""003 something that will help");
    DrawTextLine(x, 6, "\xFC""003 you out.");
    WaitSpinner(x + 27, 8);

    x = UnfoldPlayerFrame();
    DrawTextLine(x, 4, "\xFC""003 Thanks, Mr. Nukum, and");
    DrawTextLine(x, 5, "\xFC""003 good luck on your mission.");
    WaitSpinner(x + 27, 8);

    x = UnfoldDNFrame();
    DrawTextLine(x, 4, "\xFC""003 Just look for me in my");
    DrawTextLine(x, 5, "\xFC""003 next exciting adventure,");
    DrawTextLine(x, 6, "\xFC""003 Duke Nukum II!");
    WaitSpinner(x + 27, 8);

    x = UnfoldPlayerFrame();
    DrawTextLine(x, 5, "\xFC""003             Bye.");
    WaitSpinner(x + 27, 8);

    x = UnfoldDNFrame();
    DrawTextLine(x, 4, "\xFC""003 See ya... and have those");
    DrawTextLine(x, 5, "\xFC""003 spots checked...!");
    WaitSpinner(x + 27, 8);

    SelectDrawPage(!activePage);
#else
#   pragma warn -use
#endif  /* ACT_FROZEN_DN */
}
#pragma warn .use

/*
Display one message from a succession of cliffhanger messages, each triggered by
a different end line actor.
*/
void ShowE1CliffhangerMessage(word actor_type)
{
    register word x;

#ifdef E1_CLIFFHANGER
    SelectDrawPage(activePage);

    switch (actor_type) {
    case ACT_EP1_END_1:
        x = UnfoldTextFrame(2, 8, 28, "", "Press any key to exit.");
        DrawTextLine(x, 4, "\xFC""003 What's happening?  Is ");
        DrawTextLine(x, 5, "\xFC""003 Cosmo falling to his ");
        DrawTextLine(x, 6, "\xFC""003 doom?");
        WaitSpinner(x + 25, 8);
        break;

    case ACT_EP1_END_2:
        x = UnfoldTextFrame(2, 8, 28, "", "Press any key to exit.");
        DrawTextLine(x, 4, "\xFC""003 Is there no end to this ");
        DrawTextLine(x, 5, "\xFC""003 pit?  And what danger ");
        DrawTextLine(x, 6, "\xFC""003 awaits below?! ");
        WaitSpinner(x + 25, 8);
        break;

    case ACT_EP1_END_3:
        winGame = true;
        break;
    }

    SelectDrawPage(!activePage);
#else
#   pragma warn -use
#   pragma warn -par
#endif  /* E1_CLIFFHANGER */
}
#pragma warn .use
#pragma warn .par

/*
Ask for confirmation of the user's request to quit. Return true if the request
is confirmed by pressing Y, or false otherwise.
*/
bbool PromptQuitConfirm(void)
{
    word x;
    byte scancode;

    x = UnfoldTextFrame(11, 4, 18, "Are you sure you", "want to quit? ");
    scancode = WaitSpinner(x + 14, 13);

    if (scancode == SCANCODE_Y) {
        return true;
    } else {
        return false;
    }
}

/*
Invert the state of the sound effects option, then show a confirmation message
indicating the new state.
*/
void ToggleSound(void)
{
    word x;

    isSoundEnabled = !isSoundEnabled;

    if (isSoundEnabled) {
        x = UnfoldTextFrame(2, 4, 24, "Sound Toggle", "The sound is now ON!");
    } else {
        /* Minor BUG: last char gets cut off by the wait spinner */
        x = UnfoldTextFrame(2, 4, 24, "Sound Toggle", "The sound is now OFF!");
    }

    WaitSpinner(x + 21, 4);
}

/*
Invert the state of the music option, show a confirmation message indicating the
new state, and then command the AdLib service to start/stop playing the music.
*/
void ToggleMusic(void)
{
    word x;

    if (IsAdLibAbsent()) return;

    isMusicEnabled = !isMusicEnabled;

    if (isMusicEnabled) {
        x = UnfoldTextFrame(2, 4, 24, "Music Toggle", "The music is now ON!");

        SwitchMusic(activeMusic);
        StartAdLibPlayback();
    } else {
        /* Minor BUG: last char gets cut off by the wait spinner */
        x = UnfoldTextFrame(2, 4, 24, "Music Toggle", "The music is now OFF!");

        StopAdLibPlayback();
    }

    WaitSpinner(x + 21, 4);
}

/*
Display a menu where all the sound effects can by cycled through and previewed.
At any time, Up/Down changes the sound number, Enter plays the selected sound,
and Esc leaves the menu. If the sound effects option is disabled, it is
temporarily re-enabled while this menu remains open.
*/
void TestSound(void)
{
    bool enabled = isSoundEnabled;
    dword soundnum = 1;
    word x;

    isSoundEnabled = true;

    x = UnfoldTextFrame(2, 7, 34, "Test Sound", "Press ESC to quit.");
    DrawTextLine(x, 4, " Press \x18 or \x19 to change sound #.");
    DrawTextLine(x, 5, "   Press Enter to hear sound.");

    /* In this function, you'll see SCANCODE_NUM_2 and _8 tested. On the PC
    keyboard, numpad 2 (with Num Lock off) functions as Down, and numpad 8
    functions as Up. The scancodes mirror this arrangement. */

    for (;;) {
        byte scancode;
        int i;

        DrawNumberFlushRight(x + 16, 6, soundnum);
        scancode = WaitSpinner(x + 31, 7);

        if (scancode == SCANCODE_NUM_2 && soundnum > 1)  soundnum--;
        if (scancode == SCANCODE_NUM_8 && soundnum < 65) soundnum++;

        if (scancode == SCANCODE_ESCAPE) {
            isSoundEnabled = enabled;
            break;
        }

        if (scancode == SCANCODE_ENTER) {
            StartSound((word)soundnum);
        }

        for (i = 0; i < 3; i++) {
            EraseWaitSpinner(x + i + 14, 6);
        }
    }
}

/*
Pause the game with a visible message and stop the music. When the message is
dismissed by pressing any key, restart the music if it was playing before, and
resume the game.
*/
void PauseMessage(void)
{
    word x = UnfoldTextFrame(2, 4, 18, "Game Paused.", "Press ANY key.");

    StopAdLibPlayback();
    WaitSpinner(x + 15, 4);

    if (isMusicEnabled) {
        SwitchMusic(activeMusic);
        StartAdLibPlayback();
    }
}

/*
Invert the state of the god mode debug flag, then show a confirmation message
indicating the new state.
*/
void GodModeToggle(void)
{
    word x;

    isGodMode = !isGodMode;

    if (isGodMode) {
        x = UnfoldTextFrame(2, 4, 28, "God Toggle", "The god mode is now ON!");
    } else {
        x = UnfoldTextFrame(2, 4, 28, "God Toggle", "The god mode is now OFF!");
    }

    WaitSpinner(x + 25, 4);
}

/*
Display memory statistics for the game.
- "Memory free" is the number of bytes of memory that were available after all
  of the data arenas were allocated. This value is measured once during startup
  and never updated. It's roughly the amount of free memory available at this
  moment.
- "Take Up" is the number of bytes of memory that were available when the
  program started running. This is roughly the amount of conventional memory in
  the system, minus all drivers, TSRs, and the program itself.
- "Total Map Memory" is hard-coded to 65,049, which appears to be completely
  meaningless. Nothing in here adds up to that number.
- "Total Actors" is the *peak* number of actor slots that have been used since
  the current level (re)started. This does not decrease when actors die, nor
  does it increase when a new actor occupies a dead actor's slot.
*/
void MemoryUsage(void)
{
    word x = UnfoldTextFrame(2, 8, 30, "- Memory Usage -", "Press ANY key.");

    DrawTextLine(x + 6,  4, "Memory free:");
    DrawTextLine(x + 10, 5, "Take Up:");
    DrawTextLine(x + 1,  6, "Total Map Memory:  65049");
    DrawTextLine(x + 5,  7, "Total Actors:");
    DrawNumberFlushRight(x + 24, 4, totalMemFreeAfter);
    DrawNumberFlushRight(x + 24, 5, totalMemFreeBefore);
    DrawNumberFlushRight(x + 24, 7, numActors);
    WaitSpinner(x + 27, 8);
}

/*
Display the Game Redefine menu and call the appropriate function based on the
key pressed.
*/
void GameRedefineMenu(void)
{
    word x = UnfoldTextFrame(4, 11, 22, "Game Redefine", "Press ESC to quit.");

    DrawTextLine(x, 7,  " K)eyboard redefine");
    DrawTextLine(x, 8,  " J)oystick redefine");
    DrawTextLine(x, 9,  " S)ound toggle");
    DrawTextLine(x, 10, " T)est sound");
    DrawTextLine(x, 11, " M)usic toggle");

    for (;;) {
        byte scancode = WaitSpinner(29, 13);

        switch (scancode) {
        case SCANCODE_ESCAPE:
            return;
        case SCANCODE_S:
            ToggleSound();
            return;
        case SCANCODE_J:
            ShowJoystickConfiguration(JOYSTICK_A);
            return;
        case SCANCODE_K:
            ShowKeyboardConfiguration();
            return;
        case SCANCODE_T:
            TestSound();
            return;
        case SCANCODE_M:
            ToggleMusic();
            return;
        }
    }
}

/*
Attempt to load the configuration file specified by the filename. If the file
does not exist, load the default configuration where arrow keys move, Ctrl
jumps, and Alt bombs. This is where the default Simpsons high scores are set.
*/
void LoadConfigurationData(char *filename)
{
    FILE *fp;
    byte space;

    fp = fopen(filename, "rb");

    if (fp == NULL) {
        /* With Num Lock off, SCANCODE_NUM_8, _2, _4, and _6 are Up, Down, Left,
        and Right respectively. */
        scancodeNorth = SCANCODE_NUM_8;
        scancodeSouth = SCANCODE_NUM_2;
        scancodeWest = SCANCODE_NUM_4;
        scancodeEast = SCANCODE_NUM_6;
        scancodeJump = SCANCODE_CTRL;
        scancodeBomb = SCANCODE_ALT;
        isMusicEnabled = true;
        isSoundEnabled = true;

        highScoreValues[0] = 1000000L;
        strcpy(highScoreNames[0], "BART");

        highScoreValues[1] = 900000L;
        strcpy(highScoreNames[1], "LISA");

        highScoreValues[2] = 800000L;
        strcpy(highScoreNames[2], "MARGE");

        highScoreValues[3] = 700000L;
        strcpy(highScoreNames[3], "ITCHY");

        highScoreValues[4] = 600000L;
        strcpy(highScoreNames[4], "SCRATCHY");

        highScoreValues[5] = 500000L;
        strcpy(highScoreNames[5], "MR. BURNS");

        highScoreValues[6] = 400000L;
        strcpy(highScoreNames[6], "MAGGIE");

        highScoreValues[7] = 300000L;
        strcpy(highScoreNames[7], "KRUSTY");

        highScoreValues[8] = 200000L;
        strcpy(highScoreNames[8], "HOMER");
    } else {
        int i;

        scancodeNorth = fgetc(fp);
        scancodeSouth = fgetc(fp);
        scancodeWest = fgetc(fp);
        scancodeEast = fgetc(fp);
        scancodeJump = fgetc(fp);
        scancodeBomb = fgetc(fp);
        isMusicEnabled = fgetc(fp);
        isSoundEnabled = fgetc(fp);

        for (i = 0; i < 10; i++) {
            fscanf(fp, "%lu", &highScoreValues[i]);
            fscanf(fp, "%c", &space);
            fscanf(fp, "%[^\n]s", highScoreNames[i]);  /* fairly dangerous */
        }

        fclose(fp);
    }
}

/*
Save the current state of the global configuration variables to the
configuration file specified by the fileneme.
*/
void SaveConfigurationData(char *filename)
{
    FILE *fp;
    int i;

    fp = fopen(filename, "wb");

    fputc(scancodeNorth, fp);
    fputc(scancodeSouth, fp);
    fputc(scancodeWest, fp);
    fputc(scancodeEast, fp);
    fputc(scancodeJump, fp);
    fputc(scancodeBomb, fp);
    fputc(isMusicEnabled, fp);
    fputc(isSoundEnabled, fp);

    for (i = 0; i < 10; i++) {
        fprintf(fp, "%lu", highScoreValues[i]);
        fprintf(fp, " ");
        fprintf(fp, "%s\n", highScoreNames[i]);
    }

    fclose(fp);
}

/*
Display the end-game story text and cartoons appropriate for the episode being
played. Ends with ordering info and one final star bonus tally.
*/
void ShowEnding(void)
{
    word x;

#if EPISODE == 1
    SelectDrawPage(0);
    SelectActivePage(0);
    WaitHard(5);
    FadeOut();
    DrawFullscreenImage(IMAGE_END);
    WaitSpinner(39, 24);

    FadeToWhite(4);
    ClearScreen();

    x = UnfoldTextFrame(1, 24, 38, "", "Press ANY key.");
    DrawTextLine(x + 4,  13, "\xFB""016");
    DrawTextLine(x + 28, 22, "\xFB""017");
    FadeIn();
    DrawTextLine(x + 14, 4,  "\xFC""003Are Cosmo's cosmic ");
    DrawTextLine(x + 14, 5,  "\xFC""003adventuring days ");
    DrawTextLine(x + 14, 6,  "\xFC""003finally over?    ");
    DrawTextLine(x + 14, 8,  "\xFC""003Will Cosmo's parents ");
    DrawTextLine(x + 14, 9,  "\xFC""003be lightly seasoned ");
    DrawTextLine(x + 14, 10, "\xFC""003and devoured before ");
    DrawTextLine(x + 14, 11, "\xFC""003he can save them?      ");
    DrawTextLine(x + 1,  15, "\xFC""003Find the stunning ");
    DrawTextLine(x + 1,  16, "\xFC""003answers in the next two ");
    DrawTextLine(x + 1,  17, "\xFC""003NEW, shocking, amazing, ");
    DrawTextLine(x + 1,  18, "\xFC""003horrifying, wacky and ");
    DrawTextLine(x + 1,  19, "\xFC""003exciting episodes of...         ");
    DrawTextLine(x + 1,  21, "\xFC""003COSMO'S COSMIC ADVENTURE!");
    WaitSpinner(x + 35, 23);

    FadeOut();
    ClearScreen();

    x = UnfoldTextFrame(6, 4, 24, "Thank you", " for playing!");
    FadeIn();
    WaitHard(100);
    WaitSpinner(x + 21, 8);
#elif EPISODE == 2
    FadeOut();
    SelectDrawPage(0);
    SelectActivePage(0);
    ClearScreen();

    x = UnfoldTextFrame(1, 24, 38, "", "Press ANY key.");
    DrawTextLine(x + 25, 15, "\xFB""021");
    FadeIn();
    DrawTextLine(x + 1,  7,  "\xFC""003 Young Cosmo leaps ");
    DrawTextLine(x + 1,  9,  "\xFC""003 through a small hole ");
    DrawTextLine(x + 1,  11, "\xFC""003 in the cave ceiling, ");
    DrawTextLine(x + 1,  13, "\xFC""003 and finally sees what ");
    DrawTextLine(x + 1,  15, "\xFC""003 he's searching for... ");
    WaitSpinner(x + 35, 23);

    DrawFullscreenImage(IMAGE_END);
    StartMenuMusic(MUSIC_ROCKIT);

    x = UnfoldTextFrame(18, 5, 38, "", "");
    DrawTextLine(x + 1, 19, "\xFC""003 ...the city where his parents are ");
    DrawTextLine(x + 1, 20, "\xFC""003  held captive--undoubtedly being");
    DrawTextLine(x + 1, 21, "\xFC""003     readied for the big feast!");
    WaitSpinner(37, 21);

    x = UnfoldTextFrame(18, 5, 38, "", "");
    DrawTextLine(x + 1, 19, "\xFC""003    Cosmo knows what he must do.");
    DrawTextLine(x + 1, 20, "\xFC""003    Enter the city and save his ");
    DrawTextLine(x + 1, 21, "\xFC""003   parents before it's too late!");
    WaitSpinner(37, 21);

    FadeToWhite(4);
    ClearScreen();

    x = UnfoldTextFrame(6, 4, 24, "Thank you", " for playing!");
    FadeIn();
    WaitHard(100);
    WaitSpinner(x + 21, 8);
#elif EPISODE == 3
    StartMenuMusic(MUSIC_HAPPY);
    FadeOut();
    SelectDrawPage(0);
    SelectActivePage(0);
    ClearScreen();

    x = UnfoldTextFrame(1, 22, 38, "", "Press ANY key.");
    DrawTextLine(x + 1, 17, "\xFB""018");
    DrawTextLine(x + 17, 6,  "The creature is");
    DrawTextLine(x + 17, 7,  "finally defeated");
    DrawTextLine(x + 17, 8,  "and flies away.");
    DrawTextLine(x + 17, 9,  "Suddenly, a door");
    DrawTextLine(x + 17, 10, "opens and Cosmo");
    DrawTextLine(x + 17, 11, "enters slowly.");
    DrawTextLine(x + 17, 13, "A big, scary blue");
    DrawTextLine(x + 17, 14, "alien creature");
    DrawTextLine(x + 17, 15, "wraps his arm");
    DrawTextLine(x + 17, 16, "around Cosmo...");
    FadeIn();
    WaitSpinner(x + 35, 21);

    FadeOut();

    x = UnfoldTextFrame(1, 22, 38, "", "Press ANY key.");
    DrawTextLine(x + 1, 16, "\xFB""019");
    /* Unbalanced dialog quotes preserved faithfully */
    DrawTextLine(x + 10, 3,  "\"Hi Cosmo,\" says the blue");
    DrawTextLine(x + 10, 4,  "alien, \"I'm Zonk,\" and");
    DrawTextLine(x + 10, 5,  "we've been looking all");
    DrawTextLine(x + 10, 6,  "over the planet for you\"");
    DrawTextLine(x + 10, 8,  "\"This is a very dangerous");
    DrawTextLine(x + 10, 9,  "planet, and when we found");
    DrawTextLine(x + 10, 10, "your parents, we brought");
    DrawTextLine(x + 10, 11, "them here for safety.\"");
    DrawTextLine(x + 10, 13, "\"We have been looking for");
    DrawTextLine(x + 10, 14, "you all this time, but");
    DrawTextLine(x + 10, 15, "it looks like you did a");
    DrawTextLine(x + 10, 16, "better job finding us!\"");
    DrawTextLine(x + 10, 18, "\"Here, I got a surprise");
    DrawTextLine(x + 10, 19, "for you...\"");
    FadeIn();
    WaitSpinner(x + 35, 21);

    FadeOut();

    x = UnfoldTextFrame(1, 22, 38, "", "Press ANY key.");
    DrawTextLine(x + 27, 14, "\xFB""020");
    DrawTextLine(x + 2, 7,  "\"Mommy!  Daddy!\"");
    DrawTextLine(x + 2, 8,  "Cosmo and his parents");
    DrawTextLine(x + 2, 9,  "are finally reunited,");
    DrawTextLine(x + 2, 10, "and hugs are passed");
    DrawTextLine(x + 2, 11, "all around.");
    DrawTextLine(x + 2, 13, "Daddy explains that");
    DrawTextLine(x + 2, 14, "Zonk helped fix their");
    DrawTextLine(x + 2, 15, "ship, and they can");
    DrawTextLine(x + 2, 16, "resume their trip to");
    DrawTextLine(x + 2, 17, "Disney World.");
    FadeIn();
    WaitSpinner(x + 35, 21);

    FadeOut();

    x = UnfoldTextFrame(1, 22, 38, "", "Press ANY key.");
    DrawTextLine(x + 27, 19, "\xFB""003");
    DrawTextLine(x + 1,  10, "\xFB""011");
    DrawTextLine(x + 12, 6, "After saying goodbye");
    DrawTextLine(x + 12, 7, "to Zonk,");
    DrawTextLine(x + 1, 17, "Cosmo and his family");
    DrawTextLine(x + 1, 18, "blast off toward earth...");
    FadeIn();
    WaitSpinner(x + 35, 21);

    FadeOut();

    x = UnfoldTextFrame(1, 22, 38, "", "Press ANY key.");
    DrawTextLine(x + 13, 19, "\xFB""012");
    DrawTextLine(x + 2, 5, "    ...and arrive just four");
    DrawTextLine(x + 2, 6, "     galactic hours later!!");
    DrawTextLine(x + 2, 7, "Using their inviso-cloak device,");
    DrawTextLine(x + 2, 8, " they park their ship on top of");
    DrawTextLine(x + 2, 9, "        Space Mountain.");
    FadeIn();
    WaitSpinner(x + 35, 21);

    FadeOut();

    x = UnfoldTextFrame(1, 22, 38, "", "Press ANY key.");
    DrawTextLine(x + 12, 12, "\xFB""013");
    DrawTextLine(x + 2, 15, "  Disney World has always been");
    DrawTextLine(x + 2, 16, "    a great place for aliens");
    DrawTextLine(x + 2, 17, "  to visit on their vacations!");
    FadeIn();
    WaitSpinner(x + 35, 21);

    DrawFullscreenImage(IMAGE_END);

    x = UnfoldTextFrame(0, 3, 24, "WEEEEEEEE!", "");
    StartSound(SND_WEEEEEEEE);
    WaitHard(200);
    StartMenuMusic(MUSIC_ZZTOP);
    WaitSpinner(x + 21, 1);

    FadeToWhite(4);

    x = UnfoldTextFrame(0, 5, 24, "Cosmo has the best", "The End!");
    DrawTextLine(x + 1, 2, "birthday of his life.");
    FadeIn();
    WaitHard(100);
    WaitSpinner(x + 21, 3);

    ShowCongratulations();
#endif  /* EPISODE == ... */

    ShowOrderingInformation();
    ShowStarBonus();
}

/*
Display a text frame containing the numbered hint message. Each episode re-uses
hint numbers that have been used in other episodes.
*/
void ShowHintGlobeMessage(word hint_num)
{
    word x;

    SelectDrawPage(activePage);
    WaitHard(30);

#if EPISODE == 1
    /* Ep 1 has some considerably longer hints that need a bigger frame */
    if (hint_num != 0 && hint_num < 15) {
        x = UnfoldTextFrame(2, 9, 28, "COSMIC HINT!", "Press any key to exit.");
        DrawTextLine(x, 8, " Press SPACE to hurry or");
    }

    switch (hint_num) {
    case 0:
        x = UnfoldTextFrame(2, 11, 28, "COSMIC HINT!", "Press any key to exit.");
        DrawTextLine(x, 10, " Press SPACE to hurry or");
        DrawTextLine(x, 5, "\xFC""003 These hint globes will");
        DrawTextLine(x, 6, "\xFC""003 help you along your");
        DrawTextLine(x, 7, "\xFC""003 journey.  Press the up");
        DrawTextLine(x, 8, "\xFC""003 key to reread them.");
        WaitSpinner(x + 25, 11);
        break;
    case 1:
        DrawTextLine(x, 5, "\xFC""003 Bump head into switch");
        DrawTextLine(x, 6, "\xFC""003 above!");
        break;
    case 2:
        DrawTextLine(x, 5, "\xFC""003 The ice in this cave is");
        DrawTextLine(x, 6, "\xFC""003 very, very slippery.");
        break;
    case 3:
        DrawTextLine(x, 5, "\xFC""003 Use this shield for");
        DrawTextLine(x, 6, "\xFC""003 temporary invincibility.");
        break;
    case 4:
        DrawTextLine(x, 5, "\xFC""003 You found a secret");
        DrawTextLine(x, 6, "\xFC""003 area!!!  Good job!");
        break;
    case 5:
        DrawTextLine(x, 5, "\xFC""003 In high places look up");
        DrawTextLine(x, 6, "\xFC""003 to find bonus objects.");
        break;
    case 6:
        DrawTextLine(x, 5, "\xFC""003      Out of Order...");
        break;
    case 7:
        DrawTextLine(x, 5, "\xFC""003 This might be a good");
        DrawTextLine(x, 6, "\xFC""003 time to save your game!");
        break;
    case 8:
        DrawTextLine(x, 5, "\xFC""003 Press your up key to");
        DrawTextLine(x, 6, "\xFC""003 use the transporter.");
        break;
    case 9:
        DrawTextLine(x, 5, "\xFC""003  (1) FOR...");
        break;
    case 10:
        DrawTextLine(x, 5, "\xFC""003  (2) EXTRA...");
        break;
    case 11:
        DrawTextLine(x, 5, "\xFC""003  (3) POINTS,...");
        break;
    case 12:
        DrawTextLine(x, 5, "\xFC""003  (4) DESTROY...");
        break;
    case 13:
        DrawTextLine(x, 5, "\xFC""003  (5) HINT...");
        break;
    case 14:
        DrawTextLine(x, 5, "\xFC""003  (6) GLOBES!!!");
        break;
    case 15:
        x = UnfoldTextFrame(2, 11, 28, "COSMIC HINT!", "Press any key to exit.");
        DrawTextLine(x + 22, 8, "\xFE""083000");
        DrawTextLine(x, 10, " Press SPACE to hurry or");
        DrawTextLine(x, 5, "\xFC""003  The Clam Plants won't");
        DrawTextLine(x, 6, "\xFC""003  hurt you if their");
        DrawTextLine(x, 7, "\xFC""003  mouths are closed.");
        WaitSpinner(x + 25, 11);
        break;
    case 16:
        x = UnfoldTextFrame(2, 10, 28, "COSMIC HINT!", "Press any key to exit.");
        DrawTextLine(x, 9, " Press SPACE to hurry or");
        DrawTextLine(x + 23, 7, "\xFE""001002");
        DrawTextLine(x, 5, "\xFC""003  Collect the STARS to");
        DrawTextLine(x, 6, "\xFC""003  advance to BONUS");
        DrawTextLine(x, 7, "\xFC""003  STAGES.");
        WaitSpinner(x + 25, 10);
        break;
    case 17:
        x = UnfoldTextFrame(2, 10, 28, "COSMIC HINT!", "Press any key to exit.");
        DrawTextLine(x, 9, " Press SPACE to hurry or");
        DrawTextLine(x, 5, "\xFC""003  Some creatures require");
        DrawTextLine(x, 6, "\xFC""003  more than one pounce");
        DrawTextLine(x, 7, "\xFC""003  to defeat!");
        WaitSpinner(x + 25, 10);
        break;
    case 18:
        x = UnfoldTextFrame(2, 9, 30, "COSMIC HINT!", "Press any key to exit.");
        DrawTextLine(x + 25, 8, "\xFD""032");
        DrawTextLine(x, 8, "  Press SPACE to hurry or");
        /* Incorrect possessive form preserved faithfully */
        DrawTextLine(x, 5, "\xFC""003 Cosmo can climb wall's");
        DrawTextLine(x, 6, "\xFC""003 with his suction hands.");
        WaitSpinner(x + 27, 9);
        break;
    }

    if (hint_num != 0 && hint_num < 15) {
        WaitSpinner(x + 25, 9);
    }
#elif EPISODE == 2
    x = UnfoldTextFrame(2, 9, 28, "COSMIC HINT!", "Press any key to exit.");
    DrawTextLine(x, 8, " Press SPACE to hurry or");

    switch (hint_num) {
    case 0:
        DrawTextLine(x, 5, "\xFC""003 Look out for enemies");
        DrawTextLine(x, 6, "\xFC""003 from above!");
        break;
    case 1:
        DrawTextLine(x, 5, "\xFC""003    Don't...");
        break;
    case 2:
        DrawTextLine(x, 5, "\xFC""003    step...");
        break;
    case 3:
        DrawTextLine(x, 5, "\xFC""003    on...");
        break;
    case 4:
        DrawTextLine(x, 5, "\xFC""003    worms...");
        break;
    case 5:
        DrawTextLine(x, 5, "\xFC""003 There is a secret area");
        DrawTextLine(x, 6, "\xFC""003 in this level!");
        break;
    case 6:
        DrawTextLine(x, 5, "\xFC""003 You found the secret");
        DrawTextLine(x, 6, "\xFC""003 area.  Well done.");
        break;
    case 7:
        DrawTextLine(x, 5, "\xFC""003    Out of order.");
        break;
    }

    WaitSpinner(x + 25, 9);
#elif EPISODE == 3
    x = UnfoldTextFrame(2, 9, 28, "COSMIC HINT!", "Press any key to exit.");
    DrawTextLine(x, 8, " Press SPACE to hurry or");

    switch (hint_num) {
    case 0:
        DrawTextLine(x, 5, "\xFC""003 Did you find the");
        DrawTextLine(x, 6, "\xFC""003 hamburger in this level?");
        break;
    case 1:
        DrawTextLine(x, 5, "\xFC""003 This hint globe being");
        DrawTextLine(x, 6, "\xFC""003 upgraded to a 80986.");
        break;
    case 2:
        DrawTextLine(x, 5, "\xFC""003 WARNING:  Robots shoot");
        DrawTextLine(x, 6, "\xFC""003 when the lights are on!");
        break;
    case 3:
        DrawTextLine(x, 5, "\xFC""003 There is a hidden scooter");
        DrawTextLine(x, 6, "\xFC""003 in this level.");
        break;
    case 4:
        DrawTextLine(x, 5, "\xFC""003 Did you find the");
        DrawTextLine(x, 6, "\xFC""003 hamburger in level 8!");
        break;
    case 5:
        DrawTextLine(x, 5, "\xFC""003   Out of order...!");
    }

    WaitSpinner(x + 25, 9);
#endif  /* EPISODE == ... */

    SelectDrawPage(!activePage);
}

/*
Display a text frame informing the player that they have cheated.
*/
void ShowCheatMessage(void)
{
    word x = UnfoldTextFrame(3, 9, 32, "You are now cheating!", "Press ANY key.");;

    DrawTextLine(x, 6, "  You have been awarded full");
    DrawTextLine(x, 7, " health and maximum amount of");
    DrawTextLine(x, 8, "            bombs!");
    WaitSpinner(x + 29, 10);
}

/*
Add a number of points to the player's score based on the passed sprite type.
*/
void AddScoreForSprite(word sprite_type)
{
    switch (sprite_type) {
    case SPR_JUMPING_BULLET:
        AddScore(800);
        break;

    case SPR_GHOST:
    case SPR_MOON:
    case SPR_SHARP_ROBOT_FLOOR:
    case SPR_SHARP_ROBOT_CEIL:
        AddScore(400);
        break;

    case SPR_SAW_BLADE:
        AddScore(3200);
        break;

    case SPR_SPEAR:
    case SPR_STONE_HEAD_CRUSHER:
    case SPR_PARACHUTE_BALL:
        AddScore(1600);
        break;

    case SPR_SPARK:
    case SPR_RED_JUMPER:
        AddScore(6400);
        break;

    case SPR_SPIKES_FLOOR:
    case SPR_SPIKES_FLOOR_RECIP:
    case SPR_SPIKES_E:
    case SPR_SPIKES_W:
        AddScore(250);
        break;

    case SPR_SUCTION_WALKER:
    case SPR_SPITTING_TURRET:
        AddScore(1000);
        break;

    case SPR_ROAMER_SLUG:
    case SPR_HINT_GLOBE:
        AddScore(12800);
        break;

    case SPR_PUSHER_ROBOT:
        AddScore(2000);
        break;

    case SPR_ARROW_PISTON_W:
    case SPR_ARROW_PISTON_E:
    case SPR_SPIKES_E_RECIP:
    case SPR_SPIT_WALL_PLANT_E:
    case SPR_SPIT_WALL_PLANT_W:
    case SPR_RED_CHOMPER:
    case SPR_SENTRY_ROBOT:
        AddScore(500);
        break;

    case SPR_DRAGONFLY:
        AddScore(50000L);
        break;

    case SPR_74:  /* probably for ACT_BABY_GHOST_EGG_PROX; never happens */
    case SPR_BABY_GHOST_EGG:
    case SPR_EYE_PLANT:
    case SPR_96:  /* " " " ACT_EYE_PLANT_CEIL " " " */
    case SPR_PINK_WORM_SLIME:
    case SPR_BIRD:
        AddScore(100);
        break;

    case SPR_STAR:
    case SPR_CABBAGE:
    case SPR_HEART_PLANT:
    case SPR_BABY_GHOST:
    case SPR_CLAM_PLANT:
    case SPR_84:  /* probably for ACT_CLAM_PLANT_CEIL; never happens */
    case SPR_PINK_WORM:
    case SPR_ROCKET:
        AddScore(200);
        break;
    }
}

/*
Clear the screen, redraw the status bar from the background image in memory,
then redraw all the status bar number and health bar areas.
*/
void RedrawStaticGameScreen(void)
{
    word x, y;
    word src = 0x4000;

    ClearScreen();

    for (y = 19; y < 25; y++) {
        for (x = 1; x < 39; x++) {
            DrawSolidTile(src, x + (y * 320));
            src += 8;
        }
    }

    AddScore(0);
    UpdateStars();
    UpdateBombs();
    UpdateHealth();
}

/*
Draw the main menu frame and text options, but don't wait or process any input.
*/
void ShowMainMenu(void)
{
    word x;

    x = UnfoldTextFrame(2,
#ifdef FOREIGN_ORDERS
    21,
#else
    20,
#endif  /* FOREIGN_ORDERS */
    20, "MAIN MENU", "");

    DrawTextLine(x, 5,  " B)egin New Game");
    DrawTextLine(x, 6,  " R)estore A Game");
    DrawTextLine(x, 7,  " S)tory");
    DrawTextLine(x, 8,  " I)nstructions");
    DrawTextLine(x, 9,  " H)igh Scores");
    DrawTextLine(x, 10, " G)ame Redefine");

    DrawTextLine(x, 12, " O)rdering Info.");

#ifdef FOREIGN_ORDERS
    DrawTextLine(x, 14, " F)oreign Orders");
    DrawTextLine(x, 15, " A)pogee's BBS");
    DrawTextLine(x, 16, " D)emo");
    DrawTextLine(x, 17, " C)redits");
    DrawTextLine(x, 18, " T)itle Screen");

    DrawTextLine(x, 20, " Q)uit Game");
#else
    DrawTextLine(x, 14, " A)pogee's BBS");
    DrawTextLine(x, 15, " D)emo");
    DrawTextLine(x, 16, " C)redits");
    DrawTextLine(x, 17, " T)itle Screen");

    DrawTextLine(x, 19, " Q)uit Game");
#endif  /* FOREIGN_ORDERS */
}

/*
Present a text frame containing the "you need to collect bombs" hint.
*/
void ShowBombHint(void)
{
    word x;

    if (demoState != DEMOSTATE_NONE) return;

    EGA_RESET();
    SelectDrawPage(activePage);
    StartSound(SND_HINT_DIALOG_ALERT);

    x = UnfoldTextFrame(2, 4, 28, "", "");
    DrawTextLine(x + 1, 3, "You haven't found any");
    DrawTextLine(x + 1, 4, "bombs to use yet!     \xFE""056000");
    WaitHard(60);
    WaitSpinner(x + 25, 4);

    SelectDrawPage(!activePage);
}

/*
Present a text frame containing the "pounce on actors" hint.
*/
void ShowPounceHint(void)
{
    word x;

    if (demoState != DEMOSTATE_NONE) return;

    EGA_RESET();
    SelectDrawPage(activePage);
    StartSound(SND_HINT_DIALOG_ALERT);

    x = UnfoldTextFrame(2, 5, 22, "REMINDER:  Jump on", "defend yourself.  ");
    DrawTextLine(x, 4, " top of creatures to");
    WaitHard(60);
    WaitSpinner(x + 19, 5);

    x = UnfoldTextFrame(2, 13, 20, "Like this...", "Press ANY key.");
    DrawTextLine(x + 5, 9,  "   \xFD""036");
    DrawTextLine(x + 5, 11, "   \xFE""118000");
    WaitHard(60);
    WaitSpinner(x + 17, 13);

    SelectDrawPage(!activePage);
}

/*
Present a text frame containing the "now entering level n" animation.
*/
void ShowLevelIntro(word level_num)
{
    byte displayNums[] = {1, 2, 0, 0, 3, 4, 0, 0, 5, 6, 0, 0, 7, 8, 0, 0, 9, 10};
    word x;

    /* This never occurs */
    if (demoState != DEMOSTATE_NONE) return;

    x = UnfoldTextFrame(7, 3, 24, "\xFC""003  Now entering level", "");
    WaitHard(20);
    StartSound(SND_ENTERING_LEVEL_NUM);

    if (displayNums[level_num] == 10) {
        DrawNumberFlushRight(x + 21, 8, (dword)displayNums[level_num]);
    } else {
        DrawNumberFlushRight(x + 20, 8, (dword)displayNums[level_num]);
    }
}

/*
Present a text frame containing the "power up modules" hint.
*/
void ShowHealthHint(void)
{
    word x;

    if (demoState != DEMOSTATE_NONE) return;

    EGA_RESET();
    SelectDrawPage(activePage);
    StartSound(SND_HINT_DIALOG_ALERT);

    x = UnfoldTextFrame(2, 5, 22, "", "");
    DrawTextLine(x, 3, " Power Up modules");
    DrawTextLine(x, 4, " increase Cosmo's");
    DrawTextLine(x, 5, " health.         \xFE""028002");
    WaitHard(60);
    WaitSpinner(x + 8, 5);

    SelectDrawPage(!activePage);
}
