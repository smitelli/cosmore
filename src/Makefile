#**
#* Cosmore
#* Copyright (c) 2020-2023 Scott Smitelli and contributors
#*
#* This source code is licensed under the MIT license found in the LICENSE file
#* in the root directory of this source tree.
#*

# *****************************************************************************
# *                             COSMORE MAKEFILE                              *
# *                                                                           *
# * This file is designed only for use with MAKE.EXE packaged with Turbo C.   *
# * The game will require enough modification to compile on a modern OS that  *
# * there seemed to be little point in trying to write this in a portable or  *
# * overly configurable way.                                                  *
# *****************************************************************************

!if !$d(EPISODE)
EPISODE=0
!endif

# Configure these lines to match your Turbo C installation
TCDIR=C:\TC20
INCLUDEDIR=$(TCDIR)\INCLUDE
LIBDIR=$(TCDIR)\LIB
STARTUPDIR=$(TCDIR)\STARTUP

# MODEL | LIBMODEL | LONGMODEL | Description
# ------+----------+-----------+------------
# t     | S        | TINY      | CS=DS=ES=SS; near pointers
# s     | S        | SMALL     | DS=ES=SS; near pointers
# c     | C        | COMPACT   | Code uses near pointers; data uses far pointers
# m     | M        | MEDIUM    | Code uses far pointers; data uses near pointers
# l     | L        | LARGE     | Far pointers
# h     | H        | HUGE      | Far pointers; static data can exceed 64k
MODEL=l
LIBMODEL=L
LONGMODEL=LARGE

OBJS=c0$(MODEL).obj main.obj game1.obj game2.obj
OUTEXE=cosmore$(EPISODE).exe

all: $(OUTEXE)

clean:
	@del cosmore*.exe
	@del cosmore*.map
	@del *.obj

c0$(MODEL).obj: c0.asm lowlevel.asm
	# NOTE: Don't include extension in last arg; overwrite behavior is weird
	tasm /d__$(LONGMODEL)__ /i$(STARTUPDIR) c0.asm, $*

main.obj: main.c
	# main() function requires 8086-compatible code generation (-1-)
	tcc -m$(MODEL) -I$(INCLUDEDIR) -DEPISODE=$(EPISODE) -1- -c $*.c

game2.obj: game2.c
	# Original GAME2 was compiled with string deduplication disabled (-d-), and
	# needs the inline assembly hint (-B) to avoid a wasteful compile restart.
	tcc -m$(MODEL) -I$(INCLUDEDIR) -DEPISODE=$(EPISODE) -d- -B -c $*.c

.c.obj:
	tcc -m$(MODEL) -I$(INCLUDEDIR) -DEPISODE=$(EPISODE) -c $<

$(OUTEXE): $(OBJS)
	tlink /c /d /s $(OBJS), $<, , $(LIBDIR)\C$(LIBMODEL).LIB
