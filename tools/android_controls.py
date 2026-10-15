#!/usr/bin/env python3
"""Exercise actual Android touch -> SDL keys and SDL virtual-controller menus."""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import time

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--serial', default='emulator-5554')
a = p.parse_args()
sdk = Path(os.environ.get('ANDROID_HOME', '/Users/hectorx24/Downloads/patreon/tooling/sdk'))
adb = [str(sdk / 'platform-tools/adb'), '-s', a.serial]
package = 'com.hector.yoshi'
out = root / 'local/android/controls'
out.mkdir(parents=True, exist_ok=True)

def call(*words, check=True):
    return subprocess.run(adb + list(words), capture_output=True, check=check, timeout=30)

def private(name):
    return call('exec-out', 'run-as', package, 'cat', 'files/' + name, check=False).stdout.decode(errors='replace')

def wait_text(name, expected):
    deadline = time.monotonic() + 20
    while time.monotonic() < deadline:
        text = private(name)
        if expected in text:
            return text
        time.sleep(.25)
    (out / name).write_text(private(name))
    raise SystemExit(f'Missing {expected!r}; inspect {out / name}')

call('install', '-r', str(root / 'local/android/artifacts/yoshi-usa11-arm64-dev.apk'))
call('shell', 'am', 'force-stop', package)
call('shell', 'run-as', package, 'rm', '-f', 'files/input-diagnostic.txt', 'files/controller-test.txt')
call('shell', 'am', 'start', '-W', '-n', package + '/.YoshiActivity', '--ez', 'input_diag', 'true')
wait_text('input-diagnostic.txt', 'ready')
png = call('exec-out', 'screencap', '-p').stdout
(out / 'touch-ui.png').write_bytes(png)
w, h = struct.unpack_from('>II', png, 16)
s = min(h / 448, w / 800)
tests = [
    ('Start', w/2 + 42*s, h - 25*s, 13),
    ('Right', 110*s, h - 80*s, 0x4000004f),
    ('B', w - 84*s, h - 54*s, ord('z')),
    ('A', w - 38*s, h - 100*s, ord('x')),
    ('Y', w - 130*s, h - 100*s, ord('a')),
    ('X', w - 84*s, h - 146*s, ord('s')),
    ('L', 40*s, 36*s, ord('c')),
    ('R', w - 90*s, 36*s, ord('v')),
    ('Select', w/2 - 42*s, h - 25*s, 0x400000e5),
]
for label, x, y, key in tests:
    call('shell', 'input', 'swipe', str(round(x)), str(round(y)),
         str(round(x)), str(round(y)), '120')
    wait_text('input-diagnostic.txt', f'down key={key:08x}')
    wait_text('input-diagnostic.txt', f'up key={key:08x}')
    print(f'Touch {label}: native SDL press and release verified', flush=True)
(out / 'input-diagnostic.txt').write_text(private('input-diagnostic.txt'))

call('shell', 'am', 'force-stop', package)
call('shell', 'am', 'start', '-W', '-n', package + '/.YoshiActivity', '--ez', 'controller_test', 'true')
text = wait_text('controller-test.txt', 'ok: browser and rewind both opened and closed from the pad')
if '[overlay_selftest_pad] FAIL' in text:
    raise SystemExit('Virtual controller test reported failure')
(out / 'controller-test.txt').write_text(text)
call('shell', 'input', 'keyevent', 'KEYCODE_HOME')
print('Android SDL virtual controller: save-state browser and rewind navigation passed', flush=True)
subprocess.run(['python3', str(root / 'tools/checkpoint.py'), '--event',
    'Android touch-to-SDL key presses/releases and virtual controller overlay navigation passed'], check=True)
