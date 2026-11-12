# Informe de auditoría y decisión técnica — Undertale Yellow en Wii

Fecha: 11-nov-2026. Todo lo marcado como "verificado" sale de tu `data.win` o de páginas que abrí; lo demás está marcado como NO COMPROBADO.

## 0. Límites de esta auditoría (dicho sin adornos)

* El entorno donde trabajo **no tiene compilador PowerPC (devkitPPC), ni libogc, ni acceso a internet** (lo comprobé: no hay `powerpc-eabi-gcc` y ningún dominio resuelve). Por eso **no he generado un `boot.dol` real**. La causa de que el Homebrew Channel no detectara la app es esa: el paquete anterior era código fuente, sin `boot.dol`.
* Nada de lo que escribí se ha probado en Dolphin ni en una Wii. Lo único probado es la lógica del demake en PC (capturas simuladas) y la capa Wii contra una libogc simulada.

## 1. Qué es el "port de navegador" adjunto

Es un único `index.html` ("Ultimate Game Stash file"). Verificado leyendo el código:

* Descarga `game.unx` en 10 trozos (`game.unx.part1…10`), los une y después carga `runner.js` (que usa `runner.wasm`), todo desde un repositorio alojado en un CDN de terceros (ruta anónima de jsDelivr). Esos archivos **no vienen incluidos** y no puedo descargarlos desde aquí.
* Trae un gestor de partidas (IndexedDB `/_savedata`, importar/exportar ZIP), teclas WASD, contador de FPS y una lista de archivos de audio (`mus/*.ogg`, 19 efectos en `snd/*.ogg`).
* **No contiene lógica de juego, salas, diálogos, enemigos ni jefes.** Es un lanzador del runtime de GameMaker compilado a WebAssembly. `game.unx` es el nombre que GameMaker da al archivo de datos en exportaciones Linux/WASM (el equivalente de tu `data.win`).
* Consecuencia: no se puede "auditar su contenido" ni reutilizar su "código" en la Wii (la Wii no ejecuta WebAssembly). Si `game.unx` es el juego original, será tan completo como el original, pero **eso no está comprobado**. No calculo ningún porcentaje: no hay datos para hacerlo.
* Aviso: está alojado por terceros, no es la distribución oficial del equipo.

## 2. Inventario real del juego original (tu `data.win`)

| Dato | Valor |
|---|---|
| Formato | GameMaker, bytecode **WAD 17**, máquina virtual (no YYC), ventana 640x480 |
| Salas | 287 |
| Objetos / scripts / fragmentos de código | 3 226 / 2 345 / 12 192 |
| Sonidos (SOND) / entradas de audio embebido | 673 / 448 (≈106 MB) |
| Shaders / extensiones / secuencias / paths / fuentes | 26 / **0** / 35 / 68 / 11 |
| Texturas | 83 páginas (64 de 2048x2048), formato QOI+BZip2 |
| Memoria de texturas si se decodifican | ≈1 091 MB en RGBA8 · ≈273 MB en 8 bits (la Wii tiene 88 MB de RAM) |

Salas por zona (prefijo del nombre): steamworks 70 · dunes 69 · snowdin 32 · darkruins 23 (+ intro) · hotland 15 · battle 13 · mansion 11 · castle 8 · ruins 6 · newhome 4 · credits 4 · dalv 7 · flashback 3 · intro 2 · mainmenu 2 · otras.

Rutas y finales (por nombres): objetos con "geno" 93, "pacif" 52, "neutral" 28; salas `ruins_outro`, `rm_credits`, `rm_credits_geno`, `rm_credits_paci`, `rm_credits_paci_intermission`, `rm_build_ending`. Objetos con nombre de jefe/personaje: flowey 240, martlet 222, ceroba 242, axis 99, dalv 40; tiendas 57; objetos de guardado 2.

## 3. Tabla de auditoría

| Elemento | Juego original (verificado en `data.win`) | Port de navegador | Estado |
|---|---|---|---|
| Zonas | 12+ zonas por prefijo (steamworks, dunes, snowdin, darkruins, hotland, mansion, castle, ruins…) | El HTML no contiene salas | NO COMPROBADO |
| Habitaciones | 287 | idem | NO COMPROBADO |
| Jefes y enemigos | 681 objetos de batalla/enemigo; familias flowey, martlet, ceroba, axis, dalv | idem | NO COMPROBADO |
| Rutas | 93 objetos "geno", 52 "pacif", 28 "neutral"; créditos por ruta | idem | NO COMPROBADO |
| Finales | 6 salas de finales/créditos | idem | NO COMPROBADO |
| Combate | 13 salas `battle` + lógica en bytecode | idem | NO COMPROBADO |
| Guardado | 2 objetos de guardado; archivos de partida | El HTML solo trae un gestor de archivos de partida, no el sistema de guardado del juego | NO COMPROBADO |
| Música y sonidos | 673 sonidos, ≈106 MB embebidos | El HTML lista los `.ogg`; que existan o suenen no se ha verificado | NO COMPROBADO |

## 4. Las tres alternativas (con fuentes)

| Criterio | A. Port nativo (demake) | B. Butterscotch | C. Cinnamon |
|---|---|---|---|
| Compatibilidad con Wii | Sí (devkitPPC/libogc). Mi demake: sin compilar ni probar en consola | Su README lista PS2, PS3, Vita, Switch…; remite a Cinnamon para 3DS, Wii y Wii U | **Wii listada como plataforma**; su web declara "UNDERTALE: Wii done" y "DELTARUNE Cap. 1 released for Wii" |
| Compatibilidad con Undertale Yellow | Habría que reescribir todo el juego (12 192 fragmentos de código) | Soporta WAD 8–17; corre DELTARUNE 1-3 y UNDERTALE 10th (WAD 17) en escritorio. **UTY no figura** | Bytecode 16 y 17. **UTY: "Coming Soon"** en su web ("hecho de forma independiente por Casriel"); su FAQ avisa de que los fangames "pueden no funcionar" |
| Reutilización de recursos | Mis herramientas extraen sprites/fuentes | Usa `data.win` original | Usa `data.win` + **preprocesador** (convierte texturas y audio a formatos de consola) |
| Rendimiento | Bueno en la demo | Lento en hardware bajo (PS2) con muchas instancias | Un desarrollador reporta 30 fps estables en Undertale/Deltarune Wii (no verificado por mí) |
| Estabilidad | n/d | "Soporte de bytecode aún en desarrollo" | Idem; README: pueden existir diferencias con el runner oficial |
| Dificultad | Extrema para el juego completo | Media (base) | La más baja: ya existe un runner en Wii |
| Posibilidad de juego completo | No realista | No directa en Wii | **Única con precedente en Wii** (Undertale completo, controles con Wiimote horizontal según su desarrollador) |

Fuentes: README de Butterscotch (github.com/ButterscotchRunner/Butterscotch), README de Cinnamon (github.com/Project-Sunshine-Native/cinnamon), sitio de Project Sunshine, hilo de GBAtemp del desarrollador, artículo de 80.lv. No pude abrir la página de descargas de la versión Wii (v0.9), así que no he verificado su contenido ni su estructura en la SD.

## 5. Decisión

**Alternativa C (Cinnamon) como camino principal**, porque es la única con un runner de GameMaker ya funcionando en Wii con un juego completo, soporta bytecode 17 y su preprocesador ya contempla la Wii. No la elijo por ser "para Wii", sino por esa evidencia; **que UTY funcione en ella no está demostrado**.

Problemas de compatibilidad conocidos o probables para UTY (no comprobados):

* Memoria: texturas (273 MB incluso en 8 bits) y 106 MB de audio exigen carga por zonas y conversión a formatos de la Wii.
* Funciones usadas por UTY: 26 shaders, 35 secuencias y 1 sistema de partículas pueden exigir soporte que el runner aún no tenga.
* Controles: mapeo 2=Z, 1=X, B=C **por verificar** en su capa de entrada.

Solución híbrida: Cinnamon como motor + mis herramientas (decodificador de texturas QOI+BZip2 y notas de formato en `docs/NOTAS_FORMATO_UTY.md`) si su preprocesador no lee algo de UTY; el demake nativo queda como plan de reserva.

## 6. Siguiente paso concreto (experimento de 15 minutos)

1. Abre el preprocesador web de Project Sunshine, elige **Other** y **Wii**, selecciona tu carpeta de Undertale Yellow (con `data.win`) y genera el zip de recursos.
2. Descarga la versión Wii **solo desde el repositorio oficial** (`Project-Sunshine-Native/cinnamon`, sección Releases) y sigue sus instrucciones de instalación.
3. Valida el ejecutable: `python3 tools/verificar.py <ruta>/boot.dol` (debe decir CORRECTO).
4. Ábrelo desde el Homebrew Channel y anota qué ocurre (pantalla, error, mensaje en consola).

Con ese resultado se decide si basta con ajustar el preprocesado, hay que parchear el runner o conviene seguir con el demake.
