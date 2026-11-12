/* Renderizador 2D por software sobre un framebuffer indexado de 320x240.
 * Undertale dibuja sus habitaciones a 320x240 y las amplia x2 a 640x480: aqui se hace igual
 * (cada pixel se duplica al convertirlo al XFB de la Wii). */
#ifndef GFX_H
#define GFX_H
#include <stdint.h>
#include "assets.h"

#define SCR_W 320
#define SCR_H 240

/* indices fijos de paleta (ver tools/extract_assets.py) */
#define C_BLACK  0
#define C_WHITE  1
#define C_YELLOW 2
#define C_RED    3
#define C_ORANGE 4
#define C_GREEN  5
#define C_GRAY   6
#define C_DGRAY  7
#define C_LGRAY  8

extern uint8_t g_fb[SCR_W * SCR_H];

void gfx_clear(uint8_t c);
void gfx_rect(int x, int y, int w, int h, uint8_t c);
void gfx_img(const Img *im, int x, int y);        /* x,y = esquina superior izquierda */
void gfx_sprite(const Img *im, int x, int y);     /* x,y = punto de origen del sprite */
void gfx_text_n(const Font *f, int x, int y, const char *s, int maxch, uint8_t c, int scale);
void gfx_text(const Font *f, int x, int y, const char *s, uint8_t c, int scale);
int  gfx_text_w(const Font *f, const char *s, int scale);

#endif
