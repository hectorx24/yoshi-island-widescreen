#!/usr/bin/env python3
"""Verify actual C asset decoding against owner-ROM/captured BG2 planes."""
import argparse
import ctypes
import hashlib
from pathlib import Path
import subprocess
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',required=True,type=Path)
p.add_argument('captures',nargs='+',type=Path)
a=p.parse_args()
raw=a.rom.read_bytes()
if len(raw)%32768==512:raw=raw[512:]
assert hashlib.sha256(raw).hexdigest()=='bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06'
subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-Isrc',
    'src/yoshi_scene_map.c','-o','local/test_yoshi_scene_map.dylib'],check=True)
f=ctypes.CDLL('local/test_yoshi_scene_map.dylib').yoshi_bg2_asset
f.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_uint,ctypes.c_void_p];f.restype=ctypes.c_bool
rom=ctypes.create_string_buffer(raw);out=ctypes.create_string_buffer(4096)
for header in range(32):assert f(rom,len(raw),header,out),f'Invalid original descriptor{header}'
print('All32 original BG2 descriptors decode to complete4096-byte planes')
for prefix in a.captures:
 w=Path(str(prefix)+'.wram.bin').read_bytes();v=Path(str(prefix)+'.vram.bin').read_bytes()
 header=int.from_bytes(w[0x13a:0x13c],'little')
 assert f(rom,len(raw),header,out)
 assert out.raw==v[0x7000:0x8000],f'{prefix}: original BG2 differs from captured plane'
 print(f'{prefix}: header{header} original ROM plane4096/4096 bytes exact')
