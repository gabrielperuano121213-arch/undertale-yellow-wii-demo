# Notas técnicas: formato de `data.win` de Undertale Yellow

Resultado de analizar tu copia del juego (verificado con código; útil si algún día se hace un *runner* de GameMaker para Wii).

## Contenedor
* `FORM` de GameMaker, 159 790 836 bytes. `GEN8`: **bytecode 17**, ventana 640x480, nombre `Undertale_Yellow`.
* Chunks: GEN8 OPTN LANG EXTN SOND AGRP SPRT BGND PATH SCPT GLOB SHDR FONT TMLN OBJT FEDS ACRV SEQN TAGS ROOM
  DAFL EMBI PSEM PSYS TPAG TGIN CODE VARI FUNC FEAT STRG TXTR AUDO (GameMaker reciente, no 1.4).
* Tamaños: CODE 5,5 MB · ROOM 10,9 MB · OBJT 1,3 MB · TXTR 26,4 MB (83 páginas) · AUDO 106 MB (audio embebido).
* Contenido: 287 salas, 3 226 objetos, 12 192 fragmentos de código, 2 345 scripts.

## Texturas (`TXTR`) — lo que un runner antiguo NO sabe leer
Entrada de 28 bytes (7 x u32): `scaled, generatedMips, blobSize, width, height, indexInGroup, blobPtr`.

Blob = `"2zoq"` + u16 ancho + u16 alto + u32 tamaño_sin_comprimir + **BZip2**( `raw` ).
`raw` = `"fioq"` + u16 ancho + u16 alto + u32 largo + payload en una **variante de QOI de GameMaker**
(deducida desencajando el decodificador del runner de Windows; decodifica exactamente todos los bytes de todas las páginas probadas):

| Byte (tag)  | Operación |
|-------------|-----------|
| `00xxxxxx`  | INDEX: píxel = tabla[x] (no actualiza tabla) |
| `010xxxxx`  | RUN corto: repite el píxel actual (1 + x) veces |
| `011xxxxx` + 1 byte | RUN largo: 1 + (((x << 8) \| byte) + 32) veces |
| `10xxxxxx`  | DIFF 2/2/2 bits en complemento a 2 (R,G,B) |
| `110xxxxx` + 1 byte | DIFF R 5 bits, G 4 bits, B 4 bits |
| `1110xxxx` + 2 bytes | DIFF R,G,B,A de 5 bits cada uno |
| `1111abcd`  | sustituye canales R(a) G(b) B(c) A(d) con los bytes que siguen |

Píxel inicial (0,0,0,255); **hash = (R^G^B^A) & 63**; el píxel se guarda en memoria como R,G,B,A.
Implementación de referencia: `qoi_gm()` en `tools/extract_assets.py`.

## Sprites, fuentes
* `TPAG` (por recorte): 10 x u16 (srcX, srcY, srcW, srcH, tgtX, tgtY, tgtW, tgtH, boundW, boundH) + i16 página.
* `SPRT`: tras el origen (+48/+52) hay un i32; si es -1 es sprite "especial" (versión, tipo, velocidad(8)
  [+4 si ver>=2][+4 si ver>=3]) y luego `u32 n` + n punteros a `TPAG`.
* `FONT`: rango en +20/+24, `TPAG` en +28, **lista de glifos en +52 (cuenta) / +56 (punteros)**;
  glifo = u16 char,x,y,w,h + i16 shift,offset + kerning.

## Las Ruinas dentro del archivo
* `rm_ruins00`–`04`, `rm_darkruinsintro`, `rm_darkruins_01`–`19` (+ variantes `08b 09b 14b 16b`), `ruins_outro`.
* Dalv: `rm_dalvhallway`, `rm_dalvshouse`, `rm_dalvsroom`, `rm_dalvroomhall`, `rm_dalvExit` (+ `_pacifist`, `_genocide`).
* Batallas: `rm_battle`, `rm_battle_flowey`, `rm_battle_flowey_phase_2`, `rm_battle_flashback_*`.
* Rutas, puzzles, jefes y enemigos son **lógica en el bytecode**: en todo el juego hay 93 objetos "geno", 52 "pacif",
  40 de Dalv y 681 de batalla/enemigos (2 834 fragmentos de código). Por eso no se pueden "copiar" desde los sprites.
