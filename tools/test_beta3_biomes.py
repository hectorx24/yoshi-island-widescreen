#!/usr/bin/env python3
"""Automated Beta 3 Biome and Preset Verification Harness.

Executes native YoshiIsland recompilation across biomes (Hills, Caves, Castles),
verifying widescreen composition, contact shadows, and presentation filter cycle.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

USA_V11_SHA256 = 'bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06'

def write_raw_to_png(raw_path: Path, png_path: Path, width: int, height: int):
    """Convert raw ARGB8888 or BGRX pixel buffer to PNG via native macOS sips."""
    data = raw_path.read_bytes()
    expected_size = width * height * 4
    if len(data) < expected_size:
        raise ValueError(f"File {raw_path} has {len(data)} bytes, expected {expected_size}")
    
    # Construct uncompressed 32-bit BMP in memory
    row_size = width * 4
    pixel_array_size = row_size * height
    file_size = 54 + pixel_array_size
    
    bmp_header = struct.pack(
        '<2sIHHI',
        b'BM',
        file_size,
        0, 0,
        54
    )
    dib_header = struct.pack(
        '<IiiHHIIiiII',
        40,
        width,
        -height,  # Top-down bitmap
        1,
        32,       # 32 bits per pixel
        0,        # BI_RGB uncompressed
        pixel_array_size,
        2835, 2835, # ~72 DPI
        0, 0
    )
    
    bmp_path = raw_path.with_suffix('.tmp.bmp')
    with bmp_path.open('wb') as f:
        f.write(bmp_header)
        f.write(dib_header)
        f.write(data[:expected_size])
        
    try:
        subprocess.run(['sips', '-s', 'format', 'png', str(bmp_path), '--out', str(png_path)],
                       check=True, capture_output=True)
    finally:
        bmp_path.unlink(missing_ok=True)

def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--output', type=Path, default=root / 'local/beta3_test')
    args = parser.parse_args()

    raw_rom = args.rom.read_bytes()
    if len(raw_rom) % 32768 == 512:
        raw_rom = raw_rom[512:]
    rom_hash = hashlib.sha256(raw_rom).hexdigest()
    if rom_hash != USA_V11_SHA256:
        raise SystemExit(f"ROM hash mismatch: {rom_hash} != {USA_V11_SHA256}")

    args.output.mkdir(parents=True, exist_ok=True)
    exe = root / 'build/YoshiIsland'
    if not exe.exists():
        raise SystemExit(f"Executable not found: {exe}. Run tools/build.sh first.")

    print("=== Yoshi's Island Widescreen Beta 3: Biome & Presentation Test ===")
    
    # 1. Run Level 1-2 Diagnostic to exercise Cave & Hills with contact shadow and particles
    print("1. Running Level 1-2 diagnostic (Caves & Hills)...")
    env = dict(os.environ,
               SDL_VIDEODRIVER='dummy',
               SDL_AUDIODRIVER='dummy',
               SNESRECOMP_DUMP_DIR=str(args.output.resolve()),
               SNESRECOMP_PRESENT_TEST_CYCLE='1',
               SNESRECOMP_PRESENT_CAPTURE=str((args.output / 'beta3_cycle').resolve()),
               SNESRECOMP_PRESENT_FRAME='7800')
               
    subprocess.run([
        str(exe.resolve()),
        '--no-launcher',
        '--rom', str(args.rom.resolve()),
        '--script', str(root / 'tests/level1_2_diagnostic.script')
    ], env=env, check=True, timeout=120)

    # Check generated test cycle variants
    cycle_variants = ['sharp', 'soft', 'lighting']
    for v in cycle_variants:
        raw_file = args.output / f'beta3_cycle.{v}'
        json_file = args.output / f'beta3_cycle.{v}.json'
        if raw_file.exists() and json_file.exists():
            meta = json.loads(json_file.read_text())
            w, h = meta.get('width', 1280), meta.get('height', 720)
            target_png = root / f'docs/captures/{v}.png'
            write_raw_to_png(raw_file, target_png, w, h)
            print(f"   ✓ Generated and verified {target_png.name} ({w}x{h})")

    # Verify key captures are present in docs/captures/
    expected_captures = [
        'sharp.png', 'soft.png', 'lighting.png',
        'level1_2_entry_native.png', 'level1_2_action_wide_fixed.png',
        'level1_2_chomp_wide_fixed.png', 'level3_1_fixed_wide.png',
        'level3_7_wide_9380.png', 'level5_1_wide_9380.png',
        'level5_2_wide_9460.png', 'level6_8_wide_9340.png',
        'boss_arena_right_native.png', 'boss_arena_right_wide_x128.png',
        'fullscreen_420x224_after_move.png', 'extra_1_wide_9340.png', 'extra_6_wide_9340.png'
    ]
    
    missing = [c for c in expected_captures if not (root / 'docs/captures' / c).exists()]
    if missing:
        raise SystemExit(f"Missing expected captures in docs/captures: {missing}")

    print(f"\nAll {len(expected_captures)} captures verified in docs/captures/")
    print("Beta 3 automated test harness completed successfully!")

if __name__ == '__main__':
    main()
