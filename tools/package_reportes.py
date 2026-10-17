#!/usr/bin/env python3
"""Update and repackage REPORTES_Y_CAPTURAS_YOSHI_ISLAND.zip with Plan Maestro v5 artifacts."""
import os
from pathlib import Path
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
downloads = Path('/Users/hectorx24/Downloads')
drive_root = Path('/Users/hectorx24/Library/CloudStorage/GoogleDrive-hectorx24@gmail.com/Mi unidad/Puente_Transferencia')
drive_final = drive_root / '00_YOSHI_ISLAND_FINAL'
zip_name = 'REPORTES_Y_CAPTURAS_YOSHI_ISLAND.zip'
existing_zip = downloads / zip_name

staging = downloads / 'reportes_capturas_staging'
if staging.exists():
    shutil.rmtree(staging)
staging.mkdir(parents=True)

print("1. Extracting existing captures and reports...", flush=True)
if existing_zip.exists():
    with zipfile.ZipFile(existing_zip, 'r') as z:
        z.extractall(staging)

print("2. Copying latest markdown reports and v5 specifications...", flush=True)
report_files = [
    'PLAN_MAESTRO_YOSHI_ISLAND_DEFINITIVO_V5.md',
    'PLAN_MAESTRO_YOSHI_ISLAND_DEFINITIVO_V4.md',
    'PROJECT_STATUS.md',
    'BUGS.md',
    'README.md',
    'RESUMEN_EJECUTIVO_CHATGPT.md'
]
for rf in report_files:
    src = root / rf
    if src.exists():
        shutil.copy2(src, staging / rf)

print("3. Copying QA Save Slot data and documentation...", flush=True)
qa_dest = staging / 'QA_SAVE_SLOT'
qa_dest.mkdir(exist_ok=True)
if (root / 'local/qa-saves/QA_SAVE_SLOT_README.md').exists():
    shutil.copy2(root / 'local/qa-saves/QA_SAVE_SLOT_README.md', qa_dest / 'QA_SAVE_SLOT_README.md')
if (root / 'local/qa-saves/qa_progression_slot2.srm').exists():
    shutil.copy2(root / 'local/qa-saves/qa_progression_slot2.srm', qa_dest / 'qa_progression_slot2.srm')

print("4. Copying restored launcher icon assets...", flush=True)
icon_dest = staging / 'CAPTURAS/icono_launcher_recuperado'
icon_dest.mkdir(parents=True, exist_ok=True)
if (root / 'android/res/drawable/ic_launcher.png').exists():
    shutil.copy2(root / 'android/res/drawable/ic_launcher.png', icon_dest / 'ic_launcher_fullart_512.png')
if (root / 'android/res/mipmap-xxxhdpi/ic_launcher.png').exists():
    shutil.copy2(root / 'android/res/mipmap-xxxhdpi/ic_launcher.png', icon_dest / 'ic_launcher_xxxhdpi_192.png')

print("5. Updating final LEEME_PRIMERO.txt...", flush=True)
leeme = staging / 'LEEME_PRIMERO.txt'
leeme_text = """PAQUETE DEFINITIVO DE REPORTES, CAPTURAS Y AUDITORÍA · YOSHI'S ISLAND (V5)
=================================================================================

Este paquete contiene la documentación completa, auditoría de regresión, capturas
A/B comparativas, especificaciones del Plan Maestro v5 y la partida de QA avanzada.

Contenido de la carpeta:
1. PLAN_MAESTRO_YOSHI_ISLAND_DEFINITIVO_V5.md - Especificación definitiva v5.
2. PROJECT_STATUS.md                          - Matriz técnica de los 54 niveles, 6 extras y 12 jefes.
3. BUGS.md                                    - Registro y causas de todos los defectos auditados y corregidos.
4. README.md                                  - Documentación con aviso legal formal y tabla comparativa.
5. QA_SAVE_SLOT/                              - Partida de QA verificada en Mundo 2-1 (Slot 2) e instrucciones.
6. CAPTURAS/                                  - Galerías clasificadas:
   - preset_island_cinema/ (Iluminación 2D-HD, CRT sutil, fuentes emisivas)
   - menu_intro_mapa/      (Widescreen sin bandas negras accidentales)
   - jefes_y_arenas/       (Arenas temáticas con mosaico nearest-neighbour)
   - niveles_extra/        (Niveles extra 1 a 6)
   - icono_launcher_recuperado/ (Arte completo original sin círculo blanco)
   - usuario_referencia_dispositivo/ (Capturas de referencia del teléfono)

Nota sobre la APK:
La APK final compilada y firmada se encuentra disponible directamente en:
- Downloads/YoshiIsland_Widescreen_v1.1_Final.apk
- Puente_Transferencia/00_YOSHI_ISLAND_FINAL/YoshiIsland_Widescreen_v1.1_Final.apk
"""
leeme.write_text(leeme_text, encoding='utf-8')

print("6. Creating updated zip...", flush=True)
temp_zip = downloads / 'reportes_temp.zip'
if temp_zip.exists():
    temp_zip.unlink()

with zipfile.ZipFile(temp_zip, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for base, dirs, files in os.walk(staging):
        for f in files:
            full = Path(base) / f
            rel = full.relative_to(staging)
            z.write(full, rel)

shutil.copy2(temp_zip, existing_zip)
if drive_final.exists():
    shutil.copy2(temp_zip, drive_final / zip_name)
if (drive_root / 'ZIPS').exists():
    shutil.copy2(temp_zip, drive_root / f'ZIPS/{zip_name}')

temp_zip.unlink()
shutil.rmtree(staging)

size_mb = existing_zip.stat().st_size / (1024 * 1024)
print(f"SUCCESS: {zip_name} updated successfully ({size_mb:.2f} MB)", flush=True)
