.386
.model flat, stdcall
.stack 4096

.data
    contador  WORD 5
    direccion WORD 1      ; 1=subir, 0=bajar
    ultimo    WORD 0
    resultado WORD 0
    numero    WORD 7

.code
main PROC

COMMENT !
    Ejercicio 2
!

    ; --- JE / JZ  (Salta si igual | ZR=1) ---
    MOV AX, 5
    CMP AX, 5           ; iguales
    JE  je_si           ; SI SALTA
je_si:
    MOV AX, 5
    CMP AX, 10          ; distintos
    JZ  jz_no           ; NO SALTA
jz_no:

    ; --- JNE / JNZ  (Salta si no igual | ZR=0) ---
    MOV AX, 5
    CMP AX, 10          ; ZR=0
    JNE jne_si          ; SI SALTA
jne_si:
    MOV AX, 5
    CMP AX, 5           ; ZR=1
    JNZ jnz_no          ; NO SALTA
jnz_no:

    ; --- JS  (Salta si negativo | PL=1) ---
    MOV AX, -5
    CMP AX, 0           ; PL=1  (-5 - 0 = -5)
    JS  js_si           ; SI SALTA
js_si:
    MOV AX, 5
    CMP AX, 0           ; PL=0
    JS  js_no           ; NO SALTA
js_no:

    ; --- JNS  (Salta si no negativo | PL=0) ---
    MOV AX, 5
    CMP AX, 0           ; PL=0
    JNS jns_si          ; SI SALTA
jns_si:
    MOV AX, -5
    CMP AX, 0           ; PL=1
    JNS jns_no          ; NO SALTA
jns_no:

    ; --- JP / JPE  (Salta si paridad par | PE=1) ---
    MOV AL, 3           ; 00000011b
    TEST AL, AL
    JP  jp_si           ; SI SALTA
jp_si:
    MOV AL, 7           ; 00000111b
    TEST AL, AL
    JPE jpe_no          ; NO SALTA
jpe_no:

    ; --- JNP / JPO  (Salta si paridad impar | PE=0) ---
    MOV AL, 7           ; 00000111b
    TEST AL, AL
    JNP jnp_si          ; SI SALTA
jnp_si:
    MOV AL, 3           ; 00000011b
    TEST AL, AL
    JPO jpo_no          ; NO SALTA
jpo_no:

    ; --- JO  (Salta si overflow | OV=1) ---
    MOV AX, 32767       ; 0x7FFF: máximo positivo en 16 bits con signo
    ADD AX, 1           ; 32767+1 = 32768 → overflow → OV=1
    JO  jo_si           ; SI SALTA
jo_si:
    MOV AL, 50
    ADD AL, 10          ; 50+10 = 60, sin overflow → OV=0
    JO  jo_no           ; NO SALTA
jo_no:

    ; --- JNO  (Salta si no hay overflow | OV=0) ---
    MOV AL, 50
    ADD AL, 10          ; OV=0
    JNO jno_si          ; SI SALTA
jno_si:
    MOV AL, 127         ; 0x7F: máximo positivo en 8 bits con signo
    ADD AL, 1           ; 127+1 = 128 → overflow → OV=1
    JNO jno_no          ; NO SALTA
jno_no:

    ; === Saltos SIN SIGNO (unsigned) ===

    ; --- JB / JNAE  (Salta si abajo | CY=1) ---
    MOV AX, 3
    CMP AX, 5           ; 3 < 5 → CY=1
    JB  jb_si           ; SI SALTA
jb_si:
    MOV AX, 5
    CMP AX, 3           ; 5 >= 3 → CY=0
    JNAE jnae_no        ; NO SALTA
jnae_no:

    ; --- JNB / JAE  (Salta si no abajo | CY=0) ---
    MOV AX, 5
    CMP AX, 3           ; CY=0
    JNB jnb_si          ; SI SALTA
jnb_si:
    MOV AX, 3
    CMP AX, 5           ; CY=1
    JAE jae_no          ; NO SALTA
jae_no:

    ; --- JBE / JNA  (Salta si abajo o igual | CY=1 o ZR=1) ---
    MOV AX, 5
    CMP AX, 5           ; ZR=1
    JBE jbe_si          ; SI SALTA
jbe_si:
    MOV AX, 10
    CMP AX, 5           ; CY=0, ZR=0
    JNA jna_no          ; NO SALTA
jna_no:

    ; --- JNBE / JA  (Salta si no abajo ni igual | CY=0 y ZR=0) ---
    MOV AX, 10
    CMP AX, 5           ; CY=0, ZR=0
    JNBE jnbe_si        ; SI SALTA
jnbe_si:
    MOV AX, 5
    CMP AX, 5           ; ZR=1
    JA  ja_no           ; NO SALTA
ja_no:

    ; === Saltos CON SIGNO (signed) ===

    ; --- JL / JNGE  (Salta si menor | PL!=OV) ---
    MOV AX, -5
    CMP AX, 3           ; -5 < 3 → PL!=OV
    JL  jl_si           ; SI SALTA
jl_si:
    MOV AX, 5
    CMP AX, 3           ; 5 >= 3 → PL=OV
    JNGE jnge_no        ; NO SALTA
jnge_no:

    ; --- JNL / JGE  (Salta si no menor | PL=OV) ---
    MOV AX, 5
    CMP AX, 3           ; PL=OV
    JNL jnl_si          ; SI SALTA
jnl_si:
    MOV AX, -5
    CMP AX, 3           ; PL!=OV
    JGE jge_no          ; NO SALTA
jge_no:

    ; --- JLE / JNG  (Salta si menor o igual | ZR=1 o PL!=OV) ---
    MOV AX, -5
    CMP AX, 3           ; PL!=OV
    JLE jle_si          ; SI SALTA
jle_si:
    MOV AX, 10
    CMP AX, 3           ; ZR=0, PL=OV
    JNG jng_no          ; NO SALTA
jng_no:

    ; --- JNLE / JG  (Salta si no menor ni igual | ZR=0 y PL=OV) ---
    MOV AX, 10
    CMP AX, 3           ; ZR=0, PL=OV
    JNLE jnle_si        ; SI SALTA
jnle_si:
    MOV AX, -5
    CMP AX, 3           ; PL!=OV
    JG  jg_no           ; NO SALTA
jg_no:

COMMENT !
    Ejercicio 4
!

    MOV AX, numero      ; cargar número
    SUB DX, DX          ; DX=0 → DX:AX sin signo
    MOV BX, 2
    DIV BX              ; cociente→AX, resto→DX
    CMP DX, 0
    JNE impar

par:
    MOV CX, 1           ; 1 = par
    JMP guardar

impar:
    MOV CX, 0           ; 0 = impar

guardar:
    MOV resultado, CX

COMMENT !
    Ejercicio 3
!

ciclo:
    CMP direccion, 1
    JNE bajar

subir:
    ADD contador, 1
    CMP contador, 10
    JNE mostrar
    MOV direccion, 0
    JMP mostrar

bajar:
    SUB contador, 1
    CMP contador, 5
    JNE mostrar
    MOV direccion, 1

mostrar:
    MOV AX, contador
    MOV ultimo, AX
    JMP ciclo           ; ciclo infinito

    RET
main ENDP
END main
