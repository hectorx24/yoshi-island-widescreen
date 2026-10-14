"""Run the actual desktop game and verify baseline boot framebuffer hashes.

This gate detects native-promotion regressions; baseline is not an oracle of
hardware accuracy. Strict CPU/SRAM equality and reference fidelity are separate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

def main():
    root=Path(__file__).resolve().parents[1]
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom',required=True,type=Path)
    p.add_argument('--executable',type=Path,default=root/'build/YoshiIsland')
    p.add_argument('--output',type=Path,default=root/'local/regression-boot')
    args=p.parse_args()
    expected=json.loads((root/'tests/boot_frame_hashes.json').read_text())
    raw=args.rom.read_bytes()
    if len(raw)%32768==512: raw=raw[512:]
    if hashlib.sha256(raw).hexdigest()!=expected['rom_sha256']:
        raise SystemExit('Unexpected ROM identity')
    output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=True)
    for tag in expected['captures']:
        (output/(tag+'.fb.bgrx')).unlink(missing_ok=True)
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',
             SNESRECOMP_DUMP_DIR=str(output))
    with (output/'run.log').open('w') as log:
        subprocess.run([str(args.executable.resolve()),'--no-launcher','--rom',str(args.rom.resolve()),
                        '--script',str(root/'tests/boot.script')],env=env,
                       stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
    for tag,digest in expected['captures'].items():
        actual=hashlib.sha256((output/(tag+'.fb.bgrx')).read_bytes()).hexdigest()
        if actual!=digest: raise SystemExit('Framebuffer regression: '+tag)
        print(tag+': framebuffer matches baseline')
    return 0

if __name__=='__main__': raise SystemExit(main())
