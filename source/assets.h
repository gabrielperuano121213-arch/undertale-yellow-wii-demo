/* GENERADO por tools/extract_assets.py - no editar a mano */
#ifndef ASSETS_H
#define ASSETS_H
#include <stdint.h>

typedef struct { uint16_t w, h; int16_t ox, oy; const uint8_t *px; } Img;   /* px: indices de paleta, 255 = transparente */
typedef struct { uint8_t w, h; int8_t xo; uint8_t adv; uint16_t off; } Glyph;
typedef struct { const Glyph *g; const uint16_t *rows; uint8_t first, last, lineh; } Font;   /* ASCII 32..126 */

extern const uint32_t g_palette[256];
extern const Img img_clover_down[4];
extern const Img img_clover_up[4];
extern const Img img_clover_left[2];
extern const Img img_clover_right[2];
extern const Img img_soul[2];
extern const Img img_shot[5];
extern const Img img_btn_fight[2];
extern const Img img_btn_act[2];
extern const Img img_btn_item[2];
extern const Img img_btn_mercy[2];
extern const Img img_dummy;
extern const Img img_room;
extern const Img img_splash;
extern const Font font_main, font_stats;

#define START_X 70
#define START_Y 110
#define FLOOR_X0 20
#define FLOOR_Y0 40
#define FLOOR_X1 300
#define FLOOR_Y1 220
#define SIGN_X 250
#define SIGN_Y 74
#define DUMMY_X 228
#define DUMMY_Y 152
#define FLOWERS_X 112
#define FLOWERS_Y 150
#define FLOWERS_W 60
#define FLOWERS_H 40
#endif
