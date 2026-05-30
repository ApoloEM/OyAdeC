.386
.model flat, stdcall
.stack 4096

.data

	contadorFOR DWORD 0
	contadorWHILE DWORD 5
	contadorDOWHILE DWORD 10
	unaCadena BYTE "HOLA"
	unArreglo DWORD 100, 5, 2, 4, 1, 3

.code
main proc

COMMENT !
	EJERCICIO 1
!

	MOV ECX, 5
	MOV EAX, 0

	SALTO1:
		INC EAX
		LOOP SALTO1

COMMENT !
	EJERCICIO 2
!

	MOV ECX, 5
	MOV EAX, 2

	SALTO2:
		ADD EAX, EAX
		CMP EAX, 32
		JG SALTO3
		LOOP SALTO2

	SALTO3:
		NOP

COMMENT !
	EJERCICIO 3
!

	;Ciclo FOR:
	MOV contadorFOR, 0

	;for(i=0; i<10; i++)
	CICLO_FOR:
        CMP contadorFOR, 10
        JGE FIN_FOR
        NOP
        INC contadorFOR
        JMP CICLO_FOR

    FIN_FOR:
        MOV EAX, contadorFOR

;Ciclo WHILE:
	MOV ECX, contadorWHILE
	MOV EAX, 5

	;while(contadorWHILE > 0)
	CICLO_WHILE:
		CMP ECX, 0
		JE FIN_WHILE
		DEC ECX
		ADD EAX, ECX
		JMP CICLO_WHILE

	FIN_WHILE:

;Ciclo DO WHILE:
	MOV ECX, contadorDOWHILE
	MOV EDX, 0

	;doWhile(contadorDOWHILE > 0)
	CICLO_DOWHILE:
		INC EDX
		DEC ECX
		CMP ECX, 0
		JG CICLO_DOWHILE

COMMENT !
	EJERCICIO 4
!

	MOV EAX, 1
	MOV ECX, 10

	FACTORIAL_10:
		MUL ECX
		LOOP FACTORIAL_10

COMMENT !
	EJERCICIO 6
!

	MOV   ECX, LENGTHOF unaCadena
	MOV   EBX, 0

	CICLO_ARREGLO:
		MOVZX EAX, BYTE PTR [unaCadena + EBX]
		INC   EBX
		LOOP CICLO_ARREGLO

COMMENT !
	EJERCICIO 7
!

	MOV   ECX, LENGTHOF unArreglo
	DEC   ECX
	
	CONTROL_EXTERNO:
		PUSH  ECX
		MOV   ESI, OFFSET unArreglo
		MOV   EDX, ECX

	CONTROL_INTERNO:
		MOV   EAX, [ESI]
		MOV   EBX, [ESI+4]
		CMP   EAX, EBX
		JLE   NO_CAMBIAR
		MOV   [ESI], EBX
		MOV   [ESI+4], EAX

	NO_CAMBIAR:
		ADD   ESI, 4
		DEC   EDX
		JNZ   CONTROL_INTERNO
		POP   ECX   
		LOOP  CONTROL_EXTERNO

	MOV EAX, [unArreglo]
	MOV EAX, [unArreglo + 4]
	MOV EAX, [unArreglo + 8]
	MOV EAX, [unArreglo + 12]
	MOV EAX, [unArreglo + 16]
	MOV EAX, [unArreglo + 20]

	RET
main ENDP
END main
