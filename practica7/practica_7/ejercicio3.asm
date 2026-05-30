.386                    
.model flat, stdcall
.stack 4096
option casemap:none     

includelib kernel32.lib
GetStdHandle PROTO STDCALL :DWORD
WriteConsoleA PROTO STDCALL :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
ReadConsoleA PROTO STDCALL :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
ExitProcess PROTO STDCALL :DWORD

STD_INPUT_HANDLE  equ -10
STD_OUTPUT_HANDLE equ -11

LeerNumero     PROTO STDCALL 
ImprimirNumero PROTO STDCALL :SDWORD 

.data
    msgPedir     db "Ingresa un numero: ", 0
    msgRespuesta db "El valor del numero es: ", 0
    saltoLinea   db 13, 10, 0
    bufferTemp   db 32 dup(0)
    inputBuffer  db 32 dup(0)
    
    hStdIn  dd 0
    hStdOut dd 0
    rwCount dd 0 

.code
main PROC
    push STD_INPUT_HANDLE
    call GetStdHandle
    mov hStdIn, eax

    push STD_OUTPUT_HANDLE
    call GetStdHandle
    mov hStdOut, eax

    push 0
    push offset rwCount
    push 19 ;longitud del mensaje
    push offset msgPedir
    push hStdOut
    call WriteConsoleA
    
    call LeerNumero
    mov ebx, eax

    push 0
    push offset rwCount
    push 20
    push offset msgRespuesta
    push hStdOut
    call WriteConsoleA

    push ebx
    call ImprimirNumero
    
    push 0
    push offset rwCount
    push 2
    push offset saltoLinea
    push hStdOut
    call WriteConsoleA

    push 0
    call ExitProcess
main ENDP

LeerNumero PROC STDCALL
    push ecx
    push edx
    push ebx
    push esi

    push 0
    push offset rwCount
    push 30
    push offset inputBuffer
    push hStdIn
    call ReadConsoleA
    
    mov ecx, rwCount
    cmp ecx, 2
    jl _fin_lectura 
    mov byte ptr [inputBuffer + ecx - 2], 0 

    xor eax, eax
    xor ecx, ecx
    mov ebx, 10
    
    mov esi, 1
    cmp byte ptr [inputBuffer], '-'
    jne _ciclo
    
    mov esi, -1
    inc ecx

_ciclo:
    movzx edx, byte ptr [inputBuffer + ecx]
    
    cmp dl, 0
    je _fin_conversion

    sub dl, 48
    
    push edx
    mul ebx
    pop edx
    add eax, edx
    
    inc ecx
    jmp _ciclo

_fin_conversion:
    imul eax, esi

_fin_lectura:
    pop esi
    pop ebx
    pop edx
    pop ecx
    ret
LeerNumero ENDP

ImprimirNumero PROC STDCALL, valor:SDWORD
    push eax
    push ebx
    push ecx
    push edx

    mov eax, valor
    
    cmp eax, 0
    jge _es_positivo
    
    push eax
    
    mov byte ptr [bufferTemp], '-'
    push 0
    push offset rwCount
    push 1
    push offset bufferTemp
    push hStdOut
    call WriteConsoleA
    
    pop eax
    neg eax

_es_positivo:
    mov ebx, 10
    xor ecx, ecx

_loop_dividir:
    xor edx, edx
    div ebx
    
    add dl, 48
    push dx
    inc ecx
    
    test eax, eax
    jnz _loop_dividir

_loop_imprimir:
    pop dx
    mov byte ptr [bufferTemp], dl
    
    push ecx
    
    push 0
    push offset rwCount
    push 1
    push offset bufferTemp
    push hStdOut
    call WriteConsoleA
    
    pop ecx
    
    dec ecx
    cmp ecx, 0
    jg _loop_imprimir

    pop edx
    pop ecx
    pop ebx
    pop eax
    ret
ImprimirNumero ENDP

END main