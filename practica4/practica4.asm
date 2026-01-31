.386						;esto indica que se usa x86
.model flat, stdcall
.stack 4096

.data						;se declaran las variables abajo de esto

	count WORD 1

.code						;se ponen todas las instrucciones de nuestro programa, se indica donde empiza y donde termina
main PROC


	MOV ECX, 0				;se inicializa el contador en 0
	MOV CX, count			;se mueve el valor de count a CX

	RET
main ENDP
END main