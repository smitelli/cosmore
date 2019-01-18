# Abbreviated Summary of Turbo C Command-Line Options

Information for Borland tools is somewhat scarce, considering they were originally documented in paper books that predate the World Wide Web. This file contains a summary of the options used by Cosmore, with excerpts reprinted from the original manual to give more in-depth explanations.

## TCC Command-Line Options (Turbo C Reference Guide, Appendix C)

### Memory Model

* -mc: Compile using compact memory model.
* -mh: Compile using huge memory model.
* -ml: Compile using large memory model.
* -mm: Compile using medium memory model.
* -ms: Compile using small memory model (the default).
* -mt: Compile using tiny memory model. Generates almost the same code as the small memory model, but uses C0T.OBJ in any link performed to produce a tiny model program.

### #defines

* -D{xxx}: Defines the named identifier _xxx_ to the string consisting of the single space character.
* -D{xxx}={string}: Defines the named identifier _xxx_ to the string _string_ after the equal sign. _string_ cannot contain any spaces or tabs.
* -U{xxx}: Undefines any previous definitions of the named identifier _xxx_.

Turbo C allows you to make multiple `#define` entries on the command line in any of the following ways:

* You can include multiple entries after a  single `-D` option, separating entries with a semicolon (this is known as "ganging" options): `tcc -Dxxx;yyy=1;zzz=NO myfile.c`
* You can place more than one `-D` option on the command line: `tcc -Dxxx -Dyyy=1 -Dzzz=NO myfile.c`
* You can mix ganged and multiple `-D` listings: `tcc -Dxxx -Dyyy=1;zzz=NO myfile.c`

### Code Generation Options

* -1: Causes Turbo C to generate extended 80186 instructions. This option is also used to generate 80286 programs running in the real mode, such as with the IBM PC/AT under DOS. -1- turns this off.
* -a: Forces integer size items to be aligned on a machine-word boundary. Extra bytes will be inserted in a structure to ensure member alignment. Automatic and global variables will be aligned properly. **char** and **unsigned char** variables and fields may be placed at any address; all others must be placed at an even numbered address.
* -d: Merges literal strings when one string matches another; this produces smaller programs. -d- turns this off.
* -f-: Specifies that the program contains no floating-point calculations, so no floating-point libraries will be linked at the link step.
* -K: Causes the compiler to treat all **char** declarations as if they were **unsigned char** type. This allows for compatibility with other compilers that treat **char** declarations as unsigned. By default (-K-), **char** declarations are signed.

### Optimization Options

* -O: Optimizes by eliminating redundant jumps, and reorganizing loops and switch statements.
* -Z: Suppresses redundant load operations by remembering the contents of registers and reusing them as often as possible. [There is a cautionary note in the manual that is worth taking a look at.]

### Error-Reporting Options

-w{xxx}: Enables the warning message indicated by _xxx_. The option -w-_xxx_ suppresses the warning message indicated by _xxx_. The possible values for -w{xxx} are as follows:

#### ANSI Violations

* big: Hexadecimal or octal constant too large.
* dup: Redefinition of 'XXXXXXXX' is not identical.
* ret: Both return and return of a value used.
* str: 'XXXXXXXX' not part of structure.
* stu: Undefined structure 'XXXXXXXX'.
* sus: Suspicious pointer conversion.
* voi: Void functions may not return a value.
* zst: Zero length structure.

#### Common Errors

* aus: 'XXXXXXXX' is assigned a value that is never used.
* def: Possible use of 'XXXXXXXX' before definition.
* eff: Code has no effect.
* par: Parameter 'XXXXXXXX' is never used.
* pia: Possibly incorrect assignment.
* rch: Unreachable code.
* rvl: Function should return a value.

#### Less Common Errors

* amb: Ambiguous operators need parentheses.
* amp: Superfluous `&` with function or array.
* nod: No declaration for function 'XXXXXXXX'.
* pro: Call to function with no prototype.
* stv: Structure passed by value.
* use: 'XXXXXXXX' declared but never used.

#### Portability Warnings

* apt: Non-portable pointer assignment.
* cln: Constant is long.
* cpt: Non-portable pointer comparison.
* rng: Constant out of range in comparison.
* rpt: Non-portable return type conversion.
* sig: Conversion may lose significant digits.
* ucp: Mixing pointers to `signed` and `unsigned char`.

### Compilation Control Options

* -c: Compiles and assembles the named .C and .ASM files, but does not execute a link command.

### Environment Options

* -I{directory}: Searches `directory`, the drive specifier or path name of a subdirectory, for include files (in addition to searching the standard places). A drive specifier is a single letter, either uppercase or lowercase, followed by a colon (:). A directory is any valid path name of a directory file. Multiple `-I` directory options can be given.

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
