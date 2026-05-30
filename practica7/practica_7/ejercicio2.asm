.386                    
.model flat, stdcall
.stack 4096
option casemap:none     

includelib kernel32.lib
GetStdHandle PROTO STDCALL :DWORD
ReadConsoleA PROTO STDCALL :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
WriteConsoleA PROTO STDCALL :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
ExitProcess PROTO STDCALL :DWORD

STD_INPUT_HANDLE  equ -10
STD_OUTPUT_HANDLE equ -11

LeerCadena PROTO STDCALL :DWORD

.data
    msgPeticion  db "Actividad 2 - Ingresa un texto: ", 0
    msgConfirmar db "Texto capturado y limpiado: ", 0
    
    buffer db 131 dup(?) 
    
    hStdIn  dd 0
    hStdOut dd 0
    leidos  dd 0
    escritos dd 0

.code
main PROC
    push STD_INPUT_HANDLE
    call GetStdHandle
    mov hStdIn, eax

    push STD_OUTPUT_HANDLE
    call GetStdHandle
    mov hStdOut, eax

    push 0
    push offset escritos
    push 32 ; Longitud de "Actividad 2 - Ingresa un texto: "
    push offset msgPeticion
    push hStdOut
    call WriteConsoleA

    push offset buffer
    call LeerCadena

    push 0
    push offset escritos
    push 28 ; Longitud de "Texto capturado y limpiado: "
    push offset msgConfirmar
    push hStdOut
    call WriteConsoleA

    mov edx, offset buffer
    mov ecx, 0
    calculo_rapido:
        cmp byte ptr [edx], 0
        je fin_calculo
        inc ecx
        inc edx
        jmp calculo_rapido
    fin_calculo:

    push 0
    push offset escritos
    push ecx           
    push offset buffer 
    push hStdOut
    call WriteConsoleA

    push 0
    call ExitProcess
main ENDP


LeerCadena PROC STDCALL, pBuffer:DWORD
    push eax
    push ecx
    push edx

    push 0              
    push offset leidos 
    push 130            
    push pBuffer        
    push hStdIn        
    call ReadConsoleA
    
    mov eax, leidos   
    cmp eax, 2
    jl fin_funcion

    mov edx, pBuffer
    add edx, eax
    sub edx, 2

    cmp byte ptr [edx], 0Dh
    jne no_crlf
    cmp byte ptr [edx+1], 0Ah
    jne no_crlf
    mov byte ptr [edx], 0
    jmp fin_funcion

no_crlf:
    mov edx, pBuffer
    add edx, eax
    mov byte ptr [edx], 0

fin_funcion:
    pop edx
    pop ecx
    pop eax
    ret
LeerCadena ENDP

END main