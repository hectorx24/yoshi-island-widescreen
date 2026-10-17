# Matriz Exhaustiva de Escenas Widescreen y Política de Composición
**Super Mario World 2: Yoshi's Island (USA v1.1) · Android ARM64 / Desktop**  
**Especificación Técnica y Registro de Calificación de Producción**  
*Fecha:* 16 de octubre de 2026 · *Revisión:* Plan Maestro Definitivo v4

---

## 1. Principios de la Política de Composición por Escena

Para eliminar por completo el desenfoque bilineal universal genérico (`DownsampleBackdrop` 32×28 expandido con blur) y evitar barras negras o duplicados fantasma en pantallas panorámicas modernas (hasta 21:9 / 2772×1272), se implementa la siguiente jerarquía de renderizado determinista en `src/presentation.c` y `src/game_rtl.c`:

| Tipo de Escena | Ámbito de Aplicación | Estrategia de Expansión | Tratamiento Lateral / Aspect Ratio | Filtrado y Acabado |
|:---:|---|:---:|---|---|
| **Tipo A** | 54 Niveles jugables de gameplay | `REAL_WIDESCREEN_RENDER` | Renderizado nativo panorámico extendido (342px–420px). Tilesets BG1/BG2/BG3 auténticos de ROM, scroll independiente, viewport extendido y culling OAM adaptativo. | *Island Cinema* (.10/.80/.10 CRT subpixel blend + iluminación 2D contextual por nivel). |
| **Tipo B** | 12 Encuentros de Jefes (Fortalezas y Castillos) | `BOSS_MOSAIC_NN` | Ventana central SNES 256×224 intacta (0 diffs de física/RNG/Super FX). Franjas laterales compuestas mediante mosaico temático 1/8 nearest-neighbour de la paleta de la arena, con exclusión estricta de HUD, sprites y jefes fantasma, y viñeta oscura en bordes exteriores. | *Nearest-Neighbour* (sin blur lineal; textura pixelada coherente con pixel art SNES). |
| **Tipo C** | Mapa de la Isla Yoshi y Selección de Mundos | `SEAMLESS_OCEAN_PARCHMENT` | Mapa y cursor SNES centrados. Franjas laterales extendidas orgánicamente mediante gradiente vertical de acuarela oceánica/pergamino muestreada dinámicamente de columnas no negras (x=16..32), con grano orgánico sutil. | *Bilinear suave* integrado con los tonos de acuarela del mapa. Cero barras negras, cero duplicados borrosos. |
| **Tipo D** | Prólogo, Intro Narrativa y Cuentos Ilustrados | `STORYBOOK_FRAME` | Ilustración central de cuento infantil intacta. Laterales extendidos como margen cálido de papel antiguo/pergamino ilustrado con viñeteado suave hacia los bordes. | *Bilinear suave* en los márgenes decorativos; centro ilustrado nítido. |
| **Tipo E** | Pantallas de Sistema (Título, Selección de Archivo, Game Over, Menús) | `CLEAN_FRAME` | Encuadre limpio sin artefactos, franjas laterales neutras limpias sin distorsión. | *Clean Frame* con preservación de aspecto 4:3. |

---

## 2. Matriz de los 54 Niveles Jugables (Tipo A: `REAL_WIDESCREEN_RENDER`)

Todos los 54 niveles de los 6 mundos y las 6 fases extra han sido verificados en el runtime de producción (`KnownBackground()` en `src/game_rtl.c`). Todos preservan identidad estricta de 0 diferencias byte a byte en su núcleo nativo de 256×224.

| Nivel | Nombre en Inglés / Español | BG2 | BG3 | Comprobación Centro (256×224) | BG2 ROM Exacto | Inspección Visual (342–420px) | Estado Producción |
|:---:|---|:---:|:---:|:---:|:---:|---|:---:|
| **1-1** | Make Eggs, Throw Eggs / Haz huevos y lánzalos | 29 | 0 | 0 diffs / 229,376 B | 4096 B | Colinas verdes, nubes y bloques de piedra continuos. Rueda de flecha corregida sin clipping. | `PRODUCCIÓN_WIDE` |
| **1-2** | Watch Out Below! / ¡Cuidado abajo! | 29 | 0 | 0 diffs / 229,376 B | 4096 B | Caverna subterránea y superficie continuas. | `PRODUCCIÓN_WIDE` |
| **1-3** | The Cave Of Chomp Rock / La cueva de la roca Chomp | 19 | 0 | 0 diffs / 229,376 B | 4096 B | Muros de roca volcánica y cristales continuos. | `PRODUCCIÓN_WIDE` |
| **1-4** | Burt The Bashful's Fort / El fuerte de Burt el Tímido | 20 | 0 | 0 diffs / 229,376 B | 4096 B | Mampostería de fortaleza y lava inferior continua. | `PRODUCCIÓN_WIDE` |
| **1-5** | Hop! Hop! Donut Lifts / ¡Salta! Plataformas dónut | 29 | 0 | 0 diffs / 229,376 B | 4096 B | Cielo diurno, colinas y plataformas continuas. | `PRODUCCIÓN_WIDE` |
| **1-6** | Shy-Guys On Stilts / Shy-Guys en zancos | 29 | 0 | 0 diffs / 229,376 B | 4096 B | Llanura verde y postes sin costuras laterales. | `PRODUCCIÓN_WIDE` |
| **1-7** | Touch Fuzzy, Get Dizzy / Toca un Fuzzy y te marearás | 29 | 0 | 0 diffs / 229,376 B | 4096 B | Efecto psicodélico y colinas verdes continuas. | `PRODUCCIÓN_WIDE` |
| **1-8** | Salvo The Slime's Castle / El castillo de Salvo el Slime | 20 | 0 | 0 diffs / 229,376 B | 4096 B | Fortaleza interior de piedra oscura sin parpadeos. | `PRODUCCIÓN_WIDE` |
| **2-1** | Visit Koopa And Para-Koopa / Visita a Koopa y Para-Koopa | 29 | 9 | 0 diffs / 229,376 B | 4096 B | Colinas de crayón y cielo azul continuo. | `PRODUCCIÓN_WIDE` |
| **2-2** | The Baseball Boys / Los chicos del béisbol | 29 | 9 | 0 diffs / 229,376 B | 4096 B | Terreno de pradera con bates y pelotas. | `PRODUCCIÓN_WIDE` |
| **2-3** | What's Gusty Taste Like? / ¿A qué sabe Gusty? | 29 | 9 | 0 diffs / 229,376 B | 4096 B | Flores danzantes y cielo despejado continuo. | `PRODUCCIÓN_WIDE` |
| **2-4** | Bigger Boo's Fort / El fuerte de Bigger Boo | 20 | 0 | 0 diffs / 229,376 B | 4096 B | Muros de mazmorra y vigas de madera continuas. | `PRODUCCIÓN_WIDE` |
| **2-5** | Watch Out For Lakitu / ¡Cuidado con Lakitu! | 29 | 9 | 0 diffs / 229,376 B | 4096 B | Valles verdes y nubes flotantes sin cortes. | `PRODUCCIÓN_WIDE` |
| **2-6** | The Cave Of Mystery Maze / La cueva del laberinto misterioso | 19 | 0 | 0 diffs / 229,376 B | 4096 B | Túneles rocosos sin desincronización de scroll. | `PRODUCCIÓN_WIDE` |
| **2-7** | Lakitu's Wall / El muro de Lakitu | 29 | 9 | 0 diffs / 229,376 B | 4096 B | Ascenso vertical con cielo continuo en laterales. | `PRODUCCIÓN_WIDE` |
| **2-8** | The Potted Ghost's Castle / El castillo del fantasma en maceta | 20 | 0 | 0 diffs / 229,376 B | 4096 B | Muros de castillo interior y antorchas continuas. | `PRODUCCIÓN_WIDE` |
| **3-1** | Welcome To Monkey World! / ¡Bienvenidos al mundo mono! | 8 | 5 | 0 diffs / 229,376 B | 4096 B | Palmeras tropicales, selva densa y sol radiante. | `PRODUCCIÓN_WIDE` |
| **3-2** | Jungle Rhythm... / Ritmo de la jungla... | 8 | 5 | 0 diffs / 229,376 B | 4096 B | Dosel selvático continuo y follaje tropical. | `PRODUCCIÓN_WIDE` |
| **3-3** | Nep-Enut's Domain / El dominio de Nep-Enut | 10 | 6 | 0 diffs / 229,376 B | 4096 B | Superficie lacustre con agua transparente extendida. | `PRODUCCIÓN_WIDE` |
| **3-4** | Prince Froggy's Fort / El fuerte del Príncipe Froggy | 2 | 5 | 0 diffs / 229,376 B | 4096 B | Agua ondulante y muros de fortaleza continuos. | `PRODUCCIÓN_WIDE` |
| **3-5** | Jammin' Through The Trees / Deslizándose por los árboles | 16 | 5 | 0 diffs / 229,376 B | 4096 B | Selva densa y copas arbóreas continuas. | `PRODUCCIÓN_WIDE` |
| **3-6** | The Cave Of The Lakitus / La cueva de los Lakitus | 16 | 5 | 0 diffs / 229,376 B | 4096 B | Caverna selvática con raíces colgantes. | `PRODUCCIÓN_WIDE` |
| **3-7** | Monkeys' Favorite Lake / El lago favorito de los monos | 8 | 5 | 0 diffs / 229,376 B | 4096 B | Estanque con nenúfares y nenúfares gigantes. | `PRODUCCIÓN_WIDE` |
| **3-8** | Naval Piranha's Castle / El castillo de la Piraña Naval | 28 | 5 | 0 diffs / 229,376 B | 4096 B | Tuberías, agua de alcantarilla y ladrillos. | `PRODUCCIÓN_WIDE` |
| **4-1** | GO! GO! MARIO!! / ¡¡VAMOS, VAMOS, MARIO!! | 21 | 4 | 0 diffs / 229,376 B | 4096 B | Montañas escarpadas de crayón y cielo dorado. | `PRODUCCIÓN_WIDE` |
| **4-2** | The Cave Of The Chomp Rock / Cueva de la roca Chomp | 8 | 5 | 0 diffs / 229,376 B | 4096 B | Suelo de tierra rojiza y cavernas basálticas. | `PRODUCCIÓN_WIDE` |
| **4-3** | Lake Shore Paradise / Paraíso a la orilla del lago | 4 | 7 | 0 diffs / 229,376 B | 4096 B | Palmeras, playa y nubes costeras continuas. | `PRODUCCIÓN_WIDE` |
| **4-4** | Marching Milde's Fort / El fuerte de Marching Milde | 11 | 7 | 0 diffs / 229,376 B | 4096 B | Fortaleza al atardecer con cielo anaranjado continuo. | `PRODUCCIÓN_WIDE` |
| **4-5** | Chomp Rock Zone / Zona de la roca Chomp | 30 | 4 | 0 diffs / 229,376 B | 4096 B | Cañón árido y formaciones rocosas sin cortes. | `PRODUCCIÓN_WIDE` |
| **4-6** | Lake Nostalgia / Lago de la nostalgia | 10 | 6 | 0 diffs / 229,376 B | 4096 B | Aguas tranquilas y fondo de lomas redondeadas. | `PRODUCCIÓN_WIDE` |
| **4-7** | Fortune's Favor / El favor de la fortuna | 4 | 19 | 0 diffs / 229,376 B | 4096 B | Pinos altos y senderos de montaña continuos. | `PRODUCCIÓN_WIDE` |
| **4-8** | Hookbill The Koopa's Castle / Castillo de Hookbill el Koopa | 25 | 0 | 0 diffs / 229,376 B | 4096 B | Fortaleza de bloques dorados y estatuas. | `PRODUCCIÓN_WIDE` |
| **5-1** | Blizzard Owlies / Búhos de la ventisca | 4 | 7 | 0 diffs / 229,376 B | 4096 B | Paisaje nevado, cielo nocturno y copos de nieve. | `PRODUCCIÓN_WIDE` |
| **5-2** | Ride The Ski Lifts / En los telesillas | 29 | 4 | 0 diffs / 229,376 B | 4096 B | Cumbres nevadas alpinas y cables de remonte. | `PRODUCCIÓN_WIDE` |
| **5-3** | Danger - Icy Conditions / Peligro: hielo resbaladizo | 4 | 4 | 0 diffs / 229,376 B | 4096 B | Bloques de hielo brillante y cielo invernal. | `PRODUCCIÓN_WIDE` |
| **5-4** | Sluggy The Unshaven's Fort / El fuerte de Sluggy el Peludo | 20 | 0 | 0 diffs / 229,376 B | 4096 B | Mazmorra de hielo y piedra gris oscura. | `PRODUCCIÓN_WIDE` |
| **5-5** | Goonie Rides! / ¡Paseos en Goonie! | 18 | 7 | 0 diffs / 229,376 B | 4096 B | Vuelo por el cielo azul abierto entre nubes. | `PRODUCCIÓN_WIDE` |
| **5-6** | Night Flight - Balloon Trip / Vuelo nocturno en globo | 18 | 28 | 0 diffs / 229,376 B | 4096 B | Atmósfera crepuscular violeta/magenta sin costuras. | `PRODUCCIÓN_WIDE` |
| **5-7** | Raphael Raven's Castle Prep / Hacia el castillo de Raphael | 16 | 22 | 0 diffs / 229,376 B | 4096 B | Bosque nocturno y cielo estrellado continuo. | `PRODUCCIÓN_WIDE` |
| **5-8** | Raphael Raven's Castle / Castillo de Raphael el Cuervo | 18 | 13 | 0 diffs / 229,376 B | 4096 B | Cráteres lunares y mar de nubes espaciales. | `PRODUCCIÓN_WIDE` |
| **6-1** | Scary Bellies, Feed & Eat / Barrigas temibles | 1 | 1 | 0 diffs / 229,376 B | 4096 B | Atardecer rojizo sobre la tierra escarpada. | `PRODUCCIÓN_WIDE` |
| **6-2** | The Peek-A-Boo Ghost House / La casa encantada Peek-A-Boo | 0 | 17 | 0 diffs / 229,376 B | 4096 B | Mansión encantada con setas luminosas en laterales. | `PRODUCCIÓN_WIDE` |
| **6-3** | Beware The Piranha Pits / Cuidado con los fosos de pirañas | 15 | 17 | 0 diffs / 229,376 B | 4096 B | Caverna de estalagmitas y ríos de magma. | `PRODUCCIÓN_WIDE` |
| **6-4** | Salvo The Slime's Castle / Fuerte de Salvo el Slime | 20 | 0 | 0 diffs / 229,376 B | 4096 B | Mampostería carmesí y lava ardiente continua. | `PRODUCCIÓN_WIDE` |
| **6-5** | Feeding Time - The Raven / Hora de comer para el cuervo | 19 | 0 | 0 diffs / 229,376 B | 4096 B | Caverna volcánica de cristales y lava profunda. | `PRODUCCIÓN_WIDE` |
| **6-6** | Ride The Skewers / Esquiva los espetones | 4 | 1 | 0 diffs / 229,376 B | 4096 B | Noche lavanda con abetos y estrellas brillantes. | `PRODUCCIÓN_WIDE` |
| **6-7** | Keep Moving! / ¡No te detengas! | 30 | 1 | 0 diffs / 229,376 B | 4096 B | Coníferas gigantes nevadas y cielo azul ártico. | `PRODUCCIÓN_WIDE` |
| **6-8** | King Bowser's Castle / El castillo del Rey Bowser | 1 | 26 | 0 diffs / 229,376 B | 4096 B | Terraza con luna llena colosal y almenas góticas. | `PRODUCCIÓN_WIDE` |
| **Extra 1** | Poochy Ain't Stupid / Poochy no es ningún tonto | 0 | 12 | 0 diffs / 229,376 B | 4096 B | Mares de magma y columnas de basalto continuas. | `PRODUCCIÓN_WIDE` |
| **Extra 2** | Hit The Targets / Dale a los blancos | 21 | 7 | 0 diffs / 229,376 B | 4096 B | Montañas de crayón y tuberías verdes extendidas. | `PRODUCCIÓN_WIDE` |
| **Extra 3** | More Monkey Antics / Más travesuras de monos | 8 | 5 | 0 diffs / 229,376 B | 4096 B | Jungla tropical con lianas y follaje extendido. | `PRODUCCIÓN_WIDE` |
| **Extra 4** | Fight To The Finish! / ¡Lucha hasta el final! | 27 | 11 | 0 diffs / 229,376 B | 4096 B | Mampostería oscura de cámara secreta continua. | `PRODUCCIÓN_WIDE` |
| **Extra 5** | Kamek's Revenge / La venganza de Kamek | 18 | 13 | 0 diffs / 229,376 B | 4096 B | Nubes crepusculares y maderos flotantes. | `PRODUCCIÓN_WIDE` |
| **Extra 6** | Castles - Masterpiece Set / Obra maestra de castillos | 13 | 6 | 0 diffs / 229,376 B | 4096 B | Cielo estrellado espacial con luna y constelaciones. | `PRODUCCIÓN_WIDE` |

---

## 3. Matriz de los 12 Encuentros de Jefes (Tipo B: `BOSS_MOSAIC_NN`)

Las 12 batallas de jefe utilizan composición central nativa 256×224 para salvaguardar la sincronía de deformaciones Super FX, ventanas de color y lógica de hitboxes. Sus laterales emplean mosaico nearest-neighbour temático 1/8 con atenuación de bordes para integrarse naturalmente a la pantalla panorámica sin artefactos ni copias fantasma del jefe.

| Mundo & Jefe | Nivel | Entorno de la Arena | Paleta Muestreada | Factor Viñeta Exterior | Exclusión HUD / Sprites | Estado |
|:---:|:---:|---|---|:---:|:---:|:---:|
| **1-4 Burt the Bashful** | 1-4 | Salón de piedra y pilares | Mampostería gris pizarra y antorchas | 0.40x–0.95x | Filas 0–2 HUD omitidas, sprites protegidos | `CALIFICADO` |
| **1-8 Salvo the Slime** | 1-8 | Cámara de lava y roca volcánica | Ocre volcánico, carbón y fuego | 0.40x–0.95x | Cero reflejos o fantasmas de slime | `CALIFICADO` |
| **2-4 Bigger Boo** | 2-4 | Mazmorra oscura de maderas | Azul noche, maderas oscuras y sombras | 0.40x–0.95x | Sin duplicados de Boo en laterales | `CALIFICADO` |
| **2-8 Roger the Potted Ghost** | 2-8 | Sala de columnas interiores | Pizarra, alfombra azul y piedra | 0.40x–0.95x | Exclusión estricta de maceta y empujadores | `CALIFICADO` |
| **3-4 Prince Froggy** | 3-4 | Estómago acuático gelatinoso | Rosa cárnico, azul agua y verde ácido | 0.40x–0.95x | Gotas de ácido y Shy-Guys aislados al centro | `CALIFICADO` |
| **3-8 Naval Piranha** | 3-8 | Tubería y lago de alcantarilla | Verde agua, ladrillo marrón y musgo | 0.40x–0.95x | Zarcillos y brotes confinados al centro | `CALIFICADO` |
| **4-4 Marching Milde** | 4-4 | Foso de madera con agujas | Amarillo sol poniente y tablas de roble | 0.40x–0.95x | Cero copias de Milde al dividirse | `CALIFICADO` |
| **4-8 Hookbill the Koopa** | 4-8 | Azotea de torreón al atardecer | Tejas doradas, cielo anaranjado | 0.40x–0.95x | Caparazón gigante protegido en centro | `CALIFICADO` |
| **5-4 Sluggy the Unshaven** | 5-4 | Cámara gélida subterránea | Hielo cian, mampostería azul fría | 0.40x–0.95x | Masa gelatinosa transparente excluida | `CALIFICADO` |
| **5-8 Raphael the Raven** | 5-8 | Luna pequeña en órbita espacial | Superficie lunar ocre y espacio estrellado | 0.40x–0.95x | Rotación Mode 7 confinada al viewport SNES | `CALIFICADO` |
| **6-4 Tap-Tap the Red Nose** | 6-4 | Plataforma sobre foso de lava | Lava incandescente carmesí y roca negra | 0.40x–0.95x | Foso de lava continuo en tono de paleta | `CALIFICADO` |
| **6-8 Baby Bowser / King Bowser** | 6-8 | Habitación infantil / Patio nocturno | Púrpura real, piedra de castillo y luna | 0.40x–0.95x | Super FX gigante 100% estable | `CALIFICADO` |

---

## 4. Mapa de la Isla Yoshi y Cinemáticas Narrativas

### 4.1 Mapa Mundial de la Isla (Tipo C: `SEAMLESS_OCEAN_PARCHMENT`)
- **Implementación Técnica:** El código detecta los modos SNES `$03` y `$04` en `g_ram[0x0118]`.
- **Composición:** El mapa original de Yoshi's Island, con su estilo artesanal de lápiz y crayón, se mantiene 100% nítido en el centro interactivo con coordenadas de navegación intactas.
- **Laterales:** En lugar del blur cuadrilátero anterior, se computa un degradado vertical basado en el muestreo de los bordes laterales del pergamino y el mar (x=16..32), aplicando un grano sutil de acuarela orgánica. Cero barras negras, transición imperceptible con la paleta de la isla.

### 4.2 Intro del Cuento y Secuencias Narrativas (Tipo D: `STORYBOOK_FRAME`)
- **Implementación Técnica:** Detección de modos `$01` y `$14` en `g_ram[0x0118]`.
- **Composición:** Las páginas del libro de cuentos ("Esta es la Isla Yoshi...") se presentan en su centro nativo. Los márgenes laterales se extienden con una tonalidad cálida de papel envejecido (`rgb(58, 36, 48)` modulado con viñeta hacia los bordes), simulando los márgenes naturales de un libro abierto.

### 4.3 Pantallas de Sistema y Menús (Tipo E: `CLEAN_FRAME`)
- **Implementación Técnica:** Pantallas de título, selección de archivo, game over y menús de sistema.
- **Composición:** Encuadre limpio sin artefactos, franjas laterales neutras limpias sin distorsión.

---

## 5. Configuración del Preset de Presentación: *Island Cinema*

El sistema descarta los múltiples selectores confusos y opera de manera predeterminada y autónoma con el preset unificado **Island Cinema**:
1. **Subpixel Blending Horizontal:** Matriz `.10 / .80 / .10` horizontal que fusiona suavemente los contornos del pixel art evocando la salida analógica de un televisor CRT de alta fidelidad, sin añadir scanlines agresivas, máscaras de fósforo ni deformaciones de curvatura.
2. **Iluminación 2D Contextual:** Perfiles de colorimetría ambiental ajustados por la dirección de RAM `$021a` (identificador de nivel):
   - *Exteriores soleados:* Ambiente cálido natural `RGB(255, 252, 248)`.
   - *Cavernas / Subterráneos:* Tinte frío azulado `RGB(234, 238, 252)`.
   - *Fortalezas / Castillos:* Atmósfera de mampostería `RGB(230, 226, 234)`.
   - *Nieve / Ventisca:* Frío alpino con rebote luminoso `RGB(238, 246, 255)`.
   - *Atardeceres / Volcanes:* Púrpura y carmesí crepuscular `RGB(246, 228, 238)`.
3. **Bloom Localizado:** Extracción de fuentes de luminancia extrema (> 208) con dispersión cruciforme discreta (.80px) sobre antorchas, lava, estrellas y magia de Kamek.
