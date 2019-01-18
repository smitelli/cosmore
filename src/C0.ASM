        NAME    c0
        PAGE    60,132
;[]------------------------------------------------------------[]
;|      C0.ASM -- Start Up Code                                 |
;|                                                              |
;|      Turbo-C Run Time Library        version 2.0             |
;|                                                              |
;|      Copyright (c) 1988 by Borland International Inc.        |
;|      All Rights Reserved.                                    |
;[]------------------------------------------------------------[]

        INCLUDE RULES.ASI

_Strict87_	equ	false		; emulation skips peculiar details

;       Segment and Group declarations

_TEXT   SEGMENT BYTE PUBLIC 'CODE'
_TEXT   ENDS
_DATA   SEGMENT PARA PUBLIC 'DATA'
_DATA   ENDS
IFNDEF  __NOFLOAT__
_EMUSEG SEGMENT WORD COMMON 'DATA'
_EMUSEG ENDS
ENDIF
_CRTSEG SEGMENT WORD COMMON 'DATA'
_CRTSEG ENDS
_CVTSEG SEGMENT WORD PUBLIC 'DATA'
_CVTSEG ENDS
_SCNSEG SEGMENT WORD PUBLIC 'DATA'
_SCNSEG ENDS
IFNDEF __HUGE__
_BSS    SEGMENT WORD PUBLIC 'BSS'
_BSS    ENDS
_BSSEND SEGMENT BYTE PUBLIC 'STACK'
_BSSEND ENDS
ENDIF
IFNDEF __TINY__
_STACK  SEGMENT STACK 'STACK'
_STACK  ENDS
ENDIF

IFNDEF  __NOFLOAT__

IF      LDATA
IFNDEF __HUGE__
DGROUP  GROUP   _DATA, _EMUSEG, _CRTSEG, _CVTSEG, _SCNSEG, _BSS, _BSSEND
ELSE
DGROUP  GROUP   _DATA, _EMUSEG, _CRTSEG, _CVTSEG, _SCNSEG
ENDIF
ELSE
IFNDEF  __TINY__
DGROUP  GROUP   _DATA, _EMUSEG, _CRTSEG, _CVTSEG, _SCNSEG, _BSS, _BSSEND
ELSE
DGROUP  GROUP   _TEXT, _DATA, _EMUSEG, _CRTSEG, _CVTSEG, _SCNSEG, _BSS, _BSSEND
ENDIF
ENDIF

ELSE

IF      LDATA
IFNDEF __HUGE__
DGROUP  GROUP   _DATA, _CRTSEG, _CVTSEG, _SCNSEG, _BSS, _BSSEND
ELSE
DGROUP  GROUP   _DATA, _CRTSEG, _CVTSEG, _SCNSEG
ENDIF
ELSE
IFNDEF  __TINY__
DGROUP  GROUP   _DATA, _CRTSEG, _CVTSEG, _SCNSEG, _BSS, _BSSEND
ELSE
DGROUP  GROUP   _TEXT, _DATA, _CRTSEG, _CVTSEG, _SCNSEG, _BSS, _BSSEND
ENDIF
ENDIF

ENDIF

        ASSUME  CS:_TEXT, DS:DGROUP

;       External References

ExtProc@        main,   __CDECL__
ExtProc@        _setargv, __CDECL__
ExtProc@        _setenvp, __CDECL__
ExtProc@        exit,   __CDECL__

IF      LDATA EQ false
ExtSym@ _heaplen, WORD,  __CDECL__
ENDIF
ExtSym@ _stklen,  WORD,  __CDECL__

        SUBTTL  Start Up Code
        PAGE
;/*                                                     */
;/*-----------------------------------------------------*/
;/*                                                     */
;/*     Start Up Code                                   */
;/*     -------------                                   */
;/*                                                     */
;/*-----------------------------------------------------*/
;/*                                                     */
PSPHigh         equ     00002h
PSPEnv          equ     0002ch
PSPCmd          equ     00080h

IFDEF	__NOFLOAT__
	MINSTACK	equ	128	; minimal stack size in words
ELSE
	MINSTACK	equ	256	; minimal stack size in words
ENDIF
;
;       At the start, DS and ES both point to the segment prefix.
;       SS points to the stack segment except in TINY model where
;       SS is equal to CS
;
_TEXT   SEGMENT
IFDEF   __TINY__
        ORG     100h
ENDIF
STARTX          PROC    NEAR
;       Save general information, such as :
;               DGROUP segment address
;               DOS version number
;               Program Segment Prefix address
;               Environment address
;               Top of far heap

IFDEF   __TINY__
                mov     dx, cs          ; DX = GROUP Segment address
ELSE
                mov     dx, DGROUP      ; DX = GROUP Segment address
ENDIF
                mov     cs:DGROUP@@, dx
                mov     ah, 30h
                int     21h
                mov     bp, ds:[PSPHigh]; BP = Highest Memory Segment Addr
                mov     bx, ds:[PSPEnv] ; BX = Environment Segment address
                mov     ds, dx
                mov     _version@, ax   ; Keep major and minor version number
                mov     _psp@, es       ; Keep Program Segment Prefix address
                mov     _envseg@, bx    ; Keep Environment Segment address
                mov     word ptr _heaptop@ + 2, bp
                mov     _8087@, -1
;
;       Save several vectors and install default divide by zero handler.
;
                call    SaveVectors

;       Look for a '87' environment variable, and use this loop to
;       count the number of environment variables and to compute the
;       environment size.
;       Each variable is ended by a 0 and a zero-length variable stops
;       the environment. The environment can NOT be greater than 32k.

                les     di, dword ptr _envLng@
                mov     ax, di
                mov     bx, ax
                mov     cx, 07FFFh      ; Environment cannot be > 32 Kbytes
IsIt87Var       label   near
                cmp     word ptr es:[di], '78'
                jne     GetVarLng
                mov     dx, es:[di+2]
                cmp     dl, '='
                jne     GetVarLng
                and     dh, not ' '
                inc     _8087@
                cmp     dh, 'Y'
                jne     GetVarLng
                inc     _8087@
GetVarLng       label   near
                repnz   scasb
                jcxz    InitFailed      ; Bad environment !!!
                inc     bx              ; BX = Nb environment variables
                cmp     es:[di], al
                jne     IsIt87Var       ; Next variable ...
                or      ch, 10000000b
                neg     cx
                mov     _envLng@, cx     ; Save Environment size
                mov     cx, dPtrSize / 2
                shl     bx, cl
                add     bx, dPtrSize * 4
                and     bx, not ((dPtrSize * 4) - 1)
                mov     _envSize@, bx   ; Save Environment Variables Nb.

;       Determine the amount of memory that we need to keep

IF      LDATA
                mov     dx, ss
                sub     bp, dx          ; BP = remaining size in paragraphs
IFDEF   __HUGE__
                mov     di, seg _stklen@
                mov     es, di
                mov     di, es:_stklen@ ; DI = Requested stack size
ELSE
                mov     di, _stklen@    ; DI = Requested stack size
ENDIF
;
; Make sure that the requested stack size is at least MINSTACK words.
;
                cmp     di, 2*MINSTACK  ; requested stack big enough ?
                jae     AskedStackOK
                mov     di, 2*MINSTACK  ; no --> use minimal value
IFDEF   __HUGE__
                mov     es:_stklen@, di ; override requested stack size
ELSE
                mov        _stklen@, di ; override requested stack size
ENDIF
AskedStackOK    label   near
                mov     cl, 4
                shr     di, cl          ; $$$ Do not destroy CL $$$
                inc     di              ; DI = Stack size in paragraphs
                cmp     bp, di
                jnb     ExcessOfMemory  ; Much more available than needed
ELSE
                mov     dx, ds
                sub     bp, dx          ; BP = remaining size in paragraphs
                mov     di, _stklen@    ; DI = Requested stack size
;
; Make sure that the requested stack size is at least MINSTACK words.
;
                cmp     di, 2*MINSTACK  ; requested stack big enough ?
                jae     AskedStackOK
                mov     di, 2*MINSTACK  ; no --> use minimal value
                mov     _stklen@, di    ; override requested stack size
AskedStackOK    label   near
                add     di, offset DGROUP: edata@
                jb      InitFailed      ; DATA segment can NOT be > 64 Kbytes
                add     di, _heaplen@
                jb      InitFailed      ; DATA segment can NOT be > 64 Kbytes
                mov     cl, 4
                shr     di, cl          ; $$$ Do not destroy CL $$$
                inc     di              ; DI = DS size in paragraphs
                cmp     bp, di
                jb      InitFailed      ; Not enough memory
                cmp     _stklen@, 0
                je      ExpandDS        ; Expand DS up to 64 Kb
                cmp     _heaplen@, 0
                jne     ExcessOfMemory  ; Much more available than needed
ExpandDS        label   near
                mov     di, 1000h
                cmp     bp, di
                ja      ExcessOfMemory  ; Enough to run the program
                mov     di, bp
                jmp     short ExcessOfMemory  ; Enough to run the program
ENDIF

;       All initialization errors arrive here

InitFailed      label   near
                jmp     near ptr abort@

;       Return to DOS the amount of memory in excess
;       Set far heap base and pointer

ExcessOfMemory  label   near
                mov     bx, di
                add     bx, dx
                mov     word ptr _heapbase@ + 2, bx
                mov     word ptr _brklvl@ + 2, bx
                mov     ax, _psp@
                sub     bx, ax          ; BX = Number of paragraphs to keep
                mov     es, ax          ; ES = Program Segment Prefix address
                mov     ah, 04Ah
                push    di              ; preserve DI
                int     021h            ; this call clobbers SI,DI,BP !!!!!!
                pop     di              ; restore  DI
;
;	Set the program stack.  Take care to prevent the disastrous
;	interrupt that could happen with a stack that is half switched.
;
                shl     di, cl          ; $$$ CX is still equal to 4 $$$

		cli
                mov     ss, dx
                mov     sp, di
		sti

IFNDEF  __HUGE__

;       Reset uninitialized data area

                xor     ax, ax
                mov     es, cs:DGROUP@@
                mov     di, offset DGROUP: bdata@
                mov     cx, offset DGROUP: edata@
                sub     cx, di
	        rep     stosb
ENDIF

IFNDEF  __NOFLOAT__

;       Install floating point software

                push    cs              ;Simulation of a FAR call
                call    ds:[__emu1st]
ENDIF

;       Prepare main arguments

                call    _setargv@
                call    _setenvp@

                mov	ah, 0
                int	1ah			; get current BIOS time in ticks
                mov	word ptr _StartTime@,dx	; save it for clock() fn
                mov	word ptr _StartTime@+2,cx

IFNDEF  __OLDCONIO__
IF      LPROG
                push    cs              ; Simulation of a FAR call
ENDIF
                call    ds:[__crt1st]   ; Initialize window sizes, etc.
ENDIF

;       ExitCode = main(argc,argv,envp);

IF      LDATA
                push    word ptr environ@+2
                push    word ptr environ@
                push    word ptr _argv@+2
                push    word ptr _argv@
ELSE
                push    word ptr environ@
                push    word ptr _argv@
ENDIF
                push    _argc@
                call    main@

;       Flush and close streams and files

                push    ax
                call    exit@

;---------------------------------------------------------------------------
;	_exit()
;
;       Restore interrupt vectors taken during startup.  signal() functions
;	could have grabbed vectors 0, 4, 5 or 6.
;
;	Check for NULL pointer errors.
;
;	Exit to DOS.
;
;NOTE : _exit() doesn't close any files or run exit functions.  This is a
;	minimal 'cleanup & quit' program exit.
;---------------------------------------------------------------------------
PubProc@        _exit,  __CDECL__
                mov     ds, cs:DGROUP@@

IF      LPROG
                call    far ptr _restorezero@	; restore captured INT vectors
ELSE
                call    near ptr _restorezero@
ENDIF

IFNDEF  __NOFLOAT__

;       Restore interrupt vectors taken by __emu1st

                push    cs              ;Simulation of a FAR call
                call    ds:[__emuLast]
ENDIF

IF      LDATA EQ false
IFNDEF  __TINY__

;       Check for null pointers before exit

                xor     ax, ax
                mov     si, ax
                mov     cx, lgth_CopyRight
                cld
ComputeChecksum label   near
                add     al, [si]
                adc     ah, 0
                inc     si
                loop    ComputeChecksum
                sub     ax, CheckSum
                jz      ExitToDOS
                mov     cx, lgth_NullCheck
                mov     dx, offset DGROUP: NullCheck
                call    ErrorDisplay
ENDIF
ENDIF

;       Exit to DOS

ExitToDOS       label   near
                mov     bp,sp
                mov     ah,4Ch
                mov     al,[bp+cPtrSize]
                int     21h                     ; Exit to DOS
EndProc@        _exit, __CDECL__
STARTX          ENDP

        SUBTTL  Vector save/restore & default Zero divide routines
        PAGE
;[]------------------------------------------------------------[]
;|                                                              |
;| Interrupt Save/Restore routines and default divide by zero   |
;| handler.                                                     |
;|                                                              |
;[]------------------------------------------------------------[]

ZeroDivision    PROC    FAR
                mov     cx, lgth_ZeroDivMSG
                mov     dx, offset DGROUP: ZeroDivMSG
                jmp     MsgExit3
ZeroDivision    ENDP

;--------------------------------------------------------------------------
;	savevectors()
;
;	Save vectors for 0, 4, 5 & 6 interrupts.  This is for extended
;	signal()/raise() support as the signal functions can steal these
;	vectors during runtime.
;--------------------------------------------------------------------------
SaveVectors	PROC    NEAR
                push    ds
; Save INT 0
                mov     ax, 3500h
                int     021h
                mov     word ptr _Int0Vector@, bx
                mov     word ptr _Int0Vector@+2, es
; Save INT 4
                mov     ax, 3504h
                int     021h
                mov     word ptr _Int4Vector@, bx
                mov     word ptr _Int4Vector@+2, es
; Save INT 5
                mov     ax, 3505h
                int     021h
                mov     word ptr _Int5Vector@, bx
                mov     word ptr _Int5Vector@+2, es
; Save INT 6
                mov     ax, 3506h
                int     021h
                mov     word ptr _Int6Vector@, bx
                mov     word ptr _Int6Vector@+2, es
;
;	Install default divide by zero handler.
;
                mov     ax, 2500h
                mov     dx, cs
                mov     ds, dx
                mov     dx, offset ZeroDivision
                int     21h

                pop     ds
                ret
SaveVectors	ENDP

;--------------------------------------------------------------------------
;	restorezero() puts back all the vectors that SaveVectors took.
;
;NOTE : TSRs must BE AWARE that signal() functions which take these 
;	vectors will be deactivated if the keep() function is executed.
;	If a TSR wants to use the signal functions when it is active it 
;	will have to save/restore these vectors itself when activated and
;	deactivated.
;--------------------------------------------------------------------------
PubProc@        _restorezero, __CDECL__

IFDEF   __HUGE__
                push    ds
                mov     ds, cs: DGROUP@@
ENDIF
                push    ds
                mov     ax, 2500h
                lds     dx, _Int0Vector@
                int     21h
		pop	ds

		push	ds
                mov     ax, 2504h
                lds     dx, _Int4Vector@
                int     21h
		pop	ds

		push	ds
                mov     ax, 2505h
                lds     dx, _Int5Vector@
                int     21h
		pop	ds

IFNDEF   __HUGE__
		push	ds
ENDIF
                mov     ax, 2506h
                lds     dx, _Int6Vector@
                int     21h
                pop     ds

                ret
EndProc@        _restorezero, __CDECL__

        SUBTTL  Miscellaneous
        PAGE
;[]------------------------------------------------------------[]
;|                                                              |
;|      Miscellaneous functions                                 |
;|                                                              |
;[]------------------------------------------------------------[]

IFNDEF  __NOFLOAT__
NoEmulator      PROC    FAR
                mov     _8087@, 0
                ret
NoEmulator      ENDP
ENDIF

IFNDEF  __OLDCONIO__
Proc@           NoConsole, __CDECL__
                ret
EndProc@        NoConsole, __CDECL__
ENDIF

ErrorDisplay    PROC    NEAR
                mov     ah, 040h
                mov     bx, 2
                int     021h
                ret
ErrorDisplay    ENDP

PubProc@        abort,  __CDECL__
                mov     cx, lgth_abortMSG
                mov     dx, offset DGROUP: abortMSG
MsgExit3        label   near
                mov     ds, cs: DGROUP@@
                call    ErrorDisplay
CallExit3       label   near
                mov     ax, 3
                push    ax
                call    _exit@          ; _exit(3);
EndProc@        abort, __CDECL__

;       The DGROUP@ variable is used to reload DS with DGROUP

PubSym@ DGROUP@, <dw    ?>, __PASCAL__
_TEXT   ENDS

        SUBTTL  Start Up Data Area
        PAGE
;[]------------------------------------------------------------[]
;|      Start Up Data Area                                      |
;|                                                              |
;|      WARNING         Do not move any variables in the data   |
;|                      segment unless you're absolutely sure   |
;|                      that it does not matter.                |
;|                                                              |
;[]------------------------------------------------------------[]

_DATA   SEGMENT

;       The CopyRight string must NOT be moved or changed without
;       changing the null pointer check logic

CopyRight       db      4 dup(0)
                db      'Turbo-C - Copyright (c) 1988 Borland Intl.',0
lgth_CopyRight  equ     $ - CopyRight

IF      LDATA EQ false
IFNDEF  __TINY__
CheckSum        equ     00D37h
NullCheck       db      'Null pointer assignment', 13, 10
lgth_NullCheck  equ     $ - NullCheck
ENDIF
ENDIF

ZeroDivMSG      db      'Divide error', 13, 10
lgth_ZeroDivMSG equ     $ - ZeroDivMSG

abortMSG        db      'Abnormal program termination', 13, 10
lgth_abortMSG   equ     $ - abortMSG

;
;			Interrupt vector save areas
;	
;	Interrupt vectors 0,4,5 & 6 are saved at startup and then restored
;	when the program terminates.  The signal/raise functions might
;	steal these vectors during execution.
;
PubSym@         _Int0Vector	<dd     0>,             __CDECL__
PubSym@         _Int4Vector	<dd     0>,             __CDECL__
PubSym@         _Int5Vector	<dd     0>,             __CDECL__
PubSym@         _Int6Vector	<dd     0>,             __CDECL__
;
;			Miscellaneous variables
;	
PubSym@         _argc,          <dw     0>,             __CDECL__
dPtrPub@        _argv,          0,                      __CDECL__
dPtrPub@        environ,        0,                      __CDECL__
PubSym@         _envLng,        <dw     0>,             __CDECL__
PubSym@         _envseg,        <dw    0>,              __CDECL__
PubSym@         _envSize,       <dw    0>,              __CDECL__
PubSym@         _psp,           <dw    0>,              __CDECL__
PubSym@         _version,       <label word>,           __CDECL__
PubSym@         _osmajor,       <db    0>,              __CDECL__
PubSym@         _osminor,       <db    0>,              __CDECL__
PubSym@         errno,          <dw    0>,              __CDECL__
PubSym@         _8087,          <dw    0>,              __CDECL__
PubSym@         _StartTime,     <dw    0,0>,            __CDECL__


;       Memory management variables

IF      LDATA EQ false
PubSym@         __heapbase,     <dw   DGROUP:edata@>,   __CDECL__
PubSym@         __brklvl,       <dw   DGROUP:edata@>,   __CDECL__
PubSym@         __heaptop,      <dw   DGROUP:edata@>,   __CDECL__
ENDIF
PubSym@         _heapbase,      <dd   0>,       __CDECL__
PubSym@         _brklvl,        <dd   0>,       __CDECL__
PubSym@         _heaptop,       <dd   0>,       __CDECL__

IF	LDATA EQ false
	IFNDEF	__NOFLOAT__
;				Emulator variables
		INCLUDE	emuvars.asi
	ENDIF
ENDIF

_DATA   ENDS

IFNDEF  __NOFLOAT__
_EMUSEG SEGMENT
__emu1st        dw      NoEmulator
__emuLast       dw      NoEmulator
_EMUSEG ENDS
ENDIF

IFNDEF  __OLDCONIO__
_CRTSEG SEGMENT
__crt1st        dw      NoConsole@
_CRTSEG ENDS
ENDIF

_CVTSEG SEGMENT
PubSym@ _RealCvtVector, <label  word>,  __CDECL__
_CVTSEG ENDS

_SCNSEG SEGMENT
PubSym@ _ScanTodVector,  <label word>,  __CDECL__
_SCNSEG ENDS

IFNDEF __HUGE__
_BSS    SEGMENT
bdata@  label   byte
_BSS    ENDS

_BSSEND SEGMENT
edata@  label   byte
_BSSEND ENDS
ENDIF

IFNDEF  __TINY__
_STACK  SEGMENT
        dw      64 dup (?)
	IF	LDATA
		org	0
		IFNDEF	__NOFLOAT__
;						Emulator variables
			INCLUDE	emuvars.asi
			even
		ENDIF
PUBLIC		emuTop@			; for use in stack-underflow checks.
		emuTop@	label	byte
	ENDIF
_STACK  ENDS
ENDIF

	; COSMORE: This is the sole change here. All other lines are untouched.
	INCLUDE lowlevel.asm

        END     STARTX
