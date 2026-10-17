# Plan maestro definitivo v5: acabado visual, 120 Hz, mandos, guardado y QA final

## Super Mario World 2: Yoshi's Island · Android ARM64 · Antigravity

**Revisión:** 16 de octubre de 2026 · v5, incorpora las capturas reales recientes, la auditoría del icono, la presentación de escenas, la iluminación contextual, la frecuencia de pantalla, los mandos y la publicación segura\
**Propósito:** cerrar el proyecto existente con una pasada final de acabado, correcciones visuales, efectos 2D contextualizados, controles completos, rendimiento y pruebas reproducibles. No rehacer el juego ni abrir una nueva etapa de investigación indefinida: entregar una APK jugable y disfrutable, con los defectos conocidos corregidos y evidencia honesta.

### Historial y decisiones heredadas de v3/v4

-   La flecha circular hacia arriba **no es el cursor de apuntado de
    huevos**. Las capturas muestran que puede quedar tapada por una
    montaña/capa, cortarse por el borde superior o no verse en
    determinados fotogramas. Hay que rastrear su origen y comparar
    framebuffer nativo, composición panorámica y ROM original.
-   El círculo negro `10` es el contador de Baby Mario, no un contador
    de días. Se activa al separarse Baby Mario y puede recuperar tiempo
    al rescatarlo. Como aparece en una captura mientras Baby Mario
    parece estar montado en Yoshi, hay que comparar estado, visibilidad
    y temporización con la ROM original antes de llamarlo bug o
    eliminarlo.
-   La traducción española de Wave para USA Rev. 1 existe; se verifica
    su integración, no se empieza desde cero.
-   El panel lateral puede usar gesto desde el borde, pero conserva un
    acceso visible.
-   El fallback pixelado de jefes usa ampliación nearest-neighbour y
    paleta del escenario, no un blur lineal genérico.
-   La iluminación se juzga por capturas comparables y rendimiento, no
    por un límite artificial de delta RGB inferior a 8.
-   El QA exige evidencia individual de los 54 niveles y de los 12
    encuentros de jefe; una hoja de 10+ capturas es solo un resumen.
-   Las capturas no se guardan por acumularlas: cada lote se compara,
    inspecciona, registra y genera correcciones concretas. Después se
    repite la prueba para confirmar el arreglo.

> **Esta revisión v5 prevalece sobre los estados «aprobado» de documentos
> anteriores cuando el usuario ha rechazado visualmente el resultado.**
> No se acepta una fase por tener un informe antiguo o porque compile.
> Se acepta cuando la nueva captura, la prueba y la inspección visual
> cumplen los criterios de este documento.

------------------------------------------------------------------------

## 0. Instrucciones obligatorias para el agente

1.  Trabaja sobre el **checkout completo que ya existe en la
    computadora**, no sobre una reconstrucción ni sobre una extracción
    del ZIP de referencia.
2.  Antes de modificar nada, guarda `git status --short`,
    `git rev-parse HEAD`, las diferencias existentes y la ruta del
    proyecto. No borres ni sobrescribas cambios del usuario.
3.  **No compares ZIPs de builds ni reconstruyas el proyecto a partir de archivos adjuntos.** El ZIP de reportes/capturas aportado por el usuario es evidencia de contexto para esta revisión, no un checkout que debas reconstruir ni una versión de código que debas comparar. El código fuente y el historial válidos están en el checkout local. Continúa desde ahí y consulta `AGENTS.md`, `PROJECT_STATUS.md`, `HANDOFF.md`, `MASTER_PLAN_ANTIGRAVITY.md`, informes, código y pruebas existentes. No pidas otro ZIP ni gastes tiempo comparando snapshots.
4.  No pidas al usuario que prepare imágenes, amplíe vídeos o
    reconstruya assets manualmente como primer recurso. Inspecciona
    primero los recursos, las rutas de renderizado y las herramientas
    que ya existen. Solo solicita un archivo concreto si demuestras que
    la fuente necesaria no está en el proyecto ni es accesible desde el
    entorno local; pide únicamente ese archivo fuente, no que el usuario
    lo edite.
5.  Reutiliza el código, los scripts, las dependencias fijadas y las
    pruebas ya existentes. No migres de SDL, no actualices todo
    SNESRecomp, no rehagas los 54 niveles ni sustituyas el sistema de
    compilación sin una causa comprobada.
6.  No cambies física, colisiones, RNG, temporización, memoria del juego
    ni comportamiento de Super FX para mejorar una captura. Los cambios
    visuales deben ocurrir en la capa de presentación.
7.  No afirmes que un nivel, jefe, idioma, logro o dispositivo está
    probado si no existe evidencia de esta ejecución.
8.  **No te detengas tras la auditoría ni tras la primera corrección.**
    Completa las fases que puedan verificarse, corrige los fallos
    encontrados y vuelve a ejecutar las pruebas afectadas. Si falta una
    cuenta o un teléfono físico, continúa con todo lo demás y deja solo
    ese gate claramente documentado.
9.  Mantén los cambios por fases y con checkpoints. No hagas commits de
    cambios que no hayan pasado sus pruebas.
10. Al limpiar, elimina únicamente temporales creados durante esta
    ejecución. No borres ROMs, partidas, dependencias, cachés útiles ni
    trabajo previo del usuario.
11. La meta es una única APK final y una carpeta pequeña de entrega con
    evidencia esencial, no llenar el proyecto de dumps, imágenes
    temporales y APKs de prueba.

------------------------------------------------------------------------

## 1. Hallazgos comprobados en el código entregado

Estos puntos deben guiar el trabajo; no volver a investigarlos desde
cero salvo que el checkout completo demuestre que el código actual
difiere.

### 1.1 El relleno desenfocado se aplica demasiado ampliamente

En `codigo_fuente/src/presentation.c`:

-   `DownsampleBackdrop()` reduce el fotograma de 256 × 224 a **32 ×
    28** píxeles.
-   `YoshiComposeFrame()` prepara ese fondo para cualquier fotograma de
    256 × 224 si no está activada la variable de diagnóstico que lo
    deshabilita.
-   `YoshiRenderPresentation()` puede dibujarlo en los laterales siempre
    que la imagen central no ocupe toda la salida.

Por tanto, no es un tratamiento exclusivo de jefes. Puede afectar al
mapa, pantallas narrativas y otras escenas nativas. Esto contradice el
acabado que pide el usuario.

**Acción:** sustituir el fallback global por una política explícita por
tipo de escena. No volver a aplicar `DownsampleBackdrop()`
indiscriminadamente.

### 1.2 El filtro y la iluminación no cubren todas las pantallas

En `YoshiComposeFrame()` la preparación del suavizado y la iluminación
se omite cuando `g_ram[0x118]` no es `0x0e` o `0x0f`, salvo en el ciclo
de prueba. Eso puede hacer que el acabado se vea diferente entre
gameplay y otras pantallas, aunque el fondo lateral siga activo.

**Acción:** identificar los tipos reales de escena y aplicar el preset
de presentación de forma coherente a gameplay, mapa, título y escenas
narrativas, respetando las exclusiones de interfaz que correspondan.

### 1.3 La iluminación existente es una pasada genérica, no iluminación por capas

La implementación actual extrae píxeles muy luminosos con un umbral de
luminancia superior a 208 y estima el tinte ambiental a partir de la
proporción global de rojo y azul del fotograma. No identifica el sol,
una lámpara, lluvia, objetos en primer plano ni profundidad de capas. El
informe anterior mide una variación media muy baja para Lighting.

**Acción:** reemplazar esa heurística por iluminación contextual y de
bajo coste. No resolverlo únicamente aumentando el brillo global.

### 1.4 Los menús adicionales usan diálogos Android genéricos

`android/java/com/hector/yoshi/YoshiControls.java` construye el menú con
`AlertDialog.Builder`, expone modos Sharp/Authentic Soft y un
interruptor separado de iluminación, y dibuja permanentemente un círculo
con `...` en la esquina superior.

**Acción:** reemplazar el diálogo genérico por un panel lateral propio,
coherente con la estética del juego, que pause la partida al abrirse.
Puede abrirse mediante un gesto desde el borde, pero **el gesto no debe
ser el único acceso**: conservar un icono visible, pequeño y bien
diseñado en lugar del círculo `...`. Mantener controles táctiles,
guardado/carga, idioma y logros. No tapar HUD ni zonas críticas y
asegurar navegación táctil y con mando.

### 1.5 Orientación Android

`android/AndroidManifest.xml` declara
`android:screenOrientation="landscape"`. Eso fija el modo horizontal
convencional; no es lo mismo que permitir ambas orientaciones
horizontales.

**Acción:** primero comprobar el manifiesto del APK realmente instalado.
Después, si se confirma la discrepancia, usar `sensorLandscape` y
validar tanto el inicio en horizontal como el giro de 180 grados.

### 1.6 El estado real del widescreen es contradictorio en la documentación

Un informe histórico declara los 54 niveles completos, pero
`PROJECT_STATUS.md` indica que solo ciertas escenas conocidas, la
apertura, 1-1 y la entrada del castillo 1-4 están habilitadas o
cualificadas en producción. Las capturas experimentales no demuestran
que todas las escenas estén activas en la APK normal.

**Acción:** reconciliar el estado real del runtime con
`KnownBackground()`, `KnownBoundaryExtension()`, `GamePrepareFrame()`,
los scripts y la matriz de cobertura. No reiniciar el trabajo ya
validado, pero tampoco declarar 54/54 activos sin comprobarlo.

### 1.7 Logros: integración no equivale a interfaz terminada

El código contiene `rcheevos` y el diálogo de conexión, pero la pantalla
actual no es un catálogo visual de logros. La notificación básica del
título tampoco equivale a una insignia de desbloqueo bien presentada.
Las pruebas sintéticas/offline no demuestran un desbloqueo confirmado en
el servidor.

**Acción:** completar catálogo, insignias, estados, notificación visual
y pruebas; separar claramente lo que se ha probado offline de lo que
requiere una cuenta real.

### 1.8 El círculo negro con el número 10: contador de Baby Mario

El círculo negro con `10` corresponde al contador de estrellas/tiempo de
Baby Mario, no a un contador de días ni a un overlay de depuración. El
manual oficial indica que el temporizador empieza cuando Baby Mario se
separa de Yoshi; si el tiempo baja de 10, puede recuperarse lentamente
después de rescatarlo. Referencia:
https://www.nintendo.com/es-es/games/oms/snes-classic/manuals/yoshis-island/manual.pdf.

En la primera de las cuatro capturas nuevas, el `10` aparece aunque Baby
Mario parece estar sobre Yoshi; en las otras tres no se muestra en esa
misma zona. **Es una inconsistencia visual que hay que investigar, no
una prueba suficiente por sí sola para borrar el elemento.** Puede haber
diferencia de estado entre fotogramas o un error en el estado del HUD.

**Prueba obligatoria:** 1. En la ROM original y en el port, capturar el
estado normal con Baby Mario montado; recibir daño y soltarlo; seguir el
contador al empezar en 10; recoger estrellas; rescatar a Baby Mario;
observar la recuperación si el contador está por debajo de 10; y entrar
en pausa/menú. 2. Registrar por fotograma el estado de separación de
Baby Mario, valor del contador, visibilidad del HUD y modo de juego. 3.
Comparar las mismas transiciones en framebuffer nativo y salida final.
Si el estado interno dice «Baby Mario montado» pero el HUD sigue
mostrando el contador cuando la ROM original no lo hace, corregir la
condición de visibilidad/actualización. Si la ROM original lo muestra
durante la recuperación, conservar ese comportamiento. 4. Añadir casos
automatizados para caída, rescate, temporizador en 10, temporizador
menor de 10 y transición de escena.

------------------------------------------------------------------------

------------------------------------------------------------------------

------------------------------------------------------------------------

### 1.9 La flecha circular hacia arriba que se recorta o queda tapada

En las cuatro capturas nuevas, la flecha circular roja/blanca que está
sobre los bloques de piedra aparece de distintas formas: en una se ve
completa, en otra queda tapada por la montaña nevada, en otra no resulta
visible y en otra vuelve a aparecer. En algunas capturas está pegada al
borde superior de la imagen de juego. No confundirla con el botón
Android `...` superpuesto en la esquina superior derecha: son elementos
distintos.

Aún no se puede afirmar si la flecha es un sprite del juego, un tile de
una capa BG, un elemento del HUD o una composición propia del port. **El
agente debe rastrear su origen en el código y los recursos antes de
cambiarla.** El patrón visible sugiere una posible combinación de orden
de dibujo incorrecto, recorte del viewport, culling o sincronización
entre capas, pero la causa exacta debe comprobarse.

**Prueba obligatoria:** 1. Reproducir la misma zona, coordenadas de
cámara y fotogramas en la ROM original y en el port. Obtener al menos 10
fotogramas consecutivos antes, durante y después del momento en que
aparece la flecha. 2. Guardar por separado el framebuffer nativo
256×224, los buffers/capas BG disponibles, los sprites/OAM si el runtime
los expone y la imagen final panorámica. 3. Verificar si la flecha
existe en la capa esperada, si otra capa la tapa indebidamente, si queda
fuera del viewport y si el código la elimina por culling. Comprobar
también orden de composición, prioridad de sprites/tilemaps, scroll
vertical y escalado. 4. Corregir en el origen adecuado: datos/mapa de
escena si falta el recurso; prioridad/compositor si una capa se dibuja
encima de otra; viewport/culling si se recorta o descarta;
sincronización si aparece tarde. No pegar una imagen encima como parche.
5. Validar la solución en esa escena, en escenas con capas equivalentes,
al desplazarse en ambas direcciones y al girar/orientar la pantalla.
Añadir la secuencia a las pruebas de regresión. 6. Mantener la mecánica
de apuntado de huevos como prueba separada de controles; no confundirla
con esta flecha.

------------------------------------------------------------------------

## 2. Decisión visual final: un único preset automático

No quiero un menú con varias opciones de renderizado para que el usuario
tenga que adivinar cuál es la buena.

Crear un único preset por defecto, nombre interno sugerido: **Island
Cinema**. El nombre es interno; no es necesario mostrar una opción de
selección.

El preset combina:

-   Suavizado óptico horizontal muy moderado, inspirado en la mezcla de
    color de una CRT, no un desenfoque gaussiano de toda la pantalla.
-   Tratamiento distinto para fondo, plano intermedio, sprites y texto.
-   Iluminación contextual por escena y por capas, con sombras suaves o
    pixeladas donde estén justificadas.
-   Brillo localizado muy discreto en fuentes luminosas reales.
-   Corrección de color ligera que conserve la paleta original de
    Yoshi's Island.
-   Sin curvatura de pantalla, marco CRT, máscara de fósforo visible,
    scanlines marcadas ni aberración cromática. Se permite una
    modulación de scanline casi imperceptible solo si la comparación A/B
    demuestra que mejora el acabado sin oscurecer ni ensuciar la imagen.

Los switches de depuración pueden seguir existiendo mediante variables
de entorno o pruebas internas. No deben convertirse en un menú público
de múltiples presets.

### 2.1 Suavizado CRT: implementación requerida

Usar el pipeline de presentación actual siempre que sea viable. No
migrar de backend ni añadir una cadena pesada de shaders por estética.

1.  Inspeccionar `warm-composite-pass.glsl` y su licencia en el checkout
    completo, además de las implementaciones actuales en
    `presentation.c`.
2.  Usar como referencia visual/técnica el proyecto
    [Pat9496/gl-crt-pixel-shaders](https://github.com/Pat9496/gl-crt-pixel-shaders),
    que busca un suavizado CRT de una sola pasada, sin una simulación
    pesada. No asumir que su shader se puede insertar directamente en
    SDL/Android: adaptar la técnica al renderer existente y conservar
    avisos de licencia si se reutiliza código.
3.  Evitar un blur uniforme. Objetivo visual inicial:
    -   Fondo lejano: suavizado horizontal ligero, alrededor del 8--12 %
        de mezcla vecina como punto de partida.
    -   Plano intermedio: mezcla menor.
    -   Yoshi, Baby Mario, enemigos, texto, HUD y UI: conservar
        contornos legibles; aplicar como máximo un suavizado muy
        pequeño.
4.  Si el renderer no expone capas separadas, intentar primero un filtro
    consciente de bordes y máscaras. No fingir que se dispone de
    información de capas si solo se tiene el fotograma final.
5.  El suavizado debe ser sutil pero perceptible en comparación A/B:
    integrar ligeramente los píxeles, sin borrar el pixel art ni volver
    borroso el escenario completo.
6.  El preset se aplica de manera coherente en escenas compatibles. No
    debe desaparecer al entrar al mapa o a una secuencia solo porque
    cambia el modo gráfico del juego.
7.  No alterar el framebuffer original del PPU ni la simulación.
    Capturar por separado la fuente original y la salida presentada.

### 2.2 Iluminación 2D contextual

La referencia es el volumen y la profundidad de los remakes 2D-HD de
Square Enix, adaptados a una estética SNES. No se pretende convertir el
juego en 3D ni repintar toda su paleta.

Crear una estructura de perfil por escena, por ejemplo
`SceneLightingProfile`, con datos de:

-   Tinte e intensidad ambiental.
-   Dirección e intensidad de luz principal cuando la escena lo
    justifique.
-   Fuentes locales reales: magia, fuego, lámparas u objetos luminosos
    presentes.
-   Máscara de oclusión/sombra de baja resolución cuando pueda
    construirse con datos auténticos de las capas.
-   Intensidad de bloom limitada a las fuentes que realmente emiten luz.

Comportamiento visual esperado:

-   **Exterior soleado:** luz cálida direccional suave y sombras de
    contacto discretas bajo plataformas o elementos que las justifiquen.
-   **Cueva/castillo:** ambiente algo más oscuro, luces locales donde
    haya fuentes reales y profundidad entre planos.
-   **Lluvia:** ambiente más frío y reflejos pequeños; no añadir focos
    ni brillos falsos por todas partes.
-   **Nieve:** luz ambiental fría con reflejos cálidos o blancos
    controlados.
-   **Magia/jefes:** luz de color localizada y ligada al efecto real,
    sin halos gigantes.
-   **Capas:** el fondo lejano puede perder algo de contraste; el primer
    plano y los personajes deben conservar presencia y legibilidad.

Usar máscaras o buffers de iluminación de baja resolución si el coste lo
justifica. Primero comprobar si el runtime expone las capas BG/OBJ por
separado en el punto adecuado. Si solo llega el fotograma final ya
compuesto, **no fingir que existen capas separadas**: extraer datos
auténticos de PPU/VRAM o aplicar máscaras por escena cuidadosamente
justificadas. No modificar el framebuffer nativo ni la simulación.

La documentación de luces 2D, mapas normales y oclusores de
[Redot/Godot](https://github.com/redot-engine/redot-docs/blob/master/tutorials/2d/2d_lights_and_shadows.rst)
sirve como referencia conceptual, no como razón para importar otro
motor. No generar mapas normales para todos los recursos sin demostrar
que mejoran el resultado y el rendimiento.

El límite anterior de variación media `RGB < 8` no debe seguir siendo
una puerta de aceptación: ese umbral empuja el efecto hacia una
iluminación casi imperceptible. Mantener la delta RGB como dato
diagnóstico, pero aceptar por comparación visual A/B, conservación de
paleta, ausencia de halos y rendimiento. Tampoco subir la intensidad
solo para inflar el número: la luz debe ser claramente perceptible y
seguir pareciendo Yoshi's Island.

**Prueba de aceptación:** capturas comparables de exterior, nieve,
interior/castillo, lluvia si existe en el recorrido probado y al menos
una escena de magia/jefe. La iluminación debe notarse claramente más que
la pasada actual, pero los colores originales deben seguir reconocibles
y no quedar lavados.

------------------------------------------------------------------------

## 3. Política de composición panorámica por escena

Implementar una clasificación explícita. No usar el mismo relleno para
todo.

### Tipo A: gameplay con contenido auténtico disponible

-   Ampliar el campo con tiles, mapas y datos reales de la ROM,
    respetando el scroll de cada capa.
-   Revisar aparición tardía de enemigos, sprites recortados, wrapping,
    ventanas de color, HUD y cambios de modo.
-   No duplicar enemigos, monedas, personajes ni objetos jugables en los
    laterales.
-   No deformar el centro ni cambiar las colisiones.
-   Si una escena no está cualificada, mantener un fallback seguro
    temporal y documentar el motivo; no activar una ampliación solo
    porque una captura aislada se vea bien.

### Tipo B: jefe cuya arena no puede ampliarse de forma segura

El relleno pixelado y suavizado se reserva principalmente para estos
casos.

-   Diagnosticar los **12 encuentros de jefe**, dos por mundo:
-   Mundo 1: 1-4 Burt the Bashful; 1-8 Salvo the Slime.
-   Mundo 2: 2-4 Bigger Boo; 2-8 Roger the Potted Ghost.
-   Mundo 3: 3-4 Prince Froggy; 3-8 Naval Piranha.
-   Mundo 4: 4-4 Marching Milde; 4-8 Hookbill the Koopa.
-   Mundo 5: 5-4 Sluggy the Unshaven; 5-8 Raphael the Raven.
-   Mundo 6: 6-4 Tap-Tap the Red Nose; 6-8 Baby Bowser/King Bowser.
-   Capturar carga de sala, introducción, transición/transformación,
    batalla, movimiento a izquierda/derecha y cambios de fase.
-   Primero intentar obtener fondo auténtico limpio desde BG1/BG2/BG3 o
    recursos de la ROM, sin sprites/HUD.
-   Si la arena, Super FX o las ventanas de color impiden ampliar el
    mundo, conservar el combate central original y usar un fondo lateral
    **específico de ese jefe**.
-   Para ese fondo decorativo, crear un **mosaico pixelado con
    nearest-neighbour**, no el actual promedio de bloques ampliado con
    filtrado lineal. Punto de partida: una fuente de aproximadamente 1/8
    de resolución (32 × 28 para el campo SNES de 256 × 224); comparar
    una versión ligeramente mayor solo si 1/8 pierde demasiado contexto.
-   La fuente debe salir de las capas auténticas del escenario de ese
    jefe, sin HUD, retícula de apuntado, Yoshi, Baby Mario, enemigos ni
    partículas móviles. No usar el fotograma compuesto entero si eso
    crea copias fantasma.
-   Conservar una paleta derivada del escenario, sin inventar colores.
    Aplicar un degradado oscuro hacia el exterior para dar profundidad y
    disimular la transición. No usar blur lineal global; el pixelado
    debe seguir siendo visible pero discreto.
-   Ajustar tinte, tamaño del mosaico y degradado con capturas
    comparables. El objetivo es un borde temático, integrado y
    ligeramente pixelado, no un barro borroso ni un mosaico duro que
    distraiga.
-   Difuminar o atenuar la transición del borde para que no aparezca una
    línea de costura.
-   No mostrar una copia gigante o fantasma del jefe en los laterales.
-   Clasificar cada jefe: `A` = widescreen auténtico probado; `B` =
    combate nativo con composición temática; `C` = aún sin fuente
    fiable. Adjuntar evidencia a cada decisión.

No asumir que los 12 encuentros ya tienen composición temática correcta.
Verificar en el checkout real qué rutas están implementadas, qué capas
usa cada arena y qué se activa en la APK de producción.

### Tipo C: mapa de la isla

La solución actual de laterales borrosos no es aceptable como acabado final del mapa. El mapa tiene que parecer una escena diseñada para el formato panorámico, no una captura pequeña con una copia desenfocada detrás.

**Implementación obligatoria:**

1. Rastrear la ruta exacta del mapa en el código: identificar si se compone a partir de tilemaps/BG/sprites del PPU, de una textura ya compuesta o de un recurso estático del port. Capturar por separado los buffers/capas que el runtime pueda exponer.
2. Inspeccionar los tiles, tilemaps, paletas y recursos de mapa originales de USA Rev. 1. Priorizar piezas que ya pertenecen a esa misma escena, incluyendo agua, nubes, mar, islas lejanas y cielo si existen en los datos originales.
3. Mantener el mapa original, rutas, nodos, cursor, personajes y geometría de navegación en el centro. Ampliar el fondo decorativo lateral alrededor de él, sin estirar ni cambiar coordenadas, hitboxes, rutas, desbloqueos o lógica de selección.
4. Si el mapa original tiene contenido auténtico fuera del viewport, renderizarlo. Si no, extender el mar/cielo/fondo con tiles reales y variaciones no obvias. No duplicar una isla completa ni inventar nodos, caminos o iconos.
5. Si los recursos no bastan, crear una placa decorativa lateral específica del mapa a partir de los assets originales. Se permite generar una imagen estática de apoyo con IA para las franjas laterales únicamente; después hay que limitar su paleta y ajustar los bordes para que encaje con el pixel art. El centro interactivo original queda intacto.
6. Eliminar el blur universal de los laterales del mapa. No usar un desenfoque de una miniatura de toda la pantalla como resultado final. Si la extensión no integra bien, revisar las capturas y rehacer la placa/tiles, no volver al blur.
7. Probar todas las posiciones del cursor, entradas/salidas de mundo, nodos bloqueados/desbloqueados, selección de niveles secretos, desplazamiento y transición del mapa al nivel. Asegurar que las áreas añadidas son decorativas y no cambian la navegación.

No generar una nueva imagen completa del mapa ni reemplazar los iconos, rutas o personajes originales. No estirar el mapa completo para rellenar el ancho.

### Tipo D: título, introducción narrativa y cinemáticas

La introducción, las ilustraciones tipo cuento, las transiciones, los finales y cualquier secuencia que parezca «vídeo» deben recibir un tratamiento específico. **No asumir que la introducción es un archivo MP4**: primero identificar si es una secuencia renderizada en tiempo real por el PPU, una animación de sprites/tilemaps, una serie de imágenes o un archivo multimedia empaquetado.

**Procedimiento de diagnóstico y expansión:**

1. Inventariar recursos multimedia del checkout (formatos de vídeo, imágenes, atlas, tiles, secuencias, archivos generados) y rastrear en el código la entrada y salida de cada escena. Si no hay un archivo de vídeo, no inventar una dependencia de vídeo: trabajar sobre las capas y assets que realmente la dibujan.
2. Capturar cada escena completa y sus transiciones en resolución nativa y panorámica. Marcar los cortes de plano, cambios de paleta, desplazamientos, animaciones y elementos que deben permanecer centrados.
3. **Si la escena se renderiza con tiles/BG/sprites:** reutilizar sus capas originales y ampliar el fondo con los tiles y las paletas del mismo escenario. Mantener texto, personajes, marcos y elementos de historia con sus proporciones originales.
4. **Si es una imagen estática o ilustración de cuento:** usar la ilustración original en el centro y crear laterales decorativos específicos para esa imagen. Primero usar recursos originales; si no alcanzan, se permite generar una sola placa lateral con IA o dibujarla con herramientas locales. Limitar la paleta, respetar el estilo pixel-art/acuarela original según la escena y revisar la unión a mano. No generar una imagen nueva para reemplazar la ilustración completa.
5. **Si de verdad es un vídeo o una secuencia de fotogramas:** el agente debe encargarse de la expansión mediante un script reproducible, usando herramientas locales existentes (por ejemplo FFmpeg si está disponible) para inspeccionar duración, resolución y cortes de plano. Conservar intacta la región central original y componer los laterales en una capa aparte. Primero buscar fondos/capas originales que permitan una extensión animada real. Si no existen, generar una placa lateral coherente por plano/escena o derivar un relleno pixelado determinista de los bordes, con transiciones sincronizadas con los cortes. No usar IA para generar cada fotograma de forma independiente, porque provocaría parpadeo y variaciones de estilo.
6. Si la animación cambia de cámara o de fondo, segmentar la secuencia en planos y asignar una extensión coherente a cada plano. Evitar que un fondo lateral fijo atraviese un corte de escena incompatible o se desplace a una velocidad distinta del centro.
7. No aplicar blur global al vídeo. No estirar ni recortar personajes, subtítulos, texto, elementos narrativos o la escena central. El resultado debe mantener la composición central original, con bordes pixelados o ilustrados que parezcan diseñados para esa escena.
8. Crear scripts y sidecars reutilizables para regenerar los assets si se modifica la fuente. Guardar los originales y los derivados por separado, documentando origen, herramienta, parámetros y resolución.
9. Revisar título, selección de archivo, mapa, introducción, ilustraciones de cuento, tutorial, diálogos, escenas de Kamek, transiciones de nivel, muerte/game over, créditos, finales y contenido secreto/bonus.

**¿Tiene que hacer algo el usuario?** No. No debe editar ni ampliar manualmente el vídeo. El agente debe localizar la fuente y preparar los assets/sidecars. Solo si la inspección demuestra que el vídeo o una fuente imprescindible no existe en el checkout ni es accesible localmente, pedir al usuario ese archivo original concreto; no pedirle que lo procese.

### Tipo E: escenas aún no cualificadas

No dejar artefactos, una línea gris ni un fondo anterior pegado. Si no
existe una solución segura inmediata, usar el encuadre original limpio
hasta que haya una extensión verificable. No aplicar blur a todo el
juego como sustituto.

### Inventario obligatorio de escenas y política de fallback

Antes de implementar el relleno, crear `docs/WIDESCREEN_SCENE_MATRIX.md` o ampliar la matriz existente. Debe enumerar cada familia de pantalla y sus variantes reales, con columnas: ID/escena, ruta de renderizado, recursos/layers disponibles, si es estática o animada, estrategia de expansión, estado, capturas de referencia, riesgos y prueba de regresión.

Cubrir como mínimo:

- Título, logo, menú inicial y selección de archivo.
- Mapa de la isla y transiciones entre mundos.
- Introducción narrativa, ilustraciones de cuento y todas sus páginas/animaciones.
- Inicio de nivel, cámara inicial, scroll normal, scroll vertical, salas pequeñas y salidas.
- Puertas, castillos, entradas/salidas, transiciones, pantallas de puntuación y fin de nivel.
- Los 12 encuentros de jefe, incluyendo entradas, transformaciones, fases, derrota y salida.
- Bonus/extra stages, niveles secretos, bonus games, game over, créditos y finales.
- Pausa, guardado/carga, logros y demás UI añadida por el port.

Estrategias permitidas, en este orden:

1. `REAL_WIDESCREEN_RENDER`: renderizar más contenido auténtico del mundo, cuando el runtime pueda hacerlo sin alterar la lógica.
2. `ORIGINAL_LAYER_EXTENSION`: extender las capas originales con sus tiles/paletas.
3. `SCENE_SPECIFIC_PIXEL_ART`: crear una extensión estática/animada por escena a partir de recursos originales, con pixelado y paleta coherentes.
4. `SCENE_SPECIFIC_STATIC_PLATE`: solo para ilustraciones realmente estáticas o laterales de un plano estable, con IA opcional como último recurso.
5. `CLEAN_NATIVE_FRAME`: fallback temporal solo mientras se termina la extensión, no aceptación final para escenas que el proyecto pretende soportar en panorámico.

El desenfoque de una miniatura de pantalla completa no es una estrategia final aceptada para mapa, título, cuento, introducción, transiciones ni finales. El mosaico pixelado específico se reserva principalmente para arenas de jefe si no hay datos reales suficientes. No se debe imponer la misma receta visual a escenas que tienen necesidades distintas.

### Investigación técnica para widescreen

Reutilizar la guía actual de [SNESRecomp: Add
widescreen](https://github.com/RetroPortingToolKit/RetroPortingToolkit.com/blob/main/data/docs/04_guides/05_add-widescreen/index.md)
y los patrones de
[SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp). La guía advierte que ensanchar la salida no basta: también hay que revisar culling, wrapping, aparición de objetos, HUD y temporización. Aplicar esos principios a las rutas reales de Yoshi's Island; no actualizar todo el framework por una mejora puntual.

Otros proyectos muestran tres estrategias útiles:

- **Widescreen real:** [wide-snes para Super Mario World](https://github.com/VitorVilela7/wide-snes) amplía el render horizontal desde 256×224 hasta 352×224 o 384×224 con parches y configuración de bsnes-hd. No se limita a estirar la imagen ni a duplicar un fotograma borroso. Sirve como modelo conceptual, no como código directamente reutilizable.
- **Extensión de mapas y capas:** [Secret of Mana widescreen](https://github.com/tidyhf/secret-of-mana-widescreen) reconstruye filas de mapa y añade columnas de fondo, permitiendo que sprites entren en las zonas laterales mientras HUD/menús se mantienen en su encuadre original. Es un patrón útil para el mapa y las capas de fondo.
- **Panorámico adaptativo por escena:** [SuperMarioWorldRecomp](https://github.com/mstan/SuperMarioWorldRecomp) documenta un modo panorámico adaptativo y deja título, mapa del mundo, transiciones o modos no compatibles en vista nativa cuando no se pueden ampliar con seguridad. Esto confirma que no conviene forzar una sola técnica sobre todas las escenas.

**Decisión para este proyecto:** primero intentar renderizado panorámico real cuando el runtime lo permita; después extender capas originales; y solo para escenas sin datos suficientes, construir laterales específicos. Una vista nativa limpia es mejor que un efecto borroso, pero debe ser una excepción documentada, no la solución final de las escenas que sí pueden integrarse.

------------------------------------------------------------------------

### 3.1 Estrategia concreta para rellenar los laterales y corregir capas

Seguir esta jerarquía, sin saltar directamente a generar imágenes:

1.  **Recuperar contenido auténtico.** Inspeccionar el tilemap, tiles,
    paletas, datos de nivel, sprites y capas de la ROM/disassembly USA
    v1.1. Reutilizar los recursos del mismo escenario. No usar una
    imagen generada para cubrir un bug de composición.
2.  **Extender el escenario real.** Si el nivel contiene datos más allá
    de la vista original, renderizar esos datos auténticos en los
    laterales y adaptar el culling de sprites al viewport ampliado. El
    área central de 256×224 debe permanecer visualmente y mecánicamente
    estable. No cambiar colisiones, velocidad, RNG ni lógica de juego
    para fingir widescreen.
3.  **Corregir la composición.** Mantener el orden real de BG/sprites,
    prioridades, ventanas de color y scroll por capa. La flecha circular
    de estas capturas debe dibujarse en su capa y prioridad correctas;
    no duplicarla ni reemplazarla por un PNG.
4.  **Continuar cielo, mar o fondo con tiles reales.** Reutilizar las
    mismas paletas y patrones del escenario. Reflejar/repetir tiles solo
    si la repetición no se ve y no altera el mapa ni crea duplicados
    obvios.
5.  **Jefes con laterales sin datos reales.** Mantener el combate
    central original. Crear el fondo lateral por jefe a partir de una
    captura/capa limpia del propio escenario, reducirla aproximadamente
    a 1/8 de resolución y ampliarla con `nearest-neighbour`, paleta
    original y degradado oscuro hacia el borde. Excluir HUD, Yoshi, Baby
    Mario, enemigos y partículas para evitar fantasmas.
6.  **Mapa de la isla.** Priorizar tiles, tilemap y paleta reales del
    mapa. Extender únicamente las franjas laterales; mantener rutas,
    nodos, cursor y demás elementos interactivos sin deformar. No
    regenerar el mapa entero con IA.
7.  **Pantallas fijas.** Título, ilustraciones de cuento o fondos
    realmente estáticos pueden usar una placa lateral hecha una sola
    vez, fuera de línea. Primero intentar assets originales. Si no
    bastan, se permite IA solo para la franja decorativa lateral, con el
    centro original protegido; luego limitar la paleta, ajustar a la
    cuadrícula de píxeles y revisar uniones a mano. Nunca generar cada
    fotograma de una animación.
8.  **Último recurso.** Si no hay fuente fiable, mantener el centro
    original con bandas limpias y documentar la escena como pendiente;
    no inventar paisaje que parezca parte del nivel ni estirar el
    fotograma.

Cada escena debe etiquetarse como `REAL_TILEMAP`,
`ORIGINAL_LAYER_EXTENSION`, `BOSS_MOSAIC`, `STATIC_SIDE_PLATE` o
`UNRESOLVED`. Cada clasificación necesita capturas antes/después y
justificación. Las imágenes de IA son decoración estática de último
recurso, no sustituyen código, tiles, mapas ni correcciones de
renderizado.

## 4. Localización al español: usar la traducción existente como base

El informe de localización del proyecto concluyó que faltaba una
solución completa, pero se ha localizado una traducción comunitaria que
debe evaluarse antes de escribir una traducción nueva:

-   Repositorio: [sewave/translations · Yoshi's Island
    SNES](https://github.com/sewave/translations/tree/master/sfc/supermarioworld2yoshisislandsfc)
-   Ficha de referencia: [Romhack
    Plaza](https://romhackplaza.org/translations/super-mario-world-2-yoshis-island-spanish-translation-snes)
-   El repositorio de GitHub contiene
    `supermarioworld2yoshisislandsfc.ips`, archivos `.ext`, `.hex`,
    `.tbl`, scripts y un README. Romhack Plaza clasifica la traducción
    como completa, aunque su ficha actualmente indica que no aloja
    directamente el archivo; usar el repositorio enlazado como fuente de
    trabajo.
-   La base es
    `Super Mario World 2 - Yoshi's Island (USA, Asia) (Rev 1).sfc`, MD5
    `ce1e3e33b6e39d37b43d7de599f9e785`, SHA-1
    `34612a93741f156d6e497462ab7f253cb8a959a0`.
-   **No aceptar la afirmación de que no existe parche español:** sí
    existe. El problema por resolver es su compatibilidad técnica con
    esta recompilación nativa y con el código C generado desde la ROM
    original.

### Procedimiento obligatorio

1.  Localizar la ROM USA v1.1 ya autorizada en el entorno del usuario.
    Calcular su hash y confirmar que corresponde exactamente a la base
    esperada por el parche.
2.  **No modificar ni sobrescribir la ROM original.** Aplicar el parche
    solo a una copia temporal de trabajo.
3.  Inspeccionar el parche y sus archivos auxiliares para clasificar los
    cambios: texto, glifos, tilemaps, datos, código ejecutable y rutinas
    nuevas.
4.  No limitarse a cargar una ROM parcheada con código C generado desde
    la ROM original. Si el parche cambia código ejecutable o rutinas de
    fuente, evaluar si se puede regenerar la recompilación desde la
    copia parcheada o si hay que trasladar sus cambios de forma
    explícita al runtime existente.
5.  Mantener la versión original disponible para rollback y comparar el
    arranque, la jugabilidad y las escenas críticas.
6.  Traducir también la interfaz añadida por el port: opciones,
    guardado/carga, controles, errores, estado de conexión y
    notificaciones de logros.
7.  Revisar la fuente y los mapas de glifos para
    `á, é, í, ó, ú, ñ, ¿, ¡`. Si el juego no puede dibujarlos,
    implementar la extensión necesaria o usar el mapeo de caracteres del
    parche.
8.  Verificar longitud de cadenas, saltos de línea, tilemaps y límites
    de las cajas. No basta con que el texto esté traducido: debe caber y
    renderizarse bien.
9.  Establecer **español como idioma predeterminado**. Solo añadir
    selector English/Español si ambos idiomas están completos y la
    selección funciona realmente. No mostrar un selector incompleto ni
    dejar pantallas importantes en inglés sin avisar.
10. Conservar los créditos de la traducción y respetar las condiciones
    del proyecto. No incluir ni distribuir una ROM completa en Git o en
    el paquete de entrega.

### Capturas obligatorias

Título, selección de archivo, mapa, nombre de nivel, un tutorial, un
diálogo, escena narrativa, guardado/carga, opciones, pantalla de logro y
game over/final si puede alcanzarse. Revisar textos cortados, glifos
incorrectos y superficies que todavía no estén traducidas.

------------------------------------------------------------------------

## 5. Interfaz, botón superior y guardado

### 5.1 Menú principal de opciones

Sustituir el diálogo Android genérico por una interfaz propia coherente
con la estética ilustrada de Yoshi's Island.

-   Retirar el botón circular `...` tan visible y poco integrado. Crear
    un botón discreto, limpio y táctilmente cómodo, con un icono
    consistente con el juego.
-   Mantenerlo dentro de los insets seguros para que no quede debajo de
    la cámara frontal ni en una zona incómoda.
-   Evitar que tape HUD, mensajes o elementos del juego.
-   Diseñar una única superficie visual con jerarquía clara, tipografía
    coherente, buen contraste, espaciado y transiciones cortas.
-   Mantener acceso sencillo a: continuar, guardar/cargar, controles
    táctiles, idioma, logros y volver al juego.
-   No exponer controles separados para Sharp/Soft/Lighting; aplicar
    `Yoshi Look` automáticamente.
-   El menú debe funcionar con pantalla táctil y mando, sin dejar
    botones presionados al abrirse o cerrarse.

### 5.2 Menú de estados de guardado/carga

La pantalla actual muestra etiquetas inglesas, referencias de teclado y
tarjetas de slot que parecen placeholders. Investigar primero dónde vive
realmente ese menú en el checkout completo (juego o
`recomp-ui`/SNESRecomp) y modificar el componente propietario; no crear
un segundo sistema de guardado paralelo.

-   Traducir todo al español.
-   Mantener el estilo visual del juego y la jerarquía clara.
-   En Android, mostrar acciones táctiles/mando y no pistas como
    `ARROWS`, `ESC`, `Q` o `S` destinadas al teclado.
-   Mostrar una miniatura real al guardar, además de slot, mundo/nivel y
    fecha/hora si esos datos están disponibles.
-   Distinguir un slot vacío de una partida guardada.
-   Evitar un menú gigante que oculte toda la escena si una presentación
    más compacta resulta legible.
-   No alterar el formato de guardado del framework sin necesidad.

### 5.3 No confundir los dos sistemas

-   **SRAM/autoguardado:** conserva el progreso persistente del juego.
-   **Save state:** recupera el estado completo de una partida en un
    momento específico.

Reutilizar el mecanismo atómico de SRAM de 32 KiB y las pruebas de ciclo
de vida existentes. Probar SRAM y save states por separado.

------------------------------------------------------------------------

### 5.4 Autoguardado y reanudación automática sin pérdida de progreso

El usuario quiere que el juego conserve el progreso sin tener que acordarse de guardar. Implementar sobre el sistema existente y sin crear un segundo subsistema incompatible.

Separar tres mecanismos:

1. **Guardado normal del juego (SRAM/in-game save):** persistir los datos que el propio juego guarda, usando escritura atómica (temporal + rename), comprobación de integridad y recuperación si el proceso se cierra durante la escritura.
2. **Autoguardado de reanudación (save state):** snapshot del estado completo del runtime para volver al punto exacto. Solo se implementa si el framework permite serializar/restaurar de forma fiable CPU, RAM, PPU, APU/audio, coprocesadores, mapeadores, registros y estados internos relevantes. No crear un pseudo-save state que solo guarde la RAM.
3. **Slots manuales:** el usuario puede guardar/cargar estados explícitos, con miniatura, fecha, mundo/nivel y confirmación al sobrescribir. Nunca se sobrescriben automáticamente los slots manuales.

Reglas de autoguardado:

- Guardar SRAM cuando cambie de forma válida y al pasar por los puntos de guardado del juego, pausarse o irse a segundo plano, con escritura atómica y sin bloquear el hilo de juego.
- Si el runtime soporta estados completos, mantener un máximo pequeño de checkpoints automáticos rotativos y guardar en puntos seguros: inicio de nivel, checkpoint real del juego, transición estable o pausa/segundo plano. Como respaldo adicional, evaluar un intervalo de 60–120 segundos, solo si las pruebas de coste y consistencia lo permiten.
- Mantener varios slots automáticos rotativos para que una escritura dañada no destruya la última copia válida. Verificar versión del formato, hash de ROM/identificador de juego y checksum antes de restaurar. Si no coincide, ofrecer recuperación segura y no cargar a ciegas.
- No escribir snapshots enormes cada frame en almacenamiento. Hacer snapshots solo en momentos razonables y usar compresión incremental si el framework ya la ofrece.
- Al abrir la app, ofrecer `Continuar` desde la última reanudación válida, sin reemplazar la partida normal ni cargar silenciosamente un estado incompatible.
- **Regla específica de RetroAchievements:** si la app restaura un save state para reanudar una sesión, esa sesión debe pasar a Casual conforme a las reglas de Hardcore Compliance. Si se desea conservar Hardcore, no restaurar automáticamente un save state: recuperar solo los datos normales del juego y arrancar la sesión conforme a las reglas oficiales. Mostrar el comportamiento con claridad.
- Si el framework no puede serializar de forma completa y fiable el estado del runtime, completar primero SRAM y guardados normales; dejar el save state claramente como bloqueado por limitación técnica en vez de entregar guardados corruptos.

### 5.5 Rebobinado (rewind) y compatibilidad con RetroAchievements

El rebobinado requiere capturar estados completos en memoria. Primero
comprobar si el runtime ya tiene un mecanismo de snapshots/deltas;
reutilizarlo antes de programar uno nuevo.

-   Implementar un buffer circular de memoria con presupuesto fijo y
    duración inicial corta (por ejemplo, 5 segundos), ampliable solo
    después de medir memoria y rendimiento. No guardar cada frame en
    disco.
-   El rebobinado debe restaurar sincronizadamente el estado de juego y
    audio, evitar corrupción al cruzar cambios de escena y detenerse en
    límites seguros si el runtime no puede garantizar consistencia.
-   Mantener separados rewind, slots manuales, autoguardado y SRAM.
    Mostrar claramente el modo de juego actual.
-   **RetroAchievements Hardcore:** las reglas oficiales deshabilitan
    rewind y bloquean la carga de save states en Hardcore. No se debe
    burlar esta regla ni simular cumplimiento. Cuando Hardcore esté
    activo, deshabilitar rebobinado y carga de estados con una
    explicación clara. La creación de estados para depuración no implica
    que puedan cargarse en Hardcore.
-   En modo Casual, permitir rewind y carga de estados. Los logros
    pueden seguir desbloqueándose en Casual, pero las funciones de
    rewind/save-state impiden la elegibilidad de Hardcore/Mastery y
    ciertas clasificaciones competitivas. Informarlo antes de activar la
    función.
-   Nunca cambiar a Casual de forma silenciosa. Si el usuario intenta
    usar rewind o cargar un estado con Hardcore activo, explicar el
    conflicto y solicitar una elección explícita; aplicar la política de
    `rcheevos` y reiniciar la sesión si así lo exige el cliente.
-   Probar carga de estados entre niveles, cambios de idioma, estados
    con sprites/sonido activos, guardado/carga tras reanudación y
    errores de checksum. Si el runtime no permite estado completo
    fiable, no exponer la función hasta resolverlo.

### 5.6 Checkpoints automáticos de código con Git/GitHub

Git es para proteger el código y la configuración del proyecto; no debe
utilizarse como almacenamiento de partidas del juego.

-   Crear un branch de trabajo si el flujo actual ya lo utiliza; si no,
    conservar el branch existente y hacer checkpoints por fase.
-   Al terminar una fase, ejecutar pruebas afectadas y crear un commit
    local con mensaje claro únicamente si pasan. Si falla una prueba,
    conservar los cambios como trabajo en curso y registrar el bloqueo;
    no hacer un commit que diga `fix` si no se ha validado.
-   Crear un checkpoint adicional antes de una migración o modificación
    de renderer de alto riesgo.
-   Si el proyecto ya tiene un remoto privado de GitHub configurado y el
    flujo existente autoriza los pushes, subir los commits de fases
    verificadas. Si no existe un flujo de publicación previamente
    autorizado, mantener los commits locales y no crear/publicar un
    repositorio remoto por iniciativa propia.
-   Excluir ROMs, archivos de BIOS, saves, save states, claves/tokens,
    cachés de compilación y artefactos enormes mediante `.gitignore`. No
    hacer `git add .` sin revisar `git status` y los archivos staged.
-   Nunca usar Git para versionar automáticamente la partida cada pocos
    segundos. Para los datos del juego, usar el sistema de guardado
    local y, si se desea copia externa, exportación/importación
    explícita y segura.

------------------------------------------------------------------------

## 6. Háptica Android: vibraciones sutiles y basadas en eventos

La vibración es una mejora deseada, no una vibración continua del juego.
Usar las APIs hápticas nativas de Android a través del puente
Java/SDL/JNI que ya exista en el proyecto. No añadir una librería grande
si el sistema nativo resuelve el caso.

Referencias oficiales:

- [Principios de diseño](https://developer.android.com/develop/ui/views/haptics/haptics-principles)
- [APIs y compatibilidad](https://developer.android.com/develop/ui/views/haptics/haptics-apis)
- [Efectos personalizados](https://developer.android.com/develop/ui/views/haptics/custom-haptic-effects)

### Eventos propuestos

| Evento | Sensación objetivo | Intensidad inicial |
|---|---|---|
| Salto iniciado | Toque/tick corto | Muy baja |
| Lanzamiento de huevo | Click seco, breve | Baja |
| Aterrizaje normal sobre tierra/cesped | Thud corto y suave | Baja |
| Aterrizaje en madera/puente/plataforma rígida | Tick seco corto, solo al impactar | Baja |
| Chapuzón o caída al agua | Thud suave, opcional segundo tick retrasado | Baja-media |
| Daño/impacto importante | Thud definido | Media, breve |
| Golpe fuerte o jefe | Impacto único algo más marcado | Media, sin vibración prolongada |
| Lluvia activa | Pulsos muy débiles, separados y con límite de frecuencia | Muy baja; nunca vibración continua |
| Menús y confirmaciones | Feedback estándar de Android | Muy baja |

Los nombres son objetivos sensoriales, no obligación de usar un
primitive que el dispositivo no soporte. Comprobar disponibilidad del
hardware y de cada primitive. Usar `HapticFeedbackConstants` para
interacciones UI cuando corresponda y `VibrationEffect`/composiciones
compatibles para eventos de juego. Añadir el permiso `VIBRATE` solo si
el método elegido lo requiere.

### Reglas de implementación

-   La señal debe salir de transiciones reales de estado del juego
    (salto, lanzamiento, aterrizaje, splash, daño), no de comprobar una
    condición cada frame.
-   La lluvia no debe vibrar por cada gota ni en cada frame. Los materiales (madera, tierra, piedra, hielo y agua) solo pueden modificar el patrón de un impacto real si el juego permite identificar el material de forma fiable; no simular vibración por cada paso. Empezar con
    un pulso breve y de intensidad mínima cada 800–1200 ms mientras
    el estado de lluvia esté activo, con un cooldown global para evitar
    acumulación; ajustar en el teléfono real. Si resulta molesto, bajar
    frecuencia o dejarla desactivada por defecto.
-   No mantener vibraciones largas o repetitivas; evitar los patrones
    «buzzy» descritos en la guía oficial de Android.
-   No disparar háptica mientras la app está pausada, en segundo plano,
    durante menús modales o si el usuario desactiva vibración en los
    ajustes del juego/sistema.
-   Añadir un control sencillo
    `Vibración: Desactivada / Sutil / Normal`, con `Sutil` como valor
    inicial recomendado. Recordar la preferencia.
-   Evitar vibraciones duplicadas cuando el mismo evento se notifica
    desde dos capas; centralizar el throttling en un único gestor.
-   Si el dispositivo no soporta efectos avanzados, usar un fallback
    corto compatible o no vibrar; nunca romper el gameplay.
-   No prometer ahorro de batería absoluto. El diseño minimiza el coste
    al usar eventos cortos, sin bucles ni vibración continua. Medir
    estabilidad y consumo relativo en una sesión de prueba.
-   Sincronizar el impacto con el sonido y la animación. La vibración no
    debe anticiparse ni llegar tarde de forma perceptible.

### Pruebas hápticas

-   Probar en un dispositivo físico Android con motor de vibración; el
    emulador no valida la sensación física.
-   Verificar salto, huevo, aterrizaje, agua, daño, jefe, lluvia, pausa,
    segundo plano y ajustes de vibración.
-   Comprobar que lluvia prolongada no genera un zumbido constante, que
    no se acumulan pulsos y que desactivar háptica la silencia
    inmediatamente.
-   Registrar qué eventos se probaron en teléfono real y cuáles solo se
    verificaron por código.



## 7. RetroAchievements: catálogo, insignias y aviso de desbloqueo

La biblioteca `rcheevos` no proporciona por sí sola la interfaz ni el
transporte HTTP; la aplicación integradora debe implementarlos. La
referencia oficial es [rc_client
integration](https://github.com/RetroAchievements/rcheevos/wiki/rc_client-integration).

El sitio oficial confirma que el juego **558** es *Super Mario World 2:
Yoshi's Island* para SNES y actualmente tiene un set base de **63 logros
por 548 puntos**: https://retroachievements.org/game/558. Por tanto, no
es una tarea pendiente averiguar si existe un set: existe. Sí hay que
verificar que la ROM USA v1.1 local coincide con un hash admitido por el
servicio y que la API carga el catálogo esperado.

El usuario desea iniciar sesión una vez. Investigar el flujo soportado
por `rc_client`; si se persiste una sesión, guardar solo el token de
sesión mediante almacenamiento cifrado respaldado por Android Keystore
cuando sea compatible, nunca la contraseña en texto claro. Logout debe
borrar el token. No simular persistencia si la biblioteca/API no la
permite de forma fiable.

### Experiencia requerida

-   La opción debe llamarse **Logros**, no presentar al usuario una
    pantalla que parezca pedir un código de juego.
-   Jugar sin cuenta debe seguir funcionando sin conexión.
-   La conexión a RetroAchievements debe ser opcional y explícita. No
    realizar peticiones de red antes de que el usuario elija conectarse.
-   La pantalla debe mostrar el estado de la cuenta, total de logros,
    insignias, títulos, descripción y estado bloqueado/desbloqueado
    cuando esos datos estén disponibles.
-   No presentar un catálogo inventado ni declarar conectados los logros
    si no se han cargado.
-   El formulario debe explicar claramente que sirve para iniciar sesión
    en la cuenta. No pedir un «código» que no sea necesario para el
    flujo implementado.
-   Mantener Softcore si esa es la política actual. No anunciar
    compatibilidad Hardcore sin validación oficial.

### Notificación de desbloqueo

Cuando se reciba `RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED`:

1.  Mostrar una tarjeta/banner dentro del juego con la insignia, el
    texto **«¡Logro desbloqueado!»**, el título y, si está disponible,
    la descripción.
2.  Usar una animación breve y elegante, sin cubrir la acción
    importante.
3.  Mantenerlo visible unos segundos y gestionar varios logros
    consecutivos sin que se pisen.
4.  Descargar o cargar insignias solo mediante URLs válidas y con
    límites de tamaño, caché y errores. No bloquear el hilo de juego por
    una imagen.
5.  Si no se puede cargar una insignia, usar un icono de reserva bien
    diseñado.
6.  Añadir capturas de un desbloqueo real o de una prueba sintética
    claramente identificada como tal.

### Pruebas

-   Tests nativos/sintéticos de memoria, evento, logout y fallo de red.
-   Comprobar que jugar offline no se bloquea.
-   Verificar que las credenciales no aparecen en logs, archivos
    temporales o capturas.
-   La validación de cuenta real y confirmación del servidor requiere
    que el usuario introduzca sus credenciales de forma privada en el
    dispositivo. No pedirle que las comparta en este chat y no marcar
    ese gate como aprobado si no se ejecutó.

------------------------------------------------------------------------

## 8. Android: inicio horizontal, rotación y pantalla completa

1.  Comprobar el manifiesto del APK instalado, no solo el archivo
    fuente.
2.  Si el APK sigue bloqueando una sola orientación, cambiar la
    actividad a `android:screenOrientation="sensorLandscape"` y
    verificar si hace falta reforzarlo desde `YoshiActivity`.
3.  Confirmar que el juego inicia en horizontal sin requerir que el
    usuario gire primero el teléfono.
4.  Confirmar que puede rotar a la orientación horizontal opuesta y que
    el SDL drawable, el viewport y los controles se recalculan sin
    estirar o recortar la imagen.
5.  Mantener la salida del juego edge-to-edge. Aplicar insets seguros a
    los controles y elementos interactivos, no reducir accidentalmente
    el drawable de SDL.
6.  Obtener capturas PNG nativas con `adb exec-out screencap -p` a la
    resolución real. No corregir una franja negra que solo sea padding
    del archivo de captura o de su visor.
7.  Registrar `SDL_GetRenderOutputSize`, el rectángulo de destino y la
    resolución final. Si la línea gris existe en la captura nativa,
    encontrar su causa (viewport, padding, composición o textura) y
    corregirla.
8.  Probar giro durante gameplay, menú abierto, guardado/carga y
    reanudación tras segundo plano.

Referencia Android: [Activity manifest
element](https://developer.android.com/guide/topics/manifest/activity-element).

------------------------------------------------------------------------

## 9. Plan de ejecución y pruebas de regresión

### Fase 0 --- Preflight

-   [ ] Revisar Git, cambios locales, `AGENTS.md`, `PROJECT_STATUS.md`,
    `HANDOFF.md`, `MASTER_PLAN_ANTIGRAVITY.md` y dependencias fijadas.
-   [ ] Confirmar ruta de la ROM y hashes; comprobar SDK/JDK/NDK y
    herramientas existentes.
-   [ ] Identificar el APK instalado y comparar su versión/hash con los
    artefactos locales.
-   [ ] Crear carpeta ignorada para evidencia de esta ejecución.
-   [ ] Capturar baseline real del dispositivo/emulador antes de editar.

### Fase 1 --- Salida, orientación y viewport

-   [ ] Verificar resolución real de pantalla y drawable SDL.
-   [ ] Corregir el arranque en horizontal y la rotación horizontal
    opuesta.
-   [ ] Diagnosticar la línea gris con captura nativa y geometría, no a
    ojo.
-   [ ] Comprobar fullscreen y márgenes seguros de controles.

### Fase 2 --- Preset visual unificado

-   [ ] Crear `Yoshi Look` como preset predeterminado único.
-   [ ] Aplicar suavizado coherente a las escenas, sin blur universal.
-   [ ] Implementar iluminación contextual por escena/capa.
-   [ ] Retirar los selectores públicos Sharp/Soft/Lighting.
-   [ ] Capturar exactamente el mismo fotograma antes/después y revisar
    el resultado.
-   [ ] Confirmar que la fuente original del PPU y la simulación no
    cambian.
-   [ ] Medir coste de GPU/CPU y comprobar que no aparecen tirones ni
    cortes de audio.

### Fase 3 --- Cobertura panorámica y composición

-   [ ] Reconciliar las condiciones de producción reales con la matriz
    de 54 niveles.
-   [ ] No repetir pruebas históricas que sigan válidas, pero volver a
    ejecutar las afectadas por cambios del renderer.
-   [ ] Generar una captura de entrada para cada uno de los 54 niveles
    y, cuando el replay/herramienta lo permita, capturas adicionales de
    un punto de juego representativo, un tramo de scroll y los bordes
    izquierdo/derecho.
-   [ ] La hoja resumen de 10+ niveles es solo una vista rápida; no
    sustituye las 54 evidencias individuales.
-   [ ] Para cada nivel, revisar al menos una captura completa a
    resolución nativa y otra de la salida panorámica, y guardar una
    tercera captura si aparece un bug o una transición relevante.
-   [ ] Ejecutar recorridos deterministas o scripts de replay para
    comprobar movimiento, salto, hovering jump, apuntado/lanzamiento de
    huevos, lengua, ground pound, enemigos, objetos interactivos,
    checkpoints, puertas/transiciones y fin de nivel.
-   [ ] Probar el apuntado de huevos como mecánica independiente.
-   [ ] Probar la flecha circular roja/blanca de las capturas con 10+
    fotogramas consecutivos, comparando capa nativa y salida panorámica;
    corregir prioridad, clipping, culling o sincronización según la
    causa demostrada.
-   [ ] No declarar un «playthrough completo» si solo se generaron
    capturas de entrada. Separar cobertura visual, cobertura de
    mecánicas y recorrido jugable real.
-   [ ] Revisar bordes, tilemaps, capas, sprites, HUD, culling,
    wrapping, aparición tardía de enemigos y cambios de modo.
-   [ ] Probar individualmente los 12 encuentros de jefe con capturas de
    sus fases importantes.
-   [ ] Resolver el mapa de la isla con assets originales; usar IA
    únicamente para franjas laterales decorativas cuando los assets no
    alcancen.
-   [ ] Revisar título, selección de archivo, mapa, todas las páginas de
    cuento, introducción, diálogos, transiciones de inicio/fin de nivel,
    bonus/extra stages, secretos, game over, créditos y finales.
-   [ ] Inventariar si la introducción es PPU/tilemap, secuencia de
    imágenes o vídeo real; implementar la expansión en el origen
    correcto. El usuario no debe editar el vídeo manualmente.
-   [ ] Buscar activamente glitches nuevos en todo el recorrido: tiles
    que faltan, capas que parpadean, bordes que se repiten, sprites que
    aparecen tarde, clipping, wrapping, fondos con paleta errónea, texto
    cortado, HUD fuera de sitio, audio cortado, estados de guardado
    corruptos, desincronización de audio/imagen y fallos tras
    rebobinar/reanudar.
-   [ ] Probar los seis niveles extra/desbloqueables y los bonus games,
    no limitarse a los 48 niveles normales.
-   [ ] No activar un cambio panorámico sin capturas que prueben su
    procedencia y seguridad.

### Fase 4 --- Español

-   [ ] Verificar la suma de comprobación de la ROM y evaluar el parche
    comunitario.
-   [ ] Integrar la traducción sin sobrescribir la ROM original ni usar
    código generado obsoleto.
-   [ ] Traducir también los menús y las funciones añadidas al port.
-   [ ] Revisar glifos, saltos de línea, cajas, mapa, HUD y pantallas
    narrativas.
-   [ ] Hacer una pasada visual de todas las superficies prometidas en
    español.

### Fase 5 --- Interfaz, guardado y logros

-   [ ] Sustituir el botón superior y los `AlertDialog` genéricos.
-   [ ] Rediseñar el menú de estados de guardado/carga en su componente
    propietario.
-   [ ] Verificar miniaturas, slots vacíos, traducciones, controles y
    navegación.
-   [ ] Probar persistencia SRAM, autoguardado de reanudación, slots
    manuales y rewind como sistemas separados; probar segundo plano,
    force-stop y reinicio.
-   [ ] Verificar que el rewind y la carga de estados quedan bloqueados
    en Hardcore de RetroAchievements y disponibles solo en Casual.
-   [ ] Terminar catálogo de logros, insignias y notificación de
    desbloqueo.
-   [ ] Probar el modo offline y los fallos de red.
-   [ ] Probar la háptica por eventos en teléfono físico, los ajustes
    Desactivada/Sutil/Normal y el límite de frecuencia de la lluvia.
-   [ ] Crear checkpoints Git locales por fase después de pasar las
    pruebas, revisando explícitamente los archivos staged.

### Fase 6 --- Háptica y guardado robusto

-   [ ] Implementar feedback háptico con el puente Android existente,
    basado en eventos de gameplay y con intensidad `Sutil`
    predeterminada.
-   [ ] Añadir salto, lanzamiento de huevo, aterrizaje, chapuzón,
    daño/impacto y lluvia con pulsos breves; lluvia con frecuencia
    limitada y sin vibración continua.
-   [ ] Añadir control Desactivada/Sutil/Normal y respetar preferencias
    del sistema, pausa y segundo plano.
-   [ ] Probar compatibilidad/fallback en el teléfono real y verificar
    que no hay vibración duplicada ni pulsos acumulados.
-   [ ] Implementar o terminar autoguardado/reanudación, rotación de
    checkpoints y recuperación atómica de SRAM; no duplicar el sistema
    existente.
-   [ ] Implementar rewind con buffer de memoria solo si el runtime
    puede serializar estado completo y pasar pruebas de consistencia.
-   [ ] Comprobar reglas Hardcore/Casual de RetroAchievements y explicar
    al jugador qué funciones quedan bloqueadas en Hardcore.

### Fase 7 --- Pruebas finales y rendimiento

Reutilizar las herramientas existentes y revisar sus opciones antes de
añadir argumentos nuevos:

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

-   [ ] Ejecutar las pruebas Android secuencialmente si comparten el
    mismo emulador.
-   [ ] Añadir al lote las herramientas de niveles/capturas que existan
    en el checkout completo.
-   [ ] Ejecutar la suite automatizada completa **tres veces desde un
    arranque limpio**, sin saltarse fases por haber pasado una ejecución
    anterior.
-   [ ] Repetir las pruebas de regresión de la flecha/capas y del
    contador de Baby Mario al menos **10 veces por caso**; ejecutar cada
    combate de jefe al menos **3 veces** si el harness lo permite.
-   [ ] Completar un recorrido jugable real de cada nivel que pueda
    automatizarse. No basta con abrir el nivel y tomar una captura. Si
    el port no puede automatizar un tramo, crear un replay/script de
    entradas o marcar exactamente qué requiere juego manual.
-   [ ] Hacer una prueba de estabilidad de al menos 30 minutos con
    gameplay, scroll, audio, lluvia/efectos y transiciones, además de
    suspender/reanudar la app y cambiar de escena.
-   [ ] Ejecutar una pasada de exploración de glitches, no solo casos
    conocidos: jugar con límites de cámara, extremos de scroll, pausas
    rápidas, rotación, guardado/carga, muerte/rescate, niveles extra,
    transiciones repetidas y cambios de estado de logros.
-   [ ] Repetir los flujos críticos después de optimizar, porque un
    arreglo de rendimiento puede reintroducir glitches visuales o de
    estado.
-   [ ] Medir una línea base antes de optimizar y repetir exactamente la
    misma ruta después: FPS y tiempos de fotograma p50/p95/p99, caídas,
    CPU, memoria, carga/descarga de recursos, audio y temperatura si el
    dispositivo lo permite.
-   [ ] Optimizar solo los cuellos de botella medidos. No eliminar
    efectos ni degradar fidelidad para subir un promedio artificial.
    Repetir la suite tras cada cambio de renderizado/performance.
-   [ ] Comparar capturas nativas, capas intermedias, hashes del
    framebuffer de origen, diferencias visuales y logs. Inspeccionar
    cada lote y convertir los defectos detectados en tareas concretas
    que se arreglan y se vuelven a probar.
-   [ ] Probar el OPPO físico y el mando GameSir si están disponibles.
    Si solo se usa emulador, declararlo explícitamente.

### Fase 8 --- Pulido final basado en capturas reales

-   [ ] Corregir el icono de Android restaurando el recurso gráfico anterior y el empaquetado adaptativo; no rediseñar el personaje.
-   [ ] Eliminar letterboxing/bordes negros accidentales en mapa, prólogo, logo, título, cuento y gameplay. Para cada escena, decidir composición por escena; no estirar ni recortar el centro.
-   [ ] Corregir el layout táctil con una única geometría compartida para dibujo y hit-testing. Ningún botón puede salir del área segura o tener su zona activa cortada.
-   [ ] Validar la iluminación y partículas con capturas A/B del mismo estado y fotograma, no solo con métricas de píxeles.
-   [ ] Solicitar hasta 120 Hz cuando el dispositivo y el backend lo permitan, sin cambiar el reloj SNES ni acelerar cinemáticas. Comparar Auto/60/120 y elegir el comportamiento que realmente mejore la experiencia.
-   [ ] Probar detección, mapeo, desconexión y reconexión de GameSir y mandos compatibles mediante SDL3 Gamepad; comprobar que los controles táctiles se ocultan/reaparecen en modo Auto.
-   [ ] Crear y verificar un guardado avanzado de QA en un slot dedicado, sin sobrescribir ninguna partida del usuario y sin incluirlo en la APK ni en Git.
-   [ ] Crear comparativas antes/después y un changelog; guardar las capturas del juego localmente. No subir capturas/activos del juego a un repositorio público sin autorización de derechos.
-   [ ] Ejecutar el checklist de publicación/licencias de la sección 13. No hacer push público automáticamente.
-   [ ] Compilar la APK final, instalarla en los entornos disponibles y repetir los gates afectados por los cambios. Si una prueba física no puede ejecutarse desde el entorno del agente, dejarla como `BLOCKED/NOT RUN`, no como `PASS`.

### Fase 9 --- Release limpio final

-   [ ] Crear una APK candidata en una ruta temporal, sin sobrescribir
    la última APK funcional.
-   [ ] Verificar que compila, instala, arranca, gira, guarda, traduce y
    ejecuta los flujos probados.
-   [ ] Verificar firma, ABI ARM64, alineación requerida y manifiesto
    empaquetado.
-   [ ] Calcular SHA-256 del APK final.
-   [ ] Actualizar `PROJECT_STATUS.md` y este plan con resultados
    reales.
-   [ ] Mantener una carpeta final compacta: APK, hash, informe QA y una
    selección de capturas representativas/contact sheet.
-   [ ] Eliminar únicamente dumps y temporales de esta ejecución que ya
    no se necesiten. No borrar fuentes, dependencias, ROM original,
    partidas ni trabajo del usuario.

------------------------------------------------------------------------


## 10. Capturas y evidencia que se deben conservar

Guardar en un directorio ignorado por Git, por ejemplo
`local/final-qa/`, con subcarpetas por fase:

-   `baseline/`: captura nativa del dispositivo, resolución/drawable y
    versión del APK.
-   `arrow-layer-bug/`: 10+ fotogramas consecutivos de la flecha
    circular en ROM original, framebuffer nativo, capas intermedias
    disponibles y salida panorámica.
-   `baby-mario-timer/`: caída, cuenta atrás, recogida de estrellas,
    rescate y recuperación del contador, con estado del juego
    registrado.
-   `presentation/`: mismo fotograma con fuente original y preset final.
-   `levels/`: capturas individuales de los 54 niveles, incluidos los
    seis niveles secretos/desbloqueables, con identificador de nivel y,
    cuando sea posible, entrada, scroll y borde de cada escena. Mantener
    además un contact sheet resumen.
-   `bosses/`: capturas de entrada, transformación/fase especial,
    combate, movimiento por ambos lados y salida de cada uno de los 12
    encuentros de jefe, según aplique.
-   `map-intro/`: mapa, título, tutorial, diálogo y transiciones.
-   `cinematics/`: inventario de fuentes, capturas por plano, región
    central original, extensiones laterales, cortes de escena y
    comparación antes/después.
-   `haptics/`: matriz de eventos, ajustes de intensidad, pruebas de
    frecuencia de lluvia y resultado en dispositivo físico.
-   `saves-rewind/`: pruebas de SRAM, autoguardado, reanudación, slots
    manuales, checksum, rewind y comportamiento RA Hardcore/Casual.
-   `git-checkpoints/`: hashes/commits locales por fase y estado de
    pruebas que justificó cada checkpoint.
-   `bug-hunt/`: registro de glitches encontrados fuera de los defectos
    ya reportados.
-   `spanish/`: capturas de las superficies traducidas.
-   `menus/`: opciones, save/load, catálogo de logros y banner de
    desbloqueo.
-   `performance/`: logs, mediciones y resultados de audio.
-   `release/`: APK, SHA-256, versión, manifiesto y reporte final.

Cada captura debe indicar escena/nivel, fotograma o ruta de entrada,
resolución, modo de presentación y resultado de revisión. Las capturas
deben revisarse visualmente y compararse con una referencia; guardarlas
sin analizarlas no cuenta como QA. Por cada defecto: crear entrada en
`BUGS.md` con gravedad, pasos de reproducción, causa confirmada o
hipótesis, fix aplicado y evidencia de regresión. No guardar cientos de
dumps redundantes en la carpeta final: conservar la evidencia necesaria
para demostrar el resultado.

------------------------------------------------------------------------

## 11. Criterios de aceptación finales

El trabajo solo se considera terminado cuando:

-   [ ] El mapa de la isla no utiliza blur global como acabado; sus
    laterales se integran con tiles/paleta originales o con una placa
    lateral específica, sin alterar navegación.
-   [ ] Introducción, cuento, cinemáticas, inicios/finales de nivel,
    créditos y finales están inventariados. Cada escena panorámica usa
    expansión por capas/recursos originales o extensión lateral
    específica y temporalmente coherente. No se estira el centro ni se
    genera IA frame a frame.
-   [ ] El usuario no tuvo que editar manualmente vídeos ni preparar
    assets; solo se le solicita un archivo fuente si se demuestra que no
    existe localmente y es imprescindible.
-   [ ] La política de expansión está documentada en
    `WIDESCREEN_SCENE_MATRIX.md`; ninguna escena se considera aprobada
    solo por mostrar una captura estática si tiene animación o
    transiciones.
-   [ ] Los efectos hápticos funcionan en dispositivo físico para los
    eventos seleccionados, son breves, están sincronizados y se pueden
    desactivar. La lluvia no produce vibración continua.
-   [ ] El autoguardado no sobrescribe slots manuales, escribe de forma
    atómica y puede recuperarse de una interrupción. Los estados
    completos se validan antes de cargarse.
-   [ ] El rewind funciona de forma consistente en Casual, no corrompe
    audio/estado y queda bloqueado junto con la carga de save states en
    RA Hardcore.
-   [ ] El código tiene checkpoints Git locales por fase verificada;
    ROMs, saves, save states, tokens y cachés no se suben al remoto.
-   [ ] Existe un informe de búsqueda general de glitches y se han
    repetido las pruebas afectadas por cada arreglo.
-   [ ] El juego arranca en horizontal, permite el giro horizontal
    opuesto y usa la superficie real disponible.
-   [ ] No hay franjas o líneas accidentales en la captura nativa.
-   [ ] Existe un único preset visual automático que suaviza ligeramente
    el pixel art sin borrar contornos.
-   [ ] La iluminación contextual se nota, respeta la paleta y no genera
    halos excesivos ni penaliza el rendimiento.
-   [ ] El fallback pixelado de los laterales se limita a las escenas
    que lo necesitan, principalmente jefes, y nunca se aplica
    globalmente por defecto.
-   [ ] El mapa de la isla y las escenas narrativas tienen composiciones
    laterales específicas, preferentemente construidas con assets
    originales.
-   [ ] Los 54 niveles y los 12 encuentros de jefe tienen una matriz de
    estado honesta y evidencia de las pruebas ejecutadas.
-   [ ] El HUD del contador `10` se conserva cuando corresponde al juego
    original; no se elimina como supuesto overlay de depuración.
-   [ ] La flecha circular de la escena se dibuja en la capa/prioridad
    correcta, no queda tapada por la montaña ni se recorta por un
    viewport incorrecto.
-   [ ] El contador de Baby Mario coincide con los estados de la ROM
    original, incluida la caída, el rescate y la recuperación del
    tiempo.
-   [ ] El apuntado de huevos funciona y se prueba como mecánica
    independiente.
-   [ ] El español está integrado de forma completa en las superficies
    declaradas; no hay selector que prometa algo incompleto.
-   [ ] Las interfaces de opciones, guardado/carga y logros se ven
    coherentes, están traducidas y funcionan con controles táctiles.
-   [ ] El progreso persiste y los estados completos de guardado/carga
    se han probado por separado.
-   [ ] Los logros muestran insignias/estados y notifican el desbloqueo;
    el estado online solo se marca como validado si el servidor real lo
    confirmó.
-   [ ] La música no presenta cortes audibles en las rutas medidas y el
    rendimiento no ha empeorado de forma apreciable.
-   [ ] La APK final instala y arranca, tiene hash registrado y no
    incluye ROMs ni artefactos temporales.
-   [ ] El informe final diferencia claramente pruebas de escritorio,
    emulador y teléfono físico.

-   [ ] El icono vuelve a usar el arte previo solicitado; el recurso no queda encerrado en un círculo blanco artificial. Se prueba en el launcher disponible y se documenta que la máscara exterior final puede depender de Android/launcher.
-   [ ] El prólogo/logo, la intro de cuento, título, mapa y transiciones llenan la composición horizontal sin bandas negras del renderer ni bordes accidentales. El negro que pertenezca a la propia escena original no se elimina artificialmente.
-   [ ] Los botones de cruceta, A/B/X/Y, L/R, Start/Select y pausa quedan completamente dentro del área segura en todas las orientaciones admitidas; dibujo y área táctil coinciden y se comprueban con coordenadas reales.
-   [ ] El preset visual no es solo un brillo global: hay luz ambiental y fuentes locales justificadas por escena, partículas selectivas y sombras de contacto donde los datos lo permiten. La paleta y los contornos del pixel art siguen siendo reconocibles.
-   [ ] La frecuencia alta no altera física, velocidad, RNG, audio, temporización de la intro ni lógica de los 54 niveles. Se informa por separado de Hz de pantalla y FPS únicos del juego; no se anuncia 120 FPS únicos si solo se presentan 60 frames SNES repetidos.
-   [ ] GameSir sigue funcionando y se verifica el mapeo estándar de mandos compatibles de PlayStation, Xbox y Switch cuando haya hardware disponible. Los no probados se etiquetan como `NOT RUN`, no como universalmente compatibles.
-   [ ] Existe un guardado avanzado de QA probado en un slot dedicado o un archivo local importable; los slots personales previos están intactos y el guardado de QA está excluido de APK/Git público.
-   [ ] Existe una comparativa antes/después para las escenas críticas y un registro de los cambios. Las capturas con arte de Nintendo se guardan en evidencia local privada, no se publican automáticamente.
-   [ ] Se completó la auditoría de licencias y contenido antes de cualquier publicación remota; el proyecto principal no se publica en público sin una decisión explícita y una revisión de derechos.

Si algún criterio no puede probarse por falta de dispositivo, cuenta o
recurso, terminar el resto, indicar el bloqueo exacto y dar el
procedimiento mínimo para cerrar ese único gate. No declarar un éxito
total si quedan fallos conocidos.

------------------------------------------------------------------------

## 12. Referencias técnicas investigadas

1.  **SNESRecomp --- guía de widescreen:**\
    https://github.com/RetroPortingToolKit/RetroPortingToolkit.com/blob/main/data/docs/04_guides/05_add-widescreen/index.md\
    Referencia para culling, wrapping, aparición de objetos, HUD, fases
    de scroll y preservación del camino nativo.

2.  **SNESRecomp:**\
    https://github.com/RetroPortingToolKit/snesrecomp\
    Reutilizar la versión fijada del proyecto; no actualizar el
    framework entero para una mejora visual aislada.

3.  **Desensamblado de Yoshi's Island USA 1.1:**\
    https://github.com/Yoshifanatic1/Yoshi-s-Island-Disassembly\
    Referencia de código, tablas, mapas y recursos de la versión
    correcta.

4.  **Traducción española de Wave, parche y recursos:**\
    https://github.com/sewave/translations/tree/master/sfc/supermarioworld2yoshisislandsfc\
    Verificar el hash de la ROM local antes de aplicar el IPS.

5.  **Suavizado CRT de una pasada:**\
    https://github.com/Pat9496/gl-crt-pixel-shaders\
    Referencia para un suavizado ligero sin una cadena compleja. Adaptar
    al backend real; no importar sin validar compatibilidad y licencia.

6.  **Documentación de shaders CRT de Libretro:**\
    https://docs.libretro.com/shader/crt/\
    Distingue blur/blending, scanlines, máscaras, glow y curvatura; no
    todos esos efectos son deseables para este proyecto.

7.  **Luces y sombras 2D:**\
    https://github.com/redot-engine/redot-docs/blob/master/tutorials/2d/2d_lights_and_shadows.rst\
    Referencia conceptual para luz ambiental, direccional, fuentes
    puntuales, oclusión y mapas normales, sin importar el motor.

8.  **RetroAchievements --- integración de `rc_client`:**\
    https://github.com/RetroAchievements/rcheevos/wiki/rc_client-integration\
    La aplicación debe implementar su UI y transporte HTTP, además de la
    evaluación de logros.

9.  **Android --- orientación de actividad:**\
    https://developer.android.com/guide/topics/manifest/activity-element\
    Referencia para `landscape` frente a `sensorLandscape`.

------------------------------------------------------------------------

10. **Traducción técnica de referencia en portugués brasileño:**\
    https://github.com/bMatSantos/YI-ptbr\
    Puede servir para estudiar expansión de fuente y edición de gráficos
    en el disassembly, pero no sustituye la traducción española de Wave.

11. **Guía de controles de Yoshi's Island:**\
    https://gamefaqs.gamespot.com/snes/588740-super-mario-world-2-yoshis-island/faqs/74510\
    Referencia para cursor de apuntado, lanzamiento y cancelación de
    huevos.

12. **RetroAchievements, ficha oficial del juego 558:**\
    https://retroachievements.org/game/558\
    Confirma que existe el set de logros del juego SNES.

------------------------------------------------------------------------

10. **Editor de niveles, tiles, paletas y mapa de Yoshi's Island
    (referencia de análisis):**\
    https://github.com/YoriKv/shiny-egg\
    Permite inspeccionar tiles, sprites, paletas, cabeceras, mapa y
    textos. Proyecto en alpha; usar como herramienta de referencia, no
    incorporar a la app ni cambiar la ROM base a ciegas.

11. **Desensamblado alternativo USA v1.0 (referencia de
    investigación):**\
    https://github.com/brunovalads/yoshisisland-disassembly\
    Muy documentado y completo, pero su base principal es USA v1.0. Para
    offsets y assets de USA Rev. 1, priorizar
    `Yoshifanatic1/Yoshi-s-Island-Disassembly` y verificar hashes.

12. **Manual oficial de Yoshi's Island:**\
    https://www.nintendo.com/es-es/games/oms/snes-classic/manuals/yoshis-island/manual.pdf\
    Confirma el comportamiento del contador de Baby Mario.

13. **Shader zfast CRT de Libretro:**\
    https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/zfast_crt.glsl\
    Referencia ligera para filtrado CRT. No es solución para capas mal
    ordenadas ni para ampliar mapas.

14. **Repositorio público de progreso C (comparación opcional):**\
    https://github.com/Zaxaerith/SuperMarioWorld2YoshisIslandRecomp\
    Solo si coincide con el remoto del checkout actual: consultar su
    progreso y herramientas como referencia; no hacer checkout/reset ni
    sobrescribir la copia local sin verificar que es la misma
    rama/proyecto.

15. **Super Mario World Widescreen (wide-snes):**
    https://github.com/VitorVilela7/wide-snes Ejemplo real de ampliar la
    resolución/renderizado de 256×224 a 352×224 o 384×224 en vez de
    rellenar todo con blur. Es una referencia conceptual: depende de
    bsnes-hd y de parches propios; no se debe copiar directamente a
    Yoshi's Island. Fuente: README del repositorio.

16. **Super Mario World recompilado con SNESRecomp:**
    https://github.com/mstan/SuperMarioWorldRecomp Referencia de
    renderer panorámico adaptativo y de una decisión sensata: algunas
    escenas y modos no soportados permanecen en vista nativa.
    Inspeccionar la arquitectura y limitaciones actuales, sin sustituir
    el checkout de este proyecto.

17. **Secret of Mana widescreen (bsnes-hd):**
    https://github.com/tidyhf/secret-of-mana-widescreen Ejemplo de mapas
    y columnas de fondo reconstruidos/extendidos, sprites permitidos en
    los laterales y HUD/menús conservados en su encuadre original. Sirve
    para el principio de expansión por capas, no como código
    plug-and-play.

18. **Android Haptics API y principios oficiales:**
    https://developer.android.com/develop/ui/views/haptics/haptics-apis
    https://developer.android.com/develop/ui/views/haptics/haptics-principles
    Recomienda efectos claros, cortos, coherentes con el evento,
    soporte/fallback según hardware y evitar vibración larga/buzzy.

19. **RetroAchievements Hardcore Compliance Requirements:**
    https://docs.retroachievements.org/general/hardcore-compliance-requirements.html
    En Hardcore, rewind está deshabilitado y cargar save states está
    bloqueado. La app debe respetar la política en lugar de sortearla.

20. **RetroAchievements FAQ:**
    https://docs.retroachievements.org/general/faq.html Explica la
    diferencia entre Hardcore y Casual y las limitaciones de save
    states/rewind.

21. **Experiencia comunitaria sobre variación háptica en Android:**
    https://www.reddit.com/r/androiddev/comments/1u94cdv/trying-to-add-haptics-to-android-app-but-its/
    Discusión de desarrolladores sobre la variación de intensidad entre motores/dispositivos. Usar como motivo adicional para incluir niveles de intensidad y probar en teléfono real, no como especificación técnica.



### Referencias adicionales verificadas para la revisión v5

16. **PixelLight (MIT):** https://github.com/SalvaPixel/PixelLight
    Técnica de luz cuantizada, sombras alineadas a la cuadrícula y paleta restringida para pixel art. Referencia conceptual, no plugin SDL3.
17. **gl-crt-pixel-shaders (MIT):** https://github.com/Pat9496/gl-crt-pixel-shaders
    Suavizado CRT de una pasada. Conservar avisos originales si se reutiliza código.
18. **Laigter (GPL-3.0):** https://github.com/azagaya/laigter
    Herramienta offline para mapas normales, especulares, de oclusión y parallax. No incorporar su código a la app sin análisis de compatibilidad.
19. **REPLACED, Xbox Wire:** https://news.xbox.com/en-us/2026/03/10/replaced-almost-here-another-look-cyberpunk-action-platformer/
    Referencia estética moderna de pixel art con iluminación, profundidad de campo y efectos cinematográficos, no fuente de assets.
20. **Android Frame Pacing / AGDK:** https://developer.android.com/games/sdk/frame-pacing
    Sincronización y presentación de frames, compatibilidad con varias frecuencias y consumo.
21. **Android refresh-rate API:** https://developer.android.com/games/optimize/display-refresh-rate-change
    Solicitud de frecuencias altas y advertencias sobre batería/temperatura.
22. **Android game-controller input:** https://developer.android.com/games/sdk/game-controller/controller-input
    KeyEvent/MotionEvent y entradas estándar de mandos.
23. **SDL3 Gamepad API:** https://wiki.libsdl.org/SDL3/CategoryGamepad
    Mappings normalizados, hot-plug y compatibilidad entre dispositivos.
24. **Android haptics design:** https://developer.android.com/develop/ui/views/haptics/haptics-principles
    Uso breve, coherente y de baja frecuencia de efectos hápticos.
25. **GitHub DMCA policy:** https://docs.github.com/en/site-policy/content-removal-policies/dmca-takedown-policy
    Proceso de retirada y contranotificación.
26. **GitHub licensing:** https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository
    Una licencia del repositorio no concede derechos sobre componentes de terceros.

------------------------------------------------------------------------

## 13. Última pasada de acabado visual, controles y publicación

Esta sección incorpora los problemas que se aprecian en las capturas reales recientes. Prevalece sobre cualquier informe histórico que haya marcado una fase como `APROBADO` si el resultado visible actual no cumple estos criterios. No rehacer el proyecto: localizar la causa en el código existente, corregirla y volver a probar.

### 13.1 Diagnóstico de las capturas recientes

Las capturas muestran problemas de acabado que no quedan resueltos por tener 54 niveles panorámicos:

- El logo/prólogo y algunas pantallas narrativas siguen dibujándose dentro de una región pequeña, con áreas negras alrededor.
- En gameplay horizontal quedan bandas negras estrechas arriba/abajo y ciertos objetos, como la flecha circular o un bloque cuadrado con cara, se ven cortados en algunos encuadres.
- Los controles táctiles están demasiado pegados al borde; algunos quedan parcialmente cortados y la zona visible no siempre parece corresponder a su zona táctil.
- El icono de launcher cambió de una imagen de arte completo a una imagen redonda dentro de un fondo blanco. El usuario rechaza ese rediseño y pide recuperar el icono anterior.
- La iluminación existente apenas se distingue en las capturas suministradas. El informe anterior de GPU demuestra que se ejecutó una pasada, no que produzca un acabado visual suficientemente perceptible.
- Hay que revisar la velocidad de la intro/cinemáticas y descartar que un modo `Turbo`/fast-forward de diagnóstico o un loop de presentación esté afectando a la percepción de velocidad.

Las capturas de pruebas incluidas en el paquete anterior no sustituyen las capturas del APK más reciente instalado en el teléfono. Volver a capturar el build actual, registrar el hash del APK y comprobar cada corrección en el dispositivo disponible.

### 13.2 Recuperar el icono anterior sin rediseñarlo

1. Buscar el recurso original anterior en el checkout, el historial Git y los recursos `mipmap`/`drawable`. Restaurar ese mismo arte y conservar sus colores, composición y recorte. No volver a generar la ilustración ni redibujar el personaje.
2. Auditar `android:icon`, `android:roundIcon`, `mipmap-anydpi-v26`, `ic_launcher.xml`, `ic_launcher_round.xml` y sus capas foreground/background. Identificar si el cambio proviene de un foreground circular, de un fondo blanco, de padding excesivo o de una referencia de manifiesto equivocada.
3. Recuperar el aspecto de arte completo en los recursos legacy y configurar el adaptive icon para aprovechar su área segura sin añadir un círculo blanco artificial ni recortar la ilustración. No incrustar un círculo adicional dentro del propio PNG.
4. Tener presente que Android/launcher puede imponer una máscara exterior a los iconos adaptativos. No prometer un cuadrado idéntico en todos los launchers; sí garantizar que el arte y el empaquetado de la app no añadan un recorte innecesario.
5. Generar la APK e instalarla en el launcher real disponible. Guardar una captura antes/después y revisar todos los recursos por densidad. Si el arte anterior no está en el checkout ni en el historial, solicitar únicamente ese archivo fuente, pero solo después de demostrar que no puede recuperarse.

### 13.3 Resolver las franjas negras y las escenas que vuelven al formato antiguo

No aplicar una única receta a todas las escenas ni eliminar el negro que forme parte del arte original. Distinguir entre negro del juego, letterboxing intencionado y negro accidental de la superficie/viewport.

1. Capturar con `adb exec-out screencap -p` la pantalla completa y registrar `SDL_GetRenderOutputSize`, drawable real, rectángulo de destino, viewport PPU, orientación, safe insets y tipo de escena.
2. Comparar el framebuffer nativo de 256×224, el framebuffer panorámico y la captura final. Medir los rectángulos negros; no diagnosticar únicamente mirando una imagen reescalada por el visor.
3. Revisar por separado logo de Nintendo/prólogo, texto de apertura, páginas de cuento, título, selección de archivo, mapa, entrada/salida de nivel, tutorial, transición entre niveles, game over, créditos, finales y las escenas de jefe.
4. Para escenas PPU/tilemap, usar las capas, tiles, paletas y scroll reales de la escena. Para una ilustración estática, mantener la imagen original en el centro y añadir una extensión lateral hecha para esa imagen. Para una secuencia animada, mantener las extensiones sincronizadas con cada plano/corte. El agente crea assets auxiliares y scripts reproducibles; el usuario no edita ni amplía vídeos.
5. En gameplay, mantener el centro original intacto y utilizar el renderizado panorámico real que ya está calificado. Corregir las bandas estrechas arriba/abajo en el viewport final sin estirar el juego, cortar sprites, modificar la relación de aspecto ni añadir un zoom que altere la visibilidad jugable.
6. Eliminar el fallback negro accidental en las escenas compatibles con composición panorámica. El `CLEAN_FRAME` queda como fallback de compatibilidad documentado solo cuando el arte original sea realmente negro o cuando no exista una alternativa segura durante desarrollo; no es acabado final de escenas ya calificadas para expansión.
7. Probar cada arreglo en movimiento, no solo con una captura estática. Buscar costuras, cambios de paleta, duplicados, parpadeo, HUD desplazado, objetos que entran cortados, scroll lateral incorrecto y recorte al girar la pantalla.
8. La flecha circular y el objeto cuadrado con cara deben investigarse como elementos separados: determinar si son OAM/sprite, tile BG, HUD o composición del port. Comparar origen, prioridad, culling y clipping contra la salida nativa. No pegarlos encima como imágenes de parche.

### 13.4 Iluminación 2D-HD, partículas y textura visual

El objetivo visual es un **remaster de presentación 2D**, no convertir Yoshi's Island en un juego 3D ni cambiar su identidad artística. La referencia estética moderna puede ser `REPLACED`, descrito por Xbox como pixel art reforzado con iluminación, profundidad de campo y efectos cinematográficos; solo se toma como referencia de principios, no se copian recursos ni se intenta reproducir su estilo oscuro en una paleta infantil. Fuente: https://news.xbox.com/en-us/2026/03/10/replaced-almost-here-another-look-cyberpunk-action-platformer/

Proyectos técnicos que merece la pena estudiar:

- **[PixelLight](https://github.com/SalvaPixel/PixelLight)**: shader de iluminación para pixel art con luz cuantizada, sombras ajustadas a la cuadrícula y salida limitada a paletas. Su licencia publicada es MIT. Es una referencia conceptual; es un addon de Godot, no un plugin directamente utilizable en este port SDL3/C.
- **[gl-crt-pixel-shaders](https://github.com/Pat9496/gl-crt-pixel-shaders)**: shaders CRT de una pasada, con licencia MIT y atribuciones adicionales descritas en los propios archivos. Estudiar su mezcla horizontal y parámetros; adaptar al backend existente sin copiar un shader sin conservar sus avisos.
- **[Laigter](https://github.com/azagaya/laigter)**: herramienta GPL-3 para generar mapas normales, especulares, de oclusión y parallax a partir de imágenes. Es una herramienta opcional offline, no una dependencia que haya que meter dentro de la APK. No incorporar su código al port; revisar compatibilidad de licencia antes de distribuir cualquier componente y mantener derivados de arte del juego en el ámbito local.
- **[Godot: luces y sombras 2D](https://docs.godotengine.org/en/stable/tutorials/2d/2d_lights_and_shadows.html)**: referencia conceptual para normal maps, specular maps y oclusores. No migrar el proyecto a Godot.
- **[Android: principios hápticos](https://developer.android.com/develop/ui/views/haptics/haptics-principles)**: los efectos deben ser breves, significativos y acordes a la importancia/frecuencia del evento.

**Implementación requerida, en orden de menor a mayor coste:**

1. Auditar el preset actual `Island Cinema` y confirmar que está activo en la APK instalada y que no se desactiva al salir de gameplay. Separar las capturas de la fuente PPU de las de la salida final.
2. Corregir primero el brillo inicial excesivo: reducir el bloom global, proteger los blancos de la nieve/nubes y mantener el contraste de contornos. No elevar brillo ni saturación global para fingir iluminación.
3. Implementar un perfil de escena determinista (`SceneLightingProfile`) que use el tipo de nivel y las capas/recursos disponibles. Como mínimo: exterior cálido, cueva/castillo más oscuro, bosque húmedo, nieve fría, agua, lava/fuego y magia de jefe. Las transiciones de color deben ser suaves y no saltar entre fotogramas.
4. Si el runtime expone BG/PPU/sprites por separado, componer una máscara ligera de profundidad/material para diferenciar fondo lejano, plano intermedio, primer plano, personajes y HUD. Si solo se tiene el framebuffer final, no afirmar que se conoce cada objeto: usar un filtro consciente de bordes y perfiles por escena hasta que se instrumenten datos reales.
5. Añadir luces locales discretas solo a fuentes justificadas: lava/fuego, antorchas si existen, destellos mágicos, monedas/estrellas, agua brillante o nieve iluminada. Usar halos pequeños y limitados, sin borrar la paleta ni crear bloom sobre todos los blancos.
6. Añadir sombras de contacto suaves/pixeladas bajo plataformas, personajes u objetos solo cuando su posición y oclusión puedan determinarse de forma fiable. Ajustar los bordes a la cuadrícula de píxel; no aplicar sombras negras universales ni mover la sombra como si fuera parte de la física.
7. Añadir partículas de presentación ligadas a eventos reales y al entorno, con un presupuesto bajo y determinista:
   - polvo/pequeño puff en aterrizajes sobre suelo seco;
   - salpicaduras y gotas en agua, con efecto algo mayor al caer en un charco;
   - gotas muy discretas y ocasionales en escenas de lluvia;
   - pequeñas motas/hojas en zonas boscosas y copos en nieve cuando el nivel los justifique;
   - chispas/brasas en lava o fuego;
   - partículas mágicas en efectos reales de jefes, estrellas o monedas;
   - un rastro mínimo al lanzar huevo solo si no dificulta seguir la trayectoria.
8. No añadir partículas a cada sprite ni poner lluvia/nieve en niveles donde no corresponde. Limitar cantidad, tamaño, alfa, tiempo de vida y frecuencia. Reutilizar buffers y evitar asignaciones por fotograma. Usar un RNG visual independiente y semilla reproducible para no alterar RNG, logros, colisiones ni la simulación del juego.
9. En una caída al agua, sincronizar splash, sonido y háptica. La lluvia ambiental no debe crear vibración constante: solo pulsos raros, opcionales y con cooldown, según la sección 6.
10. Mantener las partículas y luces en la capa de presentación. No cambiar ROM, lógica del juego, hitboxes, estados de enemigos, física, timing del PPU ni condiciones de RetroAchievements.
11. La presentación debe poder reducirse automáticamente en dispositivos lentos, con ahorro de batería activo o temperatura elevada. No introducir IA generativa en tiempo real; los assets auxiliares se preparan offline y se guardan como recursos locales ignorados por Git si contienen arte derivado del juego.

**Criterio visual A/B:** capturar exactamente el mismo nivel, coordenadas, estado, fotograma y resolución con el preset apagado y encendido. Crear una comparación lado a lado y revisar: contraste, legibilidad, contornos, brillo, sombras, profundidad, partículas y ausencia de artefactos. Si el usuario no puede percibir una mejora clara sin que el juego parezca sobreprocesado, ajustar la implementación; no declarar la fase aprobada por tener un shader activo o por cambiar unos pocos píxeles.

### 13.5 Pantalla de 120 Hz sin acelerar la simulación

El motor recompila un juego SNES; la prioridad es preservar la cadencia temporal original, no imponer 120 ticks de juego por segundo.

1. Inspeccionar el loop principal, el scheduler de frames, `SDL_RenderPresent`, audio, flags de turbo/fast-forward y temporización de cinemáticas. Verificar que no haya modo `Turbo` activo en la build de producción ni en las capturas de QA.
2. Mantener el reloj de simulación SNES/NTSC y el avance de la historia por frames emulados. No cambiar la lógica a `deltaTime=1/120`, no duplicar updates de física y no acelerar/desacelerar escenas narrativas para alcanzar un número de FPS.
3. Separar, si la arquitectura lo permite sin reescritura, muestreo de entrada, simulación y presentación. Presentar con VSync/frame pacing estable; no renderizar en bucle ocupado cuando no haya un frame nuevo.
4. Investigar la API de frecuencia de pantalla de Android y la integración existente de SDL3. En Android compatible, solicitar 120 Hz cuando el dispositivo/usuario lo permitan; respetar los límites térmicos, ahorro de energía y decisión del sistema. No añadir Swappy/AGDK a ciegas ni migrar el backend. La guía oficial explica que el frame pacing ayuda a coordinar la presentación y que una pantalla a 120 Hz mostrando un juego de 60 FPS puede repetir cada imagen y consumir más batería: https://developer.android.com/games/sdk/frame-pacing y https://developer.android.com/games/optimize/display-refresh-rate-change
5. Informar por separado de `refresh rate` de la pantalla, FPS de presentación y FPS únicos de la simulación. El objetivo es aprovechar 120 Hz cuando aporte una mejora medible, sin prometer 120 frames de juego únicos si la simulación sigue a la cadencia original SNES.
6. No aplicar interpolación visual automática por defecto: en pixel art puede crear bordes dobles, sprites deformados y artefactos en el scrolling. Solo evaluarla como experimento aislado, reversible y comparado contra la salida original, nunca como requisito para cerrar el proyecto.
7. Probar `Auto`, 60 Hz y 120 Hz si el dispositivo los ofrece, con la misma ruta determinista: boot, prólogo, intro, mapa, nivel 1-1, scroll, jefe, agua/lluvia y pausa. Medir frame pacing, audio, input latency aproximada, temperatura y consumo relativo.
8. Confirmar que el audio no cambia de velocidad, no hay cortes ni desincronización y las cinemáticas conservan el ritmo del juego original. Los scripts de benchmark pueden acelerar el tiempo de prueba, pero deben indicarlo y no servir como evidencia de velocidad de reproducción normal.
9. Ofrecer un modo de frecuencia `Automática`/`Alta si está disponible` si hace falta una preferencia del usuario. `Automática` debe priorizar fluidez estable y batería; `Alta` solicita la frecuencia máxima compatible, sin modificar la simulación. No multiplicar las opciones visuales ni convertir el menú en un panel técnico.

### 13.6 Mandos y controles táctiles

GameSir ya funciona según la prueba manual del usuario. No reimplementar desde cero lo que ya funciona: auditar el mapeo actual y generalizarlo con las APIs estándar de SDL3.

1. Usar el modelo normalizado de `SDL_Gamepad`/`SDL3/SDL_gamepad.h` cuando el dispositivo se reconozca como gamepad. Aprovechar mappings integrados y carga de mappings de SDL; no asumir que el botón físico rotulado A siempre tiene la misma ubicación en mandos distintos.
2. Cubrir cruceta digital, sticks si se usan, botones frontales, hombros/gatillos, Start/Options y Select/Back. Gestionar `GAMEPAD_ADDED`/`GAMEPAD_REMOVED`, reconexión Bluetooth/USB-C, repeticiones, deadzones y eventos duplicados. Un dispositivo desconocido debe seguir teniendo un fallback de joystick configurable cuando sea razonable.
3. Mantener el soporte GameSir verificado como caso de regresión. Añadir pruebas con mando Xbox, PlayStation y Switch Pro/Joy-Con si están disponibles. No declarar compatibilidad universal con hardware que no se haya probado; registrar el modelo, conexión y mapeo.
4. En controles táctiles `Auto`, ocultarlos cuando se detecte un gamepad compatible conectado/activo y volver a mostrarlos cuando se desconecte. No ocultarlos por conectar auriculares, teclado o un dispositivo que no sea gamepad. Permitir modo `Siempre visibles` y `Ocultos` en ajustes.
5. Corregir los botones que se cortan. Crear una sola función `ControlLayout` que calcule por control el rectángulo dibujado y el rectángulo táctil a partir de las mismas coordenadas lógicas. Añadir un margen de seguridad proporcional al tamaño de pantalla, respetar insets del notch y esquinas curvas y mantener todos los botones dentro del drawable. No usar coordenadas del SDL drawable como si fueran píxeles de la vista Android sin conversión.
6. El bloque cuadrado con cara y la flecha de gameplay no deben cortarse por confundir el viewport de juego con la capa táctil. Separar y validar: framebuffer/viewport del juego, overlay táctil y safe insets.
7. Probar en portrait solo si se admite en producto; en horizontal normal y horizontal invertido, con notch, con mando conectado, con desconexión durante partida, y con el panel de pausa abierto. Las zonas activas deben coincidir con los dibujos, sin taps fantasma ni botones atascados.
8. Si el mando conectado soporta vibración mediante SDL3, se puede ofrecer rumble opcional separado de la háptica del teléfono. No asumir que todos los mandos exponen rumble ni que un teléfono puede reproducir la riqueza de HD Rumble de Switch.

Referencias: https://developer.android.com/games/sdk/game-controller/controller-input y https://wiki.libsdl.org/SDL3/CategoryGamepad

### 13.7 Guardado avanzado de QA y prueba real de recuperación

Además del sistema de autoguardado/rewind descrito en la sección 5:

1. Crear un guardado de QA un poco avanzado dentro del juego, usando una ruta real de gameplay y un punto estable. Preferir un slot manual dedicado libre, después de hacer una copia de seguridad del estado actual. No sobrescribir ninguno de los slots personales.
2. El guardado debe permitir comprobar gameplay y efectos sin volver a recorrer siempre la intro y el primer tramo. Debe indicar el mundo/nivel y el punto de partida en el registro de QA. No se necesita desbloquear el juego entero ni alterar permanentemente el progreso de la partida normal.
3. Verificar el guardado con cierre forzado y relanzamiento, carga desde menú, transición de nivel y carga repetida. Comprobar checksum/identidad del juego y que no se congela el audio ni se queda el mando bloqueado.
4. Si el framework ofrece save states completos fiables, usar el formato que ya soporta el proyecto. Si no, crear el guardado mediante SRAM/puntos de guardado del juego y no fabricar un estado parcial que corrompa la partida.
5. Mantener este slot/archivo en el perfil local de QA o en un directorio `local/qa-saves/` ignorado por Git. No empaquetarlo dentro de la APK final para todos los usuarios ni subirlo a un repositorio público. Si el usuario tiene partidas en el teléfono, respaldarlas antes de tocar cualquier slot.
6. Si el entorno de Antigravity no puede escribir directamente en el teléfono físico, dejar el archivo válido y una ruta de importación concreta en la entrega local. No fingir que quedó instalado en el dispositivo.

### 13.8 Búsqueda activa de glitches, comparativas y beta test

1. Usar la matriz existente de 54 niveles, seis extras y 12 encuentros de jefe. No repetir de cero pruebas que siguen válidas: ejecutar la suite y los replays existentes y repetir los casos afectados por cambios del renderer, temporización, controles o guardado.
2. Ejecutar los tests automáticos y la regresión de boot, revisar capturas visualmente y hacer un recorrido jugable manual donde el dispositivo lo permita. Incluir título, logo, intro, mapa, transición, nivel normal, nivel vertical, agua, lluvia, plataformas, jefes, extras, pausa, guardado, carga, rewind, logros y reconexión de mando.
3. Buscar activamente artefactos no listados: costuras, sprites que aparecen tarde, clipping, flechas tapadas, objetos cortados, HUD mal anclado, paletas que parpadean, fondo que se desplaza a otra velocidad, partículas sobre texto, bloom que quema blancos, sonido cortado, velocidad incorrecta, entrada duplicada, vibración acumulada y corrupción de save.
4. Reproducir cada bug, anotar gravedad y causa confirmada/hipótesis en `BUGS.md`, aplicar un fix mínimo y volver a ejecutar la prueba que falló. No cerrar un bug porque no se reproduce una vez.
5. Hacer una prueba de estabilidad de al menos 30 minutos en la plataforma más representativa disponible, con movimiento, audio, efectos y cambios de escena; registrar memoria, uso de CPU/GPU, frame pacing, batería/temperatura si es medible, y errores de audio.
6. Para las escenas clave, generar pares `before.png`/`after.png` del mismo estado y una comparación lado a lado, además de un resumen corto de qué cambió. Guardar todo bajo `local/final-qa/` o directorio ignorado equivalente.
7. El paquete de reportes puede contener capturas con arte del juego. Mantener esas comparativas localmente; no subirlas a GitHub público sin revisar derechos. Para reportes públicos, preferir métricas, código original y ejemplos sintéticos/no derivados.

### 13.9 GitHub: publicar solo lo que realmente se pueda licenciar

**No existe una manera de garantizar que Nintendo no presente una reclamación solo por quitar nombres, cambiar el README o no incluir el archivo ROM.** No se debe ocultar el origen del código ni cambiar nombres para eludir detección. Que existan otros proyectos públicos de decompilación/recompilación no demuestra que estén autorizados ni que el riesgo sea cero.

Antes de publicar cualquier parte:

1. Revisar las licencias exactas del checkout y de las dependencias fijadas, no solo las páginas web actuales. La documentación pública actual de SNESRecomp indica PolyForm Noncommercial 1.0.0 para su código original, mientras que el repositorio de desensamblado de Yoshi's Island declara GPL-3.0. Revisar qué archivos del proyecto derivan de qué componente y si las licencias son compatibles. Referencias: https://github.com/RetroPortingToolKit/snesrecomp/blob/main/LICENSE y https://github.com/Yoshifanatic1/Yoshi-s-Island-Disassembly
2. No asumir que una licencia de código abierto del desensamblado concede derechos sobre el juego, la ROM, sus gráficos, música, personajes, marcas o una APK recompilada. La disponibilidad pública de un repositorio no es una autorización de Nintendo.
3. No publicar la ROM, datos extraídos de la ROM, recursos gráficos/audio del juego, traducciones derivadas no autorizadas, APKs que incluyan el juego, partidas/save states, capturas del gameplay, icono de Yoshi, tokens, claves, rutas privadas o cachés.
4. No agregar una licencia MIT/GPL al proyecto entero sin revisar primero las licencias heredadas y el origen de los archivos generados. Mantener las licencias y avisos de terceros de forma exacta; no relicenciar código ajeno.
5. Mantener commits locales y, si ya existe un remoto privado autorizado, usarlo solo con la política privada actual. **No hacer push público ni crear un repo público automáticamente.** Requerir aprobación explícita del usuario después de la auditoría.
6. Si se desea compartir algo públicamente, preparar un `export-staging/` separado con allowlist de archivos originales y genéricos (por ejemplo, un shader de presentación independiente o utilidades Android escritas por el proyecto), un SBOM/inventario de licencias, avisos requeridos y un escaneo de secretos/ROM/assets/APK/saves. Un archivo solo puede entrar si su procedencia y licencia están documentadas y es separable sin arrastrar código derivado del juego.
7. Si no se puede demostrar que un componente es publicable, dejarlo fuera del export público. La alternativa segura es mantener el proyecto completo privado y publicar únicamente herramientas genéricas originales o una demostración sintética independiente.
8. GitHub explica que puede retirar contenido tras una notificación DMCA y que las contranotificaciones tienen un proceso formal. Esto no es una opinión legal personalizada; no se puede prometer inmunidad. Referencias: https://docs.github.com/en/site-policy/content-removal-policies/dmca-takedown-policy y https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository

### 13.10 Entrega final mínima

Entregar una única APK ARM64 final y un informe breve con:

- SHA-256, versión/build, arquitectura y resultado de instalación/arranque.
- Matriz de los 54 niveles, seis extras y 12 jefes con estado honesto.
- Lista de bugs corregidos y bugs restantes, con gravedad.
- Capturas A/B de icono, intro/logo, mapa, controles, iluminación, escena de agua/lluvia y gameplay panorámico.
- Resultados de frame pacing/120 Hz, audio, batería/temperatura si se midió y prueba de estabilidad.
- Resultado de controles GameSir y otros mandos efectivamente probados.
- Resultado de SRAM, autoguardado, rewind y slot de QA avanzado.
- Estado de publicación GitHub: `PRIVATE/LOCAL`, `PUBLICABLE` por allowlist revisada o `BLOCKED` por licencia/derechos. No se publica nada en público como parte automática del release.

El objetivo de cierre es **jugable, disfrutable y técnicamente estable**, no una perfección infinita. Una mejora visual solo se acepta si se ve mejor en el teléfono y no rompe mecánicas, timing, audio, guardados o rendimiento.

---

## 14. Mandato operativo final para Antigravity

Lee este documento completo y continúa sobre el checkout existente. No reconstruyas el proyecto desde ZIPs, no compares paquetes de builds, no reinicies el trabajo de los 54 niveles ni migres el renderer por gusto. El paquete de reportes/capturas del usuario es evidencia de contexto; el código real, el historial Git y las pruebas del checkout son la fuente de implementación.

Orden de prioridad:

1. Preflight: guardar `git status`, HEAD, cambios locales, hash del APK instalado y captura baseline. No sobrescribir trabajo ni slots del usuario.
2. Corregir el icono restaurando el arte anterior y su configuración Android, sin rediseñarlo.
3. Corregir bandas negras/letterboxing accidentales del logo, prólogo, intro, mapa, transiciones y gameplay. Inventariar escenas y resolver cada una con la estrategia visual adecuada; el usuario no edita ni expande assets.
4. Corregir clipping de controles táctiles, flecha circular y objetos cuadrados; separar geometría del juego, viewport, overlay táctil y safe insets.
5. Mejorar de forma perceptible pero moderada `Island Cinema`: luz contextual, sombras de contacto, partículas selectivas y acabado CRT sutil, con pruebas A/B y límites de rendimiento.
6. Corregir timing de la intro/cinemáticas y habilitar la mejor presentación de alta frecuencia compatible, sin acelerar la simulación SNES. Informar Hz/FPS reales por separado.
7. Validar SDL3 Gamepad, mantener GameSir y probar otros mandos disponibles; controles táctiles Auto deben ocultarse/reaparecer correctamente.
8. Verificar háptica contextual, autoguardado, slots, rewind y RA según sus reglas. Crear un guardado avanzado de QA en un slot dedicado sin tocar los personales.
9. Ejecutar suite, regresiones y beta test, buscar glitches nuevos, corregirlos y repetir las pruebas. Crear comparativas antes/después y checkpoints Git locales por fase verificada.
10. Compilar, instalar y verificar la APK final. Auditar licencias y contenidos antes de cualquier publicación; no hacer push público sin aprobación explícita.

No declarar terminado algo que no se ejecutó. Usar `PASS`, `FAIL`, `BLOCKED` y `NOT RUN`. Si el dispositivo físico o un mando concreto no está accesible desde el entorno, dejarlo como gate real y seguir con todo lo demás. No pedir al usuario que prepare recursos salvo que se demuestre que una fuente imprescindible no existe en el checkout.

Este documento es la instrucción completa y única de entrega; no crear un prompt separado.
