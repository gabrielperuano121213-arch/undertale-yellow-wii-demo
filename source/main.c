/* Undertale Yellow - demake para Wii (Homebrew Channel)
 *
 * Capa de plataforma: inicializa el video, lee el Wiimote y muestra el framebuffer del juego.
 *
 *   Cruceta ......... moverse (Wiimote en horizontal; con + se alterna a vertical)
 *   Boton 2 ......... Z  (confirmar / disparar con el alma amarilla)
 *   Boton 1 ......... X  (cancelar / mover mas lento en batalla)
 *   Boton B ......... C  (menu)
 *   HOME ............ volver al Homebrew Channel
 */
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdlib.h>
#include <stdint.h>

#include "game.h"
#include "gfx.h"

static GXRModeObj *rmode;
static void *xfb[2];
static int cur_fb;
static uint32_t pal_yuy2[256];

/* RGB -> YCbYCr (BT.601). Como cada pixel del juego se duplica en horizontal, ambos pixeles
 * del par comparten color y no hay perdida de croma. */
static uint32_t make_yuy2(uint32_t rgb)
{
    int r = (int)((rgb >> 16) & 255), g = (int)((rgb >> 8) & 255), b = (int)(rgb & 255);
    int y  = ((  66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
    int cb = (( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
    int cr = (( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
    if (y < 16) y = 16;
    if (y > 235) y = 235;
    if (cb < 16) cb = 16;
    if (cb > 240) cb = 240;
    if (cr < 16) cr = 16;
    if (cr > 240) cr = 240;
    return ((uint32_t)y << 24) | ((uint32_t)cb << 16) | ((uint32_t)y << 8) | (uint32_t)cr;
}

/* 320x240 indexado -> XFB 640x480 (x2 en ambos ejes), centrado si el modo de video es mas alto (PAL) */
static void present(void)
{
    uint32_t *dst = (uint32_t *)xfb[cur_fb];
    const int stride = (int)VIDEO_PadFramebufferWidth(rmode->fbWidth) / 2;     /* u32 por linea */
    const int lines = rmode->xfbHeight;
    const int x0 = (stride - SCR_W) / 2 > 0 ? (stride - SCR_W) / 2 : 0;
    int y0 = (lines - 2 * SCR_H) / 2;
    if (y0 < 0) y0 = 0;
    const uint8_t *src = g_fb;
    for (int y = 0; y < SCR_H; y++) {
        if (y0 + 2 * y + 1 >= lines) break;
        uint32_t *l0 = dst + (y0 + 2 * y) * stride + x0;
        uint32_t *l1 = l0 + stride;
        for (int x = 0; x < SCR_W; x++) {
            uint32_t v = pal_yuy2[src[x]];
            l0[x] = v;
            l1[x] = v;
        }
        src += SCR_W;
    }
    VIDEO_SetNextFramebuffer(xfb[cur_fb]);
    VIDEO_Flush();
    cur_fb ^= 1;
}

/* Wiimote -> botones del juego. En horizontal (estilo NES) la cruceta gira 90 grados. */
static uint32_t map_buttons(uint32_t h, int sideways)
{
    uint32_t in = 0;
    if (sideways) {
        if (h & WPAD_BUTTON_RIGHT) in |= IN_UP;
        if (h & WPAD_BUTTON_LEFT)  in |= IN_DOWN;
        if (h & WPAD_BUTTON_UP)    in |= IN_LEFT;
        if (h & WPAD_BUTTON_DOWN)  in |= IN_RIGHT;
    } else {
        if (h & WPAD_BUTTON_UP)    in |= IN_UP;
        if (h & WPAD_BUTTON_DOWN)  in |= IN_DOWN;
        if (h & WPAD_BUTTON_LEFT)  in |= IN_LEFT;
        if (h & WPAD_BUTTON_RIGHT) in |= IN_RIGHT;
    }
    if (h & WPAD_BUTTON_2) in |= IN_Z;    /* Z = boton 2 */
    if (h & WPAD_BUTTON_1) in |= IN_X;    /* X = boton 1 */
    if (h & WPAD_BUTTON_B) in |= IN_C;    /* C = boton B */
    return in;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    VIDEO_Init();
    WPAD_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    VIDEO_Configure(rmode);
    xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    VIDEO_ClearFrameBuffer(rmode, xfb[0], COLOR_BLACK);
    VIDEO_ClearFrameBuffer(rmode, xfb[1], COLOR_BLACK);
    VIDEO_SetNextFramebuffer(xfb[0]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    for (int i = 0; i < 256; i++) pal_yuy2[i] = make_yuy2(g_palette[i]);

    game_init();

    int sideways = 1;
    int dirty = 1;
    uint32_t acc = 0;                             /* acumula pulsaciones cortas entre ticks */
    uint64_t next = ticks_to_microsecs(gettime());
    const uint64_t step = 1000000ULL / GAME_TICKS_PER_SEC;

    for (;;) {
        WPAD_ScanPads();
        uint32_t down = WPAD_ButtonsDown(0);
        if (down & WPAD_BUTTON_HOME) break;       /* vuelve al Homebrew Channel */
        if (down & WPAD_BUTTON_PLUS) sideways = !sideways;
        uint32_t in = map_buttons(WPAD_ButtonsHeld(0), sideways);
        acc |= in;

        uint64_t now = ticks_to_microsecs(gettime());
        int n = 0;
        while (now >= next && n < 4) {            /* el juego corre a 30 ticks/s, como Undertale */
            game_tick(acc);
            acc = in;
            next += step;
            n++;
            dirty = 1;
        }
        if (now >= next + 250000) next = now;     /* si nos atrasamos mucho, resincronizar */

        if (dirty) { present(); dirty = 0; }
        VIDEO_WaitVSync();
    }
    return 0;
}
