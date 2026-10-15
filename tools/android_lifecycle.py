#!/usr/bin/env python3
"""Verify real Android background SRAM flush and reload after process kill.

No game memory or save payload is edited. Private bytes remain under local/.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import time

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--serial', default='emulator-5554')
a = p.parse_args()
sdk = Path(os.environ.get('ANDROID_HOME', '/Users/hectorx24/Downloads/patreon/tooling/sdk'))
adb = [str(sdk / 'platform-tools/adb'), '-s', a.serial]
out = root / 'local/android/lifecycle'
out.mkdir(parents=True, exist_ok=True)
package = 'com.hector.yoshi'
save = '/sdcard/Android/data/com.hector.yoshi/files/saves/save.srm'

def call(*words, check=True):
    return subprocess.run(adb + list(words), capture_output=True, check=check, timeout=30)

def diag():
    return call('exec-out', 'run-as', package, 'cat',
                'files/save-diagnostic.txt', check=False).stdout.decode(errors='replace')

def wait_lines(event, count):
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        text = diag()
        lines = re.findall(rf'^{event} status=ok bytes=(\d+) fnv=([0-9a-f]+)$',
                           text, re.MULTILINE)
        if len(lines) >= count:
            return lines[-1]
        time.sleep(.25)
    (out / 'diagnostic.txt').write_text(diag())
    (out / 'logcat.txt').write_bytes(call('logcat', '-d').stdout)
    raise SystemExit(f'No {event} checkpoint; inspect {out}')

def fnv(data):
    value = 2166136261
    for byte in data:
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return f'{value:08x}'

def launch():
    call('shell', 'am', 'start', '-W', '-n', package + '/.YoshiActivity',
         '--ez', 'save_diag', 'true')

call('install', '-r', str(root / 'local/android/artifacts/yoshi-usa11-arm64-dev.apk'))
call('shell', 'am', 'force-stop', package)
call('shell', 'run-as', package, 'rm', '-f', 'files/save-diagnostic.txt')
initial = call('exec-out', 'cat', save).stdout
if len(initial) != 32768:
    raise SystemExit('First create an ordinary save by running the Android smoke test')
(out / 'before.srm').write_bytes(initial)
launch()
assert wait_lines('loaded', 1) == ('32768', fnv(initial)), 'Initial SRAM load differs'
time.sleep(2)
call('shell', 'input', 'keyevent', 'KEYCODE_HOME')
size, digest = wait_lines('background', 1)
saved = call('exec-out', 'cat', save).stdout
assert len(saved) == int(size) == 32768 and fnv(saved) == digest, 'Background save differs'
assert call('shell', 'test', '-e', save + '.tmp', check=False).returncode != 0
(out / 'background.srm').write_bytes(saved)
print('Background event: atomic 32768-byte SRAM flush verified', flush=True)
call('shell', 'am', 'force-stop', package)
launch()
assert wait_lines('loaded', 2) == ('32768', digest), 'Reload after process kill differs'
assert call('exec-out', 'cat', save).stdout == saved
call('shell', 'input', 'keyevent', 'KEYCODE_HOME')
wait_lines('background', 2)
(out / 'diagnostic.txt').write_text(diag())
print('Process kill/relaunch: exact saved bytes loaded; no physical-device claim', flush=True)
subprocess.run(['python3', str(root / 'tools/checkpoint.py'), '--event',
    'Android background atomic SRAM flush and exact reload after force-stop passed'], check=True)
