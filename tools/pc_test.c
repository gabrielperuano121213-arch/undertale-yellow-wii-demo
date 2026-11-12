/* Prueba en PC (sin Wii): ejecuta el nucleo del juego con una "partitura" de botones y guarda capturas.
 *
 *   gcc -O2 -o pc_test tools/pc_test.c source/game.c source/gfx.c source/assets.c -Isource
 *   ./pc_test 40: 1:Z 10: snap:titulo 20:R 5:Z snap:dialogo
 *
 * Cada argumento es "N:TECLAS" (mantener TECLAS durante N ticks; TECLAS = U D L R Z X C) o "snap:nombre".
 * Un tick = 1/30 s. Para pulsar un boton una sola vez usa "1:Z 1:".
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "gfx.h"

static void snap(const char *name)
{
    char path[256];
    snprintf(path, sizeof path, "%s.ppm", name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return; }
    fprintf(f, "P6\n%d %d\n255\n", SCR_W, SCR_H);
    for (int i = 0; i < SCR_W * SCR_H; i++) {
        uint32_t c = g_palette[g_fb[i]];
        fputc((c >> 16) & 255, f); fputc((c >> 8) & 255, f); fputc(c & 255, f);
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    game_init();
    for (int a = 1; a < argc; a++) {
        if (!strncmp(argv[a], "snap:", 5)) { snap(argv[a] + 5); continue; }
        if (!strncmp(argv[a], "fuzz:", 5)) {     /* fuzz:TICKS:SEMILLA -> pulsaciones aleatorias (para sanitizadores) */
            long n = atol(argv[a] + 5);
            char *c2 = strchr(argv[a] + 5, ':');
            unsigned seed = c2 ? (unsigned)atoi(c2 + 1) : 1u;
            uint32_t keys = 0;
            for (long i = 0; i < n; i++) {
                seed = seed * 1664525u + 1013904223u;
                if ((seed >> 24) % 5 == 0) keys = (seed >> 8) & 0x7F;
                if ((seed >> 20) % 9 == 0) keys &= ~(uint32_t)(IN_Z | IN_X | IN_C);
                game_tick(keys);
            }
            continue;
        }
        char *colon = strchr(argv[a], ':');
        if (!colon) continue;
        int n = atoi(argv[a]);
        uint32_t keys = 0;
        for (char *p = colon + 1; *p; p++)
            switch (*p) {
            case 'U': keys |= IN_UP; break;    case 'D': keys |= IN_DOWN; break;
            case 'L': keys |= IN_LEFT; break;  case 'R': keys |= IN_RIGHT; break;
            case 'Z': keys |= IN_Z; break;     case 'X': keys |= IN_X; break;
            case 'C': keys |= IN_C; break;
            }
        for (int i = 0; i < n; i++) game_tick(keys);
    }
    return 0;
}
