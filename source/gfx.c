#include <string.h>
#include "gfx.h"

uint8_t g_fb[SCR_W * SCR_H];

void gfx_clear(uint8_t c)
{
    memset(g_fb, c, sizeof g_fb);
}

void gfx_rect(int x, int y, int w, int h, uint8_t c)
{
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > SCR_W ? SCR_W : x + w, y1 = y + h > SCR_H ? SCR_H : y + h;
    if (x1 <= x0) return;
    for (int yy = y0; yy < y1; yy++)
        memset(&g_fb[yy * SCR_W + x0], c, (size_t)(x1 - x0));
}

void gfx_img(const Img *im, int x, int y)
{
    for (int j = 0; j < im->h; j++) {
        int yy = y + j;
        if (yy < 0 || yy >= SCR_H) continue;
        const uint8_t *src = im->px + j * im->w;
        uint8_t *dst = &g_fb[yy * SCR_W];
        for (int i = 0; i < im->w; i++) {
            int xx = x + i;
            if (xx < 0 || xx >= SCR_W) continue;
            if (src[i] != 255) dst[xx] = src[i];
        }
    }
}

void gfx_sprite(const Img *im, int x, int y)
{
    gfx_img(im, x - im->ox, y - im->oy);
}

void gfx_text_n(const Font *f, int x, int y, const char *s, int maxch, uint8_t c, int scale)
{
    int cx = x;
    for (int n = 0; s[n] && n < maxch; n++) {
        unsigned char ch = (unsigned char)s[n];
        if (ch == '\n') { cx = x; y += f->lineh * scale; continue; }
        if (ch < f->first || ch > f->last) ch = ' ';
        const Glyph *g = &f->g[ch - f->first];
        for (int r = 0; r < g->h; r++) {
            uint16_t bits = f->rows[g->off + r];
            for (int col = 0; col < g->w; col++)
                if (bits & (0x8000 >> col))
                    gfx_rect(cx + (g->xo + col) * scale, y + r * scale, scale, scale, c);
        }
        cx += g->adv * scale;
    }
}

void gfx_text(const Font *f, int x, int y, const char *s, uint8_t c, int scale)
{
    gfx_text_n(f, x, y, s, 1 << 30, c, scale);
}

int gfx_text_w(const Font *f, const char *s, int scale)
{
    int w = 0;
    for (; *s && *s != '\n'; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch < f->first || ch > f->last) ch = ' ';
        w += f->g[ch - f->first].adv * scale;
    }
    return w;
}
