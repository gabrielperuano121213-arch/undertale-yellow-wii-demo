#!/usr/bin/env python3
"""
Verifica que una app para el Homebrew Channel (boot.dol + meta.xml + icon.png) sea valida.

  python3 tools/verificar.py <ruta>

<ruta> puede ser: la carpeta de la app (apps/UndertaleYellow), la raiz de la tarjeta SD (con apps/...),
la carpeta del proyecto, o directamente un boot.dol / boot.elf.
Solo usa la libreria estandar de Python 3. Sale con codigo 1 si hay algun FALLO.
"""
import os
import re
import struct
import sys
import xml.etree.ElementTree as ET

OK, BAD, WARN, INFO = "[ OK  ]", "[FALLO]", "[AVISO]", "[ i   ]"
errors = 0

MEM1 = (0x80000000, 0x81800000)      # MEM1 (24 MB), vista con cache
MEM2 = (0x90000000, 0x94000000)      # MEM2 (64 MB), vista con cache


def rep(kind, msg):
    global errors
    if kind == BAD:
        errors += 1
    print("%s %s" % (kind, msg))


def in_mem(addr, size):
    return any(lo <= addr and addr + size <= hi for lo, hi in (MEM1, MEM2))


# ----------------------------------------------------------------------------- boot.dol / boot.elf
def machine_name(m):
    return {3: "x86 (PC 32 bits)", 62: "x86-64 (PC)", 40: "ARM", 183: "ARM64", 20: "PowerPC",
            8: "MIPS"}.get(m, "desconocida (%d)" % m)


def check_elf(path, data):
    name = os.path.basename(path)
    cls, endian = data[4], data[5]
    fmt = '>H' if endian == 2 else '<H'
    machine = struct.unpack_from(fmt, data, 18)[0]
    if machine == 20 and cls == 1 and endian == 2:
        rep(OK, "%s es un ELF PowerPC de 32 bits big-endian (valido para el Homebrew Channel como boot.elf)." % name)
        if name.lower().endswith('.dol'):
            rep(BAD, "%s tiene extension .dol pero es un ELF: no es un DOL. Convierte con elf2dol o usa boot.elf." % name)
    else:
        rep(BAD, "%s es un ELF de %s, NO de Wii (PowerPC). Es un ejecutable de otra plataforma, posiblemente "
                 "renombrado: no funcionara en la consola." % (name, machine_name(machine)))


def check_dol(path):
    name = os.path.basename(path)
    data = open(path, 'rb').read()
    size = len(data)
    rep(INFO, "%s: %d bytes" % (name, size))
    if size < 0x100:
        rep(BAD, "%s mide menos de 256 bytes: no puede contener una cabecera DOL." % name)
        return
    if data[:4] == b'\x7fELF':
        check_elf(path, data)
        return
    if data[:2] == b'MZ':
        rep(BAD, "%s es un .exe/DLL de Windows, no un DOL." % name)
        return
    if data[:2] == b'PK':
        rep(BAD, "%s es un archivo ZIP: descomprimelo; no es un DOL." % name)
        return
    if data[:5] in (b'<!DOC', b'<html', b'<?xml') or data[:1] == b'{':
        rep(BAD, "%s parece texto/HTML/JSON (descarga fallida?), no un DOL." % name)
        return

    words = struct.unpack('>64I', data[:0x100])
    offs, addrs, sizes = words[0:18], words[18:36], words[36:54]
    bss_addr, bss_size, entry = words[54], words[55], words[56]
    secs = []
    bad_before = errors
    for i in range(18):
        if sizes[i] == 0:
            continue
        kind = "text%d" % i if i < 7 else "data%d" % (i - 7)
        o, a, s = offs[i], addrs[i], sizes[i]
        if o < 0x100 or o + s > size:
            rep(BAD, "%s: el desplazamiento/tamano (0x%X + 0x%X) se sale del archivo (%d bytes)." % (kind, o, s, size))
        if not in_mem(a, s):
            rep(BAD, "%s: direccion 0x%08X (+0x%X) fuera de la memoria de la Wii (MEM1 0x80000000-0x817FFFFF / MEM2 0x90000000-0x93FFFFFF)." % (kind, a, s))
        if a % 4 or o % 4:
            rep(BAD, "%s: direccion u offset no alineados a 4 bytes." % kind)
        elif o % 32:
            rep(WARN, "%s: offset no alineado a 32 bytes (elf2dol normalmente alinea)." % kind)
        secs.append((kind, o, a, s))
    texts = [s for s in secs if s[0].startswith("text")]
    if not texts:
        rep(BAD, "no hay ninguna seccion de codigo (text) con datos.")
    # solapes en memoria y en archivo
    for key, label in ((2, "memoria"), (1, "archivo")):
        ordered = sorted(secs, key=lambda t: t[key])
        for p, q in zip(ordered, ordered[1:]):
            if p[key] + p[3] > q[key]:
                rep(BAD, "las secciones %s y %s se solapan en %s." % (p[0], q[0], label))
    # punto de entrada
    if not any(a <= entry < a + s for _, _, a, s in texts):
        rep(BAD, "el punto de entrada 0x%08X no cae dentro de ninguna seccion de codigo." % entry)
    elif entry % 4:
        rep(BAD, "el punto de entrada 0x%08X no esta alineado a 4 bytes." % entry)
    else:
        for _, o, a, s in texts:
            if a <= entry < a + s:
                first = struct.unpack_from('>I', data, o + (entry - a))[0] if o + (entry - a) + 4 <= size else 0
                if first == 0:
                    rep(BAD, "la primera instruccion en el punto de entrada es 0x00000000 (no es codigo PowerPC valido).")
    if bss_size and not in_mem(bss_addr, bss_size):
        rep(BAD, "BSS 0x%08X (+0x%X) fuera de la memoria de la Wii." % (bss_addr, bss_size))
    end = max([o + s for _, o, _, s in secs] + [0x100])
    if size > end + 64:
        rep(WARN, "hay %d bytes sobrantes al final del archivo (no es un error grave)." % (size - end))
    for kind, o, a, s in secs:
        rep(INFO, "%-6s offset 0x%06X  direccion 0x%08X  tamano 0x%06X" % (kind, o, a, s))
    rep(INFO, "entrada 0x%08X | BSS 0x%08X (+0x%X)" % (entry, bss_addr, bss_size))
    if errors == bad_before:
        rep(OK, "%s tiene una estructura DOL valida para Wii (cabecera, secciones, memoria y entrada coherentes). "
                "Nota: esto valida el formato, no sustituye probarlo en Dolphin/Wii." % name)


# ----------------------------------------------------------------------------- meta.xml
def check_meta(path):
    try:
        raw = open(path, 'rb').read()
        text = raw.decode('utf-8')
    except UnicodeDecodeError:
        rep(BAD, "meta.xml no esta en UTF-8.")
        return
    try:
        root = ET.fromstring(raw)
    except ET.ParseError as e:
        rep(BAD, "meta.xml no es XML valido: %s" % e)
        return
    if not text.lstrip().startswith('<?xml'):
        rep(WARN, "meta.xml no empieza con la declaracion <?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>.")
    if root.tag != 'app' or root.get('version') != '1':
        rep(BAD, "la raiz debe ser <app version=\"1\">; encontrado <%s version=%r>." % (root.tag, root.get('version')))
    required = ['name', 'coder', 'version', 'release_date', 'short_description', 'long_description']
    missing = [t for t in required if root.find(t) is None or not (root.find(t).text or '').strip()]
    if missing:
        rep(BAD, "faltan etiquetas en meta.xml: %s" % ", ".join(missing))
    rd = (root.findtext('release_date') or '').strip()
    if rd and not re.fullmatch(r'\d{14}', rd):
        rep(BAD, "release_date debe ser AAAAMMDDhhmmss (14 digitos); encontrado %r." % rd)
    if not missing and root.tag == 'app' and root.get('version') == '1' and (not rd or re.fullmatch(r'\d{14}', rd)):
        rep(OK, "meta.xml correcto para el Homebrew Channel (nombre: %r, version %s)." % (root.findtext('name'), root.findtext('version')))


# ----------------------------------------------------------------------------- icon.png
def check_icon(path):
    data = open(path, 'rb').read()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        rep(BAD, "icon.png no es un PNG valido.")
        return
    w, h = struct.unpack('>II', data[16:24])
    if (w, h) == (128, 48):
        rep(OK, "icon.png es PNG de 128x48 (tamano recomendado), %d bytes." % len(data))
    else:
        rep(WARN, "icon.png es %dx%d; el Homebrew Channel recomienda 128x48." % (w, h))


# ----------------------------------------------------------------------------- carpeta de la app / SD
def check_app_dir(d):
    print("\n== App en: %s" % d)
    dol, elf = os.path.join(d, 'boot.dol'), os.path.join(d, 'boot.elf')
    if os.path.isfile(dol):
        check_dol(dol)
    elif os.path.isfile(elf):
        check_dol(elf)
    else:
        rep(BAD, "no hay boot.dol (ni boot.elf) en esta carpeta: el Homebrew Channel NO mostrara la app.")
    for fname, fn in (('meta.xml', check_meta), ('icon.png', check_icon)):
        p = os.path.join(d, fname)
        if os.path.isfile(p):
            fn(p)
        else:
            rep(WARN, "falta %s (opcional, pero recomendado: da nombre e icono en el menu)." % fname)
    parent = os.path.basename(os.path.dirname(os.path.abspath(d)))
    if parent.lower() != 'apps':
        rep(WARN, "la carpeta de la app no esta directamente dentro de una carpeta 'apps' (esta en %r). "
                  "Debe quedar como SD:/apps/<NombreApp>/boot.dol" % parent)


def find_app_dirs(root):
    found = []
    for dp, dn, fn in os.walk(root):
        depth = dp[len(root):].count(os.sep)
        if depth > 4:
            dn[:] = []
            continue
        if 'boot.dol' in [f.lower() for f in fn] or 'boot.elf' in [f.lower() for f in fn]:
            found.append(dp)
    return found


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    p = sys.argv[1]
    if os.path.isfile(p):
        check_dol(p)
    elif os.path.isdir(p):
        apps = find_app_dirs(p)
        if not apps:
            rep(BAD, "no se encontro ningun boot.dol/boot.elf dentro de %r (profundidad <= 4)." % p)
            has_src = os.path.isfile(os.path.join(p, 'Makefile')) or os.path.isdir(os.path.join(p, 'source'))
            if has_src:
                rep(INFO, "esta carpeta es el CODIGO FUENTE del proyecto (Makefile/source): no es una app instalable. "
                          "Hay que compilarla primero (make package) y copiar apps/UndertaleYellow a la SD.")
            rep(INFO, "el Homebrew Channel solo lista carpetas SD:/apps/<Nombre>/ que contengan boot.dol (o boot.elf).")
        for d in apps:
            check_app_dir(d)
    else:
        rep(BAD, "no existe la ruta %r" % p)
    print("\nRESULTADO:", "CORRECTO" if errors == 0 else "%d FALLO(S)" % errors)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
