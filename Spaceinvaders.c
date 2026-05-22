/*
 * ============================================================================
 *  SPACE INVADERS — MASM x86 + C/SDL3   (archivo único)
 * ============================================================================
 *  Proyecto académico — Ingeniería en Computación, UABC Mexicali
 *  Organización y Arquitectura de Computadoras
 *
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │  Separación de responsabilidades:                               │
 *  │    ASM x86   →  TODA la lógica: movimiento, colisiones,         │
 *  │                  puntaje, RNG, estados, escudos, velocidad      │
 *  │    C / SDL3  →  SOLO renderizado, ventana, input polling        │
 *  └─────────────────────────────────────────────────────────────────┘
 *
 *  Compilar: MSVC x86 (32-bit) — __asm solo disponible en modo x86
 *
 *    cl /nologo /W3 /Od /I "C:\SDL3\include" SpaceInvaders.c
 *       /link /SUBSYSTEM:CONSOLE /LIBPATH:"C:\SDL3\lib\x86" SDL3.lib
 *
 * ============================================================================
 */

 /* ════════════════════════════════════════════════════════════════════════════
  *  PARTE 0: INCLUDES Y CONFIGURACIÓN
  * ════════════════════════════════════════════════════════════════════════════ */

  /* MSVC trata sprintf como "inseguro" y genera error C4996.
     Lo deshabilitamos porque sprintf_s no es estándar C y
     nuestros buffers son de tamaño fijo y controlado.             */
#define _CRT_SECURE_NO_WARNINGS

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>

     /* ════════════════════════════════════════════════════════════════════════════
      *  PARTE 1: CONSTANTES DEL JUEGO
      * ════════════════════════════════════════════════════════════════════════════
      *  Todas las dimensiones, velocidades, y parámetros del juego están aquí.
      *  Se usan tanto desde C (rendering) como desde __asm (lógica).
      * ════════════════════════════════════════════════════════════════════════════ */

      /* ── Ventana ────────────────────────────────────────────────────────────── */
#define SCREEN_W        800
#define SCREEN_H        600

/* ── Jugador ────────────────────────────────────────────────────────────── */
#define PLAYER_W        40
#define PLAYER_H        20
#define PLAYER_SPEED    5
#define PLAYER_Y        (SCREEN_H - 60)
#define PLAYER_LIVES    3

/* ── Balas ──────────────────────────────────────────────────────────────── */
#define BULLET_W        4
#define BULLET_H        12
#define BULLET_SPEED    7
#define ALIEN_BULLET_SPEED 4
#define MAX_PLAYER_BULLETS  4
#define MAX_ALIEN_BULLETS   6

/* ── Aliens (grilla 5 filas × 11 columnas = 55 aliens) ─────────────────── */
#define ALIEN_ROWS      5
#define ALIEN_COLS      11
#define ALIEN_TOTAL     (ALIEN_ROWS * ALIEN_COLS)
#define ALIEN_W         30
#define ALIEN_H         22
#define ALIEN_PAD_X     14
#define ALIEN_PAD_Y     12
#define ALIEN_START_X   70
#define ALIEN_START_Y   60
#define ALIEN_DROP_DIST 18
#define ALIEN_BASE_DELAY 30
#define ALIEN_MIN_DELAY  3

/* ── Escudos (4 escudos, cada uno es una grilla de 8×4 bloques) ─────── */
#define SHIELD_COUNT    4
#define SHIELD_COLS     8
#define SHIELD_ROWS     4
#define SHIELD_BLOCK_W  7
#define SHIELD_BLOCK_H  6
#define SHIELD_BLOCKS   (SHIELD_COLS * SHIELD_ROWS)
#define SHIELD_Y        (SCREEN_H - 140)

/* ── Puntaje por tipo ───────────────────────────────────────────────────── */
#define POINTS_TYPE0    30      /* fila superior  — más puntos           */
#define POINTS_TYPE1    20      /* filas medias                          */
#define POINTS_TYPE2    10      /* filas inferiores                      */

/* ── Estados del juego ──────────────────────────────────────────────────── */
#define STATE_MENU      0
#define STATE_PLAYING   1
#define STATE_GAMEOVER  2
#define STATE_WIN       3

/* ── Cooldowns ──────────────────────────────────────────────────────────── */
#define SHOOT_COOLDOWN      10
#define ALIEN_SHOOT_DELAY   60

/*
 * ── Offsets de structs para acceso directo en ASM ──────────────────────
 *  Cuando recorremos arrays de structs con puntero ESI en __asm,
 *  necesitamos los offsets exactos de cada campo.
 *  Asume sizeof(int) == 4, sin padding (ints consecutivos en x86).
 */

 /* Alien: { int x, y, alive, type, points; }  → 20 bytes */
#define ALIEN_OFF_X         0
#define ALIEN_OFF_Y         4
#define ALIEN_OFF_ALIVE     8
#define ALIEN_OFF_TYPE      12
#define ALIEN_OFF_POINTS    16
#define ALIEN_SIZE          20

/* ════════════════════════════════════════════════════════════════════════════
 *  PARTE 2: ESTRUCTURAS DE DATOS
 * ════════════════════════════════════════════════════════════════════════════ */

typedef struct {
    int x, y;
    int active;                 /* 1 = en vuelo, 0 = inactiva            */
} Bullet;

typedef struct {
    int x, y;
    int alive;                  /* 1 = vivo, 0 = destruido               */
    int type;                   /* 0 = superior, 1 = medio, 2 = inferior */
    int points;                 /* puntos al destruirlo                  */
} Alien;

typedef struct {
    int x, y;
    int blocks[SHIELD_BLOCKS];  /* 1 = intacto, 0 = destruido           */
} Shield;

typedef struct {
    /* Jugador */
    int player_x;
    int player_lives;

    /* Balas */
    Bullet player_bullets[MAX_PLAYER_BULLETS];
    Bullet alien_bullets[MAX_ALIEN_BULLETS];

    /* Aliens */
    Alien aliens[ALIEN_TOTAL];
    int alien_dir;              /* +1 derecha, -1 izquierda              */
    int alien_speed;            /* píxeles por movimiento                */
    int alien_move_timer;       /* cuenta regresiva para mover           */
    int alien_move_delay;       /* delay actual (baja con muertes)       */
    int aliens_alive;           /* contador de aliens vivos              */
    int alien_anim_frame;       /* 0 o 1, alterna cada movimiento       */

    /* Escudos */
    Shield shields[SHIELD_COUNT];

    /* Estado global */
    int state;
    int score;
    int high_score;

    /* Temporizadores */
    int shoot_cooldown;
    int alien_shoot_timer;

    /* Generador pseudoaleatorio (LCG) */
    unsigned int rng_state;
} GameState;

/* ── Declaraciones forward de funciones ASM ─────────────────────────── */
/* Inicialización */
static void asm_init_game(GameState* gs);
static void asm_init_aliens(GameState* gs);
static void asm_init_shields(GameState* gs);
/* Lógica por frame */
static void asm_update_player(GameState* gs, int move_left, int move_right);
static void asm_fire_player_bullet(GameState* gs);
static void asm_update_player_bullets(GameState* gs);
static void asm_update_alien_bullets(GameState* gs);
static void asm_update_aliens(GameState* gs);
static void asm_alien_shoot(GameState* gs);
/* Colisiones */
static void asm_check_bullet_alien(GameState* gs);
static void asm_check_alien_bullet_player(GameState* gs);
static void asm_check_bullet_shield(GameState* gs);
static void asm_check_alien_bullet_shield(GameState* gs);
/* Estado y utilidades */
static void asm_update_game_state(GameState* gs);
static void asm_update_cooldowns(GameState* gs);
static unsigned int asm_random(GameState* gs);
static void asm_recalc_alien_speed(GameState* gs);
/* Game loop como máquina de estados ASM */
static void asm_check_frame_timing(unsigned int elapsed,
    int* should_delay, int* delay_ms);
static void asm_process_frame_input(GameState* gs,
    int key_left, int key_right,
    int key_shoot, int key_enter,
    int* out_move_l, int* out_move_r,
    int* out_shoot, int* out_restart);
static void asm_game_tick(GameState* gs,
    int move_left, int move_right, int shoot);
static void asm_restart_game(GameState* gs);
static void asm_get_render_flags(GameState* gs,
    int* render_scene, int* render_menu,
    int* render_gameover, int* render_win);


/* ════════════════════════════════════════════════════════════════════════════
 *  PARTE 3: LÓGICA DEL JUEGO — TODO EN ENSAMBLADOR x86 INLINE
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  Principio rector: "Si puede escribirse en ASM, DEBE escribirse en ASM."
 *
 *  El código C circundante se usa SOLO para:
 *    - Extraer campos de structs a variables locales (bridge C → ASM)
 *    - Escribir resultados de vuelta al struct (bridge ASM → C)
 *    - Controlar bucles for() sobre arrays (la aritmética interna es ASM)
 *
 *  Técnicas demostradas:
 *    - Aritmética entera: ADD, SUB, IMUL, IDIV, SHR, NEG, INC, DEC
 *    - Control de flujo: CMP + JE/JNE/JG/JGE/JL/JLE/JMP
 *    - Lógica bitwise:  XOR (toggle), AND (máscara RNG)
 *    - Punteros:        MOV ESI, ptr → [ESI + offset]
 *    - División/módulo:  CDQ + IDIV para distribución y selección
 *    - LCG:             IMUL + ADD + SHR + AND (generador aleatorio)
 * ════════════════════════════════════════════════════════════════════════════ */

 /* ────────────────────────────────────────────────────────────────────────
  *  3.1  INICIALIZACIÓN
  * ──────────────────────────────────────────────────────────────────────── */

  /* ── asm_random ─────────────────────────────────────────────────────────
   *  Generador pseudoaleatorio LCG (Linear Congruential Generator).
   *  Mismos parámetros que glibc:
   *    state = state * 1103515245 + 12345
   *    return (state >> 16) & 0x7FFF
   *
   *  La multiplicación, suma, shift y máscara se ejecutan completamente
   *  en registros x86 sin tocar C.
   * ──────────────────────────────────────────────────────────────────────── */
static unsigned int asm_random(GameState* gs) {
    unsigned int s = gs->rng_state;
    unsigned int result;

    __asm {
        mov eax, s
        imul eax, 1103515245    /* state * a                            */
        add eax, 12345          /* + c                                   */
        mov s, eax              /* guardar nuevo estado                  */

        shr eax, 16             /* descartar bits bajos (baja calidad)   */
        and eax, 0x7FFF         /* máscara de 15 bits → rango 0..32767  */
        mov result, eax
    }

    gs->rng_state = s;
    return result;
}

/* ── asm_init_game ──────────────────────────────────────────────────────
 *  Establece todos los valores iniciales del juego.
 *  La posición del jugador se centra aritméticamente con SHR (div/2).
 *  La semilla RNG se genera con multiplicación de Knuth.
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_init_game(GameState* gs) {
    memset(gs, 0, sizeof(GameState));

    int px, lives, dir, speed, delay, alive;
    unsigned int seed;

    __asm {
        /* Centrar jugador: (SCREEN_W - PLAYER_W) / 2                    */
        mov eax, SCREEN_W
        sub eax, PLAYER_W
        shr eax, 1
        mov px, eax

        mov lives, PLAYER_LIVES
        mov dir, 1              /* aliens se mueven a la derecha         */
        mov speed, 2
        mov delay, ALIEN_BASE_DELAY
        mov alive, ALIEN_TOTAL  /* 55 aliens vivos al inicio            */

        /* Semilla RNG: hash multiplicativo de Knuth                     */
        mov eax, 12345678
        xor eax, SCREEN_W
        imul eax, 2654435761
        mov seed, eax
    }

    gs->player_x = px;
    gs->player_lives = lives;
    gs->state = STATE_MENU;
    gs->alien_dir = dir;
    gs->alien_speed = speed;
    gs->alien_move_delay = delay;
    gs->alien_move_timer = delay;
    gs->aliens_alive = alive;
    gs->rng_state = seed;
    gs->alien_shoot_timer = ALIEN_SHOOT_DELAY;

    asm_init_aliens(gs);
    asm_init_shields(gs);
}

/* ── asm_init_aliens ────────────────────────────────────────────────────
 *  Calcula la posición (x, y) de cada alien en la grilla 5×11.
 *
 *  Fórmula (calculada en ASM con IMUL + ADD):
 *    x = ALIEN_START_X + col * (ALIEN_W + ALIEN_PAD_X)
 *    y = ALIEN_START_Y + row * (ALIEN_H + ALIEN_PAD_Y)
 *
 *  Asignación de tipo y puntaje por fila (CMP + JNE/JG):
 *    Fila 0:     tipo 0 → 30 pts
 *    Filas 1-2:  tipo 1 → 20 pts
 *    Filas 3-4:  tipo 2 → 10 pts
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_init_aliens(GameState* gs) {
    int step_x = ALIEN_W + ALIEN_PAD_X;    /* 44 px entre centros X     */
    int step_y = ALIEN_H + ALIEN_PAD_Y;    /* 34 px entre centros Y     */

    for (int row = 0; row < ALIEN_ROWS; row++) {
        for (int col = 0; col < ALIEN_COLS; col++) {
            int idx = row * ALIEN_COLS + col;
            int alien_x, alien_y, atype, apoints;

            __asm {
                /* ── Posición X = START_X + col * step_x ── */
                mov eax, col
                imul eax, step_x
                add eax, ALIEN_START_X
                mov alien_x, eax

                /* ── Posición Y = START_Y + row * step_y ── */
                mov eax, row
                imul eax, step_y
                add eax, ALIEN_START_Y
                mov alien_y, eax

                /* ── Tipo según fila (switch con CMP + saltos) ── */
                mov ecx, row
                cmp ecx, 0
                jne  ia_check_mid
                mov atype, 0
                mov apoints, POINTS_TYPE0
                jmp ia_type_done

                ia_check_mid :
                cmp ecx, 2
                    jg  ia_is_bottom
                    mov atype, 1
                    mov apoints, POINTS_TYPE1
                    jmp ia_type_done

                    ia_is_bottom :
                mov atype, 2
                    mov apoints, POINTS_TYPE2

                    ia_type_done :
            }

            gs->aliens[idx].x = alien_x;
            gs->aliens[idx].y = alien_y;
            gs->aliens[idx].alive = 1;
            gs->aliens[idx].type = atype;
            gs->aliens[idx].points = apoints;
        }
    }
}

/* ── asm_init_shields ───────────────────────────────────────────────────
 *  Distribuye 4 escudos equidistantes usando CDQ + IDIV.
 *  Cada escudo tiene 32 bloques (8×4), todos comienzan intactos.
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_init_shields(GameState* gs) {
    int shield_total_w = SHIELD_COLS * SHIELD_BLOCK_W;
    int spacing, start_x;

    __asm {
        /* spacing = SCREEN_W / (SHIELD_COUNT + 1) = 800 / 5 = 160      */
        mov eax, SCREEN_W
        cdq
        mov ecx, SHIELD_COUNT
        inc ecx
        idiv ecx
        mov spacing, eax

        /* start_x = spacing - shield_total_w / 2                        */
        mov eax, spacing
        mov ecx, shield_total_w
        shr ecx, 1
        sub eax, ecx
        mov start_x, eax
    }

    for (int i = 0; i < SHIELD_COUNT; i++) {
        int shield_x;
        __asm {
            mov eax, i
            imul eax, spacing
            add eax, start_x
            mov shield_x, eax
        }
        gs->shields[i].x = shield_x;
        gs->shields[i].y = SHIELD_Y;

        for (int b = 0; b < SHIELD_BLOCKS; b++)
            gs->shields[i].blocks[b] = 1;
    }
}

/* ────────────────────────────────────────────────────────────────────────
 *  3.2  LÓGICA DEL JUGADOR
 * ──────────────────────────────────────────────────────────────────────── */

 /* ── asm_update_player ──────────────────────────────────────────────────
  *  Movimiento horizontal con clamp a los bordes de pantalla.
  *  Toda la aritmética y bounds checking en ASM:
  *    - CMP + JE para evaluar input booleano
  *    - ADD/SUB para movimiento
  *    - CMP + JGE/JLE para clamp (min/max)
  * ──────────────────────────────────────────────────────────────────────── */
static void asm_update_player(GameState* gs, int move_left, int move_right) {
    int px = gs->player_x;

    __asm {
        mov eax, px

        /* Mover izquierda si la tecla está presionada */
        cmp move_left, 0
        je  up_no_left
        sub eax, PLAYER_SPEED
        up_no_left :

        /* Mover derecha si la tecla está presionada */
        cmp move_right, 0
            je  up_no_right
            add eax, PLAYER_SPEED
            up_no_right :

        /* Clamp inferior: px >= 10 */
        cmp eax, 10
            jge up_not_below
            mov eax, 10
            up_not_below :

            /* Clamp superior: px <= SCREEN_W - PLAYER_W - 10 */
            mov ecx, SCREEN_W
            sub ecx, PLAYER_W
            sub ecx, 10
            cmp eax, ecx
            jle up_not_above
            mov eax, ecx
            up_not_above :

        mov px, eax
    }

    gs->player_x = px;
}

/* ── asm_fire_player_bullet ─────────────────────────────────────────────
 *  Busca un slot de bala libre y la activa centrada sobre el jugador.
 *  Verificación de cooldown y cálculo de posición en ASM.
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_fire_player_bullet(GameState* gs) {
    int cooldown = gs->shoot_cooldown;
    int can_fire = 0;

    __asm {
        cmp cooldown, 0
        jne  fb_no_fire
        mov can_fire, 1
        fb_no_fire:
    }

    if (!can_fire) return;

    Bullet* bullets = gs->player_bullets;
    int px = gs->player_x;

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        int active = bullets[i].active;
        int found = 0;
        int bullet_x, bullet_y;

        __asm {
            cmp active, 0
            jne fb_slot_busy

            /* Centrar bala sobre el jugador:
               x = player_x + PLAYER_W/2 - BULLET_W/2
               y = PLAYER_Y  - BULLET_H                                 */
               mov eax, px
               add eax, PLAYER_W / 2
               sub eax, BULLET_W / 2
               mov bullet_x, eax

               mov eax, PLAYER_Y
               sub eax, BULLET_H
               mov bullet_y, eax

               mov found, 1
               jmp fb_slot_done

               fb_slot_busy :
        fb_slot_done:
        }

        if (found) {
            bullets[i].x = bullet_x;
            bullets[i].y = bullet_y;
            bullets[i].active = 1;
            gs->shoot_cooldown = SHOOT_COOLDOWN;
            break;
        }
    }
}

/* ────────────────────────────────────────────────────────────────────────
 *  3.3  GESTIÓN DE BALAS
 * ──────────────────────────────────────────────────────────────────────── */

 /* ── asm_update_player_bullets ──────────────────────────────────────────
  *  Mueve balas del jugador hacia arriba (SUB).
  *  Desactiva las que salen por el borde superior (CMP + JG).
  * ──────────────────────────────────────────────────────────────────────── */
static void asm_update_player_bullets(GameState* gs) {
    Bullet* bullets = gs->player_bullets;

    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (!bullets[i].active) continue;

        int by = bullets[i].y;
        int new_active;

        __asm {
            mov eax, by
            sub eax, BULLET_SPEED
            mov by, eax

            /* ¿Salió de pantalla? */
            cmp eax, -BULLET_H
            jg  pb_on_screen
            mov new_active, 0
            jmp pb_done
            pb_on_screen :
            mov new_active, 1
                pb_done :
        }

        bullets[i].y = by;
        bullets[i].active = new_active;
    }
}

/* ── asm_update_alien_bullets ───────────────────────────────────────────
 *  Mueve balas enemigas hacia abajo (ADD).
 *  Desactiva las que salen por el borde inferior.
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_update_alien_bullets(GameState* gs) {
    Bullet* bullets = gs->alien_bullets;

    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (!bullets[i].active) continue;

        int by = bullets[i].y;
        int new_active;

        __asm {
            mov eax, by
            add eax, ALIEN_BULLET_SPEED
            mov by, eax

            cmp eax, SCREEN_H
            jl  ab_on_screen
            mov new_active, 0
            jmp ab_done
            ab_on_screen :
            mov new_active, 1
                ab_done :
        }

        bullets[i].y = by;
        bullets[i].active = new_active;
    }
}

/* ────────────────────────────────────────────────────────────────────────
 *  3.4  MOVIMIENTO DE ALIENS
 * ──────────────────────────────────────────────────────────────────────── */

 /* ── asm_update_aliens ──────────────────────────────────────────────────
  *  La grilla completa se mueve como unidad:
  *    1. Decrementa timer (DEC). Si > 0, retorna.
  *    2. Mueve todos los vivos en alien_dir * alien_speed (IMUL).
  *    3. Si alguno toca un borde → invertir dir (NEG) y bajar todos.
  *    4. Alterna frame de animación (XOR con 1).
  * ──────────────────────────────────────────────────────────────────────── */
static void asm_update_aliens(GameState* gs) {
    int timer = gs->alien_move_timer;
    int should_move = 0;

    __asm {
        mov eax, timer
        dec eax
        mov timer, eax
        cmp eax, 0
        jg  ua_no_move
        mov should_move, 1
        ua_no_move:
    }

    gs->alien_move_timer = timer;
    if (!should_move) return;

    /* Resetear timer y alternar frame de animación */
    gs->alien_move_timer = gs->alien_move_delay;

    int anim = gs->alien_anim_frame;
    __asm {
        mov eax, anim
        xor eax, 1              /* toggle: 0↔1                          */
        mov anim, eax
    }
    gs->alien_anim_frame = anim;

    /* Calcular desplazamiento horizontal */
    int dir = gs->alien_dir;
    int spd = gs->alien_speed;
    int move_dx;

    __asm {
        mov eax, dir
        imul eax, spd
        mov move_dx, eax
    }

    int need_drop = 0;

    Alien* aliens = gs->aliens;
    for (int i = 0; i < ALIEN_TOTAL; i++) {
        if (!aliens[i].alive) continue;

        int axi = aliens[i].x;
        int check_drop = 0;

        __asm {
            mov eax, axi
            add eax, move_dx
            mov axi, eax

            /* ¿Tocó el borde izquierdo? */
            cmp eax, 8
            jge  ua_not_left
            mov check_drop, 1
            ua_not_left:

            /* ¿Tocó el borde derecho? */
            mov ecx, SCREEN_W
                sub ecx, ALIEN_W
                sub ecx, 8
                cmp eax, ecx
                jle  ua_not_right
                mov check_drop, 1
                ua_not_right:
        }

        aliens[i].x = axi;
        if (check_drop) need_drop = 1;
    }

    /* Si tocaron un borde: invertir dirección (NEG) y descender */
    if (need_drop) {
        int new_dir;
        __asm {
            mov eax, dir
            neg eax
            mov new_dir, eax
        }
        gs->alien_dir = new_dir;

        for (int i = 0; i < ALIEN_TOTAL; i++) {
            if (!aliens[i].alive) continue;

            int ay = aliens[i].y;
            int axi = aliens[i].x;

            __asm {
                /* Bajar ALIEN_DROP_DIST píxeles */
                mov eax, ay
                add eax, ALIEN_DROP_DIST
                mov ay, eax

                /* Corregir X: revertir movimiento + aplicar nueva dir */
                mov eax, axi
                sub eax, move_dx
                mov ecx, new_dir
                imul ecx, spd
                add eax, ecx
                mov axi, eax
            }

            aliens[i].y = ay;
            aliens[i].x = axi;
        }
    }
}

/* ── asm_alien_shoot ────────────────────────────────────────────────────
 *  Selecciona un alien vivo con RNG y dispara una bala.
 *
 *  Algoritmo (en ASM):
 *    1. DEC timer, verificar alive > 0 y timer <= 0
 *    2. Generar número aleatorio (LCG)
 *    3. target = random % aliens_alive  (DIV → EDX = remainder)
 *    4. Iterar aliens contando vivos hasta encontrar el target-ésimo
 *    5. Crear bala centrada debajo del alien seleccionado
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_alien_shoot(GameState* gs) {
    int timer = gs->alien_shoot_timer;
    int alive = gs->aliens_alive;
    int can_shoot = 0;

    __asm {
        dec timer

        cmp alive, 0
        jle  as_cannot

        cmp timer, 0
        jg  as_cannot

        mov can_shoot, 1
        jmp as_check_done

        as_cannot :
        mov can_shoot, 0
            as_check_done :
    }

    gs->alien_shoot_timer = timer;
    if (!can_shoot) return;

    unsigned int rnd = asm_random(gs);

    /* target = rnd % alive  (DIV: EDX recibe el residuo) */
    int target;
    __asm {
        mov eax, rnd
        xor edx, edx
        div alive
        mov target, edx
    }

    /* Encontrar el target-ésimo alien vivo */
    Alien* aliens = gs->aliens;
    int count = 0;
    int found_idx = -1;

    for (int i = 0; i < ALIEN_TOTAL; i++) {
        if (!aliens[i].alive) continue;

        int is_target = 0;
        __asm {
            mov eax, count
            cmp eax, target
            jne  as_not_target
            mov is_target, 1
            as_not_target:
        }

        if (is_target) { found_idx = i; break; }
        count++;
    }

    if (found_idx < 0) {
        gs->alien_shoot_timer = ALIEN_SHOOT_DELAY;
        return;
    }

    /* Buscar slot libre y crear bala */
    Bullet* ab = gs->alien_bullets;
    int ax_pos = aliens[found_idx].x;
    int ay_pos = aliens[found_idx].y;

    for (int j = 0; j < MAX_ALIEN_BULLETS; j++) {
        if (ab[j].active) continue;

        int blx, bly;
        __asm {
            /* Centrar bala bajo el alien */
            mov eax, ax_pos
            add eax, ALIEN_W / 2
            sub eax, BULLET_W / 2
            mov blx, eax

            mov eax, ay_pos
            add eax, ALIEN_H
            mov bly, eax
        }

        ab[j].x = blx;
        ab[j].y = bly;
        ab[j].active = 1;
        break;
    }

    gs->alien_shoot_timer = ALIEN_SHOOT_DELAY;
}

/* ────────────────────────────────────────────────────────────────────────
 *  3.5  DETECCIÓN DE COLISIONES (AABB)
 * ────────────────────────────────────────────────────────────────────────
 *  Todas usan Axis-Aligned Bounding Box:
 *    A.right  > B.left   AND   A.left   < B.right
 *    A.bottom > B.top    AND   A.top    < B.bottom
 *
 *  Cada test son 4 instrucciones CMP + salto condicional.
 * ──────────────────────────────────────────────────────────────────────── */

 /* ── asm_check_bullet_alien ─────────────────────────────────────────────
  *  Balas del jugador vs aliens vivos.
  *  El loop interno usa puntero ESI + offsets (ALIEN_OFF_X, ALIEN_SIZE)
  *  para iterar sobre el array de aliens directamente en memoria,
  *  demostrando aritmética de punteros en ensamblador.
  * ──────────────────────────────────────────────────────────────────────── */
static void asm_check_bullet_alien(GameState* gs) {
    Bullet* bullets = gs->player_bullets;
    Alien* aliens = gs->aliens;
    int score = gs->score;
    int alive = gs->aliens_alive;

    for (int b = 0; b < MAX_PLAYER_BULLETS; b++) {
        if (!bullets[b].active) continue;

        int bul_x = bullets[b].x;
        int by = bullets[b].y;
        int hit_idx = -1;

        /* Loop interno con puntero ESI — acceso directo a memoria */
        Alien* aptr = aliens;
        int total = ALIEN_TOTAL;

        __asm {
            mov esi, aptr
            xor ecx, ecx           /* ECX = índice                      */

            ba_loop :
            cmp ecx, total
                jge ba_no_hit

                /* ¿Alien vivo? */
                cmp dword ptr[esi + ALIEN_OFF_ALIVE], 0
                je  ba_next

                /* ════ AABB test (4 comparaciones) ════ */

                /* Test 1: bullet.right > alien.left */
                mov eax, bul_x
                add eax, BULLET_W
                cmp eax, dword ptr[esi + ALIEN_OFF_X]
                jle ba_next

                /* Test 2: bullet.left < alien.right */
                mov eax, bul_x
                mov edx, dword ptr[esi + ALIEN_OFF_X]
                add edx, ALIEN_W
                cmp eax, edx
                jge ba_next

                /* Test 3: bullet.bottom > alien.top */
                mov eax, by
                add eax, BULLET_H
                cmp eax, dword ptr[esi + ALIEN_OFF_Y]
                jle ba_next

                /* Test 4: bullet.top < alien.bottom */
                mov eax, by
                mov edx, dword ptr[esi + ALIEN_OFF_Y]
                add edx, ALIEN_H
                cmp eax, edx
                jge ba_next

                /* ════ ¡COLISIÓN! ════ */
                mov dword ptr[esi + ALIEN_OFF_ALIVE], 0    /* matar alien  */

                mov eax, score
                add eax, dword ptr[esi + ALIEN_OFF_POINTS] /* sumar puntos */
                mov score, eax

                dec alive
                mov hit_idx, ecx
                jmp ba_hit

                ba_next :
            add esi, ALIEN_SIZE     /* avanzar puntero al siguiente     */
                inc ecx
                jmp ba_loop

                ba_no_hit :
        ba_hit:
        }

        if (hit_idx >= 0)
            bullets[b].active = 0;
    }

    gs->score = score;
    gs->aliens_alive = alive;
    asm_recalc_alien_speed(gs);
}

/* ── asm_check_alien_bullet_player ──────────────────────────────────────
 *  Balas enemigas vs rectángulo del jugador. AABB completo en ASM.
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_check_alien_bullet_player(GameState* gs) {
    Bullet* ab = gs->alien_bullets;
    int px = gs->player_x;
    int py = PLAYER_Y;
    int lives = gs->player_lives;

    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (!ab[i].active) continue;

        int bul_x = ab[i].x;
        int by = ab[i].y;
        int hit = 0;

        __asm {
            /* AABB: bala enemiga vs jugador */
            mov eax, bul_x
            add eax, BULLET_W
            cmp eax, px
            jle ap_no

            mov eax, bul_x
            mov ecx, px
            add ecx, PLAYER_W
            cmp eax, ecx
            jge ap_no

            mov eax, by
            add eax, BULLET_H
            cmp eax, py
            jle ap_no

            mov eax, by
            mov ecx, py
            add ecx, PLAYER_H
            cmp eax, ecx
            jge ap_no

            mov hit, 1
            dec lives

            ap_no :
        }

        if (hit) {
            ab[i].active = 0;
            gs->player_lives = lives;
        }
    }
}

/* ── asm_check_bullet_shield ────────────────────────────────────────────
 *  Balas del jugador vs bloques de escudos.
 *  Calcula el bloque impactado con aritmética entera en ASM:
 *    block_col = (bul_x - shield_x) / SHIELD_BLOCK_W   (IDIV)
 *    block_row = (by - shield_y) / SHIELD_BLOCK_H   (IDIV)
 *    block_idx = block_row * SHIELD_COLS + block_col (IMUL + ADD)
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_check_bullet_shield(GameState* gs) {
    Bullet* pb = gs->player_bullets;

    for (int b = 0; b < MAX_PLAYER_BULLETS; b++) {
        if (!pb[b].active) continue;

        int bul_x = pb[b].x + BULLET_W / 2;
        int by = pb[b].y;
        int destroyed = 0;

        for (int s = 0; s < SHIELD_COUNT; s++) {
            int sx = gs->shields[s].x;
            int sy = gs->shields[s].y;
            int sw = SHIELD_COLS * SHIELD_BLOCK_W;
            int sh = SHIELD_ROWS * SHIELD_BLOCK_H;
            int in_shield = 0;
            int blk_col = 0, blk_row = 0, blk_idx = 0;

            __asm {
                /* ¿Bala dentro del bounding box del escudo? */
                mov eax, bul_x
                cmp eax, sx
                jl  bs_out

                mov ecx, sx
                add ecx, sw
                cmp eax, ecx
                jge bs_out

                mov eax, by
                cmp eax, sy
                jl  bs_out

                mov ecx, sy
                add ecx, sh
                cmp eax, ecx
                jge bs_out

                /* Calcular bloque impactado */
                mov eax, bul_x
                sub eax, sx
                cdq
                mov ecx, SHIELD_BLOCK_W
                idiv ecx
                mov blk_col, eax

                mov eax, by
                sub eax, sy
                cdq
                mov ecx, SHIELD_BLOCK_H
                idiv ecx
                mov blk_row, eax

                mov eax, blk_row
                imul eax, SHIELD_COLS
                add eax, blk_col
                mov blk_idx, eax

                /* Validar rango */
                cmp eax, 0
                jl  bs_out
                cmp eax, SHIELD_BLOCKS
                jge bs_out

                mov in_shield, 1
                jmp bs_done
                bs_out :
                mov in_shield, 0
                    bs_done :
            }

            if (in_shield && gs->shields[s].blocks[blk_idx]) {
                gs->shields[s].blocks[blk_idx] = 0;
                destroyed = 1;
                break;
            }
        }

        if (destroyed) pb[b].active = 0;
    }
}

/* ── asm_check_alien_bullet_shield ──────────────────────────────────────
 *  Balas enemigas también destruyen bloques de escudos.
 *  Misma lógica AABB + cálculo de bloque que la función anterior.
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_check_alien_bullet_shield(GameState* gs) {
    Bullet* ab = gs->alien_bullets;

    for (int b = 0; b < MAX_ALIEN_BULLETS; b++) {
        if (!ab[b].active) continue;

        int bul_x = ab[b].x + BULLET_W / 2;
        int by = ab[b].y + BULLET_H;
        int destroyed = 0;

        for (int s = 0; s < SHIELD_COUNT; s++) {
            int sx = gs->shields[s].x;
            int sy = gs->shields[s].y;
            int sw = SHIELD_COLS * SHIELD_BLOCK_W;
            int sh = SHIELD_ROWS * SHIELD_BLOCK_H;
            int in_shield = 0;
            int blk_col = 0, blk_row = 0, blk_idx = 0;

            __asm {
                mov eax, bul_x
                cmp eax, sx
                jl  abs_out

                mov ecx, sx
                add ecx, sw
                cmp eax, ecx
                jge abs_out

                mov eax, by
                cmp eax, sy
                jl  abs_out

                mov ecx, sy
                add ecx, sh
                cmp eax, ecx
                jge abs_out

                mov eax, bul_x
                sub eax, sx
                cdq
                mov ecx, SHIELD_BLOCK_W
                idiv ecx
                mov blk_col, eax

                mov eax, by
                sub eax, sy
                cdq
                mov ecx, SHIELD_BLOCK_H
                idiv ecx
                mov blk_row, eax

                mov eax, blk_row
                imul eax, SHIELD_COLS
                add eax, blk_col
                mov blk_idx, eax

                cmp eax, 0
                jl  abs_out
                cmp eax, SHIELD_BLOCKS
                jge abs_out

                mov in_shield, 1
                jmp abs_done
                abs_out :
                mov in_shield, 0
                    abs_done :
            }

            if (in_shield && gs->shields[s].blocks[blk_idx]) {
                gs->shields[s].blocks[blk_idx] = 0;
                destroyed = 1;
                break;
            }
        }

        if (destroyed) ab[b].active = 0;
    }
}

/* ────────────────────────────────────────────────────────────────────────
 *  3.6  MÁQUINA DE ESTADOS Y UTILIDADES
 * ──────────────────────────────────────────────────────────────────────── */

 /* ── asm_update_game_state ──────────────────────────────────────────────
  *  Máquina de estados implementada como cascada CMP + JMP:
  *    - aliens_alive == 0       → STATE_WIN
  *    - player_lives <= 0       → STATE_GAMEOVER
  *    - alien.y >= SHIELD_Y     → STATE_GAMEOVER (invasión)
  * ──────────────────────────────────────────────────────────────────────── */
static void asm_update_game_state(GameState* gs) {
    int alive = gs->aliens_alive;
    int lives = gs->player_lives;
    int state = gs->state;

    __asm {
        cmp state, STATE_PLAYING
        jne gs_done

        /* Victoria: todos los aliens muertos */
        cmp alive, 0
        jg  gs_not_win
        mov state, STATE_WIN
        jmp gs_done
        gs_not_win :

        /* Derrota: sin vidas */
        cmp lives, 0
            jg  gs_not_dead
            mov state, STATE_GAMEOVER
            jmp gs_done
            gs_not_dead :

    gs_done:
    }

    gs->state = state;

    /* Verificar invasión: algún alien alcanzó la línea de escudos */
    if (gs->state == STATE_PLAYING) {
        Alien* aliens = gs->aliens;
        int threshold = SHIELD_Y - ALIEN_H;

        for (int i = 0; i < ALIEN_TOTAL; i++) {
            if (!aliens[i].alive) continue;

            int ay = aliens[i].y;
            int invaded = 0;

            __asm {
                mov eax, ay
                cmp eax, threshold
                jl  gs_not_invaded
                mov invaded, 1
                gs_not_invaded:
            }

            if (invaded) {
                gs->state = STATE_GAMEOVER;
                break;
            }
        }
    }
}

/* ── asm_update_cooldowns ───────────────────────────────────────────── */
static void asm_update_cooldowns(GameState* gs) {
    int cd = gs->shoot_cooldown;
    __asm {
        cmp cd, 0
        jle cd_zero
        dec cd
        cd_zero :
    }
    gs->shoot_cooldown = cd;
}

/* ── asm_recalc_alien_speed ─────────────────────────────────────────────
 *  Ajusta velocidad según aliens restantes (aceleración progresiva).
 *    delay = (BASE_DELAY * alive) / TOTAL    (IMUL + IDIV)
 *    clamp a MIN_DELAY
 *    speed escala inversamente por umbrales (CMP + JG escalonado)
 *
 *  Esto recrea el comportamiento clásico: los últimos aliens
 *  se mueven extremadamente rápido.
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_recalc_alien_speed(GameState* gs) {
    int alive = gs->aliens_alive;
    int new_delay, new_speed;

    __asm {
        /* new_delay = (BASE_DELAY * alive) / TOTAL */
        mov eax, ALIEN_BASE_DELAY
        imul eax, alive
        cdq
        mov ecx, ALIEN_TOTAL
        idiv ecx
        mov new_delay, eax

        /* Clamp mínimo */
        cmp eax, ALIEN_MIN_DELAY
        jge rs_delay_ok
        mov new_delay, ALIEN_MIN_DELAY
        rs_delay_ok :

        /* Velocidad escalonada inversamente */
        mov eax, alive
            cmp eax, 5
            jg  rs_check_20
            mov new_speed, 4        /* < 5 aliens: velocidad máxima          */
            jmp rs_speed_done

            rs_check_20 :
        cmp eax, 20
            jg  rs_slow
            mov new_speed, 3        /* 6-20 aliens: rápido                   */
            jmp rs_speed_done

            rs_slow :
        mov new_speed, 2        /* > 20 aliens: normal                   */
            rs_speed_done :
    }

    gs->alien_move_delay = new_delay;
    gs->alien_speed = new_speed;
}

/* ────────────────────────────────────────────────────────────────────────
 *  3.7  GAME LOOP COMO MÁQUINA DE ESTADOS ASM
 * ────────────────────────────────────────────────────────────────────────
 *  Estas funciones mueven la lógica de control del game loop a ASM.
 *  El loop principal en C solo se encarga de:
 *    - SDL_PollEvent (obligatoriamente C, es API externa)
 *    - Llamadas de renderizado SDL (obligatoriamente C)
 *  Pero TODA la lógica de decisión — qué hacer, cuándo hacerlo,
 *  cómo interpretar el input — vive en ensamblador.
 * ──────────────────────────────────────────────────────────────────────── */

 /* ── asm_check_frame_timing ─────────────────────────────────────────────
  *  Decide si el frame necesita delay para mantener ~60 FPS.
  *  Compara tiempo transcurrido vs 16 ms con CMP + JGE.
  *  Calcula los ms a dormir con SUB.
  *
  *  Esto reemplaza el if (elapsed < 16) { delay = 16 - elapsed; }
  *  que antes estaba en C.
  * ──────────────────────────────────────────────────────────────────────── */
static void asm_check_frame_timing(unsigned int elapsed,
    int* should_delay, int* delay_ms) {
    int sd = 0, dm = 0;

    __asm {
        mov eax, elapsed
        cmp eax, 16            /* ¿Frame más rápido que 16ms?           */
        jge ft_no_delay

        /* Calcular cuánto dormir: 16 - elapsed */
        mov ecx, 16
        sub ecx, eax
        mov dm, ecx
        mov sd, 1

        ft_no_delay:
    }

    *should_delay = sd;
    *delay_ms = dm;
}

/* ── asm_process_frame_input ────────────────────────────────────────────
 *  Máquina de estados para procesamiento de input.
 *
 *  El concepto: las teclas presionadas significan cosas DIFERENTES
 *  dependiendo del estado del juego. Enter reinicia en MENU/GAMEOVER/WIN
 *  pero no hace nada en PLAYING. Las flechas mueven en PLAYING pero
 *  se ignoran en MENU. Esta lógica de dispatch es una máquina de
 *  estados implementada con CMP + JMP condicionales.
 *
 *  Estado actual     Tecla         Acción
 *  ──────────────    ──────        ──────────────────
 *  MENU              Enter         → restart = 1
 *  GAMEOVER          Enter         → restart = 1
 *  WIN               Enter         → restart = 1
 *  PLAYING           ←/A           → move_left = 1
 *  PLAYING           →/D           → move_right = 1
 *  PLAYING           Space         → shoot = 1
 *  (cualquiera)      Escape        → (manejado por SDL en C)
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_process_frame_input(GameState* gs,
    int key_left, int key_right,
    int key_shoot, int key_enter,
    int* out_move_l, int* out_move_r,
    int* out_shoot, int* out_restart) {
    int state = gs->state;
    int ml = 0, mr = 0, sh = 0, restart = 0;

    __asm {
        /* ══ Primer nivel: ¿estamos jugando? ══ */
        cmp state, STATE_PLAYING
        je  pi_playing

        /* ── Estados de espera: MENU / GAMEOVER / WIN ──
           Solo nos interesa si presionaron Enter                        */
           cmp key_enter, 0
           je  pi_done
           mov restart, 1          /* señal de reinicio                    */
           jmp pi_done

           pi_playing :
        /* ── Estado PLAYING: mapear teclas a acciones de juego ──
           Cada tecla se evalúa independientemente (CMP + JE)           */

        cmp key_left, 0
            je  pi_no_left
            mov ml, 1               /* activar movimiento izquierdo         */
            pi_no_left:

        cmp key_right, 0
            je  pi_no_right
            mov mr, 1               /* activar movimiento derecho           */
            pi_no_right :

            cmp key_shoot, 0
            je  pi_done
            mov sh, 1               /* activar disparo                      */

            pi_done :
    }

    *out_move_l = ml;
    *out_move_r = mr;
    *out_shoot = sh;
    *out_restart = restart;
}

/* ── asm_restart_game ───────────────────────────────────────────────────
 *  Transición de reinicio con preservación de high score.
 *  El cálculo del máximo (prev_score vs prev_hi) se hace en ASM
 *  con CMP + JLE (equivalente a un MAX(a, b) condicional).
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_restart_game(GameState* gs) {
    int prev_hi = gs->high_score;
    int prev_sc = gs->score;
    int new_hi;

    __asm {
        /* new_hi = MAX(prev_hi, prev_sc) */
        mov eax, prev_hi
        mov ecx, prev_sc
        cmp ecx, eax
        jle rg_keep_hi
        mov eax, ecx            /* prev_sc > prev_hi → nuevo record    */
        rg_keep_hi :
        mov new_hi, eax
    }

    asm_init_game(gs);
    gs->state = STATE_PLAYING;
    gs->high_score = new_hi;
}

/* ── asm_game_tick ──────────────────────────────────────────────────────
 *  Máquina de estados maestra: orquesta TODO lo que ocurre en un frame.
 *
 *  Este es el equivalente ASM del switch(state) que antes controlaba
 *  el game loop en C. Ahora la DECISIÓN de qué actualizar vive en
 *  ensamblador, y C solo ejecuta las llamadas a subfunciones.
 *
 *  Estructura del dispatch:
 *    CMP state, STATE_PLAYING
 *    JNE → saltar toda la lógica (frame vacío para menú/game over)
 *    JE  → ejecutar: jugador → balas → aliens → colisiones → estado
 *
 *  La evaluación del flag de disparo también es ASM (CMP + JE).
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_game_tick(GameState* gs,
    int move_left, int move_right, int shoot) {
    int state = gs->state;
    int should_update = 0;
    int should_fire = 0;

    __asm {
        /* ── Dispatch principal: solo STATE_PLAYING ejecuta lógica ── */
        cmp state, STATE_PLAYING
        jne gt_skip

        mov should_update, 1

        /* ── Evaluar flag de disparo ── */
        cmp shoot, 0
        je  gt_no_fire
        mov should_fire, 1
        gt_no_fire:

    gt_skip:
    }

    if (!should_update) return;

    /* ══ Pipeline de actualización del frame ══
       Orden crítico: mover → disparar → colisionar → evaluar estado    */

       /* 1. Movimiento del jugador (input → posición) */
    asm_update_player(gs, move_left, move_right);

    /* 2. Disparo del jugador (si se presionó Space) */
    if (should_fire) asm_fire_player_bullet(gs);

    /* 3. Mover todas las balas */
    asm_update_player_bullets(gs);
    asm_update_alien_bullets(gs);

    /* 4. Mover aliens y disparo enemigo (RNG) */
    asm_update_aliens(gs);
    asm_alien_shoot(gs);

    /* 5. Detección de colisiones (AABB) */
    asm_check_bullet_alien(gs);
    asm_check_alien_bullet_player(gs);
    asm_check_bullet_shield(gs);
    asm_check_alien_bullet_shield(gs);

    /* 6. Cooldowns y transiciones de estado */
    asm_update_cooldowns(gs);
    asm_update_game_state(gs);
}

/* ── asm_get_render_flags ───────────────────────────────────────────────
 *  Máquina de estados para el renderizado: decide QUÉ dibujar.
 *
 *  El rendering en sí es C/SDL3 (no se puede evitar), pero la
 *  DECISIÓN de qué pantalla mostrar es una cascada CMP + JMP.
 *
 *  Salida (flags independientes, pueden combinarse):
 *    render_scene    = dibujar jugador, aliens, balas, escudos, HUD
 *    render_menu     = dibujar pantalla de menú
 *    render_gameover = dibujar overlay de game over
 *    render_win      = dibujar overlay de victoria
 *
 *  GAMEOVER activa render_scene + render_gameover (escena congelada
 *  con overlay encima). WIN solo activa escudos+HUD + overlay.
 * ──────────────────────────────────────────────────────────────────────── */
static void asm_get_render_flags(GameState* gs,
    int* render_scene, int* render_menu,
    int* render_gameover, int* render_win) {
    int state = gs->state;
    int rs = 0, rm = 0, rgo = 0, rw = 0;

    __asm {
        mov eax, state

        /* ── Cascada de estados ── */
        cmp eax, STATE_MENU
        jne rf_not_menu
        mov rm, 1               /* solo menú                            */
        jmp rf_done

        rf_not_menu :
        cmp eax, STATE_PLAYING
            jne rf_not_playing
            mov rs, 1               /* escena completa                      */
            jmp rf_done

            rf_not_playing :
        cmp eax, STATE_GAMEOVER
            jne rf_not_gameover
            mov rs, 1               /* escena congelada al fondo...         */
            mov rgo, 1              /* ...con overlay de game over          */
            jmp rf_done

            rf_not_gameover :
        cmp eax, STATE_WIN
            jne rf_done
            mov rw, 1               /* overlay de victoria (+ escudos/HUD)  */

            rf_done :
    }

    *render_scene = rs;
    *render_menu = rm;
    *render_gameover = rgo;
    *render_win = rw;
}


/* ════════════════════════════════════════════════════════════════════════════
 *  PARTE 4: RENDERIZADO (C / SDL3)
 * ════════════════════════════════════════════════════════════════════════════
 *  Esta parte maneja EXCLUSIVAMENTE la presentación visual.
 *  Ningún cálculo de lógica ocurre aquí — todo se delega a las
 *  funciones asm_* de la Parte 3.
 *
 *  Incluye:
 *    - Fuente bitmap 5×7 integrada (sin dependencia de SDL_ttf)
 *    - Renderizado de jugador, aliens, balas, escudos, HUD
 *    - Pantallas de menú, game over, victoria
 *    - Game loop con timing a ~60 FPS
 * ════════════════════════════════════════════════════════════════════════════ */

 /* ── Colores del juego (estilo arcade retro) ────────────────────────── */
static const int alien_colors[3][3] = {
    { 255, 50, 255 },   /* Tipo 0: Magenta (fila superior, 30 pts) */
    { 50, 255, 255 },   /* Tipo 1: Cian    (filas medias, 20 pts)  */
    { 255, 255, 50  },  /* Tipo 2: Amarillo(filas inferiores)      */
};

/* ── Helper: SDL3 usa SDL_FRect (floats) para renderizar ────────────── */
static void fill_rect(SDL_Renderer* r, int x, int y, int w, int h) {
    SDL_FRect fr = { (float)x, (float)y, (float)w, (float)h };
    SDL_RenderFillRect(r, &fr);
}

/* ── Fuente bitmap integrada 5×7 ────────────────────────────────────── */
static const unsigned char font_digits[10][7] = {
    { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E }, /* 0 */
    { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E }, /* 1 */
    { 0x0E, 0x11, 0x01, 0x0E, 0x10, 0x10, 0x1F }, /* 2 */
    { 0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E }, /* 3 */
    { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 }, /* 4 */
    { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E }, /* 5 */
    { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E }, /* 6 */
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }, /* 7 */
    { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E }, /* 8 */
    { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C }, /* 9 */
};

static const unsigned char font_letters[26][7] = {
    { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }, /* A */
    { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E }, /* B */
    { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E }, /* C */
    { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E }, /* D */
    { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F }, /* E */
    { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 }, /* F */
    { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F }, /* G */
    { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }, /* H */
    { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E }, /* I */
    { 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C }, /* J */
    { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }, /* K */
    { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F }, /* L */
    { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 }, /* M */
    { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 }, /* N */
    { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }, /* O */
    { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 }, /* P */
    { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D }, /* Q */
    { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 }, /* R */
    { 0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E }, /* S */
    { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }, /* T */
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }, /* U */
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 }, /* V */
    { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A }, /* W */
    { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 }, /* X */
    { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 }, /* Y */
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F }, /* Z */
};

static void render_char(SDL_Renderer* ren, char ch, int x, int y,
    int scale, int r, int g, int b) {
    const unsigned char* glyph = NULL;

    if (ch >= '0' && ch <= '9')
        glyph = font_digits[ch - '0'];
    else if (ch >= 'A' && ch <= 'Z')
        glyph = font_letters[ch - 'A'];
    else if (ch >= 'a' && ch <= 'z')
        glyph = font_letters[ch - 'a'];
    else
        return;

    SDL_SetRenderDrawColor(ren, (Uint8)r, (Uint8)g, (Uint8)b, 255);

    for (int row = 0; row < 7; row++) {
        unsigned char bits = glyph[row];
        for (int col = 0; col < 5; col++) {
            if (bits & (0x10 >> col))
                fill_rect(ren, x + col * scale, y + row * scale,
                    scale, scale);
        }
    }
}

static void render_text(SDL_Renderer* ren, const char* text,
    int x, int y, int scale, int r, int g, int b) {
    int cursor_x = x;
    for (int i = 0; text[i]; i++) {
        if (text[i] == ' ') { cursor_x += 4 * scale; continue; }
        render_char(ren, text[i], cursor_x, y, scale, r, g, b);
        cursor_x += 6 * scale;
    }
}

static void render_number(SDL_Renderer* ren, int num, int x, int y,
    int scale, int r, int g, int b) {
    char buf[12];
    sprintf(buf, "%d", num);
    render_text(ren, buf, x, y, scale, r, g, b);
}

/* ── Renderizar entidades del juego ─────────────────────────────────── */

static void render_player(SDL_Renderer* ren, GameState* gs) {
    SDL_SetRenderDrawColor(ren, 50, 255, 50, 255);
    fill_rect(ren, gs->player_x, PLAYER_Y, PLAYER_W, PLAYER_H);
    /* Cañón */
    fill_rect(ren, gs->player_x + PLAYER_W / 2 - 3, PLAYER_Y - 8, 6, 10);
}

static void render_aliens(SDL_Renderer* ren, GameState* gs) {
    int frame = gs->alien_anim_frame;

    for (int i = 0; i < ALIEN_TOTAL; i++) {
        if (!gs->aliens[i].alive) continue;

        int type = gs->aliens[i].type;
        int alien_x = gs->aliens[i].x;
        int alien_y = gs->aliens[i].y;

        SDL_SetRenderDrawColor(ren,
            (Uint8)alien_colors[type][0],
            (Uint8)alien_colors[type][1],
            (Uint8)alien_colors[type][2], 255);

        /* Cuerpo principal */
        fill_rect(ren, alien_x + 2, alien_y + 4, ALIEN_W - 4, ALIEN_H - 8);

        /* Detalles visuales según tipo y frame de animación */
        if (type == 0) {
            /* Squid: antenas + patas que se alternan */
            fill_rect(ren, alien_x + 4, alien_y, 3, 6);
            fill_rect(ren, alien_x + ALIEN_W - 7, alien_y, 3, 6);
            int spread = frame ? 6 : 2;
            fill_rect(ren, alien_x + spread, alien_y + ALIEN_H - 6, 4, 6);
            fill_rect(ren, alien_x + ALIEN_W - spread - 4,
                alien_y + ALIEN_H - 6, 4, 6);
        }
        else if (type == 1) {
            /* Crab: extensiones laterales */
            int ext = frame ? 4 : 0;
            fill_rect(ren, alien_x - ext, alien_y + 6, 4 + ext, 4);
            fill_rect(ren, alien_x + ALIEN_W - 4, alien_y + 6, 4 + ext, 4);
            fill_rect(ren, alien_x + 4, alien_y + ALIEN_H - 4,
                ALIEN_W - 8, 4);
        }
        else {
            /* Octopus: tentáculos */
            int off = frame ? 3 : 0;
            for (int t = 0; t < 4; t++) {
                int tx = alien_x + 3 + t * 7 + (t % 2 ? off : -off);
                fill_rect(ren, tx, alien_y + ALIEN_H - 5, 3, 5);
            }
        }

        /* Ojos (pixeles oscuros) */
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        fill_rect(ren, alien_x + 8, alien_y + 8, 3, 3);
        fill_rect(ren, alien_x + ALIEN_W - 11, alien_y + 8, 3, 3);
    }
}

static void render_bullets(SDL_Renderer* ren, GameState* gs) {
    /* Balas del jugador: blancas */
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (!gs->player_bullets[i].active) continue;
        fill_rect(ren, gs->player_bullets[i].x,
            gs->player_bullets[i].y, BULLET_W, BULLET_H);
    }

    /* Balas enemigas: rojas */
    SDL_SetRenderDrawColor(ren, 255, 60, 60, 255);
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (!gs->alien_bullets[i].active) continue;
        fill_rect(ren, gs->alien_bullets[i].x,
            gs->alien_bullets[i].y, BULLET_W, BULLET_H);
    }
}

static void render_shields(SDL_Renderer* ren, GameState* gs) {
    SDL_SetRenderDrawColor(ren, 40, 200, 40, 255);

    for (int s = 0; s < SHIELD_COUNT; s++) {
        int sx = gs->shields[s].x;
        int sy = gs->shields[s].y;

        for (int row = 0; row < SHIELD_ROWS; row++) {
            for (int col = 0; col < SHIELD_COLS; col++) {
                int idx = row * SHIELD_COLS + col;
                if (!gs->shields[s].blocks[idx]) continue;

                fill_rect(ren,
                    sx + col * SHIELD_BLOCK_W,
                    sy + row * SHIELD_BLOCK_H,
                    SHIELD_BLOCK_W - 1, SHIELD_BLOCK_H - 1);
            }
        }
    }
}

static void render_hud(SDL_Renderer* ren, GameState* gs) {
    render_text(ren, "SCORE", 20, 10, 2, 255, 255, 255);
    render_number(ren, gs->score, 20, 30, 2, 50, 255, 50);

    render_text(ren, "HI", SCREEN_W / 2 - 40, 10, 2, 255, 255, 255);
    render_number(ren, gs->high_score, SCREEN_W / 2 + 10, 10, 2,
        255, 255, 255);

    render_text(ren, "LIVES", SCREEN_W - 180, 10, 2, 255, 255, 255);

    SDL_SetRenderDrawColor(ren, 50, 255, 50, 255);
    for (int i = 0; i < gs->player_lives; i++) {
        fill_rect(ren, SCREEN_W - 180 + i * 25, 30, 18, 10);
        fill_rect(ren, SCREEN_W - 180 + i * 25 + 6, 25, 5, 7);
    }

    /* Línea divisoria inferior */
    SDL_SetRenderDrawColor(ren, 50, 255, 50, 255);
    fill_rect(ren, 0, SCREEN_H - 20, SCREEN_W, 2);
}

/* ── Pantallas de estado ────────────────────────────────────────────── */

static void render_menu(SDL_Renderer* ren) {
    render_text(ren, "SPACE INVADERS",
        SCREEN_W / 2 - 14 * 6 * 3 / 2, SCREEN_H / 2 - 80,
        3, 50, 255, 50);

    render_text(ren, "ASM X86 EDITION",
        SCREEN_W / 2 - 15 * 6 * 2 / 2, SCREEN_H / 2 - 30,
        2, 50, 255, 255);

    render_text(ren, "PRESS ENTER TO START",
        SCREEN_W / 2 - 20 * 6 * 2 / 2, SCREEN_H / 2 + 40,
        2, 255, 255, 255);

    /* Tabla de puntaje */
    render_text(ren, "SCORE TABLE",
        SCREEN_W / 2 - 11 * 6 * 2 / 2, SCREEN_H / 2 + 100,
        2, 255, 255, 255);

    int ty = SCREEN_H / 2 + 130;
    for (int t = 0; t < 3; t++) {
        SDL_SetRenderDrawColor(ren,
            (Uint8)alien_colors[t][0],
            (Uint8)alien_colors[t][1],
            (Uint8)alien_colors[t][2], 255);
        fill_rect(ren, SCREEN_W / 2 - 80, ty + t * 25, 16, 12);

        char pts[8];
        sprintf(pts, "%d PTS", t == 0 ? 30 : (t == 1 ? 20 : 10));
        render_text(ren, pts, SCREEN_W / 2 - 50, ty + t * 25, 2,
            alien_colors[t][0], alien_colors[t][1],
            alien_colors[t][2]);
    }
}

static void render_gameover(SDL_Renderer* ren, GameState* gs) {
    render_text(ren, "GAME OVER",
        SCREEN_W / 2 - 9 * 6 * 4 / 2, SCREEN_H / 2 - 60,
        4, 255, 40, 40);

    render_text(ren, "SCORE", SCREEN_W / 2 - 70, SCREEN_H / 2 + 10,
        2, 255, 255, 255);
    render_number(ren, gs->score, SCREEN_W / 2 + 10, SCREEN_H / 2 + 10,
        2, 50, 255, 50);

    render_text(ren, "PRESS ENTER TO RESTART",
        SCREEN_W / 2 - 22 * 6 * 2 / 2, SCREEN_H / 2 + 60,
        2, 255, 255, 255);
}

static void render_win(SDL_Renderer* ren, GameState* gs) {
    render_text(ren, "YOU WIN",
        SCREEN_W / 2 - 7 * 6 * 4 / 2, SCREEN_H / 2 - 60,
        4, 50, 255, 50);

    render_text(ren, "SCORE", SCREEN_W / 2 - 70, SCREEN_H / 2 + 10,
        2, 255, 255, 255);
    render_number(ren, gs->score, SCREEN_W / 2 + 10, SCREEN_H / 2 + 10,
        2, 50, 255, 50);

    render_text(ren, "PRESS ENTER TO RESTART",
        SCREEN_W / 2 - 22 * 6 * 2 / 2, SCREEN_H / 2 + 60,
        2, 255, 255, 255);
}


/* ════════════════════════════════════════════════════════════════════════════
 *  PARTE 5: GAME LOOP PRINCIPAL
 * ════════════════════════════════════════════════════════════════════════════
 *  Ahora el loop principal es MÍNIMO: solo hace las llamadas a SDL
 *  que son obligatoriamente C (SDL_PollEvent, SDL_Render*, SDL_Delay)
 *  y delega TODA la lógica de decisión a funciones ASM:
 *
 *    asm_check_frame_timing()   → ¿dormir este frame? (CMP + SUB)
 *    asm_process_frame_input()  → interpretar teclas según estado (CMP+JMP)
 *    asm_restart_game()         → transición de reinicio (CMP + MAX)
 *    asm_game_tick()            → orquestar toda la lógica (dispatch ASM)
 *    asm_get_render_flags()     → decidir qué dibujar (cascada CMP+JMP)
 *
 *  El patrón resultante: C es el "hardware" (SDL), ASM es el "firmware"
 *  (toda la lógica de control). C no toma ninguna decisión.
 * ════════════════════════════════════════════════════════════════════════════ */

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    /* ── Inicializar SDL3 (obligatoriamente C — API externa) ── */
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Error SDL_Init: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Space Invaders  |  MASM x86 + C/SDL3",
        SCREEN_W, SCREEN_H, 0
    );
    if (!window) {
        SDL_Log("Error SDL_CreateWindow: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Error SDL_CreateRenderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderVSync(renderer, 1);

    /* ── Estado del juego (inicializado por ASM) ── */
    GameState gs;
    asm_init_game(&gs);
    gs.rng_state ^= (unsigned int)SDL_GetTicks();

    /* ── Variables de teclas crudas ──
       Estas son "registros de hardware" que SDL escribe.
       La INTERPRETACIÓN de qué significan la hace ASM.             */
    int key_left = 0, key_right = 0, key_shoot = 0;
    int key_enter_pressed = 0;

    /* ── Game loop ── */
    int running = 1;
    Uint64 last_tick = SDL_GetTicks();

    while (running) {

        /* ══ PASO 1: Frame timing (decisión en ASM) ══════════════════ */
        Uint64 now = SDL_GetTicks();
        unsigned int elapsed = (unsigned int)(now - last_tick);
        int should_delay, delay_ms;

        asm_check_frame_timing(elapsed, &should_delay, &delay_ms);

        if (should_delay)
            SDL_Delay((Uint32)delay_ms);    /* SDL_Delay es C (API)      */

        last_tick = SDL_GetTicks();

        /* ══ PASO 2: Leer eventos SDL (obligatoriamente C — API) ═════
           Solo almacenamos el estado crudo de las teclas.
           NO tomamos decisiones aquí — eso es trabajo de ASM.          */
        key_enter_pressed = 0;  /* Enter es un evento puntual, no held */

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = 0;
            }
            else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE)  running = 0;
                if (e.key.key == SDLK_LEFT || e.key.key == SDLK_A)
                    key_left = 1;
                if (e.key.key == SDLK_RIGHT || e.key.key == SDLK_D)
                    key_right = 1;
                if (e.key.key == SDLK_SPACE)
                    key_shoot = 1;
                if (e.key.key == SDLK_RETURN && !e.key.repeat)
                    key_enter_pressed = 1;
            }
            else if (e.type == SDL_EVENT_KEY_UP) {
                if (e.key.key == SDLK_LEFT || e.key.key == SDLK_A)
                    key_left = 0;
                if (e.key.key == SDLK_RIGHT || e.key.key == SDLK_D)
                    key_right = 0;
                if (e.key.key == SDLK_SPACE)
                    key_shoot = 0;
            }
        }

        /* ══ PASO 3: Procesar input (máquina de estados ASM) ═════════
           ASM interpreta las teclas según el estado actual del juego.
           Esto reemplaza el switch(key) + if(state) que antes era C.  */
        int move_left, move_right, shoot, restart;
        asm_process_frame_input(&gs,
            key_left, key_right,
            key_shoot, key_enter_pressed,
            &move_left, &move_right,
            &shoot, &restart);

        /* Transición de reinicio (lógica de MAX score en ASM) */
        if (restart) {
            asm_restart_game(&gs);
            gs.rng_state ^= (unsigned int)SDL_GetTicks();
        }

        /* ══ PASO 4: Tick de lógica (máquina de estados ASM) ═════════
           ASM decide si ejecutar la lógica (solo en STATE_PLAYING),
           evalúa el flag de disparo, y orquesta todas las subfunciones.
           C no toma ninguna decisión aquí.                             */
        asm_game_tick(&gs, move_left, move_right, shoot);

        /* Mezclar ticks para variación del RNG */
        gs.rng_state ^= (unsigned int)SDL_GetTicks();

        /* ══ PASO 5: Renderizado (decisión ASM, ejecución SDL3) ══════
           ASM decide QUÉ dibujar con una cascada CMP+JMP.
           C solo ejecuta las llamadas SDL correspondientes.            */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        int rf_scene, rf_menu, rf_gameover, rf_win;
        asm_get_render_flags(&gs, &rf_scene, &rf_menu,
            &rf_gameover, &rf_win);

        if (rf_menu) {
            render_menu(renderer);
        }
        if (rf_scene) {
            render_player(renderer, &gs);
            render_aliens(renderer, &gs);
            render_bullets(renderer, &gs);
            render_shields(renderer, &gs);
            render_hud(renderer, &gs);
        }
        if (rf_win) {
            render_shields(renderer, &gs);
            render_hud(renderer, &gs);
            render_win(renderer, &gs);
        }
        if (rf_gameover) {
            render_gameover(renderer, &gs);
        }

        SDL_RenderPresent(renderer);
    }

    /* ── Limpieza (obligatoriamente C — API SDL3) ── */
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}