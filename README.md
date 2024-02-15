# Cosmore: _Cosmo_ Reconstructed

by [Scott Smitelli](mailto:scott@smitelli.com) and [contributors](AUTHORS.md)

Cosmore is a reconstruction of the source code of _Cosmo's Cosmic Adventure_ version 1.20, using the original DOS compiler and toolchain. Its goal is to duplicate every detail, quirk, and bug of the original game as faithfully as possible. A player should not be able to distinguish a Cosmore binary from the original.

The reconstruction is **96.29%** accurate by the following metric: The load images (skipping the EXE headers and relocation tables), when compared byte-by-byte against the corresponding uncompressed originals, have the same byte values in the same locations 96% of the time. The remaining 4% consists of uninitialized data segment memory addresses that have not (yet!) been arranged to match the original layout.

## Requirements

Cosmore requires a DOS environment to compile and run. [DOSBox](https://www.dosbox.com/) is suitable for this purpose, but a real computer running DOS standalone should also work.

There is no game data provided with this project, so you must have a copy of `COSMOx.STN` and `COSMOx.VOL` for the episode(s) you wish to play. Episode 1 is shareware and the files are widely available. Episodes 2 and 3 are registered versions and the files are **not** free to download.

The build process requires Borland Turbo C version 2.0 and Turbo Assembler version 1.0. The original game was most likely developed with Turbo C 2.0 Professional, which included both products as well as Turbo Debugger 1.5. Borland's successor, Embarcadero Technologies, no longer hosts these exact versions of their long-discontinued products, but they do still exist out there.

**Note:** Turbo C was released as version 2.0 in 1988 and 2.01 in 1989. Turbo Assembler was released as version 1.0 in 1988 and 1.01 in 1989. All of these versions are interchangeable and can build correctly, but the resulting binaries will have subtle differences. For maximum accuracy, use Turbo C version **2.0** and Turbo Assembler version **1.0**.

The full build process requires an obscene amount of free conventional memory -- about 630 KiB -- at peak. It's possible to shave about 45 KiB off this requirement by eschewing `make` and running the compiler manually on troublesome files. Additionally, another 40 KiB can be spared by running the `*.C` files through the standalone C preprocessor (`cpp -P- ... *.c`) before compiling to slim down the total number of symbols, but that's all a bit Rube Goldberg-esque.

## Environment Setup

There are many different ways to set up a DOS environment depending on your host platform and individual preferences. This guide will make some reasonable assumptions and suggestions, but you are free to deviate as desired. See the [DOSBox Wiki page for MOUNT](https://www.dosbox.com/wiki/MOUNT) for information about mapping the host filesystem into DOS and mounting installation disk images. **All of the instructions in this guide should be done within the DOS environment, not the host.**

For the most pleasant experience, the Turbo C and Turbo Assembler binaries should both be installed into the `C:\TC20` directory, and this location should be added to the DOS `%PATH%` variable. The library and support files included with these programs should be installed into subdirectories named `INCLUDE`, `LIB`, and `STARTUP`.

This directory tree shows the relevant files and their locations:

    C:\
     └──TC20\
         ├──MAKE.EXE
         ├──TASM.EXE
         ├──TCC.EXE
         ├──TLINK.EXE
         ├──INCLUDE\
         │   ├──ALLOC.H
         │   ├──CONIO.H
         │   ├──DOS.H
         │   ├──IO.H
         │   ├──MEM.H
         │   ├──STDARG.H
         │   ├──STDIO.H
         │   ├──STDLIB.H
         │   └──STRING.H
         ├──LIB\
         │   └──CL.LIB
         └──STARTUP\
             ├──EMUVARS.ASI
             └──RULES.ASI

Your installation will undoubtedly have many more files, but these are the only ones strictly required for this project.

To append the `TC20` directory to `%PATH%`, making its `*.EXE` files available for use regardless of the current working directory, run:

    SET PATH=%PATH%;C:\TC20

To make this change persist across restarts, add the line to the `[autoexec]` section of your [DOSBox configuration](https://www.dosbox.com/wiki/Dosbox.conf) or, for true DOS environments, add it to `AUTOEXEC.BAT`.

Execute the following four commands and verify the output to test the installation of each program and its place in `%PATH%`. These should work regardless of the current working directory:

### MAKE

    >make -h
    MAKE  Version 2.0   Copyright (c) 1987, 1988 Borland International
    Syntax: MAKE [options ...] target[s]
    ...

### TASM (Turbo Assembler)

    >tasm
    Turbo Assembler  Version 1.0  Copyright (c) 1988 by Borland International
    Syntax:  TASM [options] source [,object] [,listing] [,xref]
    ...

### TCC (Turbo C Compiler)

    >tcc
    Turbo C  Version 2.0  Copyright (c) 1987, 1988 Borland International
    Syntax is: TCC [ options ] file[s]       ...

### TLINK (Turbo Link)

    >tlink
    Turbo Link  Version 2.0  Copyright (c) 1987, 1988 Borland International
    Syntax:  TLINK objfiles, exefile, mapfile, libfiles
    ...

If all four commands produce the expected output, the environment is ready.

## Compiling

`CD` into the `src` directory before running any compilation commands. All compilation is performed by `make`.

**Note:** It is not possible to compile (or even load!) the game source files in the Turbo C development environment (`tc`) due to the sheer size of the files. Even if the files were sized more modestly, the use of inline assembly in some functions prevents compilation in this environment from succeeding.

### Customize the build process (if necessary):

Before beginning, examine the contents of `Makefile` to ensure the filenames and paths match the actual setup of the environment:

* The Turbo C standard library file is expected to be at `C:\TC20\LIB\Cx.LIB`, with _x_ always set to `L` unless the memory model is being messed with.
* The include directory, where the Borland-provided `*.H` files are found, is expected to be at `C:\TC20\INCLUDE`.
* The startup directory, where the Borland-provided `*.ASI` files are found, is expected to be at `C:\TC20\STARTUP`.

If any of these do not match the environment, edit `Makefile` as needed.

### To clean up all generated `*.EXE`, `*.OBJ`, and `*.MAP` files:

    make clean

### To build the game:

    make -DEPISODE=n

where _n_ is an episode number: `1`, `2`, or `3`.

**IMPORTANT:** **Always** run `make clean` before changing the `EPISODE` number. `make` is not smart enough to understand that the game needs to be fully recompiled when the episode number changes, and failure to clean the directory may result in the wrong episode (or a broken game!) being generated.

The game will be built as `COSMOREx.EXE`, and a map file containing the addresses of all public symbols will be built as `COSMOREx.MAP`. (The _x_ here -- and subsequently in this document -- matches the specified episode number.)

Compilation of a single episode takes about 1 minute and 50 seconds under DOSBox at the default 3,000 cycles, and about 6 seconds at 75,000 cycles. At higher cycle values, speed gains tend to be negligible on all host systems I've tried.

### To run the game:

Ensure you have the `COSMOx.STN` and `COSMOx.VOL` files for the episode you intend to play in the same directory as the generated `COSMOREx.EXE` file. `CD` to that directory if not already there, then run `COSMOREx`.

## Building a Release

There is a `build.bat` script that will compile all three episodes sequentially, and compress episodes 1 and 2 with LZEXE. (Episode 3 was not compressed before release, so Cosmore does not do it either.) To build the release, run:

    build

When the script finishes, there will be a new `DIST` directory containing the complete set of Cosmore binaries.

## Similarities and Differences

Cosmore is a meticulous reconstruction of _Cosmo_, but it is not perfect. Compared to the (UNLZEXE'd, where applicable) original:

### Similarities

* **No perceptible difference in gameplay.** As stated in the introduction, a player should not be able to distinguish a Cosmore binary from the original.
* **Fully compatible with original demo, save, and configuration files.** Any data file that works correctly with the original game will work correctly with Cosmore, and vice versa. All demo files should play perfectly, without any desynchronization.
* **Same file size.** The reconstructed executable takes the exact same amount of space on disk, and the same amount in memory, as the original. *(TODO: Not precisely. Episode 2, after LZEXE, is 64 bytes smaller than the original.)*
* **Same segment alignment and size.** The position of the C startup code, assembly routines, code segments, library functions, and data segments all match.
* **Same function alignment and size.** All of the functions appear in the same order as the original, each beginning and ending at the same addresses as the original.
* **Same instruction sequence.** All of the x86 instructions appear in the same order as the original. The CPU is performing the same steps in the same sequence, presumably taking the same number of clock cycles as the original.
* **Identical static data.** The entire static data area matches identically.

### Differences

* **Different arrangement of entries in the relocation table.** I have not yet determined what influences the arrangement of the relocation table entries in the EXE file header, so these tables do not match. They have the same length and content, just in different orders.
* **Different arrangement of data in BSS.** All of the variables in the BSS area (that is, variables that exist outside of any function which are not given an initial value in the source code) are out of order. This means that any instruction that references an affected memory address in this area will have different displacement bytes. The arrangement of these memory addresses appears to be influenced by the original variable names in the C source, none of which are definitively known. If there are any bugs in the original game that involve out-of-bounds memory access, Cosmore will likely behave differently.
* **Different file hashes.** Because the reconstruction is not byte-for-byte identical, the file hashes do not match the originals.

## Standing on the Shoulders of Giants

This project includes a few files that are not my own work, but which are required to build and package the final binaries. These files are:

* `src/C0.ASM`: This is the startup code for the Turbo C runtime library version 2.0, copyright 1988 by Borland International. This file is not needed to build the vast majority of projects under Turbo C, and (in situations where it is required) the compiler ships with its own copy for the developer's use. In Cosmore's case, there is a one-line change near the very bottom of this file to include the assembly code in `lowlevel.asm`, and there seemed to be no other obvious way to replicate the way the original game was built without embedding this file into the project.
* `src/LZEXE91/LZEXE.EXE`: This is LZEXE version 0.91, copyright 1989 by Fabrice Bellard. This is used by the build script to compress the final compiled binaries in the same manner as the original game. Also included is `LZEXE.DOC`, the documentation for the tool as originally released; and `lzexe.txt`, a duplicate copy of this documentation with the CP437 encoding translated to UTF-8. The related tools COMTOEXE, INFOEXE, and UPACKEXE are not used and therefore not included in this project. Note that the tool and its documentation are entirely in French.

## License

Except for the aforementioned files, MIT. All rights and trademarks to this game and its associated assets belongs to Apogee Software.
