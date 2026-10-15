#!/usr/bin/env python3
"""Compare the actual C world-map resolver against captured native VRAM.

Private owner ROM/state files are inputs only; no asset fixtures are tracked.
Usage: python3 tools/verify_bg1_map.py --rom ROM CAPTURE_PREFIX [PREFIX ...]
"""
import argparse
import ctypes
from pathlib import Path
import struct
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--rom', required=True, type=Path)
parser.add_argument('captures', nargs='+', type=Path)
args = parser.parse_args()
library = root / 'local/test_yoshi_bg1_map.dylib'
subprocess.run(['clang', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared',
                '-fPIC', '-Isrc', 'src/yoshi_bg1_map.c', '-o', str(library)],
               cwd=root, check=True)
resolve = ctypes.CDLL(str(library)).yoshi_bg1_tile
resolve.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
                    ctypes.c_size_t, ctypes.c_uint, ctypes.c_uint,
                    ctypes.POINTER(ctypes.c_uint16)]
resolve.restype = ctypes.c_bool
rom = ctypes.create_string_buffer(args.rom.read_bytes())
failed = 0
for prefix in args.captures:
    wram = Path(str(prefix) + '.wram.bin').read_bytes()
    gsu = ctypes.create_string_buffer(Path(str(prefix) + '.sram.bin').read_bytes())
    vram = Path(str(prefix) + '.vram.bin').read_bytes()
    memory = ctypes.create_string_buffer(wram)
    x, y = struct.unpack_from('<HH', wram, 0x39)
    good = bad = 0
    for ty in range(y // 8, (y + 224) // 8):
        for tx in range(x // 8, (x + 256) // 8):
            # Independent PPU 64x32 ring addressing, observed BG1 base $6800.
            address = 0x6800 + (tx % 32) + (ty % 32) * 32
            if tx % 64 >= 32:
                address += 0x400
            actual = struct.unpack_from('<H', vram, address * 2)[0]
            expected = ctypes.c_uint16()
            valid = resolve(memory, gsu, rom, len(rom) - 1, tx, ty,
                            ctypes.byref(expected))
            if valid and actual == expected.value:
                good += 1
            else:
                bad += 1
    print(f'{prefix.name}: {good} native tiles exact; {bad} different')
    failed += bad
raise SystemExit(bool(failed))
