@rem **************************************************************************
@rem *                          COSMORE BUILD SCRIPT                          *
@rem *                                                                        *
@rem * This is a batch file that builds all three episode of the game         *
@rem * sequentially. If successful, all files will be in the DIST\ directory. *
@rem **************************************************************************

@echo off

md dist

make clean
del dist\cosmore1.exe
make -DEPISODE=1
lzexe91\lzexe cosmore1
del cosmore1.old
ren cosmore1.exe dist\cosmore1.exe

make clean
del dist\cosmore2.exe
make -DEPISODE=2
lzexe91\lzexe cosmore2
del cosmore2.old
ren cosmore2.exe dist\cosmore2.exe

make clean
del dist\cosmore3.exe
make -DEPISODE=3
rem Episode 3 wasn't compressed for release, so we don't do it either
rem lzexe91\lzexe cosmore3
rem del cosmore3.old
ren cosmore3.exe dist\cosmore3.exe
