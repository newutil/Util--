_a	WS	1
_b	WS	1
_tc	WS	1
.c	WS	1
_y	WS	1
_ar	WS	1
_main	PUSH	FP
	LD	FP,SP
	PUSH	G3
	PUSH	G4
	CALL	__stkChk
	LD	G3,#10
	LD	G4,#0
	LD	G0,#1
	PUSH	G0
	CALL	_arcx
	ADD	SP,#2
	LD	G4,G0
	LD	G0,G4
	POP	G4
	POP	G3
	POP	FP
	RET
