#**
#* Copyright (c) 2020 Scott Smitelli
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

# MODEL | LONGMODEL | Description
# ------+-----------+------------
# t     | TINY      | CS=DS=ES=SS; near pointers; NOTE: Set CLIBFILE to CS.LIB!
# s     | SMALL     | DS=ES=SS; near pointers
# c     | COMPACT   | Code uses near pointers; data uses far pointers
# m     | MEDIUM    | Code uses far pointers; data uses near pointers
# l     | LARGE     | Far pointers
# h     | HUGE      | Far pointers; static data can exceed 64k
MODEL=l
LONGMODEL=LARGE

# Configure these lines to match your Turbo C installation
CLIBFILE=C:\TC20\LIB\C$(MODEL).LIB
INCLUDEDIR=C:\TC20\INCLUDE
STARTUPDIR=C:\TC20\STARTUP

OBJS=c0$(MODEL).obj main.obj game1.obj game2.obj
OUTEXE=cosmore$(EPISODE).exe

all: $(OUTEXE)

clean:
	@del cosmore*.exe
	@del *.map
	@del *.obj

$(OUTEXE): $(OBJS)
	tlink /c /d /s $(OBJS), $<, , $(CLIBFILE)

c0$(MODEL).obj: c0.asm lowlevel.asm
	# $* instead of $< avoids weird overwrite misbehavior if the target exists
	tasm /d__$(LONGMODEL)__ /i$(STARTUPDIR) c0.asm, $*

main.obj: main.c
	# main() function requires 8086-compatible code generation (-1-)
	tcc -m$(MODEL) -I$(INCLUDEDIR) -DEPISODE=$(EPISODE) -1- -c main.c

game2.obj: game2.c
	# Original GAME2 was compiled with string deduplication disabled (-d-), and
	# needs the inline assembly hint (-B) to avoid a wasteful compile restart.
	tcc -m$(MODEL) -I$(INCLUDEDIR) -DEPISODE=$(EPISODE) -d- -B -c game2.c

.c.obj:
	tcc -m$(MODEL) -I$(INCLUDEDIR) -DEPISODE=$(EPISODE) -c $<
