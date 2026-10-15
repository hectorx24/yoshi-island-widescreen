#!/usr/bin/env python3
"""Verify Sharp/Soft/lighting through GPU readback of ONE gameplay field."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--serial', default='emulator-5554')
a = p.parse_args()
sdk = Path(os.environ.get('ANDROID_HOME', '/Users/hectorx24/Downloads/patreon/tooling/sdk'))
adb = [str(sdk / 'platform-tools/adb'), '-s', a.serial]
package = 'com.hector.yoshi'
out = root / 'local/android/presentation'
out.mkdir(parents=True, exist_ok=True)
names = ['sharp', 'soft', 'lighting']

def call(*words, check=True):
    return subprocess.run(adb + list(words), capture_output=True, check=check, timeout=30)

call('install', '-r', str(root / 'local/android/artifacts/yoshi-usa11-arm64-dev.apk'))
call('shell', 'am', 'force-stop', package)
call('shell', 'run-as', package, 'rm', '-f',
     *['files/presentation.bgrx.' + name + suffix for name in names for suffix in ['', '.json']])
call('shell', 'am', 'start', '-W', '-n', package + '/.YoshiActivity',
     '--ez', 'regression', 'true', '--ez', 'adaptive', 'true', '--ez', 'display_test', 'true')
deadline = time.monotonic() + 90
while time.monotonic() < deadline:
    if call('shell', 'run-as', package, 'test', '-s',
            'files/presentation.bgrx.lighting.json', check=False).returncode == 0:
        break
    time.sleep(1)
else:
    (out / 'logcat.txt').write_bytes(call('logcat', '-d').stdout)
    raise SystemExit('GPU readback did not complete; inspect local/android/presentation')

metadata, frames = {}, {}
for name in names:
    remote = 'files/presentation.bgrx.' + name
    data = call('exec-out', 'run-as', package, 'cat', remote).stdout
    meta = json.loads(call('exec-out', 'run-as', package, 'cat', remote + '.json').stdout)
    assert len(data) == meta['width'] * meta['height'] * 4
    (out / (name + '.bgrx')).write_bytes(data)
    (out / (name + '.json')).write_text(json.dumps(meta, indent=2) + '\n')
    frames[name], metadata[name] = data, meta
proof = [(m['frame'], m['source_fnv'], m['source_width'], m['source_height'])
         for m in metadata.values()]
assert len(set(proof)) == 1, 'Presentation test advanced the source field'
assert [metadata[n]['applied'] for n in names] == [0, 1, 2], 'GPU mode fell back or lighting unavailable'
assert proof[0][2] > 256, 'GPU test did not reach adaptive gameplay'
base = frames['sharp']
stats = {}
for name in ['soft', 'lighting']:
    delta = sum(abs(base[i] - frames[name][i]) for i in range(len(base)) if i % 4 != 3)
    mean = delta / (len(base) / 4 * 3)
    changed = sum(base[i:i + 3] != frames[name][i:i + 3] for i in range(0, len(base), 4))
    assert changed > 100 and 0 < mean < (20 if name == 'soft' else 8), (name, mean, changed)
    stats[name] = {'mean_rgb_delta': mean, 'changed_pixels': changed}
    print(f'{name}: GPU applied, mean RGB delta {mean:.3f}, changed pixels {changed}', flush=True)
(out / 'verified.json').write_text(json.dumps({'same_source': proof[0], 'stats': stats}, indent=2) + '\n')
call('shell', 'input', 'keyevent', 'KEYCODE_HOME')
print(f'All modes used the same native field {proof[0][0]} ({proof[0][2]}x224)', flush=True)
subprocess.run(['python3', str(root / 'tools/checkpoint.py'), '--event',
    'Android GPU Sharp/Authentic Soft/lighting readbacks verified on one identical gameplay field'], check=True)
