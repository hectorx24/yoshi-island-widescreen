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
