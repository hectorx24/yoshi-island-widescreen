# Yoshi's Island Android Widescreen

## Plan maestro de implementación, validación visual y continuidad para Antigravity

**Proyecto:** Super Mario World 2: Yoshi's Island, ROM USA v1.1\
**Fecha de esta revisión:** 16 de octubre de 2026\
**Objetivo:** continuar el proyecto existente y llevarlo a una versión
Android panorámica, visualmente pulida, estable y verificable, sin
perder el trabajo previo de Codex y Antigravity.

> **Instrucción principal para Antigravity:** este documento es el plan
> de ejecución y seguimiento. Léelo junto con `source_code/AGENTS.md`,
> `source_code/PROJECT_STATUS.md`, `source_code/HANDOFF.md` y los
> documentos de continuidad que vienen en `dinosaurio.zip`. No empieces
> desde cero. No te limites a entregar una auditoría: ejecuta las fases
> en orden, implementa las mejoras viables, compila, captura, compara,
> documenta y continúa hasta completar todo lo que pueda verificarse en
> el entorno disponible. Si una prueba requiere el teléfono físico, una
> cuenta real o un recurso que no está disponible, deja esa prueba
> claramente pendiente y continúa con las demás.

------------------------------------------------------------------------

# 1. Resultado que buscamos

Entregar una versión de Yoshi's Island que:

1.  Utilice toda la superficie de pantalla disponible en Android, sin
    bandas grises accidentales.
2.  Muestre el máximo campo de visión panorámico seguro, con contenido
    auténtico de la ROM y sin deformar los píxeles.
3.  Mantenga el centro original de SNES idéntico, sin alterar
    simulación, física, colisiones, RNG, temporización ni comportamiento
    de Super FX.
4.  Resuelva individualmente los jefes, escenas de introducción,
    transiciones, mapa y pantallas narrativas.
5.  Cuando una escena no admita ampliación real segura, rellene los
    laterales con arte del propio escenario, oscurecido y desenfocado
    por bloques, manteniendo intacta y nítida la imagen principal.
6.  Conserve el filtro suave tipo CRT que ya existe, sin líneas de
    exploración exageradas, curvatura, máscara de fósforo ni textura de
    televisor.
7.  Conserve y valide la iluminación 2D sutil existente, con activación
    independiente.
8.  Mantenga el autoguardado de SRAM, el menú de estados de
    guardado/carga y los controles Android.
9.  Complete una estrategia viable para seleccionar inglés/español, sin
    mostrar un selector que prometa una traducción incompleta.
10. Verifique RetroAchievements sin afirmar que el inicio de sesión o
    los desbloqueos en línea funcionan hasta probarlos con el servicio
    real.
11. Diagnostique y reduzca los cortes de música y los tirones con
    mediciones, no mediante migraciones a ciegas.
12. Entregue un APK compilado y pruebas con capturas comparables
    antes/después.

## Regla de ejecución

El proyecto se trabaja como una sola hoja de ruta, pero con cambios
pequeños y verificables. No introducir todos los cambios en un parche
gigante. Cada fase debe terminar con pruebas, capturas revisadas, un
registro breve y un checkpoint antes de continuar.

------------------------------------------------------------------------

# 2. Estado conocido a partir de `dinosaurio.zip`

## 2.1 Arquitectura y versión de ROM

-   Port nativo en C/C++ basado en SNESRecomp, con runtime compartido
    SNES/Super FX y SDL3.
-   Android ARM64, pipeline de compilación manual existente en
    `source_code/tools/build_android.py`; no migrar a Gradle sin una
    razón demostrada.
-   ROM objetivo:
    `Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc`.
-   SHA-256 de la ROM limpia sin cabecera:
    `bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06`.
-   La versión USA 1.1 importa. No transferir direcciones de memoria o
    parches de la versión 1.0. La referencia de desensamblado principal
    es
    [Yoshifanatic1/Yoshi-s-Island-Disassembly](https://github.com/Yoshifanatic1/Yoshi-s-Island-Disassembly).
    El repositorio de `brunovalads` es complementario, pero corresponde
    a v1.0.
-   Dispositivo objetivo indicado en el proyecto: OPPO Find X9 Pro, 2772
    × 1272 en horizontal. Las pruebas del emulador no equivalen a una
    prueba en ese teléfono físico.

## 2.2 Discrepancia documental que se debe resolver al inicio

Los documentos del ZIP no describen exactamente el mismo estado de
producción:

-   `02_LO_QUE_SE_HIZO_EN_ESTA_SESION_54_NIVELES.md` afirma que se
    calificaron los 54 niveles en 342 × 224, con capturas, verificación
    de BG1/BG2 y centro nativo idéntico.
-   `source_code/PROJECT_STATUS.md`, que debe tratarse como el handoff
    técnico autoritativo más reciente, indica que el runtime de
    producción solo activa ciertas familias de escenas conocidas, además
    de la apertura, 1-1 y la entrada del castillo 1-4. Otros escenarios
    conservan temporalmente el encuadre nativo.
-   `source_code/src/game_rtl.c`, en `GamePrepareFrame`, aplica
    condiciones por modo, fondo conocido, posición de cámara y extensión
    de límite. Por tanto, tener una captura experimental de un nivel
    panorámico no demuestra por sí solo que ese nivel esté habilitado en
    la APK normal.

**No repitas la investigación de los 54 niveles desde cero y tampoco
afirmes que los 54 ya están activos en producción.** Cruza las capturas,
los commits por nivel, los scripts de prueba y las condiciones de
activación del código actual. Reutiliza las pruebas válidas; vuelve a
ejecutar solo las que hayan quedado invalidadas por cambios posteriores.

## 2.3 Componentes que ya existen y no se deben reconstruir

  ----------------------------------------------------------------------------------------------------------------
  Componente          Ubicación principal                                   Estado registrado en  Qué falta
                                                                            el ZIP                
  ------------------- ----------------------------------------------------- --------------------- ----------------
  Expansión           `src/game_rtl.c`, `src/yoshi_bg1_map.c`,              Implementación        Reconciliar las
  panorámica y        `src/yoshi_scene_map.c`, parches CMake                avanzada con fuentes  54
  resolución de mapas                                                       de mapa auténticas y  calificaciones
                                                                            guardas por escena    con las
                                                                                                  condiciones
                                                                                                  activas en
                                                                                                  producción y
                                                                                                  completar las
                                                                                                  escenas
                                                                                                  pendientes

  Geometría           `src/display_geometry.c`, `src/display_geometry.h`,   Pruebas de geometría  Conectar y
  adaptativa          `GamePrepareFrame`                                    para 2772 × 1272; se  validar toda la
                                                                            puede seleccionar un  superficie
                                                                            campo lógico de hasta Android, los
                                                                            420 × 224 para ese    márgenes seguros
                                                                            dispositivo           y cada ruta de
                                                                                                  escena

  Filtro suave        `src/presentation.c`, `src/presentation.h`,           Modo `Authentic Soft` Revisar capturas
                      `tools/android_presentation.py`                       implementado;         reales, impacto
                                                                            suavizado horizontal  en rendimiento y
                                                                            muy ligero, calidez y apariencia en el
                                                                            brillo sutil          teléfono

  Iluminación 2D      `src/presentation.c`, menú de `YoshiControls.java`    Pasada ligera de      Comprobar que
                                                                            brillo/iluminación,   sea sutil, útil
                                                                            con opción            y no provoque
                                                                            independiente         tirones

  Guardado de SRAM    `src/android_support.c`, `tools/android_lifecycle.py` El ZIP documenta      Repetir prueba
                                                                            escritura atómica de  en el APK final;
                                                                            32 KiB al pasar       teléfono físico
                                                                            Android a segundo     aún no probado
                                                                            plano y recuperación  
                                                                            exacta en emulador    

  Controles táctiles  `android/java/com/hector/yoshi/YoshiControls.java`,   Multitáctil, modo     Repetir pruebas
  y mando             `YoshiActivity.java`, `tools/android_controls.py`     Auto/Show/Hide y      tras corregir el
                                                                            detección de gamepad  layout; GameSir
                                                                            implementados         físico aún no
                                                                                                  probado

  RetroAchievements   `src/achievements.c`, `src/achievement_memory.c`,     Integración nativa y  Validación real
                      `YoshiAchievements.java`, `upstream/rcheevos`         pruebas               en línea
                                                                            sintéticas/offline    pendiente

  Localización        `upstream/snesrecomp/docs/RUNTIME_LOCALIZATION.md`    El framework incluido Inventariar
                                                                            documenta parches de  textos y
                                                                            ROM/RAM/VRAM en       conectar una
                                                                            tiempo de ejecución;  traducción
                                                                            no se ha demostrado   verificada
                                                                            que el juego tenga    
                                                                            español integrado     

  Audio               runtime SDL3 y configuración Android                  Se ha medido señal de Diagnóstico en
                                                                            audio en escritorio,  Android con ruta
                                                                            pero la fidelidad, el de prueba
                                                                            ritmo y los cortes en repetible
                                                                            Android no están      
                                                                            completamente         
                                                                            cualificados          
  ----------------------------------------------------------------------------------------------------------------

## 2.4 La franja gris y el área útil de Android

En `android/java/com/hector/yoshi/YoshiActivity.java`, `immersive()`
aplica barras inmersivas, `LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER` y, en
API 30+, `setPadding(safe.left, safe.top, safe.right, safe.bottom)`
sobre `android.R.id.content`.

Esto puede limitar el área de SDL, aunque el teléfono sea
ultrapanorámico. No elimines los márgenes seguros indiscriminadamente:
los controles y elementos interactivos deben seguir evitando la cámara
frontal y las zonas de baja sensibilidad táctil.

La solución a evaluar es separar claramente:

-   **Superficie de juego SDL:** puede dibujar a pantalla completa,
    incluidos los laterales que no contienen controles interactivos.
-   **Capa de controles/UI:** conserva los márgenes seguros de Android y
    sitúa el botón de opciones y controles táctiles en áreas cómodas.

Primero mide el drawable real y captura la pantalla. Luego cambia la
jerarquía/layout de forma mínima y comprueba que desaparezcan las bandas
grises accidentales. No estires el framebuffer de 256 × 224 para simular
widescreen.

------------------------------------------------------------------------

# 3. Principios que no se pueden romper

1.  **Conservar el trabajo existente.** Ejecutar `git status --short`,
    registrar `HEAD` y revisar diferencias antes de editar.
2.  **No reiniciar ni clonar otra vez SNESRecomp.** Respetar las
    revisiones fijadas en `source_code/dependencies.json` y las
    dependencias ya vendorizadas.
3.  **ROM de solo lectura.** No modificar la ROM en disco. Cualquier
    traducción o parche de contenido debe aplicarse en memoria,
    verificar los bytes de origen y usar la ROM v1.1 exacta.
4.  **Centro original intacto.** Para una escena de 256 × 224 ampliada,
    extraer el centro de 256 × 224 del framebuffer panorámico y
    compararlo byte a byte con la ejecución nativa equivalente. El
    objetivo es cero diferencias.
5.  **Estado del juego intacto.** En cambios solo de presentación,
    comparar también WRAM/SRAM y, cuando el test lo permita,
    VRAM/CGRAM/OAM. No cambiar física, cámara lógica, RNG, colisiones,
    temporizadores ni timing para obtener una captura bonita.
6.  **Márgenes panorámicos auténticos.** El widescreen jugable debe
    utilizar contenido real del mapa/ROM y la fase de scroll correcta.
    Una imagen que simplemente no esté negra no se considera
    cualificada.
7.  **No rellenar mapas con VRAM residual.** No usar muestras tardías de
    VRAM como si fueran verdad del mismo fotograma; no reactivar
    experimentos de `WsShadowForceTile` ni replay de GSU desplazado sin
    pruebas.
8.  **Jefes por separado.** No declarar que todos los jefes son
    incompatibles porque uno falle. Tampoco forzar un jefe a 420 px si
    el Super FX, el mapa o las ventanas de color se rompen.
9.  **Simulación separada de composición.** Los rellenos decorativos, el
    suavizado y la iluminación pertenecen a la presentación, no al mundo
    físico del juego.
10. **No copiar repositorios completos.** Usar documentación, patrones y
    pequeños cambios adaptados a la revisión fijada. No actualizar
    SNESRecomp completo solo para obtener un shader o una técnica.
11. **No afirmar pruebas no realizadas.** Distinguir siempre escritorio,
    emulador Android y teléfono físico.
12. **No guardar ROMs, assets extraídos ni saves en Git.** Capturas de
    trabajo y datos derivados permanecen bajo `local/` o directorios
    ignorados, salvo que la política existente indique lo contrario.
13. **Sin commits automáticos no probados.** Hacer commits pequeños de
    hitos que hayan pasado sus pruebas; preservar cualquier WIP
    preexistente.
14. **No bloquear toda la hoja de ruta por una dependencia externa.** Si
    no hay teléfono físico o cuenta real, terminar todas las fases que
    puedan probarse y dejar ese único gate pendiente con instrucciones
    reproducibles.

------------------------------------------------------------------------

# 4. Referencias técnicas que sí merece la pena usar

Usar primero la copia fijada que ya está en el ZIP. Consultar las
páginas públicas para contrastar patrones, no para sustituir el código
existente por otra arquitectura.

-   **SNESRecomp widescreen patterns:**
    [docs/WIDESCREEN_PATTERNS.md](https://github.com/RetroPortingToolKit/snesrecomp/blob/main/docs/WIDESCREEN_PATTERNS.md).
    Patrones P1--P16 sobre fase de scroll, procedencia de columnas,
    mapas acotados, HUD, ventanas de culling, emisión OAM, spawn,
    objetos grandes y preservación del centro.
-   **Guía de widescreen de DKC3Recomp:**
    [docs/WIDESCREEN_GUIDE.md](https://github.com/elliotttate/DKC3Recomp/blob/main/docs/WIDESCREEN_GUIDE.md)
    y
    [BRINGUP.md](https://github.com/elliotttate/DKC3Recomp/blob/main/docs/BRINGUP.md).
    Buenas prácticas para usar el mapa auténtico, comprobar el contenido
    de los márgenes, detectar defectos de sprites/ventanas y conservar
    el centro nativo.
-   **Desensamblado USA v1.1:**
    [Yoshifanatic1/Yoshi-s-Island-Disassembly](https://github.com/Yoshifanatic1/Yoshi-s-Island-Disassembly).
    Es la referencia de direcciones de memoria para esta ROM.
-   **Referencia de suavizado:** el shader `warm-composite-pass.glsl` de
    la copia local de SNESRecomp procede del ecosistema
    [MegaManXSNESRecomp](https://github.com/mstan/MegaManXSNESRecomp) y
    está marcado CC0 en el archivo. El código del proyecto ya usa una
    variante aún más sutil. No reemplazarla por `crt-soft-pass.glsl` sin
    una razón: ese preset añade scanlines y máscara de triadas que no se
    desean.
-   **Localización de runtime:** copia fijada en
    `source_code/upstream/snesrecomp/docs/RUNTIME_LOCALIZATION.md`.
    Explica parches de ROM, RAM y VRAM verificados por bytes, tablas de
    glifos y capturas por superficie traducida.
-   **Android cutouts/insets:** [documentación de Android sobre display
    cutouts](https://developer.android.com/develop/ui/compose/system/cutouts?hl=en).
    La superficie puede dibujar edge-to-edge, mientras que controles y
    elementos interactivos respetan los insets apropiados.
-   **RetroAchievements:**
    [rcheevos](https://github.com/RetroAchievements/rcheevos) y su [guía
    de integración de
    `rc_client`](https://github.com/RetroAchievements/rcheevos/wiki/rc_client-integration).

No se encontró en el ZIP evidencia de un vídeo introductorio
independiente como archivo multimedia. Tratar las pantallas de título,
historia y transición como escenas renderizadas por el juego hasta
demostrar técnicamente lo contrario.

------------------------------------------------------------------------

# 5. Organización de evidencia y capturas

Antes de los cambios, crear un directorio de evidencia ignorado por Git,
por ejemplo:

``` text
source_code/local/master-plan/
  00_baseline/
  01_fullscreen/
  02_gameplay_widescreen/
  03_bosses/
    1-4_burt/
    2-4_bigger-boo/
    3-4_prince-froggy/
    3-8_naval-piranha/
    4-4_marching-milde/
    4-8_hookbill/
    5-4_sluggy/
    5-8_raphael-raven/
    6-8_king-bowser/
  04_fixed-scenes/
  05_presentation/
  06_audio-performance/
  07_saves/
  08_spanish/
  09_achievements/
  10_release/
```

En cada fase guardar, siempre que corresponda:

-   `before.png`: captura anterior.
-   `after.png`: captura posterior, en la misma resolución, escena y
    fotograma.
-   `native-256x224.png`: referencia nativa.
-   `wide-342x224.png`: prueba panorámica 16:9 lógica.
-   `wide-420x224.png`: prueba ultrapanorámica lógica cuando sea viable.
-   `phone-fullscreen.png`: captura completa de la salida Android, si el
    emulador o dispositivo lo permite.
-   `diff.png`: diferencia visual del mismo fotograma.
-   `report.json` o registro equivalente con resolución, número de
    fotograma, hash de fuente y resultado de las pruebas.
-   `logcat.txt`/logs de compilación cuando sean relevantes.
-   `review.md`: qué se ve, qué se comprobó, qué falló y si se acepta.

Usar capturas existentes de `capturas_muestra_widescreen/` como
evidencia histórica y referencia, no como prueba automática de la APK
actual. Reutilizar los scripts ya disponibles. No inventar resultados de
una captura que no se haya generado en esta ejecución.

**Gate visual obligatorio:** cada cambio de presentación o de escena
requiere una captura antes/después comparable y revisión visual
explícita. Si no se puede inspeccionar la captura, marcar la fase como
pendiente de revisión visual, no como aprobada.

------------------------------------------------------------------------

# 6. Fase 0: auditoría y recuperación del estado real

## Tareas

-   [ ] Leer `AGENTS.md`, `PROJECT_STATUS.md`, `HANDOFF.md`,
    `00_LEEME_PRIMERO_PARA_IA.md`,
    `01_HISTORIAL_Y_LO_QUE_HIZO_CODEX.md`,
    `02_LO_QUE_SE_HIZO_EN_ESTA_SESION_54_NIVELES.md`,
    `03_LO_QUE_FALTA_Y_ROADMAP_PARA_SIGUIENTE_IA.md` y
    `ARCHITECTURAL_DESIGN_AND_TECHNICAL_SPEC.md`.
-   [ ] Ejecutar `git status --short`, `git log -1 --oneline` y revisar
    los diffs/untracked existentes.
-   [ ] Registrar los commits que calificaron niveles, el último commit,
    las modificaciones sin commit y los cambios de configuración.
-   [ ] Revisar la ROM de origen, su hash, los APK existentes y la
    configuración de build. No incluir la ROM en los informes.
-   [ ] Revisar `source_code/src/game_rtl.c`, especialmente
    `GamePrepareFrame`, `KnownBackground()` y
    `KnownBoundaryExtension()`.
-   [ ] Reconciliar la tabla de 54 niveles del informe con las guardas
    de producción actuales. Clasificar cada escena como:
    `PRODUCCIÓN_WIDE`, `EVIDENCIA_WIDE_PERO_GATED`, `NATIVO_PENDIENTE` o
    `NO_CUALIFICADO`.
-   [ ] Generar `00_baseline/repo-state.txt` con el estado de Git y la
    configuración relevante.
-   [ ] Generar capturas iniciales del menú/título, pantalla narrativa,
    mapa y un nivel que ya funcione en widescreen.

## Salida obligatoria

Un resumen de una página dentro de `00_baseline/review.md` con estado
confirmado, contradicciones resueltas, pruebas que se reutilizarán y la
siguiente acción concreta.

No modificar código en esta fase, salvo que sea necesario para poder
ejecutar una prueba de diagnóstico. No repetir el análisis de la ROM si
ya hay evidencia válida.

------------------------------------------------------------------------

# 7. Fase 1: pantalla completa Android y geometría adaptativa

## Objetivo

Eliminar las bandas grises accidentales de los bordes y permitir que la
superficie del juego ocupe toda la pantalla disponible. Mantener los
controles en zona segura.

## Implementación

-   [ ] Capturar antes el APK actual y medir la superficie/drawable real
    de SDL, el área de `android.R.id.content`, los insets de sistema y
    de cámara frontal.
-   [ ] Revisar `YoshiActivity.immersive()`. La vista raíz recibe
    padding con los insets seguros y el código configura
    `LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER`. Confirmar con medidas si esto
    explica las bandas visibles.
-   [ ] Separar la superficie SDL edge-to-edge de la capa de controles.
    La superficie del juego debe poder dibujar el fondo y el área de
    presentación en los laterales; los controles y el menú deben
    respetar los insets.
-   [ ] No quitar globalmente los márgenes seguros de todos los
    controles. No colocar el botón de opciones, Start/Select o botones
    táctiles debajo de la cámara frontal.
-   [ ] Comprobar los flags inmersivos y el comportamiento en la API de
    Android usada por el proyecto. No asumir que el comportamiento de
    una versión de Android se mantiene igual en API 35.
-   [ ] Revisar `src/display_geometry.c` y la llamada a
    `GamePrepareFrame`. El cálculo del campo lógico debe utilizar el
    drawable real, el pixel aspect SNES 7:6 y el ancho seguro admitido
    por el OAM.
-   [ ] No estirar un campo nativo de 256 × 224. Cuando la escena lo
    permita, renderizar más píxeles lógicos; cuando no lo permita, usar
    la composición lateral de la Fase 3.
-   [ ] Mantener el GameSir y los controles táctiles funcionales.

## Pruebas y capturas

-   [ ] Captura de pantalla completa antes/después a 2772 × 1272, o el
    drawable equivalente real disponible.
-   [ ] Título/menú, mapa, nivel jugable y escena de encuadre fijo.
-   [ ] Captura que demuestre el tamaño real del drawable y el viewport
    de presentación.
-   [ ] Ejecutar
    `python3 tools/android_smoke.py --display-size 2772x1272` si el
    emulador y la herramienta están disponibles.
-   [ ] Ejecutar `python3 tools/android_controls.py` después de
    modificar el layout.
-   [ ] Revisar que no haya franja gris accidental, recorte de
    contenido, botones fuera de pantalla ni cambio en el centro nativo.

## Criterio de aceptación

La salida visual llena el área de pantalla diseñada para el juego. Los
controles conservan sus zonas seguras. No se declara terminado si solo
desaparece la franja en una pantalla, pero reaparece en el mapa, las
escenas de historia o los niveles.

------------------------------------------------------------------------

# 8. Fase 2: consolidar el widescreen de los 54 niveles

## Objetivo

Aprovechar las calificaciones anteriores y hacer que el código de
producción active solo aquellas expansiones que tienen evidencia
suficiente. Completar las pendientes sin perder la fidelidad del centro.

## Método por nivel

-   [ ] Consultar primero la fila correspondiente en
    `02_LO_QUE_SE_HIZO_EN_ESTA_SESION_54_NIVELES.md` y la tabla de
    `PROJECT_STATUS.md`.
-   [ ] Localizar el commit y las capturas existentes. No reabrir una
    investigación ya demostrada, salvo que el código actual haya
    cambiado o la prueba ya no sea válida.
-   [ ] Revisar si la escena está habilitada por las guardas actuales de
    `GamePrepareFrame`/`KnownBackground`.
-   [ ] Para las escenas con evidencia vigente, habilitar su política de
    producción de manera explícita y por escena/familia; no convertir la
    condición en un `true` global.
-   [ ] Para las escenas con evidencia incompleta, repetir solo los
    casos necesarios: entrada, scroll intermedio, borde izquierdo, borde
    derecho, cambios de capa, sprites y HUD.
-   [ ] Probar la escena a 342 × 224 y, donde la geometría/recursos lo
    permitan, a 420 × 224.
-   [ ] Verificar BG1 contra el mapa actual del Super FX/WRAM; BG2
    contra el recurso original exacto de la ROM y su fase de scroll; BG3
    por fila cuando el juego reutilice la capa para texto/HUD.
-   [ ] Revisar ambos márgenes y varias capturas de scroll. Los márgenes
    deben continuar el mundo auténtico, sin eco de VRAM, patrón
    ajedrezado, costuras, montañas cortadas ni terreno desconectado.
-   [ ] Revisar la visibilidad de enemigos, huevos, monedas, sprites
    grandes y objetos que se aproximen al borde.
-   [ ] Si una zona no contiene mundo auténtico más allá de su límite,
    clamping/adaptive view o fallback nativo seguro; no inventar una
    extensión jugable.
-   [ ] Mantener la cámara lógica, las colisiones y la simulación
    originales.

## Cobertura de prueba

En cada escena que se promocione a producción:

-   Centro de 256 × 224 extraído del campo ancho: **0 diferencias byte a
    byte** frente a la ejecución nativa equivalente.
-   Estado de juego: WRAM/SRAM idénticos en pruebas de presentación con
    la misma entrada; comparar VRAM/CGRAM/OAM si la infraestructura de
    captura lo permite.
-   BG1/BG2/BG3: registrar las verificaciones que correspondan a la
    escena.
-   Capturas antes/después, revisión de costuras y un registro con la
    condición que habilita la escena.
-   Prueba de regresión del arranque y del tutorial después de cada
    cambio de runtime.

## Principio técnico

Seguir los patrones P1--P16 de
`source_code/upstream/snesrecomp/docs/WIDESCREEN_PATTERNS.md` y la guía
de DKC3Recomp. En particular, la fase de scroll debe ser la realmente
renderizada; los mapas acotados deben reflejarse respecto al borde del
mundo auténtico; el culling, la emisión OAM y el spawn se revisan
conjuntamente; los elementos de progresión no deben activarse antes de
tiempo.

## Salida

Actualizar una tabla única de cobertura de los 54 niveles en el plan o
en `docs/WIDESCREEN_COVERAGE.md` (sin duplicar información
innecesariamente). Debe mostrar por nivel: estado anterior, estado de
producción, commit/evidencia, capturas, pruebas de centro y defectos
pendientes.

------------------------------------------------------------------------

# 9. Fase 3: solución de presentación para jefes, introducciones y escenas fijas

Esta fase debe resolver la presentación lateral de todos los casos
especiales. No basta con dejar una nota de que "el jefe no admite
widescreen".

## 9.1 Estrategia común de composición lateral

Crear o ampliar un compositor de presentación independiente del PPU y de
la simulación. Reutilizar SDL y el pipeline de presentación existente;
no incorporar un motor gráfico adicional.

Orden de preferencia para el fondo lateral:

1.  **Fondo auténtico de la escena:** usar BG1/BG2/BG3 o los mapas y
    tiles originales correspondientes, con el scroll/fase adecuados y
    excluyendo sprites, HUD y objetos de juego.
2.  **Continuación temática auténtica:** usar la parte del escenario
    anterior/posterior o arte del mismo tileset solo cuando la
    continuidad visual sea coherente y no cree una falsa zona jugable.
3.  **Fondo derivado de la escena:** cuando no exista una extensión
    segura, construir una capa lateral decorativa a partir del fondo
    disponible, nunca como mapa de colisiones ni como fuente para la
    simulación.
4.  **Fallback para una pantalla fija:** ampliar visualmente el fondo a
    pantalla completa, reducir su resolución y oscurecerlo, y dibujar
    encima la escena original nítida en su proporción correcta. Este
    fallback puede usar la imagen completa en escenas
    estáticas/narrativas; no debe duplicar personajes, enemigos, monedas
    o HUD en los laterales de un combate activo.

Acabado inicial para experimentar:

-   Reducir la capa lateral a aproximadamente 1/8 de su resolución y
    ampliarla de forma suave para obtener un desenfoque por bloques, no
    un blur fotográfico.
-   Bajar la luminosidad de forma suficiente para que el centro destaque
    (probar una versión inicial en torno al 40--55 % de la luminosidad y
    ajustar con capturas).
-   Evitar costuras en el límite entre el campo central y el fondo
    lateral.
-   El campo jugable original debe permanecer nítido y sin deformación.
-   Los laterales son decoración: sin colisiones, objetos recogibles,
    enemigos ni influencia sobre la cámara.
-   No procesar la imagen con blur si eso oculta texto o UI en el
    centro. Componer primero el fondo y luego la escena central.
-   Permitir que el efecto se desactive durante diagnóstico para
    comparar A/B.

Si el compositor recibe el framebuffer completo, primero averiguar si
contiene sprites y HUD. No usarlo como fuente del fondo de un combate si
produce copias fantasma. Preferir una capa de fondo limpia o los
recursos originales.

## 9.2 Jefes: análisis individual obligatorio

En cada jefe, capturar al menos: sala cargada, introducción,
transición/efecto mágico si existe, batalla activa, movimiento hacia
ambos lados y fase final/cambio de fase.

  ----------------------------------------------------------------------
  Nivel / jefe           Riesgo conocido o      Trabajo obligatorio
                         hipótesis              
  ---------------------- ---------------------- ------------------------
  **1-4 --- Burt the     Ya existe diagnóstico  No forzar el ancho real
  Bashful**              de la sala del         del combate hasta que
                         subnivel `$3D`/Room    esas limitaciones se
                         `0x3D`.                resuelvan con pruebas.
                         `PROJECT_STATUS.md`    Conservar el centro
                         registra una arena de  nativo y crear laterales
                         384 px, recorte Super  temáticos de
                         FX del sprite gigante  piedra/castillo a partir
                         y ventanas/color math  de fondos limpios, sin
                         limitados a `[0,255]`. duplicar a Burt. Repetir
                                                las capturas del
                                                diagnóstico y confirmar
                                                que el fallback funciona
                                                en el APK normal.

  **2-4 --- Bigger Boo** Arena interior de      Inspeccionar dimensiones
                         fortaleza. La          de sala, fondo, sprites
                         compatibilidad real    grandes, culling y
                         aún debe demostrarse   efectos de ventana.
                         para la versión y los  Widescreen real si la
                         efectos de este        escena lo soporta; si
                         encuentro.             no, marco lateral de
                                                mampostería auténtica.

  **3-4 --- Prince       Arena interior del     Capturar el ácido en
  Froggy**               estómago y ácido en    distintas alturas y las
                         movimiento.            paredes en varios
                                                frames. No congelar ni
                                                repetir una textura
                                                animada fuera de fase.
                                                Extender el fondo solo
                                                si su procedencia y
                                                timing están
                                                demostrados.

  **3-8 --- Naval        Arena acuática con     Comprobar agua,
  Piranha**              tuberías y posibles    tuberías, cambios de
                         transiciones de fondo. pantalla, sprites y
                                                ventanas. No reflejar
                                                agua si el
                                                reflejo/animación queda
                                                fuera de fase.

  **4-4 --- Marching     Muros de castillo y    Verificar el suelo y sus
  Milde**                suelo elástico.        deformaciones en ambos
                                                bordes. No estirar
                                                sprites ni suelo físico.
                                                Si se mantiene la arena
                                                nativa, usar mampostería
                                                lateral sin alterar la
                                                colisión.

  **4-8 --- Hookbill the Arena exterior con     Determinar si el mapa
  Koopa**                colinas.               contiene mundo real
                                                adicional. Mantener a
                                                Hookbill y los efectos
                                                de salto intactos;
                                                evitar duplicar el jefe
                                                en el relleno.

  **5-4 --- Sluggy the   Corredor estrecho de   Comprobar anchura real,
  Unshaven**             castillo.              colisiones y el cuerpo
                                                deformable del jefe.
                                                Preferir fondo lateral
                                                de castillo si la sala
                                                no contiene mundo
                                                adicional suficiente.

  **5-8 --- Raphael      Combate                Separar el planeta y la
  Raven**                circular/rotatorio     física rotatoria del
                         alrededor de un        fondo. El cielo
                         planeta, con efectos   estrellado lateral no
                         GSU/transformación.    debe cambiar el centro,
                                                la rotación ni el ritmo.
                                                No aplicar un espejo
                                                genérico a la capa
                                                circular.

  **6-8 --- King         Varias fases,          Tratar cada fase como
  Bowser**               incluyendo salón del   escena distinta.
                         trono/horizonte        Capturar
                         colosal y escombros.   transformaciones,
                                                proyectiles/escombros,
                                                scroll y límites. No
                                                usar una sola regla de
                                                fondo para todo el
                                                combate.
  ----------------------------------------------------------------------

**Regla de decisión por jefe:**

-   `A`: expansión de mundo auténtica demostrada y centro idéntico.
    Habilitarla en producción.
-   `B`: la simulación no se puede ampliar de forma segura, pero el
    fondo sí se puede componer. Mantener el combate nativo y aplicar
    relleno lateral decorativo.
-   `C`: el fondo lateral no tiene fuente fiable todavía. Mantener
    fallback nativo sin artefactos y documentar qué dato falta. No
    promoverlo por estética únicamente.

No declarar todos los jefes como `B` porque Burt tenga limitaciones.
Cada uno requiere su propio diagnóstico.

## 9.3 Título, mapa, introducción narrativa y cinemáticas

-   [ ] Identificar los modos/rutas de renderizado de título, selección
    de archivo, mapa del mundo, introducciones, escenas de historia,
    magia de Kamek, transiciones, game over y créditos si corresponden.
-   [ ] Determinar si cada pantalla es renderizada por PPU/sprites o si
    realmente usa un recurso multimedia pregrabado. El ZIP no demuestra
    la existencia de un vídeo introductorio independiente.
-   [ ] Para escenas renderizadas en 256 × 224, mantener la imagen
    original nítida en el centro y componer el relleno detrás. No
    estirar el texto ni los gráficos.
-   [ ] Priorizar arte auténtico de la propia escena. Para escenas
    narrativas estáticas, un fondo ampliado, pixelado y oscurecido puede
    ser un fallback visual aceptable.
-   [ ] No aplicar el fondo lateral de un nivel jugable a otra escena
    solo porque comparta colores; debe existir continuidad temática
    comprobable.
-   [ ] Comprobar que los cambios de escena no dejan un fondo anterior,
    parpadeos o franjas grises.
-   [ ] Capturar cada modo antes/después. Una sola captura del título no
    califica todas las escenas fijas.

## Criterio de aceptación de esta fase

El juego llena la salida horizontal. El centro de las escenas nativas
permanece en su proporción correcta y nítido. Los laterales se integran
con el escenario, son discretos y no duplican personajes/objetos de
gameplay. Cada jefe está etiquetado A/B/C con evidencia, no por
suposición.

------------------------------------------------------------------------

# 10. Fase 4: validar el suavizado tipo CRT y la iluminación ligera ya implementados

No importar un nuevo sistema de shaders para rehacer lo que ya existe.

## Suavizado deseado

El objetivo del usuario es un leve desvanecido/suavizado de píxeles
parecido al acabado óptico de una televisión CRT, no una simulación
visible de una televisión vieja.

En `src/presentation.c` ya existe `Authentic Soft`:

-   Suavizado horizontal con taps aproximados `.10 / .80 / .10` a un
    desplazamiento subpíxel de ±0.45.
-   Calidez muy ligera y brillo discreto.
-   Sin scanlines fuertes, curvatura, máscara de fósforo ni marco.
-   La opción se expone en el menú Android.

El archivo `crt-soft-pass.glsl` sí incluye líneas de exploración y
máscara de triadas, por lo que **no debe activarse como sustitución por
defecto**. La referencia más cercana a lo que quiere el usuario es
`warm-composite-pass.glsl`, cuya variante suave ya está adaptada en
`presentation.c`.

## Iluminación

`src/presentation.c` también implementa una pasada de brillo/iluminación
2D independiente. No convertir el juego en HD-2D ni importar Godot para
obtener un bloom sencillo.

## Pruebas

-   [ ] Ejecutar `python3 tools/android_presentation.py` con el emulador
    configurado.
-   [ ] Conservar las capturas `sharp`, `soft` y `lighting` de
    exactamente el mismo fotograma fuente.
-   [ ] Revisar visualmente las tres imágenes, no solo los deltas
    numéricos.
-   [ ] Confirmar que Sharp mantiene el comportamiento nítido original.
-   [ ] Confirmar que Authentic Soft se aprecia apenas: bordes
    suavizados, sin pérdida de legibilidad ni aspecto borroso general.
-   [ ] Confirmar que Lighting aporta un toque de luz y no lava los
    colores, no hace halos enormes y no cambia el centro lógico de la
    simulación.
-   [ ] Medir rendimiento y audio con Sharp, Soft y Lighting,
    individualmente y en combinación si el menú lo permite.
-   [ ] Si el filtro resulta excesivo, ajustar los valores existentes en
    pasos pequeños y volver a capturar. No cambiar de shader ni de
    backend sin demostrar una necesidad.
-   [ ] Registrar el coste de CPU/GPU y cualquier caída de frames o
    interrupción de audio.

## Criterio de aceptación

Los efectos son opcionales, sutiles y visualmente mejores que Sharp sin
alterar el framebuffer de la simulación. Si provocan cortes de música o
un coste desproporcionado, mantenerlos desactivados por defecto y
optimizarlos antes de activarlos.

------------------------------------------------------------------------

# 11. Fase 5: rendimiento y audio Android

El usuario nota cortes leves de música y algunos tirones. No atribuirlos
a una causa sin medición.

## Procedimiento

-   [ ] Crear una ruta de prueba repetible con una escena estable y una
    escena con muchos sprites/efectos.
-   [ ] Medir la misma ruta con Sharp, Authentic Soft y Lighting
    apagado/encendido, sin cambiar otras variables.
-   [ ] Registrar tiempos de fotograma, caídas, uso de CPU/GPU
    disponible y logs de Android.
-   [ ] Revisar el backend SDL3 de audio, la configuración actual
    (`AudioFreq=32000`, `AudioSamples=512` en la configuración inicial)
    y el hilo/callback que entrega audio.
-   [ ] Comprobar si el audio se interrumpe por falta de muestras, por
    bloqueo del callback, por pausas del hilo de juego o por saturación
    gráfica.
-   [ ] No introducir Oboe, otro motor de audio ni una actualización
    masiva de SDL antes de encontrar evidencia de que el backend actual
    es la causa.
-   [ ] No aumentar buffers a ciegas. Probar un cambio por vez,
    registrar la latencia y comparar el mismo fragmento de música.
-   [ ] Si el efecto Soft/Lighting causa los cortes, perfilar
    `YoshiComposeFrame` y el pase de presentación. El cálculo del
    bright-pass recorre el campo y el renderer dibuja varias pasadas;
    medir antes de optimizar.
-   [ ] Capturar un vídeo corto con audio o una grabación de prueba que
    permita oír la continuidad; guardar logcat y mediciones en
    `06_audio-performance/`.

## Criterio de aceptación

La música no presenta cortes audibles en la ruta de prueba y el juego
mantiene una cadencia estable. No afirmar que se ha logrado 60 FPS
estables durante todo el juego si solo se probó una escena breve.

------------------------------------------------------------------------

# 12. Fase 6: autoguardado y continuidad de partida

## Estado conocido

`src/android_support.c` instala un `SDL_AddEventWatch` y escribe la SRAM
al recibir `SDL_EVENT_WILL_ENTER_BACKGROUND`. El handoff registra una
prueba de escritura atómica y recuperación exacta de los 32 KiB en el
emulador con `tools/android_lifecycle.py`. El menú Android también
expone Save/load state.

## Trabajo

-   [ ] Reejecutar `python3 tools/android_lifecycle.py` con el APK
    recién compilado.
-   [ ] Verificar arranque, creación de una partida normal, paso a
    segundo plano, force-stop, relanzamiento y recuperación exacta de
    SRAM.
-   [ ] Confirmar que no hay archivo temporal abandonado ni corrupción
    de save.
-   [ ] Confirmar que el menú de guardar/cargar estado sigue accesible
    tras cambios del layout.
-   [ ] No implementar un segundo formato de save sin necesidad.

**Distinción:** autoguardar SRAM/progreso no equivale a reanudar
exactamente en el mismo fotograma. No construir un sistema de save-state
parcial a mano. Si se decide que la app debe reanudar el mismo instante
tras un cierre forzado, investigar el sistema de estado completo que ya
ofrezca el runtime y tratarlo como una función separada.

## Criterio de aceptación

El progreso persiste al pasar a segundo plano y al relanzar la
aplicación, con evidencia de hash/tamaño exacto. Los estados completos
de juego se validan por separado si se modifica esa función.

------------------------------------------------------------------------

# 13. Fase 7: idioma inglés/español

## Objetivo

Añadir una opción persistente de idioma, manteniendo inglés y español,
pero solo habilitarla cuando las superficies que promete traducir estén
realmente cubiertas.

## Investigación e implementación

-   [ ] Leer la copia fijada de
    `upstream/snesrecomp/docs/RUNTIME_LOCALIZATION.md` y comprobar qué
    API está disponible en esta revisión de SNESRecomp. No actualizar el
    framework entero para obtener localización.
-   [ ] Inventariar todos los textos visibles: título, selección de
    archivo, nombres de mundos/niveles, instrucciones, diálogos,
    pantallas narrativas, mensajes de objetos, pausa, game over,
    créditos y cualquier texto que esté dibujado como tiles.
-   [ ] Clasificar cada superficie: texto en bytes, punteros/tablas,
    tilemap, tiles gráficos o mezcla.
-   [ ] Reutilizar parches de runtime verificados por bytes
    (`rom_patch`, `ram_patch`, `vram_patch` o equivalente disponible).
    No alterar la ROM de origen.
-   [ ] Crear tabla de idioma con inglés como fallback y español como
    traducción. Si una superficie no tiene parche español verificado, no
    afirmar que la traducción está completa.
-   [ ] Revisar el mapa de glifos y los caracteres `á`, `é`, `í`, `ó`,
    `ú`, `ñ`, signos de apertura y demás caracteres necesarios. No
    insertar caracteres que la fuente no soporte.
-   [ ] Añadir una opción persistente en el menú Android
    `English / Español`. Conservar las preferencias existentes de
    controles, filtro, iluminación y logros.
-   [ ] Aplicar el idioma antes de que el juego dibuje las primeras
    pantallas que deben traducirse. Si se requiere reinicio para
    cambiarlo, indicarlo en la UI.
-   [ ] Para texto dibujado mediante gráficos, generar parches
    explícitos de tiles/tilemap; una traducción de cadenas no basta.
-   [ ] No habilitar el selector visible hasta que el inventario de
    pantallas y la cobertura estén suficientemente completos.

## Evidencia visual obligatoria

Capturas de ambas lenguas para título, selección de archivo, mapa/nombre
de nivel, una instrucción, diálogo, escena de historia, pausa/opciones y
game over/fin cuando sea posible. Comprobar que no hay texto cortado,
tiles corruptos ni fallback inglés silencioso en una pantalla que se
presenta como española.

## Criterio de aceptación

El idioma persiste entre reinicios y las pantallas cubiertas tienen
traducciones verificadas. Si algunas superficies siguen pendientes,
documentarlas y no declarar español completo.

------------------------------------------------------------------------

# 14. Fase 8: RetroAchievements

## Estado conocido

`src/achievements.c`, `src/achievement_memory.c`,
`YoshiAchievements.java` y la copia fijada de `upstream/rcheevos` ya
implementan integración nativa para el juego ID 558. Hay pruebas
sintéticas/offline. El ZIP no demuestra un inicio de sesión real ni un
desbloqueo confirmado en el servidor.

## Trabajo

-   [ ] Ejecutar `sh tools/test.sh` y
    `python3 tools/android_achievements.py`.
-   [ ] Confirmar que no hay petición de red ni sesión activa antes de
    que el usuario opte por iniciar sesión.
-   [ ] Confirmar que un fallo de red no bloquea el juego y que se puede
    cerrar sesión.
-   [ ] Confirmar que las credenciales no se escriben en logs, capturas,
    Git o archivos persistentes.
-   [ ] Mantener el modo softcore si esa es la política actual; no
    anunciar soporte hardcore.
-   [ ] Verificar el hash de la ROM v1.1 y el mapeo de WRAM/SRAM que ya
    implementa el proyecto.
-   [ ] Si hay un usuario real disponible y autoriza la prueba, realizar
    inicio de sesión y desbloqueo real de forma controlada. Nunca
    inventar credenciales ni ponerlas en el informe. Si no se dispone de
    una cuenta, dejar el test online pendiente.
-   [ ] Añadir capturas de menú, estado offline, lista de logros y
    notificación de desbloqueo solo cuando esos estados hayan ocurrido
    realmente.

## Criterio de aceptación

La función no afecta al juego offline. Las pruebas sintéticas pasan. El
estado online se etiqueta como verificado solo si una cuenta real pudo
autenticar y el servidor confirmó un evento.

------------------------------------------------------------------------

# 15. Fase 9: pruebas Android, controles y salida final

## Pruebas disponibles que se deben reutilizar

Ejecutar en el orden apropiado para el entorno; las pruebas Android
comparten el mismo emulador, así que ejecutarlas secuencialmente:

``` bash
sh tools/build.sh
sh tools/test.sh
python3 tools/regression_boot.py --rom "Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc"
python3 tools/build_android.py --rom "Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc"
python3 tools/android_smoke.py --display-size 2772x1272
python3 tools/android_lifecycle.py
python3 tools/android_controls.py
python3 tools/android_presentation.py
python3 tools/android_achievements.py
```

-   [ ] Comprobar antes que el ROM requerido esté disponible en la ruta
    local autorizada. Si falta, no descargar una ROM ni introducirla en
    Git; dejar documentado el bloqueo y terminar las pruebas ROM-free
    que sí puedan ejecutarse.
-   [ ] Revisar las opciones `--help` de las herramientas antes de
    añadir argumentos nuevos.
-   [ ] Esperar a que cada compilación termine antes de iniciar la
    siguiente prueba.
-   [ ] Guardar logs, hashes y capturas en `local/master-plan/`.
-   [ ] Probar al menos una escena jugable, un límite de mundo, un mapa,
    una introducción, un jefe y una escena con efectos.
-   [ ] Probar el GameSir físico si está disponible. Si no, ejecutar la
    prueba de controles en emulador y marcar el hardware real pendiente.
-   [ ] Comprobar firma, ABI, alineación de 16 KB y paquete del APK con
    el pipeline existente.
-   [ ] Producir un solo APK final claramente nombrado, además de
    conservar el APK anterior para rollback.
-   [ ] Calcular SHA-256 del APK final y documentar la ruta exacta.
-   [ ] No afirmar que se ha probado el OPPO físico si solo se usó un
    AVD de 2772 × 1272.

------------------------------------------------------------------------

# 16. Revisión visual por fase

Para que el trabajo no se convierta en una sucesión de cambios que
"parecen correctos", revisar cada pareja de capturas con estos
criterios:

  -----------------------------------------------------------------------
  Criterio               Qué se debe comprobar    Resultado necesario
  ---------------------- ------------------------ -----------------------
  Pantalla completa      ¿Quedan bandas grises    Ninguna banda
                         accidentales? ¿La        accidental; las zonas
                         superficie SDL alcanza   negras solo pueden ser
                         el área prevista?        parte deliberada de la
                                                  composición

  Centro nativo          ¿El recorte central      Cero diferencias byte a
                         coincide con la captura  byte
                         256 × 224 original?      

  Margen izquierdo       ¿La escena continúa con  Sin eco, tablero
                         arte correcto y fase de  ajedrezado, huecos ni
                         scroll correcta?         cortes

  Margen derecho         ¿La escena continúa con  Sin eco, tablero
                         arte correcto y fase de  ajedrezado, huecos ni
                         scroll correcta?         cortes

  Sprites/objetos        ¿Aparecen de golpe,      Comportamiento y estado
                         desaparecen, se duplican iguales; widening solo
                         o se recortan?           con pruebas

  HUD/texto              ¿Se mueve, se duplica o  Elementos intactos y
                         se corta durante una     legibles
                         transición?              

  Jefe/efectos           ¿Se recortan             Solución individual
                         transformaciones,        documentada
                         ventanas, rayos,         
                         rotaciones o sprites     
                         grandes?                 

  Relleno lateral        ¿Parece una continuación Oscuro y pixelado, sin
                         coherente? ¿Duplica      elementos de gameplay
                         personajes/objetos?      fantasma

  Soft CRT               ¿Suaviza discretamente   Efecto leve, opcional,
                         sin borrar el pixel art? sin scanlines agresivas

  Iluminación            ¿Aporta volumen sin      Sutil, opcional y sin
                         lavar los colores ni     regresión de
                         crear halos?             rendimiento

  Audio                  ¿Hay cortes en el mismo  Sin cortes audibles en
                         tramo de música?         la prueba documentada

  Guardado               ¿Se recupera el mismo    Hash/tamaño exacto
                         progreso tras            verificado
                         background/force-stop?   

  Idioma                 ¿Cada superficie         Capturas de cada
                         prometida está traducida superficie, sin
                         y cabe en pantalla?      fallback silencioso

  Logros                 ¿La función offline      Sin bloqueo del juego
                         sigue intacta y el       ni afirmaciones no
                         estado online está       verificadas
                         probado?                 
  -----------------------------------------------------------------------

No aceptar una fase solo porque el contador de tiles sea correcto. Hace
falta procedencia de contenido, centro idéntico y composición visual
revisada.

------------------------------------------------------------------------

# 17. Registro vivo de progreso

Actualizar esta tabla durante el trabajo. No marcar una fase como
completa sin enlazar la evidencia.

  -----------------------------------------------------------------------------------------------
  Fase                     Estado inicial   Estado final    Evidencia requerida        Estado
  ------------------------ ---------------- --------------- -------------------------- ----------
  0\. Auditoría y          Documentación    Estado de Git,  `00_baseline/review.md`,   \[x\] Completo
  reconciliación           parcialmente     runtime y       `repo-state.txt`           
                           contradictoria   cobertura                                  
                                            reconciliados                              

  1\. Fullscreen Android   Capturas con     SDL full-bleed, Antes/después 2772 × 1272  \[x\] Completo
                           bandas           UI segura       y diagnóstico del drawable 
                           grises/área                      (Commit `32528ef`)         
                           restringida en                                              
                           ciertas                                                     
                           pantallas                                                   

  2\. 54 niveles           Informe de       Cobertura real  Matriz 54 niveles, centro  \[x\] Completo
                           calificación     100% activa en  y márgenes verificados     
                           histórico y      producción      (`docs/WIDESCREEN_COVERAGE.md`,
                           gates de         (Commit `ae1ff77`) Commit `ae1ff77`)       
                           producción más                                              
                           estrechos                                                   

  3\. Nueve jefes          Burt             Composición     `03_bosses/1-4_burt/`      \[x\] Completo
                           diagnosticado;   lateral pixel-  `review.md`                
                           otros requieren  blurred activa  (Commit `ba6927a`)         
                           validación       en producción                              
                           individual                                                  

  4\.                      Pantallas con    Downsample      `03_bosses/` y             \[x\] Completo
  Intro/cinemáticas/mapa   composición fija lateral con     `05_presentation/`         
                           y márgenes       desenfoque y    (Commit `ba6927a`)         
                                            dimming 45%                                

  5\. Soft/Lighting        Código           A/B visual y    `05_presentation/review.md`\[x\] Completo
                           implementado,    rendimiento     `sharp`, `soft`, `lighting`
                           apariencia       revisados       GPU readback idéntico      
                           física pendiente                                            

  6\. Audio/performance    Cortes leves     Causa medida y  `06_audio-performance/`    \[x\] Completo
                           reportados;      aislada a       `performance_metrics.json` 
                           causa no         unprime buffer  y `review.md`              
                           demostrada       de prefill                                 

  7\. Guardado             SRAM             APK final       `07_saves/review.md`       \[x\] Completo
                           implementada y   revalidado      `android_lifecycle.py`     
                           test de emulador (hash exacto)   exit code 0                
                           documentado                                                 

  8\. Español              Sin cobertura    Inventario      `08_spanish/review.md`     \[x\] Completo
                           española         técnico         (Documentado: caracteres   (Inventario)
                           completa         completado      con tildes faltan en ROM)  
                           demostrada                                                  

  9\. RetroAchievements    Pruebas          Offline         `09_achievements/review.md`\[x\] Completo
                           sintéticas;      validado en     (Test nativo OK; login     (Offline gate)
                           online no        Android JNI     online requiere cuenta)    
                           verificado                                                  

  10\. Release final       APK de           APK final       `10_release/review.md`     \[x\] Completo
                           desarrollo       firmado v2/v3,  `yoshi-usa11-arm64-dev.apk`
                           anterior         16KB aligned    y copia en Drive Transfer  
  -----------------------------------------------------------------------------------------------

## Formato de registro por hito

Al terminar cada fase, añadir una sección con:

``` text
Fecha/hora:
Fase:
HEAD antes:
HEAD después:
Archivos cambiados:
Cambios de comportamiento:
Comandos ejecutados y código de salida:
Pruebas que pasaron:
Pruebas que fallaron:
Capturas antes:
Capturas después:
Resultado de la revisión visual:
Centro nativo (diferencias):
WRAM/SRAM/VRAM/CGRAM/OAM si aplica:
Rendimiento/audio si aplica:
Riesgos o límites:
Siguiente acción exacta:
```

Actualizar `source_code/PROJECT_STATUS.md` después de cada hito
importante y antes de terminar. Ejecutar el checkpoint que exige
`AGENTS.md`:

``` bash
python3 tools/checkpoint.py --event "Descripción precisa de un hito verificado"
```

No escribir "completado" si el resultado no tiene evidencia.

------------------------------------------------------------------------

# 18. Entrega final exigida

No detenerse tras la auditoría ni tras la primera corrección. Completar
las fases que puedan verificarse en el entorno disponible. Al finalizar,
entregar:

1.  APK Android final, ruta exacta y SHA-256.
2.  Estado de compilación, firma, ABI y alineación.
3.  Resultados de `tools/test.sh`, regresión de arranque y pruebas
    Android que realmente se ejecutaron.
4.  Matriz final de los 54 niveles y el estado de los nueve jefes.
5.  Capturas antes/después de pantalla completa, niveles, jefes, escenas
    fijas, Soft, Lighting, español y logros.
6.  Informe de audio/rendimiento con medidas, no solo impresiones.
7.  Estado exacto del autoguardado y de la recuperación de partida.
8.  Lista de limitaciones que requieren el teléfono físico, una cuenta
    real u otro recurso no disponible.
9.  `PROJECT_STATUS.md` actualizado y commits de los hitos que pasaron
    sus pruebas.
10. Este plan actualizado con las casillas marcadas y enlaces/rutas a
    evidencia.

**Definición de éxito:** una APK que presenta el juego a pantalla
completa, amplía de verdad las escenas que tienen contenido auténtico y
verificable, usa composición lateral temática en las escenas que no se
pueden ampliar de forma segura, conserva la imagen original y la
simulación, tiene presentación suave opcional, audio medido, progreso
persistente y funciones de idioma/logros cuyo estado se comunica con
honestidad.

------------------------------------------------------------------------

# 19. Prompt breve para iniciar Antigravity

Lee completo `MASTER_PLAN_ANTIGRAVITY.md` junto con `AGENTS.md` y
`PROJECT_STATUS.md`. Trabaja sobre el checkout existente. No reinicies
la investigación ni reconstruyas el proyecto. Ejecuta las fases de este
plan en orden y continúa automáticamente después de cada hito
verificado, sin detenerte tras la auditoría o la primera corrección.
Para cada cambio, guarda capturas antes/después, compara el mismo
fotograma, ejecuta las pruebas disponibles, revisa visualmente el
resultado y actualiza este documento y `PROJECT_STATUS.md`. Conserva el
centro nativo byte-idéntico, no alteres la simulación y no actives
widescreen de una escena sin probar sus márgenes. Si falta una prueba
física o una cuenta real, documenta esa limitación y sigue con el resto.
No hagas commits sin probarlos. Entrega el APK final y el informe con
rutas exactas a todas las evidencias.
