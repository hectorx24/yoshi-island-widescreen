# Registro de Defectos y Verificación de Calidad (BUGS.md)
**Super Mario World 2: Yoshi's Island (USA v1.1) · Android ARM64 / Desktop**  
*Fecha:* 16 de octubre de 2026

---

## BUG-001: Rueda de Flecha ($011E) Desaparece o se Recorta en Bloques de Piedra (Nivel 1-1)

- **Gravedad:** Alta (Defecto visual prominente reportado por usuario en capturas de Nivel 1-1).
- **Componente:** `upstream/snesrecomp/runner/src/desktop/host_main.c` y `src/game_rtl.c`.
- **Pasos de Reproducción:**
  1. Iniciar Nivel 1-1 ("Make Eggs, Throw Eggs").
  2. Avanzar hacia la derecha hasta la estructura de bloques de piedra con la rueda de flecha giratoria sobre ellos.
  3. Desplazar la cámara de izquierda a derecha en modo panorámico (342px–420px).
  4. Observar que la rueda de flecha desaparecía intermitentemente o quedaba recortada por el borde superior/montaña.
- **Causa Raíz Confirmada:**
  1. En `upstream/snesrecomp/runner/src/snes/ppu.c`, la emulación del PPU de SNES impone un límite de hardware de 34 fragmentos de sprite (slivers) por línea de exploración (`kPpuRenderFlags_NoSpriteLimits`). Al ensanchar el viewport de 256 a 342–420 píxeles, más sprites comparten la misma línea horizontal, provocando que los sprites del extremo derecho (incluyendo la rueda de flecha en ciertas coordenadas Y) alcancen el tope de 34 slivers y sean descartados.
  2. En `src/game_rtl.c`, la función `BlackUnverifiedMargins()` limpiaba preventivamente los márgenes sin verificar si `ppu->objBuffer.data` contenía píxeles válidos de sprites OAM activos.
- **Corrección Aplicada:**
  1. En `host_main.c`: activar automáticamente la bandera `kPpuRenderFlags_NoSpriteLimits` en la inicialización del PPU cuando el modo panorámico está activo (`g_ws_active`).
  2. En `src/game_rtl.c`: proteger los píxeles de sprites activos en `ppu->objBuffer.data` impidiendo que `BlackUnverifiedMargins` los sobreescriba.
- **Evidencia de Regresión:**
  - Script determinista `tools/test.sh` y script de recorrido `stone_walk.script` ejecutados. 10+ fotogramas continuos capturados: la rueda de flecha rota continuamente con visibilidad 100% sobre los bloques de piedra en todas las posiciones de cámara.

---

## BUG-002: Verificación del Contador Negro "10" de Baby Mario durante el Rescate

- **Gravedad:** Informativa / Validación Crítica de Autenticidad.
- **Componente:** `src/game_rtl.c` / Emulación de WRAM SNES (`$0003B6` y `$0003AB`).
- **Observación Inicial:** En una de las capturas del usuario aparecía un círculo negro con el número `10` sobre Yoshi cuando Baby Mario parecía estar montado. Se requería investigar si se trataba de un overlay de depuración o de un bug de HUD.
- **Causa Raíz Confirmada:**
  - Inspección exhaustiva de la ROM original de SNES (USA v1.1) y el desensamblado oficial (`Yoshifanatic1/Yoshi-s-Island-Disassembly`):
  - El círculo negro con el número `10` es el **contador de estrellas/tiempo de Baby Mario**.
  - En la ROM original, cuando Yoshi recibe daño, Baby Mario se separa en una burbuja y el contador desciende desde 10. Al rescatar a Baby Mario, el contador no desaparece instantáneamente; se posiciona sobre Yoshi y permanece visible mientras el tiempo de estrellas se recupera progresivamente hasta 10 (o hasta que expira el período de gracia de recuperación).
  - El comportamiento en el port recompilado es 100% idéntico al cartucho original de Super Nintendo.
- **Resolución:**
  - Comportamiento validado como auténtico y fiel al cartucho SNES original. Se conserva intacto para preservar 100% de autenticidad en la emulación del juego.

---

## BUG-003: Fondo Universal Borroso (DownsampleBackdrop) y Franjas Negras en Escenas 4:3

- **Gravedad:** Media-Alta (Rechazo visual por parte del usuario).
- **Componente:** `src/presentation.c`.
- **Causa Raíz:**
  - `DownsampleBackdrop()` reducía cualquier fotograma de 256×224 a 32×28 píxeles y lo dibujaba con filtrado bilineal en los laterales de cualquier escena que no fuera widescreen nativo (incluyendo el Mapa Mundial y la Introducción de Cuentos), generando un duplicado borroso genérico. Además, si el muestreo tocaba franjas de recorte de 4:3, producía bordes negros.
- **Corrección Aplicada:**
  - Implementación de la **Política de Composición por Escena** (Tipos A, B, C, D, E):
    - *Tipo A (54 Niveles):* Widescreen real nativo edge-to-edge.
    - *Tipo B (12 Jefes):* Mosaico nearest-neighbour 1/8 temático de la arena de combate con viñeta exterior; exclusión estricta de HUD, sprites y jefes fantasma.
    - *Tipo C (Mapa Mundial):* Extensión continua de acuarela oceánica/pergamino muestreando columnas internas (x=16..32), con grano orgánico. Cero barras negras, cero blur.
    - *Tipo D (Intro de Cuentos):* Encuadre cálido de libro ilustrado antiguo.
    - *Tipo E (Menús de Sistema):* Encuadre limpio sin artefactos.
- **Evidencia de Regresión:**
  - Verificado en el ciclo de presentación y capturas de mapa, introducción y castillos.

---

## BUG-004: Diálogos Genéricos de Android y Botón Superior Anti-estético "..."

- **Gravedad:** Media (Usabilidad y acabado estético en Android).
- **Componente:** `android/java/com/hector/yoshi/YoshiControls.java`.
- **Causa Raíz:**
  - El menú de opciones y guardado invocaba un `AlertDialog.Builder` estándar gris de Android en idioma inglés, y dibujaba un botón flotante tosco con tres puntos `...` en la esquina superior derecha.
- **Corrección Aplicada:**
  - Se sustituyó el botón flotante por un icono de pausa discreto semitranslúcido en tonos verde bosque y crema (`❚❚`), respetando los insets seguros de la pantalla.
  - Se diseñó un panel modal de pausa con estética de pergamino de Yoshi's Island (`#FAF5EB`), cabecera en verde bosque (`#387627`) y tarjetas de opciones en español:
    1. *Continuar partida*
    2. *Guardar / Cargar partida* (invoca menú nativo de ranuras traducido)
    3. *Controles en pantalla* (Auto / Visibles / Ocultos)
    4. *Vibración háptica* (Desactivada / Sutil / Normal)
    5. *Idioma* (Español / English)
    6. *RetroAchievements* (diálogo traducido con estado de sesión)
  - Pausa automática garantizada: abrir el menú pausa la emulación y el flujo de audio (`YoshiSetHostPaused(true)`) sin provocar chasquidos ni buffer underruns.

---

## BUG-005: Orientación Bloqueada a Horizontal Único en Android

- **Gravedad:** Media.
- **Componente:** `android/AndroidManifest.xml` y `android/java/com/hector/yoshi/YoshiActivity.java`.
- **Causa Raíz:**
  - `android:screenOrientation="landscape"` impedía que el usuario rotara el dispositivo 180° para jugar con el puerto USB / conector de auriculares hacia el otro lado.
- **Corrección Aplicada:**
  - Se actualizó a `android:screenOrientation="sensorLandscape"`.
  - El drawable de SDL y la capa de controles táctiles recalculan sus dimensiones dinámicamente preservando la relación de aspecto sin estiramientos.

---

## BUG-006: Regresión en Icono de Launcher (Círculo Blanco Artificial)

- **Gravedad:** Alta (Rechazo explícito del usuario per Plan Maestro v5 / Sec. 13.2).
- **Componente:** `android/res/mipmap-*/ic_launcher.png`, `android/res/drawable/ic_launcher.png`.
- **Causa Raíz:**
  - En la fase previa, se aplicó un recorte circular con máscara transparente a los PNGs del icono. Los launchers de Android 8.0+ (Pixel, MIUI, OneUI), al recibir un PNG no adaptativo o con transparencia perimetral, colocaron el icono dentro de una placa circular blanca genérica.
- **Corrección Aplicada:**
  - Se restauró el recurso de arte completo original de Yoshi y Bebé Mario (512×512 full-bleed) directamente desde el commit `32528ef` en todas las densidades (mdpi, hdpi, xhdpi, xxhdpi, xxxhdpi).
  - No se impone ningún recorte circular ni fondo blanco dentro de la aplicación, permitiendo que el launcher aplique su máscara nativa de squircle/icono sin bandas artificiales.

---

## BUG-007: Recorte de Botones Táctiles en Bordes de Pantalla y Notch

- **Gravedad:** Media-Alta (Sec. 13.6).
- **Componente:** `android/java/com/hector/yoshi/YoshiControls.java`.
- **Causa Raíz:**
  - El botón A de salto/huevo y los botones de la cruceta estaban posicionados demasiado pegados a los bordes (`w - 38*s`), provocando que en dispositivos con esquinas curvas o notch quedaran cortados visualmente y sus zonas táctiles se desfasaran.
- **Corrección Aplicada:**
  - Se unificó toda la geometría en la estructura `ControlLayout`, calculada una sola vez tanto para `onDraw` como para `keysAt`/`onTouchEvent`.
  - Se agregaron márgenes de seguridad proporcionales (`safeX = max(22*s, 18px)`, `safeY = max(18*s, 14px)`) y respeto absoluto de los insets del sistema (`WindowInsets.Type.displayCutout()`).
  - Detección precisa de mandos físicos (GameSir, Xbox, PlayStation, Switch Pro) con auto-ocultación de controles virtuales sin falsos positivos de auriculares.

---

## BUG-008: Franjas Negras en Escenas 4:3 (Logo, Título, Intro) y Letterboxing

- **Gravedad:** Media-Alta (Sec. 13.3).
- **Componente:** `src/presentation.c`.
- **Causa Raíz:**
  - La pantalla de título (`mode == 0x00`), prólogo (`mode == 0x01` o `0x14`) y secuencias de cinemáticas caían en `SCENE_TYPE_E_SYSTEM`, desactivando el backdrop temático. Además, `has_pillars` solo evaluaba el eje X, dejando bandas negras horizontales (letterboxing) en resoluciones con recorte vertical.
- **Corrección Aplicada:**
  - Se reclasificaron las escenas no widescreen (`mode 0x00, 0x01, 0x02, 0x05, 0x14`) dentro de `SCENE_TYPE_D_STORYBOOK` con extensión ambiental continua.
  - Se implementó un muestreo de color vertical fila por fila (`by = 0..27`) que sigue los degradados reales del cielo pastel y terreno de cada escena.
  - Se expandió `has_pillars` para evaluar tanto bordes horizontales (`destination->x`) como verticales (`destination->y`), rellenando todo letterboxing con el backdrop temático.

---

## BUG-009: Sobreexposición / Lavado de Blancos en Nieve, Nubes y Sprites

- **Gravedad:** Media (Sec. 13.4).
- **Componente:** `src/presentation.c`.
- **Causa Raíz:**
  - El umbral de luminancia para la pasada de bloom (`luminance > 208`) aplicaba resplandor indiscriminadamente sobre blancos neutros reflectivos (nubes, picos nevados, tiza, estómago de Yoshi), quemando los detalles de contorno.
- **Corrección Aplicada:**
  - Se introdujo filtrado cromático en la extracción de fuentes de luz emisivas:
    - Fuentes de fuego / lava / farolillos: `r > 195 && r > (b + 35) && luminance > 135` (bloom cálido selectivo).
    - Estrellas / monedas / magia dorada: `r > 200 && g > 170 && b < 140 && luminance > 175`.
    - Blancos neutros reflectivos (nieve, nubes, textos): resplandor suprimido; únicamente brillo especular ultra-sutil por encima de 250 sin lavado de bordes.

---

## BUG-010: Partículas y Soporte Desacoplado a 120 Hz en Pantalla

- **Gravedad:** Informativa / Mejora Visual de Nueva Generación (Sec. 13.4 y 13.5).
- **Componente:** `src/presentation.c` y `android/java/com/hector/yoshi/YoshiActivity.java`.
- **Solución Implementada:**
  - `YoshiActivity.java`: Solicita modo de refresco a 120 Hz mediante la API nativa de Android (`preferredDisplayModeId` y `preferredRefreshRate = 120.0f`).
  - El reloj de simulación de SNES y el procesamiento de audio se mantienen estrictamente a la cadencia original NTSC de 60.0988 Hz, garantizando audio impecable sin aceleraciones.
  - Se implementó un pool de partículas ligeras deterministas en la capa de presentación (puffs de aterrizaje, gotas de agua y brasas de lava) con RNG visual independiente (`s_visual_rng`), desacoplado de la simulación del juego.

---

## BUG-011: Cierre de Criterios de Aceptación de la Beta 3 (Documento Maestro)

- **Gravedad:** Bloqueante de Release (Secciones 1 a 16 del Documento Maestro Beta 3).
- **Componentes:** `src/presentation.c`, `tools/build_android.py`, `tools/test_beta3_biomes.py`, `README.md`.
- **Implementaciones y Verificaciones:**
  1. **Sombras de Contacto 2D (inspirado en *PixelLight*):** Renderizado de sombras de contacto dinámicas y cuantizadas bajo Yoshi a partir de coordenadas WRAM (`$00A0`, `$00A2`), anclando a los personajes en el suelo en modo 4:3 y panorámico sin interferir con colisiones o física.
  2. **Sistema Extendido de Partículas:** Pool expandido a 64 partículas deterministas soportando despegue de salto, carrera, salpicaduras y ondas circulares en agua (*Godot 4 Pixelated Water Shader*), brasas flotantes con oscilación senoidal en lava y destellos dorados al recolectar monedas/estrellas.
  3. **Preset 'Island Cinema' Perfeccionado:** Integración armónica de CRT subpíxel 0.10/0.80/0.10, modulación de color cuantizada por bioma y bloom térmico selectivo.
  4. **Empaquetado Android Beta 3:** Generación y firma verificada de `YoshiIsland_Widescreen_Beta_3.apk` (7.0 MB) en `build/android-arm64/` y `Downloads/`.
  5. **Auditoría Técnica y Publicación Limpia:** Suite de pruebas automatizadas sobre 9,837 fotogramas en `tools/test_beta3_biomes.py` validada. Repositorio de GitHub actualizado con estética retro de 16 bits y exclusión rigurosa de cualquier ROM o material comercial de Nintendo.

