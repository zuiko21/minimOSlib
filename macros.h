; minimOSlib 0.7a1 MACRO definitions
; adapted to generic 65xx firmware
; (c) 2012-2024 Carlos J. Santisteban
; last modified 20240704-1714

; **************************
; *** standard addresses ***
; **************************

; *** new BIOS/BDOS jumptable interface *** stub
b_gestalt	=	$FF00	; GESTALT, return machine type
b_conio		=	$FF03	; CONIO, non-locking console I/O
;b_xxxxx	=	$FF06

; *** classic minimOS addresses *** TBD
kerncall	=	$FFC0	; ending in RTS/RTI, 816 will use COP handler and a COP,RTS wrapper for 02
;adm_call	=	$FFDA	; ending in RTS, intended for kernel/drivers ONLY, revamped address
;adm_appc	=	$FFD0	; special interface for 65816 firmware call from USER software!
rom_id		=	$FFD6	; new "DmOS" (Durango/36 pin) or "VmOS" (Veracruz/44 pin) cartridge ID
rom_switch	=	$FFE1	; new location of JMP ($FFFC) as standard for shadow-RAM systems
signature	=	$FFEC	; Fletcher-16 checksum ** new address, supposedly 816-savvy
cpu_lock	=	$FFF0	; more-or-less 816 savvy address ** new label AND address!
brk_02		=	$FFF6	; supposedly emulated BRK, may be 816-savvy!

; new NMOS definition
nmos_ii		=	$E2		; pointer for JMPX macro

; *************************************
; *** considerations for minimOS·16 ***
; *************************************
; kernel return is via RTI (note CLC trick)
; kernel functions are expected to be in bank zero! At least the entry points
; 6502 apps CANNOT work bank-agnostic, bank zero only
; driver routines are expected to be in bank zero too... standard JSR/RTS, must return to kernel!

; *** common function calls ***

; system calling interface *** unified ·65 and ·16 macros

#ifndef	C816
#define		_KERNEL(a)		LDX #(a): JSR kerncall
#endif
#else
; new COP signature as per WDC reccomendations, CLC cannot be into firmware!
#define		_KERNEL(a)		LDX #(a): CLC: COP #$7F
#endif

; * C816 API functions ending in RTI and redefined EXIT_OK and ERR endings! note pre-CLC

; administrative calls unified for 6502 and 65816, all ending in RTS (use DR_OK and DR_ERR macros)
; * deprecated, may turn into standard BIOS/BDOS calls
#define		_ADMIN(a)		LDX #(a): JSR adm_call

; specific user-mode firmware call, needed for 65816
; future versions may limit app-access to admininstrative kernel
#ifdef	C816
#define		_U_ADM(a)		LDX #(a): JSL adm_appc
#else
#define		_U_ADM(a)		_ADMIN(a)
#endif

; *** function endings ***
; new FINISH and ABORT macros for app exit to shell, using RTL for 65816
; firmware interface can no longer use EXIT_OK and ERR, now reserved for the kernel API

#ifndef	C816
#define		_FINISH		CLC: RTS
#define		_EXIT_OK	CLC: RTS
#define		_ABORT(a)	LDY #(a): SEC: RTS
#define		_ERR(a)		LDY #(a): SEC: RTS
#else
#define		_FINISH		CLC: RTL
#define		_EXIT_OK	RTI
#define		_ABORT(a)	LDY #(a): SEC: RTL
#define		_ERR(a)		LDY #(a): PLP: SEC: PHP: RTI
#endif

; most code endings (except kernel API and apps) for both 6502 and 816 (expected to be in bank zero anyway)
; such code without error signaling (eg. shutdown, jiffy interrupt) may just end on RTS no matter the CPU
#define		_DR_OK		CLC: RTS
#define		_DR_ERR(a)	LDY #(a): SEC: RTS

; new exit for asynchronous driver routines when not satisfied
#define		_NXT_ISR	SEC: RTS
#define		_ISR_OK		CLC: RTS
; can no longer use EXIT_OK because of 65816 reimplementation!!! check drivers!

; new macros for critical sections, do not just rely on SEI/CLI
#define		_CRITIC		PHP: SEI
#define		_NO_CRIT	PLP

; ** interrupt enable/disable macros deprecated 20161003 and replaced by the above macros **

; ** panic call, now using BRK in case of error display ** new BRK handled 20161010
#define		_PANIC(a)	BRK: .asc a, 0

; *****************************
; *** usual ASCII constants ***
; *****************************
; * newer, minimOS names *
#define		BELL		7
#define		BKSP		8
; renamed TAB for 6800 compatibility!
#define		HTAB		9
#define		DOWN		10
#define		CLRS		12
#define		NEWL		13
#define		DLE			16
#define		ESC			27
; these may give trouble on xa65
#define		COLON		58
#define		CARET		94
; * older ASCII names for compatibility *
#define		BEL			7
#define		BS			8
#define		LF			10
#define		FORMFEED	12
#define		CR			13

; *** GESTALT machine types *** stub
#define		GT_NONE		0
#define		GT_SDD		1
#define		GT_CHIPLUS	2
#define		GT_DURANGO	3
#define		GT_VERACRUZ	4

; *** conditional opcode assembly ***
#ifdef	NMOS
; in case of a NMOS-binary is executed on a 65816 machine, use this faster version
; now keeps A, should be compatible with everything, albeit slower...
#define		_JMPX(a)	PHA: LDA a+1, X: STA nmos_ii+1: LDA a, X: STA nmos_ii: PLA: JMP (nmos_ii)
#define		_JMPXA(a)	LDA a+1, X: STA nmos_ii+1: LDA a, X: STA nmos_ii: JMP (nmos_ii)

#define		_PHX		TXA: PHA
#define		_PHY		TYA: PHA
#define		_PLX		PLA: TAX
#define		_PLY		PLA: TAY
#define		_STAX(a)	LDX #0: STA (a, X)
; STAY/LDAY macros new 20150225 for faster NMOS execution
#define		_STAY(a)	LDY #0: STA (a), Y
#define		_LDAX(a)	LDX #0: LDA (a, X)
#define		_LDAY(a)	LDY #0: LDA (a), Y
#define		_INC		CLC: ADC #1
#define		_DEC		SEC: SBC #1
#define		_BRA		JMP
; faster than CLC-BCC and the very same size, and leaves status intact, but no longer position-independent
#define		_STZX		LDX #0: STX
#define		_STZY		LDY #0: STY
#define		_STZA		LDA #0: STA
; new instructions implemented from www.6502.org/tutorials/65c02opcodes.html
; ...but they are not ATOMIC!!!
#define		_TRB(a)		BIT a: PHP: PHA: EOR #$FF: AND a: STA a: PLA: PHP
#define		_TSB(a)		BIT a: PHP: PHA: ORA a: STA a: PLA: PLP
#else
; standard CMOS opcodes
; note JMP workaround for xa bug outside bank 0!!!
#define		_JMPX(a)	JMP (a & $FFFF, X)
#define		_JMPXA(a)	JMP (a & $FFFF, X)
#define		_PHX		PHX
#define		_PHY		PHY
#define		_PLX		PLX
#define		_PLY		PLY
#define		_STAX(a)	STA (a)
#define		_STAY(a)	STA (a)
#define		_LDAX(a)	LDA (a)
#define		_LDAY(a)	LDA (a)
#define		_INC		INC
#define		_DEC		DEC
#define		_BRA		BRA
#define		_STZX		STZ
#define		_STZY		STZ
#define		_STZA		STZ
#define		_TRB(a)		TRB a
#define		_TSB(a)		TSB a
#endif

; *** include here the syntax conversion of RMB/SMB/BBR/BBS for xa65 ***
#define		RMB0	RMB #0,
#define		RMB1	RMB #1,
#define		RMB2	RMB #2,
#define 	RMB3	RMB #3,
#define		RMB4	RMB #4,
#define		RMB5	RMB #5,
#define		RMB6	RMB #6,
#define		RMB7	RMB #7,
#define		SMB0	SMB #0,
#define		SMB1	SMB #1,
#define		SMB2	SMB #2,
#define		SMB3	SMB #3,
#define		SMB4	SMB #4,
#define		SMB5	SMB #5,
#define		SMB6	SMB #6,
#define		SMB7	SMB #7,
#define		BBR0	BBR #0,
#define		BBR1	BBR #1,
#define		BBR2	BBR #2,
#define		BBR3	BBR #3,
#define		BBR4	BBR #4,
#define		BBR5	BBR #5,
#define		BBR6	BBR #6,
#define		BBR7	BBR #7,
#define		BBS0	BBS #0,
#define		BBS1	BBS #1,
#define		BBS2	BBS #2,
#define		BBS3	BBS #3,
#define		BBS4	BBS #4,
#define		BBS5	BBS #5,
#define		BBS6	BBS #6,
#define		BBS7	BBS #7,
