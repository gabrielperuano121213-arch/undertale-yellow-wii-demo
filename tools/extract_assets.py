#!/usr/bin/env python3
"""
Extrae de tu copia de Undertale Yellow (data.win + splash.png) los recursos que usa el
demake de Wii y genera source/assets.c y source/assets.h.

  python3 tools/extract_assets.py --data data.win --splash splash.png --out source --preview preview

Requisitos: Python 3 + Pillow (pip install pillow). No modifica tus archivos del juego.
Probado con Undertale Yellow (GameMaker 2023, WAD 17, texturas "2zoq" = QOI+BZip2).
"""
import argparse, bz2, mmap, os, struct, sys
from PIL import Image


# ----------------------------------------------------------------------------- lector data.win
class Win:
    def __init__(self, path):
        f = open(path, 'rb')
        self.mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        mm = self.mm
        if mm[0:4] != b'FORM':
            sys.exit('data.win no es un archivo GameMaker (FORM)')
        self.chunks = {}
        o = 8
        end = 8 + self.u32(4)
        while o < end:
            self.chunks[mm[o:o+4].decode('latin1')] = (o + 8, self.u32(o+4))
            o += 8 + self.u32(o+4)
        n = self.u32(self.chunks['SPRT'][0])
        self.sprites = [self.u32(self.chunks['SPRT'][0] + 4 + 4*i) for i in range(n)]
        self.names = {self.string(self.u32(p)): i for i, p in enumerate(self.sprites)}
        t = self.chunks['TXTR'][0]
        self.txtr = [self.u32(t + 4 + 4*i) for i in range(self.u32(t))]
        self.pages = {}

    def u32(self, o): return struct.unpack_from('<I', self.mm, o)[0]
    def i32(self, o): return struct.unpack_from('<i', self.mm, o)[0]
    def u16(self, o): return struct.unpack_from('<H', self.mm, o)[0]
    def string(self, p): return self.mm[p:p + self.u32(p-4)].decode('utf8', 'replace')

    # --- textura: "2zoq" = cabecera + BZip2( "fioq" = variante QOI de GameMaker )
    def page(self, i):
        if i in self.pages:
            return self.pages[i]
        e = self.txtr[i]
        size, blob = self.u32(e+8), self.u32(e+24)
        b = bytes(self.mm[blob:blob+size])
        if b[:4] == b'\x89PNG':
            import io
            im = Image.open(io.BytesIO(b)).convert('RGBA')
        else:
            raw = bz2.BZ2Decompressor().decompress(b[12:]) if b[:4] == b'2zoq' else b
            if raw[:4] != b'fioq':
                sys.exit('formato de textura desconocido: %r' % raw[:4])
            w, h = raw[4] | raw[5] << 8, raw[6] | raw[7] << 8
            im = Image.frombytes('RGBA', (w, h), bytes(qoi_gm(raw, w*h)))
        self.pages[i] = im
        return im

    def tpag(self, p): return struct.unpack_from('<10Hh', self.mm, p)

    def sprite(self, name):
        """Devuelve (lista de lienzos RGBA w x h, (origenX, origenY))."""
        p = self.sprites[self.names[name]]
        w, h, ox, oy = self.u32(p+4), self.u32(p+8), self.i32(p+48), self.i32(p+52)
        q = p + 56
        x = self.i32(q)
        if x == -1:   # sprite GMS2 "especial": version, tipo, velocidad(8) [+seq][+9slice]
            ver = self.i32(q+4)
            q += 12 + 8 + (4 if ver >= 2 else 0) + (4 if ver >= 3 else 0)
            x = self.u32(q)
        q += 4
        frames = []
        for i in range(x):
            sx, sy, sw, sh, tx, ty, _, _, _, _, pg = self.tpag(self.u32(q + 4*i))
            cv = Image.new('RGBA', (w, h), (0, 0, 0, 0))
            cv.alpha_composite(self.page(pg).crop((sx, sy, sx+sw, sy+sh)), (tx, ty))
            frames.append(cv)
        return frames, (ox, oy)

    def font(self, name):
        """Devuelve ({codigo: (filas_bits, ancho, desplazamiento_x, avance)}, alto_de_linea)."""
        o, _ = self.chunks['FONT']
        for i in range(self.u32(o)):
            p = self.u32(o + 4 + 4*i)
            if self.string(self.u32(p)) != name:
                continue
            sx, sy, _, _, _, _, _, _, _, _, pg = self.tpag(self.u32(p+28))
            page = self.page(pg)
            out = {}
            lineh = 0
            for k in range(self.u32(p+52)):
                g = self.u32(p + 56 + 4*k)
                ch, x, y, w, h = struct.unpack_from('<5H', self.mm, g)
                sh, off = struct.unpack_from('<2h', self.mm, g+10)
                rows = []
                for yy in range(h):
                    bits = 0
                    for xx in range(w):
                        if page.getpixel((sx+x+xx, sy+y+yy))[3] >= 128:
                            bits |= 0x8000 >> xx
                    rows.append(bits)
                out[ch] = (rows, w, off, sh)
                lineh = max(lineh, h)
            return out, lineh
        sys.exit('fuente no encontrada: ' + name)


def qoi_gm(raw, npix):
    """Decodificador de la variante QOI de GameMaker (deducida del runner de Windows)."""
    out = bytearray(npix * 4)
    idx = [(0, 0, 0, 0)] * 64
    r = g = b = 0
    a = 255
    p, o, n = 12, 0, len(raw)
    s = lambda v, bits: (v ^ (1 << (bits-1))) - (1 << (bits-1))
    while o < npix and p < n:
        t = raw[p]
        p += 1
        extra = 0
        if t < 0x40:
            r, g, b, a = idx[t]
        elif t < 0x80:
            extra = t & 0x1F
            if t & 0x20:
                extra = ((extra << 8) | raw[p]) + 0x20
                p += 1
        else:
            if t < 0xC0:
                r = (r + s((t >> 4) & 3, 2)) & 255
                g = (g + s((t >> 2) & 3, 2)) & 255
                b = (b + s(t & 3, 2)) & 255
            elif t < 0xE0:
                t2 = raw[p]
                p += 1
                r = (r + s(t & 0x1F, 5)) & 255
                g = (g + s(t2 >> 4, 4)) & 255
                b = (b + s(t2 & 15, 4)) & 255
            elif t < 0xF0:
                v = (t << 16) | (raw[p] << 8) | raw[p+1]
                p += 2
                r = (r + s((v >> 15) & 31, 5)) & 255
                g = (g + s((v >> 10) & 31, 5)) & 255
                b = (b + s((v >> 5) & 31, 5)) & 255
                a = (a + s(v & 31, 5)) & 255
            else:
                if t & 8: r = raw[p]; p += 1
                if t & 4: g = raw[p]; p += 1
                if t & 2: b = raw[p]; p += 1
                if t & 1: a = raw[p]; p += 1
            idx[(r ^ g ^ b ^ a) & 63] = (r, g, b, a)
        c = min(extra + 1, npix - o)
        out[o*4:(o+c)*4] = bytes((r, g, b, a)) * c
        o += c
    return out


# ----------------------------------------------------------------------------- utilidades de imagen
def down2(im):
    """Reduce a la mitad un sprite dibujado al doble (elige la fase que mejor coincide)."""
    w, h = im.size
    best = None
    for py in (0, 1):
        for px in (0, 1):
            small = Image.new('RGBA', (w // 2, h // 2))
            small.putdata([im.getpixel((min(2*x+px, w-1), min(2*y+py, h-1))) for y in range(h // 2) for x in range(w // 2)])
            up = small.resize((small.size[0]*2, small.size[1]*2), Image.NEAREST)
            sc = sum(1 for p, q in zip(up.getdata(), im.crop((0, 0, up.size[0], up.size[1])).getdata()) if p == q)
            if best is None or sc > best[0]:
                best = (sc, small)
    return best[1]


class Palette:
    FIXED = [(0, 0, 0), (255, 255, 255), (255, 255, 0), (255, 0, 0), (252, 166, 0),
             (0, 255, 0), (128, 128, 128), (64, 64, 64), (192, 192, 192)]

    def __init__(self):
        self.cols = list(self.FIXED)
        self.map = {c: i for i, c in enumerate(self.cols)}

    def index(self, c):
        if c[3] < 128:
            return 255
        k = c[:3]
        if k not in self.map:
            if len(self.cols) >= 255:
                sys.exit('Demasiados colores (>255); hay que cuantizar')
            self.map[k] = len(self.cols)
            self.cols.append(k)
        return self.map[k]

    def convert(self, im):
        return bytes(self.index(c) for c in im.convert('RGBA').getdata())


# ----------------------------------------------------------------------------- sala de las ruinas
LAYOUT = dict(floor=(20, 40, 300, 220), start=(70, 110), sign=(250, 74), flowers=(112, 150, 60, 40),
              dummy=(228, 152), leaves=(40, 188))


def build_room(win):
    ts = win.sprite('ts_ruins')[0][0]
    crop = lambda x, y, w, h: ts.crop((x, y, x+w, y+h))
    room = Image.new('RGBA', (320, 240), (0, 0, 0, 255))
    floor = [[crop(0, 60, 40, 40), crop(40, 60, 40, 40)], [crop(0, 100, 40, 40), crop(40, 100, 40, 40)]]
    for by in range(0, 240, 40):
        for bx in range(0, 320, 40):
            room.alpha_composite(floor[(by // 40) % 2][(bx // 40) % 2], (bx, by))
    wall = [crop(40, 0, 20, 20), crop(40, 20, 20, 20)]     # ladrillo oscuro (2 filas)
    for x in range(0, 320, 20):
        room.alpha_composite(wall[0], (x, 0))
        room.alpha_composite(wall[1], (x, 20))
        room.alpha_composite(wall[0], (x, 220))
    for y in range(40, 220, 20):
        room.alpha_composite(wall[1], (0, y))
        room.alpha_composite(wall[1], (300, y))
    L = LAYOUT
    room.alpha_composite(crop(16, 235, 28, 29), (L['leaves'][0] - 14, L['leaves'][1] - 29))   # hojas rojas
    room.alpha_composite(crop(100, 240, 60, 40), (L['flowers'][0], L['flowers'][1]))          # flores doradas
    room.alpha_composite(crop(0, 280, 20, 20), (L['sign'][0] - 10, L['sign'][1] - 20))        # letrero
    return room


# ----------------------------------------------------------------------------- generacion de C
def carr(data, per=24):
    return ',\n'.join(','.join('%d' % v for v in data[i:i+per]) for i in range(0, len(data), per))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--data', required=True)
    ap.add_argument('--splash', required=True)
    ap.add_argument('--out', default='source')
    ap.add_argument('--preview', default='preview')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    os.makedirs(a.preview, exist_ok=True)
    win = Win(a.data)
    pal = Palette()
    imgs = []   # (nombre_c, indice, ancho, alto, ox, oy, bytes)

    def add(cname, frames, origin=(0, 0), half=False):
        for k, fr in enumerate(frames):
            if half:
                fr = down2(fr)
            ox, oy = (origin[0] // 2, origin[1] // 2) if half else origin
            imgs.append((cname, k, fr.size[0], fr.size[1], ox, oy, pal.convert(fr)))
            fr.save('%s/%s_%d.png' % (a.preview, cname, k))

    for d, n in (('down', 4), ('up', 4), ('left', 2), ('right', 2)):
        fr, o = win.sprite('spr_pl_' + d)
        add('img_clover_' + d, fr[:n], o)
    fr, o = win.sprite('spr_heart_yellow_up')
    add('img_soul', fr[:2], o, half=True)
    fr, o = win.sprite('spr_heart_yellow_shot')
    add('img_shot', fr[:5], o, half=True)
    for nm, sp in (('fight', 'spr_fight_yellow'), ('act', 'spr_act'), ('item', 'spr_item'), ('mercy', 'spr_mercy')):
        fr, o = win.sprite(sp)
        add('img_btn_' + nm, fr[:2], o, half=True)
    fr, o = win.sprite('spr_dummy_training_dummy')
    add('img_dummy', fr[:1], o, half=True)
    add('img_room', [build_room(win)])
    sp = Image.open(a.splash).convert('RGBA')
    if sp.size == (640, 480):
        sp = sp.resize((320, 240), Image.NEAREST)
    add('img_splash', [sp])

    fonts = []
    for cname, gm_name in (('font_main', 'fnt_main'), ('font_stats', 'fnt_stats')):
        glyphs, lineh = win.font(gm_name)
        rows, gl = [], []
        for c in range(32, 127):
            r, w, off, adv = glyphs.get(c, ([], 0, 0, glyphs[32][3]))
            gl.append('{%d,%d,%d,%d,%d}' % (w, len(r), off, adv, len(rows)))
            rows += r
        fonts.append((cname, gl, rows, lineh))

    L = LAYOUT
    names = {}
    for n, k, *_ in imgs:
        names[n] = max(names.get(n, 0), k + 1)
    with open(a.out + '/assets.h', 'w') as f:
        f.write('/* GENERADO por tools/extract_assets.py - no editar a mano */\n#ifndef ASSETS_H\n#define ASSETS_H\n#include <stdint.h>\n\n')
        f.write('typedef struct { uint16_t w, h; int16_t ox, oy; const uint8_t *px; } Img;   /* px: indices de paleta, 255 = transparente */\n')
        f.write('typedef struct { uint8_t w, h; int8_t xo; uint8_t adv; uint16_t off; } Glyph;\n')
        f.write('typedef struct { const Glyph *g; const uint16_t *rows; uint8_t first, last, lineh; } Font;   /* ASCII 32..126 */\n\n')
        f.write('extern const uint32_t g_palette[256];\n')
        for n, c in names.items():
            f.write('extern const Img %s%s;\n' % (n, '[%d]' % c if c > 1 else ''))
        f.write('extern const Font font_main, font_stats;\n\n')
        f.write('#define START_X %d\n#define START_Y %d\n' % L['start'])
        f.write('#define FLOOR_X0 %d\n#define FLOOR_Y0 %d\n#define FLOOR_X1 %d\n#define FLOOR_Y1 %d\n' % L['floor'])
        f.write('#define SIGN_X %d\n#define SIGN_Y %d\n#define DUMMY_X %d\n#define DUMMY_Y %d\n' % (L['sign'] + L['dummy']))
        f.write('#define FLOWERS_X %d\n#define FLOWERS_Y %d\n#define FLOWERS_W %d\n#define FLOWERS_H %d\n#endif\n' % L['flowers'])
    with open(a.out + '/assets.c', 'w') as f:
        f.write('/* GENERADO por tools/extract_assets.py a partir de tu copia de Undertale Yellow */\n#include "assets.h"\n\n')
        f.write('const uint32_t g_palette[256] = {\n' + ',\n'.join('0x%02X%02X%02X' % c for c in pal.cols) + (',\n0x000000' * (256 - len(pal.cols))) + '\n};\n\n')
        for n, k, w, h, ox, oy, data in imgs:
            f.write('static const uint8_t px_%s_%d[%d] = {\n%s};\n' % (n, k, len(data), carr(data)))
        for n, c in names.items():
            es = ['{%d,%d,%d,%d,px_%s_%d}' % (w, h, ox, oy, n, k) for nn, k, w, h, ox, oy, _ in imgs if nn == n]
            f.write('const Img %s%s = %s;\n' % (n, '[%d]' % c if c > 1 else '', ('{' + ','.join(es) + '}') if c > 1 else es[0]))
        for cname, gl, rows, lineh in fonts:
            f.write('\nstatic const Glyph gl_%s[95] = {%s};\nstatic const uint16_t rows_%s[%d] = {%s};\n'
                    % (cname, ','.join(gl), cname, len(rows), ','.join(str(r) for r in rows)))
            f.write('const Font %s = {gl_%s, rows_%s, 32, 126, %d};\n' % (cname, cname, cname, lineh))

    # icono para el Homebrew Channel (128x48)
    logo = Image.open(a.splash).convert('RGB').crop((70, 150, 570, 300)).resize((120, 36), Image.NEAREST)
    icon = Image.new('RGB', (128, 48), (0, 0, 0))
    icon.paste(logo, (4, 6))
    icon.save('icon.png')
    print('Paleta: %d colores | imagenes: %d | filas de glifos (main): %d' % (len(pal.cols), len(imgs), len(fonts[0][2])))


if __name__ == '__main__':
    main()
