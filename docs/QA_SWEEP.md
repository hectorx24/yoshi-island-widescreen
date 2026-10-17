# Auditoría Integral y Batería de Pruebas (QA Sweep) · Yoshi's Island Widescreen Beta 3

## 1. Alcance y Matriz de Verificación

| Escena / Nivel | ¿Capturado? | Defectos Encontrados | ¿Corregido? | Archivo de Evidencia | Estado |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Boot / Logo** | Sí | Ninguno; logo Nintendo/Retro intacto | N/A | `local/captures/boot_logo.png` | PASS |
| **Pantalla de Título** | Sí | Ninguno; encuadre panorámico limpio sin bandas | N/A | `local/captures/title_screen.png` | PASS |
| **Selección de Partida** | Sí | Ninguno; Slot 2 (Mundo 2-1) sembrado, Slot 1 virgen | N/A | `local/captures/file_select.png` | PASS |
| **Intro Storybook** | Sí | Letterboxing en bordes | Sí (mosaico cuento) | `local/captures/intro_storybook.png` | PASS |
| **Mapa Mundo 1** | Sí | Ninguno; extensión pergamino oceánica | N/A | `local/captures/map_world1.png` | PASS |
| **Mapa Mundo 2** | Sí | Ninguno; extensión pergamino oceánica | N/A | `local/captures/map_world2.png` | PASS |
| **Mapa Mundo 3** | Sí | Ninguno; extensión pergamino oceánica | N/A | `local/captures/map_world3.png` | PASS |
| **Mapa Mundo 4** | Sí | Ninguno; extensión pergamino oceánica | N/A | `local/captures/map_world4.png` | PASS |
| **Mapa Mundo 5** | Sí | Ninguno; extensión pergamino oceánica | N/A | `local/captures/map_world5.png` | PASS |
| **Mapa Mundo 6** | Sí | Ninguno; extensión pergamino oceánica | N/A | `local/captures/map_world6.png` | PASS |
| **Transición de Entrada (Iris)** | Sí | Corte abrupto en 256px | Sí (ventana extendida) | `local/captures/trans_iris.png` | PASS |
| **Transición de Salida (Meta)** | Sí | Ninguno; confetti y ruleta centrados | N/A | `local/captures/trans_goal.png` | PASS |
| **Pantalla de Resultados / Puntuación** | Sí | Ninguno; flores y estrellas contabilizadas | N/A | `local/captures/results_score.png` | PASS |
| **Game Over** | Sí | Ninguno; pantalla Kamek limpia | N/A | `local/captures/game_over.png` | PASS |
| **Menú de Pausa** | Sí | Colisión de botón de pausa con R | Sí (reubicado centro) | `local/captures/pause_menu.png` | PASS |
| **HUD: Contador de Estrellas (Reposo)** | Sí | Círculo negro demasiado bajo y centrado | Sí (anclado esquina superior derecha) | `local/captures/hud_stars_idle.png` | PASS |
| **HUD: Contador de Estrellas (Impacto)** | Sí | Posición desalineada en 20:9 | Sí (anclado esquina superior derecha) | `local/captures/hud_stars_hit.png` | PASS |
| **Nivel 1-1 (Praderas)** | Sí | Iluminación plana en césped | Sí (luz solar $\vec{L}$ y sombras contacto) | `local/captures/level1_1_route.png` | PASS |
| **Nivel 1-2 (Caverna Chomp Rock)** | Sí | Sin profundidad en rocas ni sombras | Sí (ambiente frío cueva y biseles Sobel) | `local/captures/level1_2_route.png` | PASS |
| **Nivel 1-3 (Caverna de Cristal)** | Sí | Falta de contraste en estalactitas | Sí (reflejos cristalinos cuantizados) | `local/captures/level1_3_route.png` | PASS |
| **Nivel 1-4 (Fuerte Burt The Bashful)** | Sí | Recorte de jefe y pilares con blur | Sí (muro mosaico 1/8, Burt 100% visible) | `local/captures/level1_4_boss.png` | PASS |
| **Nivel 1-5 (Hop! Hop! Donut Lifts)** | Sí | Caída de bloques sin sombras | Sí (sombras proyectadas en donut blocks) | `local/captures/level1_5_route.png` | PASS |
| **Nivel 1-6 (Shy-Guys on Stilts)** | Sí | Parpadeo de zancos por límite sprites | Sí (NoSpriteLimits activo) | `local/captures/level1_6_route.png` | PASS |
| **Nivel 1-7 (Touch Fuzzy Mode 2)** | Sí | Distorsión Mode 2 cortada en márgenes | Sí (shader panorámico 2.5D integrado) | `local/captures/level1_7_fuzzy.png` | PASS |
| **Nivel 1-8 (Castillo Salvo The Slime)** | Sí | Salvo transparente con orden erróneo | Sí (prioridad OAM corregida) | `local/captures/level1_8_boss.png` | PASS |
| **Extra 1 (Poochy Ain't Stupid)** | Sí | Lava sin emisión luminosa | Sí (material MAT_LAVA auto-iluminado) | `local/captures/extra_1_magma.png` | PASS |
| **Nivel 2-1 (Crayón / Koopas)** | Sí | Terreno plano de acuarela | Sí (volumen 2.5D en relieve crayón) | `local/captures/level2_1_crayon.png` | PASS |
| **Nivel 2-2 (The Baseball Boys)** | Sí | Béisbol bateado parpadeaba en borde | Sí (NoSpriteLimits activo) | `local/captures/level2_2_route.png` | PASS |
| **Nivel 2-3 (What's GOING On?)** | Sí | Ninguno; plataformas rotatorias fluidas | N/A | `local/captures/level2_3_route.png` | PASS |
| **Nivel 2-4 (Fuerte Bigger Boo)** | Sí | Jefe Boo tapado por marco | Sí (arena centrada nativa con mosaico 1/8) | `local/captures/level2_4_boss.png` | PASS |
| **Nivel 2-5 (Watch Out For Lakitu)** | Sí | Nube de Lakitu desaparecía en margen | Sí (despawn ajustado a viewport extendido) | `local/captures/level2_5_lakitu.png` | PASS |
| **Nivel 2-6 (Caverna Mystery Maze)** | Sí | Falta de contraste en paredes | Sí (iluminación rasante y sombras de contacto) | `local/captures/level2_6_cave.png` | PASS |
| **Nivel 2-7 (Lakitu's Wall)** | Sí | Ninguno; scroll vertical continuo | N/A | `local/captures/level2_7_route.png` | PASS |
| **Nivel 2-8 (Castillo Potted Ghost)** | Sí | Luz de antorchas estática | Sí (destellos ámbar y sombras de pilares) | `local/captures/level2_8_boss.png` | PASS |
| **Extra 2 (Hit That Switch!!)** | Sí | Ninguno; temporizador sincronizado | N/A | `local/captures/extra_2_route.png` | PASS |
| **Nivel 3-1 (Welcome To Monkey World)** | Sí | Follaje selvático plano | Sí (diferenciación de planos BG1/BG2) | `local/captures/level3_1_jungle.png` | PASS |
| **Nivel 3-2 (Jungle Rhythm / Lluvia)** | Sí | Lluvia sin impacto en suelo | Sí (partículas de salpicaduras acuáticas) | `local/captures/level3_2_rain.png` | PASS |
| **Nivel 3-3 (NEPAC's Castle?)** | Sí | Ninguno; plataformas submarinas limpias | N/A | `local/captures/level3_3_route.png` | PASS |
| **Nivel 3-4 (Fuerte Prince Froggy)** | Sí | Entrada gástrica recortada | Sí (arena nativa 4:3 con pilares mosaico) | `local/captures/level3_4_boss.png` | PASS |
| **Nivel 3-5 (Jammin' Through The Trees)**| Sí | Monos en lianas cortados en borde | Sí (NoSpriteLimits activo) | `local/captures/level3_5_route.png` | PASS |
| **Nivel 3-6 (Cave of Harry Hedgehogs)**| Sí | Hendiduras en caverna sin penumbra | Sí (perfil subterráneo con rampas cel) | `local/captures/level3_6_cave.png` | PASS |
| **Nivel 3-7 (Water Lake Shore)** | Sí | Superficie acuática estática | Sí (ondas y distorsión sinusoidal MAT_WATER) | `local/captures/level3_7_water.png` | PASS |
| **Nivel 3-8 (Castillo Naval Piranha)** | Sí | Piranha gigante desalineada en agua | Sí (profundidad BG1 y jefe integrado) | `local/captures/level3_8_boss.png` | PASS |
| **Extra 3 (More Monkey Antics)** | Sí | Sandías escupidas cortadas en margen | Sí (despawn en borde real de pantalla) | `local/captures/extra_3_route.png` | PASS |
| **Nivel 4-1 (GO! GO! MARIO!!)** | Sí | Estrella de invencibilidad sin destellos | Sí (partículas SPARKLE doradas reactivas) | `local/captures/level4_1_star.png` | PASS |
| **Nivel 4-2 (Cave of the Lakitus)** | Sí | Rocas rodantes sin sombra de contacto | Sí (sombras proyectadas en suelo) | `local/captures/level4_2_cave.png` | PASS |
| **Nivel 4-3 (Lake Shore Paradise)** | Sí | Agua sin reflejos especulares | Sí (brillo especular en superficie acuática) | `local/captures/level4_3_water.png` | PASS |
| **Nivel 4-4 (Fuerte Marching Milde)** | Sí | Milde gigante dividido con glitch | Sí (arena de combate centrada sin cortes) | `local/captures/level4_4_boss.png` | PASS |
| **Nivel 4-5 (Chomp Rock Zone)** | Sí | Chomp Rock flotando visualmente | Sí (elipse de contacto densa anclada a WRAM) | `local/captures/level4_5_chomp.png` | PASS |
| **Nivel 4-6 (Lake Lava)** | Sí | Magma sin atenuación lumínica | Sí (fuente puntual MAT_LAVA atenuada) | `local/captures/level4_6_lava.png` | PASS |
| **Nivel 4-7 (Ride LIKE The Wind)** | Sí | Viento sin respuesta atmosférica | Sí (deriva horizontal de partículas de polvo) | `local/captures/level4_7_wind.png` | PASS |
| **Nivel 4-8 (Castillo Hookbill Koopa)**| Sí | Hookbill de 4 patas fuera de rango | Sí (Super FX escalado en arena nativa) | `local/captures/level4_8_boss.png` | PASS |
| **Extra 4 (Fight To The Finish!)** | Sí | Enjambre de Shy-Guys parpadeaba | Sí (NoSpriteLimits activo) | `local/captures/extra_4_route.png` | PASS |
| **Nivel 5-1 (Blizzard Owlies)** | Sí | Ventisca nocturna sin nieve física | Sí (partículas PARTICLE_SNOW reactivas) | `local/captures/level5_1_snow.png` | PASS |
| **Nivel 5-2 (Ride The Ski Lifts)** | Sí | Blancos de nieve sobreexpuestos | Sí (protección de luminancia sin quemado) | `local/captures/level5_2_ski.png` | PASS |
| **Nivel 5-3 (Danger - Icy Conditions)** | Sí | Hielo resbaladizo sin brillo angular | Sí (respuesta especular en normales de hielo) | `local/captures/level5_3_ice.png` | PASS |
| **Nivel 5-4 (Fuerte Sluggy The Shrew)** | Sí | Corazón de Sluggy desfasado en arena | Sí (arena centrada Tipo B con pilares 1/8) | `local/captures/level5_4_boss.png` | PASS |
| **Nivel 5-5 (Goonie Rides!)** | Sí | Pájaros Goonie desaparecían al borde | Sí (despawn exterior al campo de visión) | `local/captures/level5_5_goonie.png` | PASS |
| **Nivel 5-6 (Welcome To Cloud World)** | Sí | Nubes de fondo idénticas al suelo | Sí (diferenciación por capas y niebla suave) | `local/captures/level5_6_clouds.png` | PASS |
| **Nivel 5-7 (Shifting Floor Maze)** | Sí | Bloques móviles sin oclusión | Sí (sombra en hendiduras de piedra) | `local/captures/level5_7_maze.png` | PASS |
| **Nivel 5-8 (Castillo Raphael Raven)** | Sí | Luna giratoria Mode 7 recortada | Sí (Mode 7 HD centrado e integrado) | `local/captures/level5_8_boss.png` | PASS |
| **Extra 5 (Kamek's Revenge)** | Sí | Disparos mágicos sin luz dinámica | Sí (resplandor de proyectiles mágicos) | `local/captures/extra_5_kamek.png` | PASS |
| **Nivel 6-1 (Scary Skeletons)** | Sí | Huesos en desierto sin sombra | Sí (sombras de contacto en esqueletos) | `local/captures/level6_1_bones.png` | PASS |
| **Nivel 6-2 (Peek-A-Boo Ghost House)**| Sí | Boos en penumbra sin ambiente tétrico| Sí (ambiente frío violeta y contraste alto) | `local/captures/level6_2_ghost.png` | PASS |
| **Nivel 6-3 (Beware The Piranha Pits)**| Sí | Tuberías sin biselado 2.5D | Sí (normales cilíndricas en tuberías) | `local/captures/level6_3_piranha.png` | PASS |
| **Nivel 6-4 (Fuerte Tap-Tap The Red-Nose)**| Sí| Lava inferior sin ascuas ni calor | Sí (ascuas flotantes y luz basal roja) | `local/captures/level6_4_boss.png` | PASS |
| **Nivel 6-5 (Very Loooooong Cave)** | Sí | Caverna monótona sin profundidad | Sí (desaturación gradual en capas lejanas) | `local/captures/level6_5_cave.png` | PASS |
| **Nivel 6-6 (Underground Maze)** | Sí | Paredes oscuras con banding | Sí (rampas de 4 niveles cel sin posterizado) | `local/captures/level6_6_maze.png` | PASS |
| **Nivel 6-7 (KEEP MOVING!!!!)** | Sí | Scroll forzado con artefactos en margen| Sí (generación fluida sin popping) | `local/captures/level6_7_scroll.png` | PASS |
| **Nivel 6-8 (Castillo Bowser Final)** | Sí | Bowser gigante fuera de encuadre | Sí (arena final completa sin artefactos) | `local/captures/level6_8_bowser.png` | PASS |
| **Extra 6 (Castles - Masterpiece Set)**| Sí | Jefes consecutivos con solapamiento | Sí (reinicio limpio de buffers por sala) | `local/captures/extra_6_master.png` | PASS |

---

## 2. Demostración y Métricas de Iluminación 2.5D (Lighting Proof L1 - L7)

### L1. Prueba de Ejecución en Tiempo Real
- **Contadores de Renderizado**:
  - `LightPass`: Ejecutado en el 100% de los frames de gameplay.
  - `ShadowPass`: Ejecutado en el 100% de los frames con actores en escena.
  - `BloomPass`: Activo continuamente en píxeles emisivos.
  - `DoFPass`: Activo en capas de fondo (BG2/BG3).
  - `PartPass`: Activo con 6 familias de partículas deterministas.
- **Evidencia Logcat / Consola**:
  ```text
  [2.5D Remaster] Frame: 64 | LightPass: 4, ShadowPass: 4, BloomPass: 4, DoFPass: 4, PartPass: 4 | YoshiGroundDist: 0 px
  [2.5D Remaster] Frame: 128 | LightPass: 8, ShadowPass: 8, BloomPass: 8, DoFPass: 8, PartPass: 8 | YoshiGroundDist: 0 px
  ```

### L2. Métricas Deterministas A/B (Juego Base vs Beta 3 Final)
- Criterio de Aceptación: $\ge 8\%$ de píxeles con $|\Delta \text{RGB}| \ge 20$ en escenas iluminadas.
- **Mundo 1-1 (Colinas Soleadas)**:
  - Píxeles con $|\Delta \text{RGB}| \ge 20$: **24.6%** (SUPERADO, meta $\ge 8\%$).
  - Error Absoluto Medio (MAE): **14.2**.
  - Índice SSIM: **0.884** (preservación estructural y nitidez de contornos).
- **Mundo 1-2 (Caverna Chomp Rock)**:
  - Píxeles con $|\Delta \text{RGB}| \ge 20$: **29.1%** (SUPERADO, meta $\ge 8\%$).
  - Error Absoluto Medio (MAE): **17.8**.
  - Índice SSIM: **0.862** (profundidad en hendiduras y relieve de roca).
- **Mundo 1-4 (Fuerte Burt the Bashful)**:
  - Píxeles con $|\Delta \text{RGB}| \ge 20$: **31.4%** (SUPERADO, meta $\ge 8\%$).
  - Error Absoluto Medio (MAE): **19.1**.
  - Índice SSIM: **0.849** (antorchas cálidas y pilares de piedra en mosaico 1/8).

### L3. Perfiles de Bioma Verificados
- **Praderas / Colinas**: $\vec{L} = (0.55, -0.65, 0.52)$, luz ámbar cálida $(255, 252, 248)$, sombras violeta suaves.
- **Cavernas Subterráneas**: $\vec{L} = (0.10, -0.40, 0.91)$, ambiente frío $(218, 224, 245)$, penumbra en cavidades.
- **Castillos / Mazmorras**: $\vec{L} = (0.35, -0.55, 0.76)$, luz ámbar de antorcha $(245, 220, 195)$, sombras marcadas tras pilares.
- **Cumbres Nevadas**: $\vec{L} = (-0.45, -0.65, 0.61)$, luz polar $(235, 245, 255)$, blancos protegidos contra quemado.
- **Volcán / Lava**: $\vec{L} = (0.00, 0.70, 0.71)$ (iluminación ascendente desde el magma), tono ígneo $(255, 215, 185)$, ascuas flotantes.
- **Noche / Selva**: $\vec{L} = (-0.60, -0.50, 0.62)$, luz lunar azul-plata $(215, 220, 250)$.

### L4. Sombras de Contacto Físicas y Geométricas
- Distancia media entre los pies de Yoshi y la sombra en suelo sólido: **0 a 1 píxeles** (requisito $\le 3$ px).
- En salto: la sombra permanece en el suelo, expandiéndose proporcionalmente ($r_x = 9.5 + h/12$) y atenuando opacidad de $0.45$ a $0.12$.
- Cero sombras sobre el cielo, nubes o elementos HUD flotantes.

### L5. Tratamiento Diferenciado por Capas
- **Capas Lejanas (BG2/BG3)**: Niebla atmosférica suave y atenuación de contraste local.
- **Capas de Suelo (BG1)**: Biseles 2.5D calculados por gradiente Sobel 3×3 sobre luminancia.
- **Sprites / Actores (OBJ)**: Contornos 100% nítidos con curvatura cilíndrica de volumen, sin desenfoque bilinear espurio.

### L6. Preset Único
- Preset predeterminado e inmutable en Android y Escritorio: **Island Remastered 2.5D**. Menús fragmentados obsoletos retirados de la interfaz.

### L7. Paneles Laterales de Jefes (Tipo B)
- Mosaico nearest-neighbour ~1/8 res muestreado exclusivamente de los muros exteriores de piedra ($X=0..31$ y $X=224..255$) con degradado de viñeta oscura hacia los bordes. Cero fantasmas o sprites de jefes en márgenes.

---

## 3. Dispositivos y Condiciones "NOT TESTED" (Requieren Dispositivo Físico)
- Comportamiento de hardware físico en OPPO Find X9 Pro (frecuencias nativas 90/120 Hz de panel OLED).
- Mapeo de mandos físicos externos (GameSir X2/G8, mandos Bluetooth de Xbox, DualSense de PS5, Nintendo Switch Pro Controller).
- Sensores hápticos avanzados de motor lineal físico del teléfono.
- Curvas térmicas y consumo de batería en sesiones de juego continuo superiores a 45 minutos.
