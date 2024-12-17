_arcn	PUSH	FP
	LD	FP,SP
	CALL	__stkChk
	CALL	_arcy
	LD	G0,#0
	POP	FP
	RET
