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
 *                         COSMORE HEADER for ACTORS                         *
 *                                                                           *
 * This file contains a list of every actor and special actor type number    *
 * defined by the game, as well as a few relevant constants. The type        *
 * numbers defined here are generally only used when building new actors     *
 * from a map file or spawner. These are _not_ the sprite numbers as used in *
 * the group entry file (although in many cases the actor and sprite type    *
 * numbers have some amount of coorelation).                                 *
 *****************************************************************************/

/*
Global size limits for actor and actor-like arrays.
*/
#define MAX_ACTORS              410
#define MAX_DECORATIONS         10
#define MAX_EXPLOSIONS          7
#define MAX_FOUNTAINS           10
#define MAX_LIGHTS              200
#define MAX_PLATFORMS           10
#define MAX_SHARDS              16
#define MAX_SPAWNERS            6

/*
Special constant used in the propagation of worm crate explosions.
*/
#define WORM_CRATE_EXPLODE      0xf1f1

/*
Special actor types. In map files, these are represented by types 0..31.
*/
#define SPA_PLAYER_START        0  /* map type 0 */
#define SPA_PLATFORM            1
#define SPA_FOUNTAIN_SMALL      2
#define SPA_FOUNTAIN_MEDIUM     3
#define SPA_FOUNTAIN_LARGE      4
#define SPA_FOUNTAIN_HUGE       5
#define SPA_LIGHT_WEST          6
#define SPA_LIGHT_MIDDLE        7
#define SPA_LIGHT_EAST          8

/*
Actor types. In map files, these are represented by types 32..296.
*/
#define ACT_BASKET_NULL         0  /* not expressible in map format */
#define ACT_STAR_FLOAT          1  /* map type 32 */
#define ACT_JUMP_PAD_FLOOR      2
#define ACT_ARROW_PISTON_W      3
#define ACT_ARROW_PISTON_E      4
#define ACT_FIREBALL_W          5
#define ACT_FIREBALL_E          6
#define ACT_HEAD_SWITCH_BLUE    7
#define ACT_HEAD_SWITCH_RED     8
#define ACT_HEAD_SWITCH_GREEN   9
#define ACT_HEAD_SWITCH_YELLOW  10
#define ACT_DOOR_BLUE           11
#define ACT_DOOR_RED            12
#define ACT_DOOR_GREEN          13
#define ACT_DOOR_YELLOW         14
#define ACT_JUMP_PAD_ROBOT      16
#define ACT_SPIKES_FLOOR        17
#define ACT_SPIKES_FLOOR_RECIP  18
#define ACT_SAW_BLADE_VERT      20
#define ACT_SAW_BLADE_HORIZ     22
#define ACT_BOMB_ARMED          24
#define ACT_CABBAGE             25
#define ACT_POWER_UP_FLOAT      28
#define ACT_BARREL_POWER_UP     29
#define ACT_BASKET_GRN_TOMATO   31
#define ACT_GRN_TOMATO          32
#define ACT_BASKET_RED_TOMATO   33
#define ACT_RED_TOMATO          34
#define ACT_BARREL_YEL_PEAR     35
#define ACT_YEL_PEAR            36
#define ACT_BARREL_ONION        37
#define ACT_ONION               38
#define ACT_EXIT_SIGN           39
#define ACT_SPEAR               40
#define ACT_SPEAR_RECIP         41
#define ACT_GRN_SLIME_THROB     42
#define ACT_GRN_SLIME_DRIP      43
#define ACT_FLYING_WISP         44
#define ACT_TWO_TONS_CRUSHER    45
#define ACT_JUMPING_BULLET      46
#define ACT_STONE_HEAD_CRUSHER  47
#define ACT_PYRAMID_CEIL        48
#define ACT_PYRAMID_FALLING     49
#define ACT_PYRAMID_FLOOR       50
#define ACT_GHOST               51
#define ACT_BASKET_GRN_GOURD    52
#define ACT_BASKET_BLU_SPHERES  53
#define ACT_MOON                54
#define ACT_HEART_PLANT         55
#define ACT_BARREL_BOMB         56
#define ACT_BOMB_IDLE           57
#define ACT_BARREL_JUMP_PAD_FL  58
#define ACT_SWITCH_PLATFORMS    59
#define ACT_SWITCH_MYSTERY_WALL 61
#define ACT_MYSTERY_WALL        62
#define ACT_SPIKES_FLOOR_BENT   63
#define ACT_MONUMENT            64
#define ACT_BABY_GHOST          65
#define ACT_PROJECTILE_SW       66
#define ACT_PROJECTILE_SE       67
#define ACT_PROJECTILE_S        68
#define ACT_ROAMER_SLUG         69
#define ACT_PIPE_CORNER_N       70
#define ACT_PIPE_CORNER_S       71
#define ACT_PIPE_CORNER_W       72
#define ACT_PIPE_CORNER_E       73
#define ACT_BABY_GHOST_EGG_PROX 74
#define ACT_BABY_GHOST_EGG      75
#define ACT_SHARP_ROBOT_FLOOR   78
#define ACT_SHARP_ROBOT_CEIL    80
#define ACT_BASKET_HAMBURGER    81
#define ACT_HAMBURGER           82
#define ACT_CLAM_PLANT_FLOOR    83
#define ACT_CLAM_PLANT_CEIL     84
#define ACT_GRAPES              85
#define ACT_PARACHUTE_BALL      86
#define ACT_SPIKES_E            87
#define ACT_SPIKES_E_RECIP      88
#define ACT_SPIKES_W            89
#define ACT_BEAM_ROBOT          90
#define ACT_SPLITTING_PLATFORM  91
#define ACT_SPARK               92
#define ACT_BASKET_DANCE_MUSH   93
#define ACT_DANCING_MUSHROOM    94
#define ACT_EYE_PLANT_FLOOR     95
#define ACT_EYE_PLANT_CEIL      96
#define ACT_BARREL_CABB_HARDER  100
#define ACT_RED_JUMPER          101
#define ACT_BOSS                102
#define ACT_PIPE_OUTLET         104
#define ACT_PIPE_INLET          105
#define ACT_SUCTION_WALKER      106
#define ACT_TRANSPORTER_1       107
#define ACT_TRANSPORTER_2       108
#define ACT_PROJECTILE_W        109
#define ACT_PROJECTILE_E        110
#define ACT_SPIT_WALL_PLANT_E   111
#define ACT_SPIT_WALL_PLANT_W   112
#define ACT_SPITTING_TURRET     113
#define ACT_SCOOTER             114
#define ACT_BASKET_PEA_PILE     115
#define ACT_BASKET_LUMPY_FRUIT  116
#define ACT_BARREL_HORN         117
#define ACT_RED_CHOMPER         118
#define ACT_BASKET_POD          119
#define ACT_SWITCH_LIGHTS       120
#define ACT_SWITCH_FORCE_FIELD  121
#define ACT_FORCE_FIELD_VERT    122
#define ACT_FORCE_FIELD_HORIZ   123
#define ACT_PINK_WORM           124
#define ACT_HINT_GLOBE_0        125
#define ACT_PUSHER_ROBOT        126
#define ACT_SENTRY_ROBOT        127
#define ACT_PINK_WORM_SLIME     128
#define ACT_DRAGONFLY           129
#define ACT_WORM_CRATE          130
#define ACT_BOTTLE_DRINK        134
#define ACT_GRN_GOURD           135
#define ACT_BLU_SPHERES         136
#define ACT_POD                 137
#define ACT_PEA_PILE            138
#define ACT_LUMPY_FRUIT         139
#define ACT_HORN                140
#define ACT_RED_BERRIES         141
#define ACT_BARREL_BOTL_DRINK   142
#define ACT_SATELLITE           143
#define ACT_IVY_PLANT           145
#define ACT_YEL_FRUIT_VINE      146
#define ACT_HEADDRESS           147
#define ACT_BASKET_HEADDRESS    148
#define ACT_EXIT_MONSTER_W      149
#define ACT_EXIT_LINE_VERT      150
#define ACT_SMALL_FLAME         151
#define ACT_TULIP_LAUNCHER      152
#define ACT_ROTATING_ORNAMENT   153
#define ACT_BLU_CRYSTAL         154
#define ACT_RED_CRYSTAL_FLOOR   155
#define ACT_BARREL_RT_ORNAMENT  156
#define ACT_BARREL_BLU_CRYSTAL  157
#define ACT_BARREL_RED_CRYSTAL  158
#define ACT_GRN_TOMATO_FLOAT    159
#define ACT_RED_TOMATO_FLOAT    160
#define ACT_YEL_PEAR_FLOAT      161
#define ACT_BEAR_TRAP           162
#define ACT_FALLING_FLOOR       163
#define ACT_EP1_END_1           164
#define ACT_EP1_END_2           165
#define ACT_EP1_END_3           166
#define ACT_BASKET_ROOT         167
#define ACT_ROOT                168
#define ACT_BASKET_RG_BERRIES   169
#define ACT_REDGRN_BERRIES      170
#define ACT_BASKET_RED_GOURD    171
#define ACT_RED_GOURD           172
#define ACT_BARREL_GRN_EMERALD  173
#define ACT_GRN_EMERALD         174
#define ACT_BARREL_CLR_DIAMOND  175
#define ACT_CLR_DIAMOND         176
#define ACT_SCORE_EFFECT_100    177
#define ACT_SCORE_EFFECT_200    178
#define ACT_SCORE_EFFECT_400    179
#define ACT_SCORE_EFFECT_800    180
#define ACT_SCORE_EFFECT_1600   181
#define ACT_SCORE_EFFECT_3200   182
#define ACT_SCORE_EFFECT_6400   183
#define ACT_SCORE_EFFECT_12800  184
#define ACT_EXIT_PLANT          186
#define ACT_BIRD                187
#define ACT_ROCKET              188
#define ACT_INVINCIBILITY_CUBE  189
#define ACT_PEDESTAL_SMALL      190
#define ACT_PEDESTAL_MEDIUM     191
#define ACT_PEDESTAL_LARGE      192
#define ACT_BARREL_CYA_DIAMOND  193
#define ACT_CYA_DIAMOND         194
#define ACT_BARREL_RED_DIAMOND  195
#define ACT_RED_DIAMOND         196
#define ACT_BARREL_GRY_OCTAHED  197
#define ACT_GRY_OCTAHEDRON      198
#define ACT_BARREL_BLU_EMERALD  199
#define ACT_BLU_EMERALD         200
#define ACT_INVINCIBILITY_BUBB  201
#define ACT_THRUSTER_JET        202
#define ACT_EXIT_TRANSPORTER    203
#define ACT_HINT_GLOBE_1        204
#define ACT_HINT_GLOBE_2        205
#define ACT_HINT_GLOBE_3        206
#define ACT_HINT_GLOBE_4        207
#define ACT_HINT_GLOBE_5        208
#define ACT_HINT_GLOBE_6        209
#define ACT_HINT_GLOBE_7        210
#define ACT_HINT_GLOBE_8        211
#define ACT_HINT_GLOBE_9        212
#define ACT_CYA_DIAMOND_FLOAT   213
#define ACT_RED_DIAMOND_FLOAT   214
#define ACT_GRY_OCTAHED_FLOAT   215
#define ACT_BLU_EMERALD_FLOAT   216
#define ACT_JUMP_PAD_CEIL       217
#define ACT_BARREL_HEADPHONES   218
#define ACT_HEADPHONES_FLOAT    219
#define ACT_HEADPHONES          220
#define ACT_FROZEN_DN           221
#define ACT_BANANAS             223
#define ACT_BASKET_RED_LEAFY    224
#define ACT_RED_LEAFY_FLOAT     225
#define ACT_RED_LEAFY           226
#define ACT_BASKET_BRN_PEAR     227
#define ACT_BRN_PEAR_FLOAT      228
#define ACT_BRN_PEAR            229
#define ACT_BASKET_CANDY_CORN   230
#define ACT_CANDY_CORN_FLOAT    231
#define ACT_CANDY_CORN          232
#define ACT_FLAME_PULSE_W       233
#define ACT_FLAME_PULSE_E       234
#define ACT_SPEECH_OUCH         235
#define ACT_RED_SLIME_THROB     236
#define ACT_RED_SLIME_DRIP      237
#define ACT_HINT_GLOBE_10       238
#define ACT_HINT_GLOBE_11       239
#define ACT_HINT_GLOBE_12       240
#define ACT_HINT_GLOBE_13       241
#define ACT_HINT_GLOBE_14       242
#define ACT_HINT_GLOBE_15       243
#define ACT_SPEECH_WHOA         244
#define ACT_SPEECH_UMPH         245
#define ACT_SPEECH_WOW_50K      246
#define ACT_EXIT_MONSTER_N      247
#define ACT_SMOKE_EMIT_SMALL    248
#define ACT_SMOKE_EMIT_LARGE    249
#define ACT_EXIT_LINE_HORIZ     250
#define ACT_CABBAGE_HARDER      251
#define ACT_RED_CRYSTAL_CEIL    252
#define ACT_HINT_GLOBE_16       253
#define ACT_HINT_GLOBE_17       254
#define ACT_HINT_GLOBE_18       255
#define ACT_HINT_GLOBE_19       256
#define ACT_HINT_GLOBE_20       257
#define ACT_HINT_GLOBE_21       258
#define ACT_HINT_GLOBE_22       259
#define ACT_HINT_GLOBE_23       260
#define ACT_HINT_GLOBE_24       261
#define ACT_HINT_GLOBE_25       262
#define ACT_POWER_UP            263
#define ACT_STAR                264
#define ACT_EP2_END_LINE        265
