.386						;esto indica que se usa x86
.model flat, stdcall
.stack 4096

.data						;se declaran las variables abajo de esto
	
	MyVar1 BYTE ?			;declaro variable
	MyVar2 WORD 23			;declaro variable e inicializo
	MyVar3 BYTE 7

.code						;se ponen todas las instrucciones de nuestro programa, se indica donde empiza y donde termina
main PROC

	MOV ECX, 5 				;Movemos inmediato a registro

	MOV MyVar, 6			;Movemos inmediato a variable
	MOV AL, MyVar			;Movemos variable a registro para mostrarla

	MOV AX, BX				;Direccionamiento directo registro a registro

	MOV BX, MyVar2			;Direccionamiento directo a memoria

	MOV ESI, OFFSET MyVar3	;Direccionamiento indirecto
	MOV AL, [ESI]			;Direccionamiento indirecto memoria a registro

	RET
main ENDP
END main