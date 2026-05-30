.386
.model flat, stdcall
.stack 4096
option casemap:none

includelib kernel32.lib
GetStdHandle PROTO STDCALL :DWORD
WriteConsoleA PROTO STDCALL :DWORD, :DWORD, :DWORD, :DWORD, :DWORD

STD_OUTPUT_HANDLE equ -11

ImprimirCadena PROTO STDCALL :DWORD

.data

	mensaje1 DB "Esta es la primera cadena. Es corta.", 13, 10, 0
	mensaje2 DB "Esta es una segunda cadena, que como puedes ver, es mucho mas larga que la anterior.", 13, 10, 0
	mensaje3 DB "Adios!", 13, 10, 0

	hStdOut dd 0
	caracteresEscritos dd 0

.code
main PROC

	PUSH STD_OUTPUT_HANDLE
    CALL GetStdHandle
    MOV hStdOut, EAX
	PUSH OFFSET mensaje1
    CALL ImprimirCadena

    PUSH OFFSET mensaje2
    CALL ImprimirCadena

    PUSH OFFSET mensaje3
    CALL ImprimirCadena

	RET
main ENDP

ImprimirCadena PROC STDCALL, pCadena:DWORD
    
    push ecx
    push edx
    
    mov edx, pCadena
    mov ecx, 0

_loop_calcular_longitud:
    cmp byte ptr [edx], 0
    je _fin_loop

    inc ecx
    inc edx
    jmp _loop_calcular_longitud

_fin_loop:

    push 0                    
    push offset caracteresEscritos
    push ecx
    push pCadena
    push hStdOut
    
    call WriteConsoleA

    pop edx
    pop ecx

    RET 

ImprimirCadena ENDP

END main