# Undertale Yellow — Demo para Nintendo Wii (Homebrew Channel)

> **Esto es código fuente, no una app instalable.** El Homebrew Channel solo lista carpetas `SD:/apps/<Nombre>/` que
> contengan un `boot.dol` real. Copiar este proyecto a la SD **no** hace que aparezca. Hay que compilarlo primero.

Demake jugable de **Undertale Yellow** para la Wii. No es el juego completo: es una demo corta que conserva
la esencia (arte, fuente y botones reales sacados de tu `data.win`, 30 ticks/s, 320x240 ampliado x2):

* Pantalla de título (tu `splash.png`).
* Una sala de las Ruinas con Clover caminando (4 direcciones), un letrero, flores doradas y un Training Dummy.
* Cuadros de diálogo con efecto de máquina de escribir.
* Menú de pausa (**C**): ITEM / STAT / CELL.
* Batalla con el **alma amarilla**: FIGHT (barra de ataque), ACT (Check / Cheer), ITEM, MERCY (Spare / Flee).
  En el turno enemigo el alma se mueve con la cruceta y **dispara con Z** para destruir las bolas.
* Game Over y reinicio.

Sin música ni sonido todavía.

## Controles (Wiimote en horizontal, como un mando de NES)

| Juego            | Wiimote       |
|------------------|---------------|
| Moverse          | Cruceta (Wiimote siempre horizontal) |
| **Z** confirmar / disparar | Botón **2** |
| **X** cancelar / ir lento  | Botón **1** |
| **C** menú       | Botón **B**   |
| Volver al Homebrew Channel | HOME |

## 1) Conseguir `boot.dol` (elige UNA opción)

No se puede distribuir un `.dol` ya compilado desde aquí, así que hay que compilarlo una vez.

**A. Sin instalar nada (GitHub Actions)**
1. Crea un repositorio en GitHub y sube **todo el contenido de esta carpeta** (incluida `.github/`).
2. Pestaña **Actions** → *Compilar boot.dol (Wii)* → espera ~2 minutos.
3. Descarga el artefacto **UndertaleYellow-Wii**: contiene la carpeta `UndertaleYellow/` lista.

**B. Docker**
```
docker run --rm -v "%cd%":/project -w /project devkitpro/devkitppc make package      (Windows CMD)
docker run --rm -v "$PWD":/project -w /project devkitpro/devkitppc make package      (Linux/macOS)
```

**C. devkitPro instalado**
Instala devkitPro (https://devkitpro.org) con el componente *Wii Development* y, en esta carpeta:
```
make package
```

Las tres opciones dejan `apps/UndertaleYellow/` con `boot.dol`, `meta.xml` e `icon.png`.

Atajos: `compilar_docker.bat` (Windows) o `./compilar_docker.sh` (Linux/macOS) ejecutan la opción B y verifican el resultado.
Si en GitHub no se subió la carpeta `.github`, crea el archivo `.github/workflows/build.yml` desde la web ("Add file" → "Create new file") y pega su contenido.

## Verificar el resultado (importante)

```
python3 tools/verificar.py apps/UndertaleYellow      # o la ruta de tu SD
```
Comprueba que `boot.dol` sea un DOL válido de Wii (no un ELF de PC renombrado), que `meta.xml` tenga el formato del
Homebrew Channel y que `icon.png` sea de 128x48. También puedes pasarle la raíz de la tarjeta SD.

## Si el Homebrew Channel no detecta la app

1. ¿Existe `SD:/apps/UndertaleYellow/boot.dol`? (nombre exacto, directamente dentro de `apps/<Nombre>/`, sin carpetas intermedias).
2. ¿Copiaste el *proyecto* en vez de la carpeta compilada? El proyecto no contiene `boot.dol`.
3. SD en **FAT32** (las tarjetas de más de 32 GB suelen venir en exFAT y la Wii no las lee).
4. Vuelve a abrir el Homebrew Channel tras insertar la SD.
5. Ejecuta `python3 tools/verificar.py <ruta>` y revisa los [FALLO].

## 2) Instalar en la Wii

Copia la carpeta **`UndertaleYellow`** a **`SD:/apps/`** (tarjeta FAT32). Quedará así:

```
SD:/apps/UndertaleYellow/boot.dol
SD:/apps/UndertaleYellow/meta.xml
SD:/apps/UndertaleYellow/icon.png
```
Abre el Homebrew Channel: aparecerá "Undertale Yellow (Demo)" con su icono. También puedes probar `boot.dol` en el emulador **Dolphin**.

## Estructura

```
source/main.c        capa de la Wii: video (XFB), Wiimote -> Z/X/C, bucle a 30 ticks/s
source/game.c        juego completo (portátil, no depende de la consola)
source/gfx.c         renderizador 2D por software (framebuffer indexado 320x240)
source/assets.c/.h   recursos GENERADOS desde tu data.win (no editar a mano)
tools/extract_assets.py   regenera assets.c/.h desde data.win y splash.png
tools/pc_test.c      prueba en PC sin Wii: simula botones y guarda capturas .ppm
Makefile, meta.xml, icon.png, .github/workflows/build.yml
```

Regenerar recursos (opcional, requiere `pip install pillow`):
```
python3 tools/extract_assets.py --data data.win --splash splash.png --out source
```

Probar la lógica en PC (ejemplo: título → diálogo → captura):
```
gcc -O2 -o pc_test tools/pc_test.c source/game.c source/gfx.c source/assets.c -Isource
./pc_test 20: 1:Z 1: 100: snap:captura      # genera captura.ppm
```

## De la demo a las Ruinas completas (con rutas neutral / pacifista / genocida)

Esta demo es un *demake*: la lógica está escrita a mano. Las Ruinas reales (≈30 salas + las de Dalv + batallas),
sus puzzles, jefes, enemigos y **las tres rutas** viven en el bytecode de GameMaker de tu `data.win`
(ver `docs/NOTAS_FORMATO_UTY.md`). La forma fiel de tenerlos es un *runner* de GameMaker para Wii que ejecute ese
bytecode con un paquete que contenga solo esa zona. Hitos sugeridos: 1) cargador + máquina virtual en PC,
2) salas y sprites, 3) texto y batallas, 4) puzzles, jefes y rutas, 5) backend Wii + audio.

## Notas

* Todo el texto del juego está en inglés (como el original) dentro de `source/game.c`.
* Este proyecto incluye arte y fuente de Undertale Yellow extraídos de tu copia: es para **uso personal**.
  Si quieres compartirlo públicamente, pide permiso al equipo de Team Undertale Yellow.
* Ampliar el juego: más salas = más imágenes de fondo + rectángulos de colisión; más enemigos = más patrones
  en `dodge_update()`.
