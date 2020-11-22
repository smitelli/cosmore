/**
 * Copyright (c) 2020 Scott Smitelli
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

/*****************************************************************************
 *                        COSMORE HEADER for SPRITES                         *
 *                                                                           *
 * This is a list of every sprite type number available in the game. These   *
 * are the values that are used in the majority of places in the code, and   *
 * these are also what the actual graphics data is indexed by. To save a bit *
 * of compile-time memory, sprite types that are never referenced are        *
 * commented out.                                                            *
 *****************************************************************************/

#define SPR_BASKET              0
#define SPR_STAR                1
#define SPR_JUMP_PAD            2
#define SPR_ARROW_PISTON_W      3
#define SPR_ARROW_PISTON_E      4
#define SPR_FIREBALL            5
#define SPR_6                   6  /* (incorrectly) intends to say ACT_FIREBALL_E */
#define SPR_HEAD_SWITCH_BLUE    7
#define SPR_HEAD_SWITCH_RED     8
#define SPR_HEAD_SWITCH_GREEN   9
#define SPR_HEAD_SWITCH_YELLOW  10
#define SPR_DOOR_BLUE           11
#define SPR_DOOR_RED            12
#define SPR_DOOR_GREEN          13
#define SPR_DOOR_YELLOW         14
#define SPR_SPARKLE_SHORT       15
#define SPR_JUMP_PAD_ROBOT      16
#define SPR_SPIKES_FLOOR        17
#define SPR_SPIKES_FLOOR_RECIP  18
#define SPR_SCOOTER_EXHAUST     19
#define SPR_SAW_BLADE           20
#define SPR_POUNCE_DEBRIS       21
/*#define SPR_22                  22*/
#define SPR_SPARKLE_LONG        23
#define SPR_BOMB_ARMED          24
#define SPR_CABBAGE             25
#define SPR_EXPLOSION           26
#define SPR_RAINDROP            27
#define SPR_POWER_UP            28
#define SPR_BARREL              29
#define SPR_BARREL_SHARDS       30
/*#define SPR_31                  31*/
#define SPR_GRN_TOMATO          32
/*#define SPR_33                  33*/
#define SPR_RED_TOMATO          34
/*#define SPR_35                  35*/
#define SPR_YEL_PEAR            36
/*#define SPR_37                  37*/
#define SPR_ONION               38
#define SPR_EXIT_SIGN           39
/*#define SPR_40                  40*/
#define SPR_SPEAR               41
/*#define SPR_42                  42*/
#define SPR_GREEN_SLIME         43
#define SPR_FLYING_WISP         44
#define SPR_TWO_TONS_CRUSHER    45
#define SPR_JUMPING_BULLET      46
#define SPR_STONE_HEAD_CRUSHER  47
#define SPR_48                  48  /* (incorrectly) intends to say ACT_PYRAMID_CEIL */
#define SPR_PYRAMID             49
#define SPR_50                  50  /* (incorrectly) intends to say ACT_PYRAMID_FLOOR */
#define SPR_GHOST               51
/*#define SPR_52                  52*/
/*#define SPR_53                  53*/
#define SPR_MOON                54
#define SPR_HEART_PLANT         55
/*#define SPR_56                  56*/
#define SPR_BOMB_IDLE           57
/*#define SPR_58                  58*/
/*#define SPR_59                  59*/
#define SPR_FOOT_SWITCH         60
/*#define SPR_61                  61*/
#define SPR_MYSTERY_WALL        62
#define SPR_SPIKES_FLOOR_BENT   63
#define SPR_MONUMENT            64
#define SPR_BABY_GHOST          65
/*#define SPR_66                  66*/
/*#define SPR_67                  67*/
#define SPR_PROJECTILE          68
#define SPR_ROAMER_SLUG         69
#define SPR_PIPE_CORNER_N       70
#define SPR_PIPE_CORNER_S       71
#define SPR_PIPE_CORNER_W       72
#define SPR_PIPE_CORNER_E       73
#define SPR_74                  74  /* (incorrectly) intends to say ACT_BABY_GHOST_EGG_PROX */
#define SPR_BABY_GHOST_EGG      75
#define SPR_BGHOST_EGG_SHARD_1  76
#define SPR_BGHOST_EGG_SHARD_2  77
#define SPR_SHARP_ROBOT_FLOOR   78
#define SPR_FOUNTAIN            79
#define SPR_SHARP_ROBOT_CEIL    80
/*#define SPR_81                  81*/
#define SPR_HAMBURGER           82
#define SPR_CLAM_PLANT          83
#define SPR_84                  84  /* (incorrectly) intends to say ACT_CLAM_PLANT_CEIL */
#define SPR_GRAPES              85
#define SPR_PARACHUTE_BALL      86
#define SPR_SPIKES_E            87
#define SPR_SPIKES_E_RECIP      88
#define SPR_SPIKES_W            89
#define SPR_BEAM_ROBOT          90
#define SPR_SPLITTING_PLATFORM  91
#define SPR_SPARK               92
/*#define SPR_93                  93*/
#define SPR_DANCING_MUSHROOM    94
#define SPR_EYE_PLANT           95
#define SPR_96                  96  /* (incorrectly) intends to say ACT_EYE_PLANT_CEIL */
#define SPR_SMOKE               97
#define SPR_SMOKE_LARGE         98
#define SPR_SPARKLE_SLIPPERY    99
/*#define SPR_100                 100*/
#define SPR_RED_JUMPER          101
#define SPR_BOSS                102
/*#define SPR_103                 103*/
/*#define SPR_104                 104*/
#define SPR_PIPE_END            105
#define SPR_SUCTION_WALKER      106
#define SPR_TRANSPORTER_107     107
#define SPR_TRANSPORTER_108     108
/*#define SPR_109                 109*/
/*#define SPR_110                 110*/
#define SPR_SPIT_WALL_PLANT_E   111
#define SPR_SPIT_WALL_PLANT_W   112
#define SPR_SPITTING_TURRET     113
#define SPR_SCOOTER             114
/*#define SPR_115                 115*/
/*#define SPR_116                 116*/
/*#define SPR_117                 117*/
#define SPR_RED_CHOMPER         118
/*#define SPR_119                 119*/
/*#define SPR_120                 120*/
/*#define SPR_121                 121*/
#define SPR_FORCE_FIELD_VERT    122
#define SPR_FORCE_FIELD_HORIZ   123
#define SPR_PINK_WORM           124
#define SPR_HINT_GLOBE          125
#define SPR_PUSHER_ROBOT        126
#define SPR_SENTRY_ROBOT        127
#define SPR_PINK_WORM_SLIME     128
#define SPR_DRAGONFLY           129
#define SPR_WORM_CRATE          130
#define SPR_WORM_CRATE_SHARDS   131
#define SPR_BGHOST_EGG_SHARD_3  132
#define SPR_BGHOST_EGG_SHARD_4  133
#define SPR_BOTTLE_DRINK        134
#define SPR_GRN_GOURD           135
#define SPR_BLU_SPHERES         136
#define SPR_POD                 137
#define SPR_PEA_PILE            138
#define SPR_LUMPY_FRUIT         139
#define SPR_HORN                140
#define SPR_RED_BERRIES         141
/*#define SPR_142                 142*/
#define SPR_SATELLITE           143
#define SPR_SATELLITE_SHARDS    144
#define SPR_IVY_PLANT           145
#define SPR_YEL_FRUIT_VINE      146
#define SPR_HEADDRESS           147
/*#define SPR_148                 148*/
#define SPR_EXIT_MONSTER_W      149
#define SPR_150                 150  /* copy of _SMALL_FLAME; never shown */
#define SPR_SMALL_FLAME         151
#define SPR_TULIP_LAUNCHER      152
#define SPR_ROTATING_ORNAMENT   153
#define SPR_BLU_CRYSTAL         154
#define SPR_RED_CRYSTAL         155
/*#define SPR_156                 156*/
/*#define SPR_157                 157*/
/*#define SPR_158                 158*/
/*#define SPR_159                 159*/
/*#define SPR_160                 160*/
/*#define SPR_161                 161*/
#define SPR_BEAR_TRAP           162
#define SPR_FALLING_FLOOR       163
#define SPR_164                 164  /* copy of _ROOT; never shown */
/*#define SPR_165                 165*/
/*#define SPR_166                 166*/
/*#define SPR_167                 167*/
#define SPR_ROOT                168
/*#define SPR_169                 169*/
#define SPR_REDGRN_BERRIES      170
/*#define SPR_171                 171*/
#define SPR_RED_GOURD           172
/*#define SPR_173                 173*/
#define SPR_GRN_EMERALD         174
/*#define SPR_175                 175*/
#define SPR_CLR_DIAMOND         176
#define SPR_SCORE_EFFECT_100    177
#define SPR_SCORE_EFFECT_200    178
#define SPR_SCORE_EFFECT_400    179
#define SPR_SCORE_EFFECT_800    180
#define SPR_SCORE_EFFECT_1600   181
#define SPR_SCORE_EFFECT_3200   182
#define SPR_SCORE_EFFECT_6400   183
#define SPR_SCORE_EFFECT_12800  184
#define SPR_BASKET_SHARDS       185
#define SPR_EXIT_PLANT          186
#define SPR_BIRD                187
#define SPR_ROCKET              188
#define SPR_INVINCIBILITY_CUBE  189
/*#define SPR_190                 190*/
/*#define SPR_191                 191*/
#define SPR_PEDESTAL            192
/*#define SPR_193                 193*/
#define SPR_CYA_DIAMOND         194
/*#define SPR_195                 195*/
#define SPR_RED_DIAMOND         196
/*#define SPR_197                 197*/
#define SPR_GRY_OCTAHEDRON      198
/*#define SPR_199                 199*/
#define SPR_BLU_EMERALD         200
#define SPR_INVINCIBILITY_BUBB  201
#define SPR_THRUSTER_JET        202
/*#define SPR_203                 203*/
/*#define SPR_204                 204*/
/*#define SPR_205                 205*/
/*#define SPR_206                 206*/
/*#define SPR_207                 207*/
/*#define SPR_208                 208*/
/*#define SPR_209                 209*/
/*#define SPR_210                 210*/
/*#define SPR_211                 211*/
/*#define SPR_212                 212*/
/*#define SPR_213                 213*/
/*#define SPR_214                 214*/
/*#define SPR_215                 215*/
/*#define SPR_216                 216*/
/*#define SPR_217                 217*/
/*#define SPR_218                 218*/
/*#define SPR_219                 219*/
#define SPR_HEADPHONES          220
#define SPR_FROZEN_DN           221
#define SPR_SPEECH_MULTI        222  /* "YIKES!", "OH NO!", "YEOW!", "UH OH!", "MOMMY!" */
#define SPR_BANANAS             223
/*#define SPR_224                 224*/
/*#define SPR_225                 225*/
#define SPR_RED_LEAFY           226
/*#define SPR_227                 227*/
/*#define SPR_228                 228*/
#define SPR_BRN_PEAR            229
/*#define SPR_230                 230*/
/*#define SPR_231                 231*/
#define SPR_CANDY_CORN          232
#define SPR_FLAME_PULSE_W       233
#define SPR_FLAME_PULSE_E       234
#define SPR_SPEECH_OUCH         235
/*#define SPR_236                 236*/
#define SPR_RED_SLIME           237
/*#define SPR_238                 238*/
/*#define SPR_239                 239*/
/*#define SPR_240                 240*/
/*#define SPR_241                 241*/
/*#define SPR_242                 242*/
/*#define SPR_243                 243*/
#define SPR_SPEECH_WHOA         244
#define SPR_SPEECH_UMPH         245
#define SPR_SPEECH_WOW_50K      246
#define SPR_EXIT_MONSTER_N      247
#define SPR_248                 248  /* copy of _CABBAGE; never shown */
#define SPR_249                 249  /* copy of _CABBAGE; never shown */
#define SPR_250                 250  /* copy of _CABBAGE; never shown */
/*#define SPR_251                 251*/
/*#define SPR_252                 252*/
/*#define SPR_253                 253*/
/*#define SPR_254                 254*/
/*#define SPR_255                 255*/
/*#define SPR_256                 256*/
/*#define SPR_257                 257*/
/*#define SPR_258                 258*/
/*#define SPR_259                 259*/
/*#define SPR_260                 260*/
/*#define SPR_261                 261*/
/*#define SPR_262                 262*/
/*#define SPR_263                 263*/
/*#define SPR_264                 264*/
#define SPR_265                 265  /* copy of _DEMO_OVERLAY; never shown */
#define SPR_DEMO_OVERLAY        266
