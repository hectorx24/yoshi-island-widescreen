# Super Mario World 2: Yoshi's Island · Android & Desktop Native Port

> ### Aviso legal
> Este es un proyecto independiente, no oficial y sin afiliación, patrocinio ni respaldo de Nintendo. Yoshi's Island, Super Mario, sus personajes, gráficos, música y demás elementos originales pertenecen a Nintendo y a sus respectivos titulares de derechos. Las imágenes mostradas se utilizan únicamente para ilustrar y comparar las mejoras técnicas y visuales del proyecto.
> 
> Este repositorio no incluye ni distribuye ROMs, archivos del juego original ni recursos protegidos extraídos de ellos. Cada usuario deberá aportar su propia copia del juego obtenida legalmente.

---

## 1. Descripción del Proyecto

Recompilación nativa y ejecución en arquitecturas ARM64 (Android y macOS) de *Super Mario World 2: Yoshi's Island* (USA v1.1), incorporando:

1. **Widescreen Nativo (16:9 / 20:9):** Extensión panorámica del viewport jugable con geometría real de fondos BG1/BG2 y sprites OAM extendidos (hasta 420×224 píxeles), manteniendo el centro original de 256×224 píxeles 100% byte-idéntico.
2. **Preset Visual "Island Cinema":**
   - Suavizado CRT de una pasada con pesos subpíxel 0.10 / 0.80 / 0.10 (sin scanlines artificiales ni curvatura de tubo distorsionada).
   - Iluminación 2D-HD contextual por escena (exteriores cálidos, cavernas azuladas, castillos tenues, jungla verdosa y volcanes ígneos).
   - Extracción selectiva de luz emisiva (fuego, lava, antorchas, estrellas y monedas) protegiendo las nubes y la nieve de sobreexposición y halos.
   - Partículas visuales sutiles y deterministas (polvo de aterrizaje, gotas de agua y brasas de lava) en la capa de presentación, sin alterar la simulación.
3. **Política de Composición Panorámica por Escena:**
   - *Tipo A (54 Niveles):* Widescreen ultra-amplio con renderizado nativo.
   - *Tipo B (12 Jefes):* Mosaico nearest-neighbour temático de la arena de combate con viñeta exterior sin interferencia de HUD.
   - *Tipo C (Mapa Mundial):* Extensión continua en acuarela/pergamino muestreando el degradado vertical real.
   - *Tipo D (Intro y Cinemáticas):* Encuadre de libro ilustrado cálido sin franjas negras accidentales.
4. **Localización Española de Calidad:**
   - Integración en memoria de la traducción de *Traducciones Wave (USA Rev 1)* con ortografía española completa (`á, é, í, ó, ú, ñ, ¿, ¡`), diálogos fluidos y adaptación fiel.
   - Conmutable en caliente en opciones sin modificar jamás la ROM del usuario en disco.
5. **Presentación a 120 Hz Desacoplada:**
   - Solicitud de modo de alta frecuencia a 120 Hz en pantallas Android compatibles.
   - Simulación SNES estricta y auténtica a 60.0988 Hz sin alterar el tempo de la música, el pitch ni la física.
6. **Controles Táctiles y Mandos Físicos:**
   - Distribución táctil con márgenes de seguridad unificados (`ControlLayout`) respetando esquinas curvas, notch y barras del sistema.
   - Detección normalizada de mandos (GameSir, Xbox, PlayStation, Switch Pro) con auto-ocultación de controles virtuales al conectar mando físico.
7. **Háptica Inmersiva por Eventos:**
   - Pulsos de vibración diferenciados para saltos, huevos, aterrizajes y daño.
8. **Guardado y QA:**
   - Sistema de autoguardado SRAM robusto, rebobinado (rewind) y slot de QA avanzado (`local/qa-saves/`).

---

## 2. Comparativa Técnica y Visual (Antes y Después)

| Aspecto | Port Base / Emulación Tradicional | Acabado Final "Island Cinema" |
| :--- | :--- | :--- |
| **Relación de Aspecto** | 4:3 estricto con barras negras laterales en pantallas modernas. | Panorámico nativo 16:9 / 20:9 en los 54 niveles con composiciones temáticas para mapas e intros. |
| **Iluminación** | Imagen plana 2D original sin sombras ni luces ambientales. | Iluminación contextual 2D-HD con fuentes emisivas (lava, estrellas) y contraste preservado en nieve y nubes. |
| **Filtro de Imagen** | Escalado lineal borroso o píxeles crudos sin suavizado. | Subpíxel CRT suave 0.10/0.80/0.10 preservando los contornos de tiza y el arte de acuarela. |
| **Idioma** | Inglés original únicamente. | Español de alta calidad (*Traducciones Wave*) con acentos, apertura de interrogación/exclamación y terminología adecuada. |
| **Tasa de Refresco** | Limitado rígidamente a 60 Hz en pantalla. | Modo 120 Hz en Android con simulación y audio mantenidos al 100% en tiempo real. |
| **Controles Táctiles** | Botones genéricos pegados al borde de la pantalla que se recortaban. | Layout calibrado con insets seguros, sin solapamientos y auto-ocultable con mandos Bluetooth/USB. |
| **Icono de Aplicación** | Cuadrado recortado o círculo pequeño sobre placa blanca. | Arte completo original de Yoshi y Bebé Mario respetando las directrices de launcher adaptativo. |

---

## 3. Instrucciones de Compilación y Uso

Este repositorio contiene exclusivamente código fuente original, herramientas de recompilación y utilidades de configuración.

### Requisitos
- macOS o Linux ARM64 / x86_64
- Android NDK r25+ y Android SDK Platform 35 (para build de APK)
- Python 3.9+, Rust 1.80+, CMake 3.22+, Ninja
- Copia legal y privada de la ROM: *Super Mario World 2: Yoshi's Island (USA v1.1)* (SHA-256: `bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06`).

### Compilación para Escritorio (macOS / Linux)
```sh
# 1. Configurar y compilar herramientas nativas
sh tools/build.sh

# 2. Ejecutar la suite de pruebas unitarias y de regresión
sh tools/test.sh
python3 tools/regression_boot.py --rom "/ruta/a/tu/ROM.smc"

# 3. Lanzar el juego en escritorio
build/YoshiIsland --no-launcher --rom "/ruta/a/tu/ROM.smc"
```

### Generación de APK para Android
```sh
python3 tools/build_android.py --rom "/ruta/a/tu/ROM.smc"
```
El instalador firmado generado se ubica en: `build/android-arm64/YoshiIsland_Widescreen_v1.1_Final.apk`.

---

## 4. Política de Licencias y Distribución Legal

- **SNESRecomp Runner & Host:** Licenciado bajo PolyForm Noncommercial 1.0.0.
- **Yoshi's Island Disassembly:** Sujeto a GPL-3.0.
- **Herramientas y Shaders Originales:** Licencia MIT.
- **Assets de Nintendo:** Este repositorio **no distribuye** ningún archivo ROM, gráficos propietarios ni pistas de audio del juego original. El usuario debe proporcionar su propia ROM legal.
