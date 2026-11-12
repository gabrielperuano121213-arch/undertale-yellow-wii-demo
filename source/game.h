/* Nucleo del juego: logica + dibujo, sin dependencias de la consola (se prueba tambien en PC).
 * La capa de la Wii (main.c) solo traduce el Wiimote a estos botones y muestra g_fb en pantalla. */
#ifndef GAME_H
#define GAME_H
#include <stdint.h>

/* Undertale solo usa cruceta + 3 botones */
enum {
    IN_UP = 1, IN_DOWN = 2, IN_LEFT = 4, IN_RIGHT = 8,
    IN_Z = 16,   /* confirmar / disparar   -> Wiimote 2 */
    IN_X = 32,   /* cancelar / ir lento    -> Wiimote 1 */
    IN_C = 64    /* menu                   -> Wiimote B */
};

#define GAME_TICKS_PER_SEC 30     /* igual que Undertale */

void game_init(void);
void game_tick(uint32_t held);    /* actualiza y dibuja un frame en g_fb (gfx.h) */

#endif
