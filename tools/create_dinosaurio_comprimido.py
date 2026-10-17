#!/usr/bin/env python3
"""Build ultra-lightweight dinosaurio_comprimido.zip for ChatGPT / other AI analysis."""
import os
from pathlib import Path
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
drive_root = Path('/Users/hectorx24/Library/CloudStorage/GoogleDrive-hectorx24@gmail.com/Mi unidad/Puente_Transferencia')
source_dinosaurio = drive_root / 'dinosaurio.zip'
target_zip = drive_root / 'dinosaurio_comprimido.zip'
target_zips_zip = drive_root / 'ZIPS/dinosaurio_comprimido.zip'
staging = Path('/Users/hectorx24/Downloads/dinosaurio_comprimido_staging')

if staging.exists():
    shutil.rmtree(staging)
staging.mkdir(parents=True)

print("1. Extracting handoff documentation from dinosaurio.zip...", flush=True)
doc_names = [
    '00_LEEME_PRIMERO_PARA_IA.md',
    '01_HISTORIAL_Y_LO_QUE_HIZO_CODEX.md',
    '02_LO_QUE_SE_HIZO_EN_ESTA_SESION_54_NIVELES.md',
    '03_LO_QUE_FALTA_Y_ROADMAP_PARA_SIGUIENTE_IA.md',
    '04_TRANSCRIPCIONES_Y_CONVERSACIONES_COMPLETAS.md',
    'ARCHITECTURAL_DESIGN_AND_TECHNICAL_SPEC.md'
]
if source_dinosaurio.exists():
    with zipfile.ZipFile(source_dinosaurio, 'r') as z:
        for name in doc_names:
            try:
                z.extract(name, staging)
            except KeyError:
                pass

print("2. Copying executive summaries and master plans...", flush=True)
for fname in ['RESUMEN_EJECUTIVO_CHATGPT.md', 'MASTER_PLAN_ANTIGRAVITY.md', 'PLAN_MAESTRO_YOSHI_ISLAND_DEFINITIVO_V4.md', 'PLAN_MAESTRO_YOSHI_ISLAND_DEFINITIVO_V5.md', 'PROJECT_STATUS.md', 'BUGS.md', 'README.md']:
    p = root / fname
    if p.exists():
        shutil.copy2(p, staging / fname)

print("3. Copying essential source code...", flush=True)
code_dir = staging / 'codigo_fuente'
code_dir.mkdir(parents=True)

# Copy src, android, tools, docs, cmake, CMakeLists.txt, tests
for folder in ['src', 'android', 'tools', 'docs', 'cmake', 'tests']:
    src_path = root / folder
    if src_path.exists():
        shutil.copytree(src_path, code_dir / folder,
                        ignore=shutil.ignore_patterns('*.tmp', '*.pyc', '__pycache__', '*.sfc.bkp', '*.apk'))

if (root / 'CMakeLists.txt').exists():
    shutil.copy2(root / 'CMakeLists.txt', code_dir / 'CMakeLists.txt')

print("4. Copying curated PNG screenshots...", flush=True)
caps_dir = staging / 'capturas_clave'
caps_dir.mkdir(parents=True)

key_screenshots = [
    root / 'local/master-plan/01_fullscreen/phone-fullscreen_2772x1272.png',
    root / 'local/master-plan/01_fullscreen/touch-ui.png',
    root / 'local/master-plan/01_fullscreen/fullscreen_420x224_after_move.png',
    root / 'local/master-plan/03_bosses/1-4_burt/burt_themed_fullscreen_2772x1272.png',
    root / 'local/master-plan/03_bosses/1-4_burt/boss_battle_native.png',
    root / 'local/master-plan/05_presentation/sharp.png',
    root / 'local/master-plan/05_presentation/soft.png',
    root / 'local/master-plan/05_presentation/lighting.png'
]

for s in key_screenshots:
    if s.exists():
        shutil.copy2(s, caps_dir / s.name)

# Also extract 5 sample widescreen levels from dinosaurio.zip
sample_levels = [
    'capturas_muestra_widescreen/extra_1_wide_9340.png',
    'capturas_muestra_widescreen/extra_6_wide_9340.png',
    'capturas_muestra_widescreen/level1_2_action_wide_fixed.png',
    'capturas_muestra_widescreen/level3_1_fixed_wide.png',
    'capturas_muestra_widescreen/level6_8_wide_9340.png'
]
if source_dinosaurio.exists():
    with zipfile.ZipFile(source_dinosaurio, 'r') as z:
        for item in sample_levels:
            try:
                data = z.read(item)
                (caps_dir / Path(item).name).write_bytes(data)
            except KeyError:
                pass

print("5. Copying phase review reports...", flush=True)
reviews_dir = staging / 'informes_fases'
reviews_dir.mkdir(parents=True)

for review_path in (root / 'local/master-plan').glob('*/review.md'):
    phase_name = review_path.parent.name
    dest = reviews_dir / f'{phase_name}_review.md'
    shutil.copy2(review_path, dest)

print("6. Creating compressed zip archive...", flush=True)
tmp_zip = Path('/Users/hectorx24/Downloads/dinosaurio_comprimido_temp.zip')
if tmp_zip.exists():
    tmp_zip.unlink()

with zipfile.ZipFile(tmp_zip, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for base, dirs, files in os.walk(staging):
        for f in files:
            full = Path(base) / f
            rel = full.relative_to(staging)
            z.write(full, rel)

size_mb = tmp_zip.stat().st_size / (1024 * 1024)
print(f"Compressed zip created: {size_mb:.2f} MB", flush=True)

print("7. Moving to Puente_Transferencia and Downloads...", flush=True)
shutil.copy2(tmp_zip, target_zip)
shutil.copy2(tmp_zip, target_zips_zip)
downloads_zip = Path('/Users/hectorx24/Downloads/dinosaurio_comprimido.zip')
shutil.copy2(tmp_zip, downloads_zip)
tmp_zip.unlink()
shutil.rmtree(staging)

print(f"SUCCESS: dinosaurio_comprimido.zip placed in Puente_Transferencia and Downloads ({size_mb:.2f} MB)", flush=True)
