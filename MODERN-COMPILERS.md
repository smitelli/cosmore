# Modern Compilers

It is technically possible, with some modification, to get the code in this repository to compile to an object file using a modern C compiler. You might want to do this to gain access to the more robust diagnostic capabilities that compilers have gained in the last 35 years, or simply as an interesting challenge.

Such object files _will not_ link into an executable program, since modern computers don't have the same core I/O and system management routines that the DOS environment did. It certainly would not run either, since no modern computer has the graphics hardware that the game expects.

Still, the bulk of this code performs no such I/O, and should compile (and theoretically work) with suitable translation to modern libraries. This document is a rough guide on how to take that first step.

## Removing Borland/DOS-specific elements

Since there is really no replacement for any of these in modern systems, your best bet is to rip out the parts that have no current equivalent, and patch up the holes left behind with function prototypes.

1. Remove the `interrupt` keyword from the `KeyboardInterruptService()`, `ProfileCPUService()`, and `TimerInterruptService()` function declarations. Also remove it from the `InterruptFunction` typedef declaration.

2. Comment out or remove every line that begins with `asm`. Even if your compiler has a way of including inline assembly, it probably will not accept the 16-bit Intel instruction mnemonics used here.

3. Comment out or remove the `#include` lines for alloc.h, conio.h, dos.h, and mem.h.

4. In place of the lines removed in step 3, add the following code adapted from the Borland header files:

    ```c
    #define C80 3
    #define MK_FP(seg, ofs) ((void *)(((unsigned long)(seg) << 16) | \
        (unsigned)(ofs)))
    #define random(num) (rand() % (num))

    enum COLORS {
        BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY, DARKGRAY,
        LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW, WHITE
    };

    union REGS {
        struct {unsigned int ax, bx, cx, dx, si, di, cflag, flags;} x;
        struct {unsigned char al, ah, bl, bh, cl, ch, dl, dh;} h;
    };

    unsigned long coreleft(void);
    void disable(void);
    void enable(void);
    long filelength(int handle);
    int getch(void);
    void (*getvect(int interruptno))(void);
    unsigned char inportb(int portid);
    int int86(int intno, union REGS *inregs, union REGS *outregs);
    void movmem(void *src, void *dest, unsigned length);
    void outport(int portid, int value);
    void outportb(int portid, unsigned char value);
    void setvect(int interruptno, void (*isr)(void));
    void textmode(int newmode);
    char *ultoa(unsigned long value, char *string, int radix);

    extern unsigned int _CX, _DX;
    ```

## Next steps

What's left should successfully compile to an object file (using e.g. the `-c` option on most compilers). Even with many warnings turned on (`-Wall`) there are not many significant problems with the code. Most of the warnings are either known and commented on, or are the direct result of the `asm` lines being removed.

At higher warning levels (`-Weverything`) much of the output becomes noise. Lots of changes in signedness and loss of integer precision during assignment operations. That's simply the way the original code was written, and does not cause issues in practice.
