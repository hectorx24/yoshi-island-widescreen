#!/usr/bin/env python3
"""Check isolated margin pixels against that world span in a native capture.

Uses frame-start PPU replay writes, not end-of-frame register snapshots.
Rejects scroll-changing raster bands; those require a per-band oracle.
The outer 8 scanlines are excluded from both captures (native crop borders).
"""
import argparse
import json
import struct
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--layer', type=int, choices=range(1, 5), required=True)
p.add_argument('--side', choices=['left', 'right'], required=True)
p.add_argument('--wide', type=Path, required=True)
p.add_argument('--wide-state', type=Path, required=True)
p.add_argument('--native-state', type=Path, required=True)
p.add_argument('--check-source-colors', action='store_true',
               help='Separately verify pre-color-math source colors for RGB differences')
a = p.parse_args()

def phase(prefix):
    addresses = [0x210d + (a.layer - 1) * 2, 0x210e + (a.layer - 1) * 2]
    values = {k: [] for k in addresses}
    for row in Path(str(prefix) + '.ppuw.tsv').read_text().splitlines():
        if row.startswith('#'):
            continue
        _, line, adr, value, source = row.split('\t')
        adr = int(adr, 16)
        if adr not in values:
            continue
        if int(line) > 0:
            raise SystemExit('Raster scroll changed; use a band-specific comparison')
        if source == 'replay' and len(values[adr]) < 2:
            values[adr].append(int(value, 16))
    return tuple((values[k][0] | values[k][1] << 8) & 1023 for k in addresses)

wide = a.wide.read_bytes()
native = Path(str(a.native_state) + '.fb.bgrx').read_bytes()
width = len(wide) // (224 * 4)
assert len(native) == 256 * 224 * 4 and width > 256 and width % 2 == 0
extra = (width - 256) // 2
wp, np = phase(a.wide_state), phase(a.native_state)
dx, dy = ((wp[i] - np[i] + 512) % 1024 - 512 for i in (0, 1))
good = bad = 0
source_good = 0
def source_color(prefix, phase_xy, sx, sy):
    if prefix not in source_color.cache:
        regs = json.loads(Path(str(prefix) + '.regs.json').read_text())
        source_color.cache[prefix] = (regs,
            Path(str(prefix) + '.vram.bin').read_bytes(),
            Path(str(prefix) + '.cgram.bin').read_bytes())
    regs, vr, cg = source_color.cache[prefix]
    layer = a.layer - 1
    big = bool(regs['bgmode'] & (0x10 << layer))
    shift = 4 if big else 3
    sc = regs['bgsc'][layer]
    x, y = (sx + phase_xy[0]) & 1023, (sy + 1 + phase_xy[1]) & 1023
    col, row = x >> shift, y >> shift
    address = ((sc & 0xfc) << 8) + (col & 31) + (row & 31) * 32
    if sc & 1 and col & 32:
        address += 0x400
    if sc & 2 and row & 32:
        address += 0x800 if sc & 1 else 0x400
    entry = struct.unpack_from('<H', vr, (address & 32767) * 2)[0]
    px, py = x & ((1 << shift) - 1), y & ((1 << shift) - 1)
    if entry & 0x4000:
        px = (1 << shift) - 1 - px
    if entry & 0x8000:
        py = (1 << shift) - 1 - py
    tile = ((entry & 1023) + (px >> 3) + (py >> 3) * 16) & 1023
    # This oracle currently covers Mode 1's BG1/BG2 4bpp planes.
    assert regs['bgmode'] & 7 == 1 and layer < 2
    char = ((regs['bg_tile_adr'] >> (layer * 4)) & 15) << 12
    address = (char + tile * 16 + (py & 7)) & 32767
    lo = struct.unpack_from('<H', vr, address * 2)[0]
    hi = struct.unpack_from('<H', vr, ((address + 8) & 32767) * 2)[0]
    bit = 7 - (px & 7)
    color = ((lo >> bit) & 1) | (((lo >> (8 + bit)) & 1) << 1)
    color |= ((hi >> bit) & 1) << 2 | ((hi >> (8 + bit)) & 1) << 3
    return None if not color else struct.unpack_from('<H', cg,
        (((entry >> 10) & 7) * 16 + color) * 2)[0]
source_color.cache = {}
xs = range(-extra, 0) if a.side == 'left' else range(256, 256 + extra)
for y in range(8, 216):
    ny = y + dy
    if not 8 <= ny < 216:
        continue
    for x in xs:
        nx = x + dx
        if not 0 <= nx < 256:
            continue
        i, j = (y * width + x + extra) * 4, (ny * 256 + nx) * 4
        if wide[i:i + 4] == native[j:j + 4]:
            good += 1
        else:
            bad += 1
            if a.check_source_colors and source_color(a.wide_state, wp, x, y) == \
                    source_color(a.native_state, np, nx, ny):
                source_good += 1
print(f'BG{a.layer} {a.side}: {good} same-world pixels exact; {bad} different; '
      f'PPU phases {wp} -> {np}')
if a.check_source_colors:
    print(f'RGB differences with identical original pre-color-math pixels: '
          f'{source_good}/{bad}')
raise SystemExit(bool(bad != source_good or not good))
