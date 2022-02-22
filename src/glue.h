/**
 * Cosmore
 * Copyright (c) 2020-2022 Scott Smitelli
 *
 * Based on COSMO{1..3}.EXE distributed with "Cosmo's Cosmic Adventure"
 * Copyright (c) 1992 Apogee Software, Ltd.
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

/*****************************************************************************
 *                         COSMORE MAIN "GLUE" HEADER                        *
 *                                                                           *
 * The apparent intention of the author(s) of the original game was to have  *
 * every C function and variable in a single massive file. This is because   *
 * discrete C files compile into discrete OBJ files, and function calls that *
 * cross an OBJ file boundary become "far" calls which reload the            *
 * processor's CS register and take more clock cycles to execute. There is   *
 * too much code to compile as a single unit under DOS, so there are two     *
 * C files: GAME1 and GAME2.                                                 *
 *                                                                           *
 * This header file is literally the glue that holds these two files         *
 * together, declaring every include, typedef, variable, and function that   *
 * the pieces share.                                                         *
 *****************************************************************************/

#ifndef GLUE_H
#define GLUE_H

/* Replace the in-game status bar with a snapshot of some global variables */
/*#define DEBUG_BAR*/

/* Enable this to add vanity text inside the game */
/*#define VANITY*/

#include <alloc.h>  /* for coreleft() only */
#include <conio.h>
#include <dos.h>
#include <io.h>  /* for filelength() only */
#include <mem.h>  /* for movmem() only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Don't typedef here, or bools can't use registers and get forced on stack. */
enum {false = 0, true};

typedef unsigned char byte;
typedef unsigned int  word;
typedef unsigned long dword;
typedef word bool;
typedef byte bbool;  /* bool in a byte */
typedef void interrupt (*InterruptFunction)(void);

#define BYTE_MAX 0xffU
#define WORD_MAX 0xffffU

#include "actor.h"
#include "def.h"
#include "graphics.h"
#include "lowlevel.h"
#include "music.h"
#include "player.h"
#include "scancode.h"
#include "sound.h"
#include "sprite.h"

#if EPISODE == 1
#   include "episode1.h"
#elif EPISODE == 2
#   include "episode2.h"
#elif EPISODE == 3
#   include "episode3.h"
#else
#   error "EPISODE must be defined to 1, 2, or 3"
#endif  /* EPISODE == ... */

/*****************************************************************************
 * GAME1.C                                                                   *
 *****************************************************************************/

typedef char HighScoreName[16];
typedef void (*ActorTickFunction)(word);
typedef void (*DrawFunction)(byte *, word, word);

typedef struct {
    word sprite;
    word frame;
    word x;
    word y;
    bool forceactive;
    bool stayactive;
    bool acrophile;
    bool weighted;
    word private1;
    word private2;
    word data1;
    word data2;
    word data3;
    word data4;
    word data5;
    bool dead;
    word fallspeed;
    byte damagecooldown;
    ActorTickFunction tickfunc;
} Actor;

typedef struct {
    bool alive;
    word sprite;
    word numframes;
    word x;
    word y;
    word dir;
    word numtimes;
} Decoration;

typedef struct {
    word age;
    word x;
    word y;
} Explosion;

typedef struct {
    word x;
    word y;
    word dir;
    word stepcount;
    word height;
    word stepmax;
    word delayleft;
} Fountain;

typedef struct {
    word side;
    word x;
    word y;
    word junk;  /* unused; required to keep struct padding correct */
} Light;

typedef struct {
    word length;
    word datahead;
} Music;

typedef struct {
    word x;
    word y;
    word mapstash[5];
} Platform;

typedef struct {
    word sprite;
    word x;
    word y;
    word frame;
    word age;
    word inclination;
    bool bounced;
} Shard;

typedef struct {
    word actor;
    word x;
    word y;
    word age;
} Spawner;

extern bbool isInGame;
extern bool winGame;
extern dword gameScore, gameStars;
extern word miscDataContents;
extern word playerHealth, playerMaxHealth, playerBombs;
extern byte demoState;
extern word activePage;
extern word gameTickCount;
extern bool isGodMode;
extern char *stnGroupFilename, *volGroupFilename;
extern char *musicNames[];
extern dword totalMemFreeBefore, totalMemFreeAfter;
extern HighScoreName highScoreNames[];
extern dword highScoreValues[];
extern byte *fontTileData, *maskedTileData, *miscData;
extern dword lastGroupEntryLength;
extern byte lastScancode;
extern bbool isKeyDown[];
extern bool isJoystickReady;
extern bbool cmdWest, cmdEast, cmdNorth, cmdSouth, cmdJump, cmdBomb;
extern bool isMusicEnabled, isSoundEnabled;
extern byte scancodeWest, scancodeEast, scancodeNorth, scancodeSouth, scancodeJump, scancodeBomb;
extern Music *activeMusic;
extern word numActors;

void DrawTextLine(word x_origin, word y_origin, char *text);
void DrawFullscreenImage(word image_num);
void StartSound(word sound_num);
void PCSpeakerService(void);
void ShowStarBonus(void);
void InnerMain(int argc, char *argv[]);

/*****************************************************************************
 * GAME2.C                                                                   *
 *****************************************************************************/

typedef char KeyName[6];

typedef struct {
    word junk;  /* in IDLIBC.C/ControlJoystick(), this is movement direction */
    bool button1;
    bool button2;
} JoystickState;

extern word yOffsetTable[];
extern bbool isAdLibPresent;

void StartAdLib(void);
void StopAdLib(void);
void WaitHard(word delay);
void WaitSoft(word delay);
void FadeToWhite(word delay);
void FadeOutCustom(word delay);
void FadeIn(void);
void FadeOut(void);
byte WaitSpinner(word x, word y);
void ClearScreen(void);
JoystickState ReadJoystickState(word stick);
word UnfoldTextFrame(int top, int height, int width, char *top_text, char *bottom_text);
void ReadAndEchoText(word x_origin, word y_origin, char *dest, word max_length);
void DrawNumberFlushRight(word x_origin, word y_origin, dword value);
void AddScore(dword points);
void UpdateStars(void);
void UpdateBombs(void);
void UpdateHealth(void);
void ShowHighScoreTable(void);
void CheckHighScore(void);
FILE *GroupEntryFp(char *entry_name);
void ShowOrderingInformation(void);
void ShowStory(void);
void StartGameMusic(word music_num);
void StartMenuMusic(word music_num);
void StopMusic(void);
void DrawScancodeCharacter(word x, word y, byte scancode);
void PrepareBackdropVScroll(byte *p1, byte *p2, byte *p3);
void PrepareBackdropHScroll(byte *p1, byte *p2);
void ShowHintsAndKeys(word y);
void ShowInstructions(void);
void ShowPublisherBBS(void);
#ifdef FOREIGN_ORDERS
void ShowForeignOrders(void);
#endif  /* FOREIGN_ORDERS */
void ShowRestoreGameError(void);
void ShowCopyright(void);
void ShowAlteredFileError(void);
void ShowRescuedDNMessage(void);
void ShowE1CliffhangerMessage(word actor_type);
bbool PromptQuitConfirm(void);
void ToggleSound(void);
void ToggleMusic(void);
void PauseMessage(void);
void GodModeToggle(void);
void MemoryUsage(void);
void GameRedefineMenu(void);
void LoadConfigurationData(char *filename);
void SaveConfigurationData(char *filename);
void ShowEnding(void);
void ShowHintGlobeMessage(word hint_num);
void ShowCheatMessage(void);
void AddScoreForSprite(word sprite_type);
void RedrawStaticGameScreen(void);
void ShowMainMenu(void);
void ShowBombHint(void);
void ShowPounceHint(void);
void ShowLevelIntro(word level_num);
void ShowHealthHint(void);

#endif  /* GLUE_H */
