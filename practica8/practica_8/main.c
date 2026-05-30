#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

typedef struct {
    char titulo[50];
    char autor[50];
    int anio;
    int cantidad;
    int cantidad_total;
} Libro;

Libro biblioteca[100];
int totalLibros = 0;

void imprimir_libro_c(char* t, char* a, int an, int cant) {
    printf("Titulo: %s \n Autor: %s \n Anio: %d \n Disponibles: %d\n", t, a, an, cant);
    printf("-------------------------\n");
}

void agregar_libro(char* t, char* a, int an, int cant) {
    int resultado = 0;

    __asm {
        xor ebx, ebx
        mov ecx, totalLibros
        lea edx, biblioteca

        loop_verificar :
        cmp ebx, ecx
            jge es_nuevo

            push ecx
            mov esi, t
            mov edi, edx

            cmp_check :
        mov al, [esi]
            mov ah, [edi]
            cmp al, ah
            jne next_check
            test al, al
            jz encontrado_update
            inc esi
            inc edi
            jmp cmp_check

            next_check :
        pop ecx
            add edx, 112
            inc ebx
            jmp loop_verificar

            encontrado_update :
        pop ecx
            mov eax, cant
            add[edx + 104], eax
            add[edx + 108], eax
            mov resultado, 1
            jmp fin_agregar

            es_nuevo :
        mov eax, totalLibros
            imul eax, eax, 112
            lea edi, biblioteca[eax]

            mov esi, t
            xor ecx, ecx
            copy_titulo :
        mov al, [esi + ecx]
            mov[edi + ecx], al
            test al, al
            jz fin_titulo
            inc ecx
            jmp copy_titulo
            fin_titulo :

        add edi, 50
            mov esi, a
            xor ecx, ecx
            copy_autor :
        mov al, [esi + ecx]
            mov[edi + ecx], al
            test al, al
            jz fin_autor
            inc ecx
            jmp copy_autor
            fin_autor :

        add edi, 50
            mov eax, an
            mov[edi], eax

            add edi, 4
            mov eax, cant
            mov[edi], eax

            add edi, 4
            mov eax, cant
            mov[edi], eax

            inc totalLibros
            mov resultado, 0

            fin_agregar:
    }

    if (resultado == 0) {
        printf("Se agregó el libro.\n");
    }
    else {
        printf("El libro ya existía. Se sumó al stock.\n");
    }
}

void prestar_libro(char* t) {
    int encontrado = 0;
    __asm {
        xor ebx, ebx
        mov ecx, totalLibros
        lea edx, biblioteca

        loop_buscar :
            cmp ebx, ecx
            jge fin_busqueda

            push ecx
            mov esi, t
            mov edi, edx

            cmp_str :
                mov al, [esi]
                mov ah, [edi]
                cmp al, ah
                jne next_book_pop
                test al, al
                jz found
                inc esi
                inc edi
                jmp cmp_str

            next_book_pop :
                pop ecx
                add edx, 112
                inc ebx
                jmp loop_buscar

            found :
                pop ecx
                mov eax, [edx + 104]
                cmp eax, 0
                jle sin_stock

            dec eax
            mov[edx + 104], eax
            mov encontrado, 1
            jmp fin_busqueda

            sin_stock :
            mov encontrado, 2

            fin_busqueda :
    }

    if (encontrado == 1) printf("Se prestó Libro.\n");
    else if (encontrado == 2) printf("No quedan libros disponibles.\n");
    else printf("No se encontró tu Libro :(\n");
}

void devolver_libro(char* t) {
    int encontrado = 0;
    __asm {
        xor ebx, ebx
        mov ecx, totalLibros
        lea edx, biblioteca

        loop_buscar_dev :
            cmp ebx, ecx
            jge fin_busqueda_dev

            push ecx
            mov esi, t
            mov edi, edx

            cmp_str_dev :
            mov al, [esi]
            mov ah, [edi]
            cmp al, ah
            jne next_book_pop_dev
            test al, al
            jz found_dev
            inc esi
            inc edi
            jmp cmp_str_dev

            next_book_pop_dev :
            pop ecx
            add edx, 112
            inc ebx
            jmp loop_buscar_dev

            found_dev :
            pop ecx

            mov eax, [edx + 104]
            mov ecx, [edx + 108]

            cmp eax, ecx
            jge inventario_lleno

            inc eax
            mov[edx + 104], eax
            mov encontrado, 1
            jmp fin_busqueda_dev

            inventario_lleno :
            mov encontrado, 2

            fin_busqueda_dev :
    }

    if (encontrado == 1) printf("Se devolvió el libro :D\n");
    else if (encontrado == 2) printf("La biblioteca ya tiene todos las copias de este libro.\n");
    else printf("No se encontró el Libro\n");
}

void buscar_libro(char* t) {
    int encontrado = 0;
    char* resTitulo;
    char* resAutor;
    int resAnio;
    int resCant;

    __asm {
        xor ebx, ebx
        mov ecx, totalLibros
        lea edx, biblioteca

        loop_buscar_info :
        cmp ebx, ecx
            jge fin_busqueda_info

            push ecx
            mov esi, t
            mov edi, edx

            cmp_str_info :
        mov al, [esi]
            mov ah, [edi]
            cmp al, ah
            jne next_book_info
            test al, al
            jz found_info
            inc esi
            inc edi
            jmp cmp_str_info

            next_book_info :
        pop ecx
            add edx, 112
            inc ebx
            jmp loop_buscar_info

            found_info :
        pop ecx
            mov encontrado, 1
            mov resTitulo, edx

            lea eax, [edx + 50]
            mov resAutor, eax

            mov eax, [edx + 100]
            mov resAnio, eax

            mov eax, [edx + 104]
            mov resCant, eax

            fin_busqueda_info :
    }

    if (encontrado) {
        printf("\n");
        printf("--- Informacion del Libro ---\n");
        imprimir_libro_c(resTitulo, resAutor, resAnio, resCant);
    }
    else {
        printf("No se encontró tu Libro.\n");
    }
}

void listar_libros() {
    printf("\n");
    printf("--- Lista de Libros ---\n");
    __asm {
        xor ebx, ebx
        lea esi, biblioteca

        loop_listar :
        cmp ebx, totalLibros
            jge fin_listar

            mov eax, [esi + 104]
            push eax
            mov eax, [esi + 100]
            push eax
            lea eax, [esi + 50]
            push eax
            push esi

            call imprimir_libro_c
            add esp, 16

            add esi, 112
            inc ebx
            jmp loop_listar

            fin_listar :
    }
}

void inicializar_datos() {
    agregar_libro("El Anticristo", "Friedrich Nietzsche", 1888, 5);
    agregar_libro("Lenguaje Ensamblador", "Kip Irvine", 2014, 3);
    agregar_libro("Redes de Computadoras", "Andrew Tanenbaum", 2011, 4);
    agregar_libro("Crimen y Castigo", "Dostoyevsky", 1911, 1);
    agregar_libro("Habitos atomicos", "James Clear", 2018, 4);
}

int main() {
    int opc;
    char titulo[50];
    char autor[50];
    int anio, cant;

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    inicializar_datos();

    while (1) {
        printf("\n1. Agregar Libro\n2. Prestar Libro\n3. Devolver Libro\n4. Buscar Libro\n5. Listar Libros\n6. Salir\nOpcion: ");
        scanf_s("%d", &opc);
        getchar();

        switch (opc) {
        case 1:
            printf("Titulo: "); gets_s(titulo, 50);
            printf("Autor: "); gets_s(autor, 50);
            printf("Anio: "); scanf_s("%d", &anio);
            printf("Cantidad: "); scanf_s("%d", &cant);
            agregar_libro(titulo, autor, anio, cant);
            break;
        case 2:
            printf("Titulo a prestar: "); gets_s(titulo, 50);
            prestar_libro(titulo);
            break;
        case 3:
            printf("Titulo a devolver: "); gets_s(titulo, 50);
            devolver_libro(titulo);
            break;
        case 4:
            printf("Titulo a buscar: "); gets_s(titulo, 50);
            buscar_libro(titulo);
            break;
        case 5:
            listar_libros();
            break;
        case 6:
            return 0;
        }
    }
    return 0;
}