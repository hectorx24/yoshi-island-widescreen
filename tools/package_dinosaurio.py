#!/usr/bin/env python3
"""Package complete project snapshot into dinosaurio.zip for transfer bridge."""
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
drive_root = Path('/Users/hectorx24/Library/CloudStorage/GoogleDrive-hectorx24@gmail.com/Mi unidad/Puente_Transferencia')
target_zip = drive_root / 'dinosaurio.zip'
zips_zip = drive_root / 'ZIPS/dinosaurio.zip'
staging = Path('/Users/hectorx24/Downloads/dinosaurio_staging')

if staging.exists():
    shutil.rmtree(staging)
staging.mkdir(parents=True)

print("1. Extracting metadata and raw sessions from existing dinosaurio.zip...", flush=True)
if target_zip.exists():
    with zipfile.ZipFile(target_zip, 'r') as z:
        for item in z.infolist():
            if not item.filename.startswith('source_code/') and not item.filename.startswith('APK/'):
                z.extract(item, staging)

print("2. Packaging latest APKs...", flush=True)
apk_dir = staging / 'APK'
apk_dir.mkdir(exist_ok=True)
dev_apk = root / 'local/android/artifacts/yoshi-usa11-arm64-dev.apk'
shutil.copy2(dev_apk, apk_dir / 'yoshi-usa11-arm64-dev.apk')
shutil.copy2(dev_apk, apk_dir / 'YoshiIsland_Widescreen_v1.1_54Levels_Complete.apk')

print("3. Updating conversation transcripts...", flush=True)
ag_dir = staging / 'antigravity_session_raw'
ag_dir.mkdir(exist_ok=True)
log_dir = Path('/Users/hectorx24/.gemini/antigravity/brain/32738820-e93d-44ec-88fb-4d6a01be9c79/.system_generated/logs')
if (log_dir / 'transcript.jsonl').exists():
    shutil.copy2(log_dir / 'transcript.jsonl', ag_dir / 'transcript.jsonl')
if (log_dir / 'transcript_full.jsonl').exists():
    shutil.copy2(log_dir / 'transcript_full.jsonl', ag_dir / 'transcript_full.jsonl')

print("4. Copying master plan and root markdown files...", flush=True)
for doc_file in ['MASTER_PLAN_ANTIGRAVITY.md', 'PLAN_MAESTRO_YOSHI_ISLAND_DEFINITIVO_V4.md', 'PLAN_MAESTRO_YOSHI_ISLAND_DEFINITIVO_V5.md', 'PROJECT_STATUS.md', 'BUGS.md', 'ARCHITECTURAL_DESIGN_AND_TECHNICAL_SPEC.md', 'README.md']:
    if (root / doc_file).exists():
        shutil.copy2(root / doc_file, staging / doc_file)

print("5. Copying source_code...", flush=True)
source_dest = staging / 'source_code'
exclude_dirs = {'.git', 'build', 'venv', 'android-toolchain'}

def ignore_patterns(path, names):
    ignored = set()
    for name in names:
        if name in exclude_dirs:
            ignored.add(name)
        elif name.endswith('.tmp') or name.endswith('.sfc.bkp'):
            ignored.add(name)
    return ignored

shutil.copytree(root, source_dest, ignore=ignore_patterns)

print("6. Creating output zip...", flush=True)
out_zip_tmp = Path('/Users/hectorx24/Downloads/dinosaurio_updated.zip')
if out_zip_tmp.exists():
    out_zip_tmp.unlink()

with zipfile.ZipFile(out_zip_tmp, 'w', compression=zipfile.ZIP_DEFLATED) as z:
    for base, dirs, files in os.walk(staging):
        for f in files:
            full = Path(base) / f
            rel = full.relative_to(staging)
            z.write(full, rel)

print("7. Moving to destination in Puente_Transferencia and Downloads...", flush=True)
shutil.copy2(out_zip_tmp, target_zip)
shutil.copy2(out_zip_tmp, zips_zip)
downloads_zip = Path('/Users/hectorx24/Downloads/dinosaurio.zip')
shutil.copy2(out_zip_tmp, downloads_zip)
out_zip_tmp.unlink()
shutil.rmtree(staging)

size_mb = target_zip.stat().st_size / (1024 * 1024)
print(f"SUCCESS: dinosaurio.zip updated ({size_mb:.2f} MB)", flush=True)
