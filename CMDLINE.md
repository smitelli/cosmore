# Abbreviated Summary of Turbo C Command-Line Options

Information for Borland tools is somewhat scarce, considering they were originally documented in paper books that predate the World Wide Web. This file contains a summary of the options used by Cosmore, with excerpts reprinted from the original manual to give more in-depth explanations.

## TCC Command-Line Options (Turbo C Reference Guide, Appendix C)

### Memory Model

* **-mc:** Compile using compact memory model.
* **-mh:** Compile using huge memory model.
* **-ml:** Compile using large memory model.
* **-mm:** Compile using medium memory model.
* **-ms:** Compile using small memory model (the default).
* **-mt:** Compile using tiny memory model. Generates almost the same code as the small memory model, but uses C0T.OBJ in any link performed to produce a tiny model program.

### #defines

* **-D{xxx}:** Defines the named identifier _xxx_ to the string consisting of the single space character.
* **-D{xxx}={string}:** Defines the named identifier _xxx_ to the string _string_ after the equal sign. _string_ cannot contain any spaces or tabs.
* **-U{xxx}:** Undefines any previous definitions of the named identifier _xxx_.

Turbo C allows you to make multiple `#define` entries on the command line in any of the following ways:

* You can include multiple entries after a  single `-D` option, separating entries with a semicolon (this is known as "ganging" options):

    `tcc -Dxxx;yyy=1;zzz=NO myfile.c`
* You can place more than one `-D` option on the command line:

    `tcc -Dxxx -Dyyy=1 -Dzzz=NO myfile.c`
* You can mix ganged and multiple `-D` listings:

    `tcc -Dxxx -Dyyy=1;zzz=NO myfile.c`

### Code Generation Options

* **-1:** Causes Turbo C to generate extended 80186 instructions. This option is also used to generate 80286 programs running in the real mode, such as with the IBM PC/AT under DOS. -1- turns this off.
* **-a:** Forces integer size items to be aligned on a machine-word boundary. Extra bytes will be inserted in a structure to ensure member alignment. Automatic and global variables will be aligned properly. **char** and **unsigned char** variables and fields may be placed at any address; all others must be placed at an even numbered address.
* **-d:** Merges literal strings when one string matches another; this produces smaller programs. -d- turns this off.
* **-f-:** Specifies that the program contains no floating-point calculations, so no floating-point libraries will be linked at the link step.
* **-K:** Causes the compiler to treat all **char** declarations as if they were **unsigned char** type. This allows for compatibility with other compilers that treat **char** declarations as unsigned. By default (-K-), **char** declarations are signed.

### Optimization Options

* **-O:** Optimizes by eliminating redundant jumps, and reorganizing loops and switch statements.
* **-Z:** Suppresses redundant load operations by remembering the contents of registers and reusing them as often as possible.

    **Note:** You should exercise caution when using this option, because the compiler cannot detect if a register has been modified/invalidated indirectly by a pointer.

    For example, if a variable _A_ is loaded into register DX, it is retained. If _A_ is later assigned a value, the value of DX is reset to indicate that its contents are no longer current. Unfortunately, if the value of _A_ is modified indirectly (by assigning through a pointer that points to _A_), Turbo C will not catch this and will continue to remember that DX contains the (now obsolete) value of _A_.

    The `-Z` optimization is designed to suppress register loads when the value being loaded is already in a register. This can eliminate whole instructions and also convert instructions from referring to memory locations to using registers instead.

    The following artificial sequence illustrates both the benefits and the drawbacks of this optimization, and demonstrates why you need to exercise caution when using `-Z`.

    ```
            C Code           Optimized Assembler
    ----------------------   -------------------
    func()
    {
        int A, *P, B;

        A = 4;               mov A,4
        ...
        B = A;               mov ax,A
                             mov B,ax
        P = &A;              lea bx,A
                             mov P,bx
        *P = B + 5;          mov dx,ax
                             add dx,5
                             mov [bx],dx
        printf("%d\n", A);   push ax
    }
    ```

    Note first that on the statement `*P = B + 5`, the code generated uses a move from AX to DX first. Without the `-Z` optimization, the move would be from _B_, generating a longer and slower instruction.

    Second, the assignment into _*P_ recognizes that _P_ is already in BX, so a move from _P_ to BX after the `add` instruction has been eliminated. These improvements are harmless and generally useful.

    The call to **printf**, however, is not correct. Turbo C sees that AX contains the value of _A_, and so pushes the contents of the register rather than the contents of the memory location. The **printf** will then display a value of 4 rather than the correct value of 9. The indirect assignment through _P_ has hidden the change to _A_.

    If the statement `*P = B + 5` had been written as `A = B + 5`, Turbo C would recognize a change in value.

    The contents of registers are forgotten whenever a function call is made or when a point is reached where a jump could go (such as a label, a case statement, or the beginning or end of a loop). Because of this limit and the small number of registers in the 8086 family of processors, most programs using this optimization will never behave incorrectly.

### Error-Reporting Options

* **-w{xxx}:** Enables the warning message indicated by _xxx_. The option -w-{xxx} suppresses the warning message indicated by _xxx_. The possible values for -w{xxx} are as follows:

#### ANSI Violations

* **big:** Hexadecimal or octal constant too large.
* **dup:** Redefinition of 'XXXXXXXX' is not identical.
* **ret:** Both return and return of a value used.
* **str:** 'XXXXXXXX' not part of structure.
* **stu:** Undefined structure 'XXXXXXXX'.
* **sus:** Suspicious pointer conversion.
* **voi:** Void functions may not return a value.
* **zst:** Zero length structure.

#### Common Errors

* **aus:** 'XXXXXXXX' is assigned a value that is never used.
* **def:** Possible use of 'XXXXXXXX' before definition.
* **eff:** Code has no effect.
* **par:** Parameter 'XXXXXXXX' is never used.
* **pia:** Possibly incorrect assignment.
* **rch:** Unreachable code.
* **rvl:** Function should return a value.

#### Less Common Errors

* **amb:** Ambiguous operators need parentheses.
* **amp:** Superfluous `&` with function or array.
* **nod:** No declaration for function 'XXXXXXXX'.
* **pro:** Call to function with no prototype.
* **stv:** Structure passed by value.
* **use:** 'XXXXXXXX' declared but never used.

#### Portability Warnings

* **apt:** Non-portable pointer assignment.
* **cln:** Constant is long.
* **cpt:** Non-portable pointer comparison.
* **rng:** Constant out of range in comparison.
* **rpt:** Non-portable return type conversion.
* **sig:** Conversion may lose significant digits.
* **ucp:** Mixing pointers to `signed` and `unsigned char`.

### Compilation Control Options

* **-c:** Compiles and assembles the named .C and .ASM files, but does not execute a link command.

### Environment Options

* **-I{directory}:** Searches _directory_, the drive specifier or path name of a subdirectory, for include files (in addition to searching the standard places). A drive specifier is a single letter, either uppercase or lowercase, followed by a colon (:). A directory is any valid path name of a directory file. Multiple `-I` directory options can be given.

## TASM Options

* /d{symbol} or /d{symbol}={value}: Define symbol _symbol_ = 0 in the first case, or _symbol_ = _value_ in the second.
* /i{path}: Search _path_ for include files.
* /ml: Case sensitivity on all symbols.
* /w2: Set warning level (warnings on).

### Error Codes

`/w+xxx` turns on warnings for `xxx`, and `/w-xxx` turns them off.

* ALN: Segment alignment
* ASS: Assumes segment is 16-bit
* BRK: Brackets needed
* ICG: Inefficient code generation
* LCO: Location counter overflow
* OPI: Open IF conditional
* OPP: Open procedure
* OPS: Open segment
* OVF: Arithmetic overflow
* PDC: Pass-dependent construction
* PQK: Assuming constant for [const] warning
* PRO: Write-to-memory in protected mode needs CS override
* RES: Reserved word warning
* TPI: Turbo Pascal illegal warning

## TLINK Options

* /c: The **/c** option forces the case to be significant in public and external symbols. For example, by default, TLINK regards _cloud_, _Cloud_, and _CLOUD_ as equal; the **/c** option makes them different.
* /d: TLINK will list all symbols duplicated in libraries, even if those symbols are not going to be used in the program.
* /s: The **/s** option creates a map file with segments, public symbols and the program start address just like the **/m** option, but also adds a detailed segment map.
