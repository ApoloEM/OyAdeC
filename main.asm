.386
.model flat, stdcall
.stack 4096

.data

	; Ejercicio 3
	MyVar1  BYTE  ?				; 00h
	MyVar2  WORD  23			; 17h
	MyVar3  BYTE  7				; 07h

	; Ejercicio 4
	val1    BYTE   14h			; 20d
	val2    SBYTE  -5			; FBh
	val3    WORD   64h			; 100d

	; Ejercicio 5
	Xval    SWORD 25			; 0019h
	Yval    SWORD 50			; 0032h
	Zval    SWORD 112			; 0070h
	Rval    SWORD ?				; variable para almacenar resultados

.code
main proc

COMMENT !
	EJERCICIO 3
!
	MOV ECX, 5					; inmediato -> registro

	MOV MyVar1, 6				; inmediato -> memoria
	MOV AL, MyVar1				; memoria -> registro

	MOV AX, BX					; directo registro -> registro

	MOV MyVar2, AX				; directo registro -> memoria
	MOV CX, MyVar2				; visualizacion (directo memoria -> registro)

	MOV ESI, OFFSET MyVar3		; indirecto: cargar direccion en ESI
	MOV AL, [ESI]				; indirecto: leer desde memoria

COMMENT !
	EJERCICIO 4
!
	; --- MOV, MOVZX, MOVSX, XCHG ---
	MOV   BX, val3			; BX = 0064h (100)
	MOVZX AX, val1			; AX = 0014h (20)
	MOVSX CX, val2			; CX = FFFBh (-5)
	XCHG  AX, BX			; AX = 0064h (100), BX = 0014h (20)

	; --- INC, DEC, ADD, SUB, NEG ---
	INC   BX				; BX = 0015h (21)
	DEC   AX				; AX = 0063h (99)
	ADD   AX, BX			; AX = 0078h (120)
	SUB   AX, BX			; AX = 0063h (99)
	NEG   CX				; CX = 0005h (5)

	; --- MUL AX * BX ---
	XOR   DX, DX			; DX = 0
	MUL   BX				; DX:AX = 0000:081Fh (99 * 21 = 2079)

	; --- IMUL val2 * BX ---
	MOVSX AX, val2			; AX = FFFBh (-5)
	IMUL  BX				; DX:AX = FFFF:FF97h (-5 * 21 = -105)

	; --- DIV: val3 / CX ---
	MOV   AX, val3			; AX = 0064h (100)
	XOR   DX, DX			; DX = 0
	DIV   CX				; AX = 0014h (20), DX = 0000h (100 / 5 = 20)

COMMENT !
	EJERCICIO 5
!
	; Xval = 25, Yval = 50, Zval = 112
	; Rval = -Xval - (Yval + Zval) = -25 - (50 + 112) = -25 - 162 = -187 = FF45h
	MOV AX, Xval				; AX = 0019h  (25)
	NEG AX						; AX = FFE7h  (-25)
	MOV CX, Yval				; CX = 0032h  (50)
	ADD CX, Zval				; CX = 00A2h  (162)
	SUB AX, CX					; AX = FF45h  (-187)
	MOV Rval, AX				; Rval = FF45h

	; Rval = (-Xval / Zval) * Yval = (-25 / 112) * 50 = 0 * 50 = 0000h
	MOV AX, Xval				; AX = 0019h  (25)
	NEG AX						; AX = FFE7h  (-25)
	MOV DX, 0FFFFh				; Se coloca DX en FFFFh para que el resultado de la división sea correcto (cociente = 0, residuo = -25)
	;CWD							; DX:AX = FFFF:FFE7h  (extension de signo automatica)
	MOV CX, Zval				; CX = 0070h  (112)
	IDIV CX						; AX = 0000h  (cociente), DX = FFE7h (residuo)
	IMUL Yval					; DX:AX = 0000h  (0 * 50)
	MOV Rval, AX				; Rval = 0000h

	; Rval = (Zval * Xval) / (Xval - Yval) = (112 * 25) / (25 - 50) = 2800 / (-25) = -112 = FF90h
	MOV AX, Zval				; AX = 0070h  (112)
	IMUL Xval					; DX:AX = 0000:0AF0h  (2800)
	MOV CX, Xval				; CX = 0019h  (25)
	SUB CX, Yval				; CX = FFE7h  (-25)
	IDIV CX						; AX = FF90h  (-112)
	MOV Rval, AX				; Rval = FF90h

COMMENT !
	EJERCICIO 6
!
	; Rval = !(10100111) + (11100010)
	MOV AL, 10100111b			; AL = A7h
	NOT AL						; AL = 58h
	MOV BL, 11100010b			; BL = E2h
	OR  AL, BL					; AL = FAh
	MOV Rval, AX				; Rval = 00FAh

	; Rval = !((11101101)(10101010)(10111101))
	MOV AL, 11101101b			; AL = EDh
	MOV BL, 10101010b			; BL = AAh
	MOV CL, 10111101b			; CL = BDh
	AND AL, BL					; AL = A8h
	AND AL, CL					; AL = A8h
	NOT AL						; AL = 57h
	MOV Rval, AX				; Rval = 0057h

	RET
main ENDP
END main
