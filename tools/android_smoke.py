"""Install private dev APK on the isolated emulator and collect boot regression."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--serial',default='emulator-5554')
args=p.parse_args()
sdk=Path(os.environ.get('ANDROID_HOME','/Users/hectorx24/Downloads/patreon/tooling/sdk'))
adb=[str(sdk/'platform-tools/adb'),'-s',args.serial]
out=root/'local/android/smoke';out.mkdir(parents=True,exist_ok=True)
def call(*words,check=True):
    return subprocess.run(adb+list(words),stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=check,timeout=30)
deadline=time.monotonic()+120
while time.monotonic()<deadline:
    if call('shell','getprop','sys.boot_completed',check=False).stdout.strip()==b'1':break
    time.sleep(1)
else:raise SystemExit('Emulator did not boot within 120 seconds')
apk=root/'local/android/artifacts/yoshi-usa11-arm64-dev.apk'
print(call('install','-r',str(apk)).stdout.decode(),flush=True)
call('shell','am','force-stop','com.hector.yoshi')
capture_names=['boot600','intro1200','after_start']
call('shell','run-as','com.hector.yoshi','rm','-f',
     *[f'files/captures/{tag}.fb.bgrx' for tag in capture_names],
     'files/captures/adaptive-width.txt',check=False)
call('logcat','-c')
print(call('shell','am','start','-W','-n','com.hector.yoshi/.YoshiActivity','--ez','regression','true').stdout.decode(),flush=True)
expected=json.loads((root/'tests/boot_frame_hashes.json').read_text())['captures']
deadline=time.monotonic()+90
while time.monotonic()<deadline:
    result=call('shell','run-as','com.hector.yoshi','ls','files/captures/after_start.fb.bgrx',check=False)
    if result.returncode==0:break
    time.sleep(1)
else:
    (out/'logcat.txt').write_bytes(call('logcat','-d').stdout)
    raise SystemExit('Android boot regression did not complete; inspect local/android/smoke/logcat.txt')
(out/'logcat.txt').write_bytes(call('logcat','-d').stdout)
for tag,digest in expected.items():
    data=call('exec-out','run-as','com.hector.yoshi','cat',f'files/captures/{tag}.fb.bgrx').stdout
    (out/(tag+'.fb.bgrx')).write_bytes(data)
    if hashlib.sha256(data).hexdigest()!=digest:raise SystemExit('Android framebuffer differs at '+tag)
    print(tag+': exact desktop framebuffer match',flush=True)

# Exercise the unresolved-scene safety fallback through actual tutorial
# gameplay. Until its extra tiles have verified provenance, the adaptive path
# must retain the authentic 256px field and match the same-run capture.
call('shell','am','force-stop','com.hector.yoshi')
call('shell','run-as','com.hector.yoshi','rm','-f',
     *[f'files/captures/{tag}.fb.bgrx' for tag in capture_names],
     'files/captures/after_move.fb.bgrx','files/captures/adaptive-width.txt',
     'files/captures/wide-after_move.bgrx',check=False)
call('logcat','-c')
call('shell','am','start','-W','-n','com.hector.yoshi/.YoshiActivity',
     '--ez','regression','true','--ez','adaptive','true')
deadline=time.monotonic()+90
while time.monotonic()<deadline:
    result=call('shell','run-as','com.hector.yoshi','ls','files/captures/after_move.fb.bgrx',check=False)
    if result.returncode==0:break
    time.sleep(1)
else:
    (out/'adaptive-logcat.txt').write_bytes(call('logcat','-d').stdout)
    raise SystemExit('Adaptive Android tutorial capture did not complete')
center=call('exec-out','run-as','com.hector.yoshi','cat','files/captures/after_move.fb.bgrx').stdout
wide=call('exec-out','run-as','com.hector.yoshi','cat','files/captures/wide-after_move.bgrx').stdout
if len(center)!=256*224*4:
    raise SystemExit(f'Unexpected native tutorial framebuffer size: {len(center)} bytes')
if len(wide)%(224*4):
    raise SystemExit(f'Adaptive full-field capture has invalid size: {len(wide)} bytes')
wide_width=len(wide)//(224*4)
if wide_width != 256:
    raise SystemExit(f'Unqualified tutorial scene did not use safe native framing: {wide_width}px')
if center!=wide:
    raise SystemExit('Safe-framed tutorial field differs from its native capture')
(out/'adaptive-after_move.fb.bgrx').write_bytes(wide)
log=call('logcat','-d').stdout
(out/'adaptive-logcat.txt').write_bytes(log)
diag=call('exec-out','run-as','com.hector.yoshi','cat','files/captures/adaptive-width.txt').stdout
(out/'adaptive-width.txt').write_bytes(diag)
import re
matches=re.findall(rb'drawable=(\d+)x(\d+) frame=(\d+)x(\d+)',diag)
if not matches or int(matches[-1][2])!=256:
    raise SystemExit('Android adaptive fallback did not select the native field; inspect adaptive-width.txt')
if int(matches[-1][2])!=wide_width:
    raise SystemExit(f'Diagnostic frame width {matches[-1][2].decode()} disagrees with captured width {wide_width}')
print(f"adaptive tutorial PPU field: {wide_width}x224; unresolved scene safely retains native framing",flush=True)
subprocess.run(['python3',str(root/'tools/checkpoint.py'),'--event','Android boot hashes match desktop; unqualified tutorial safely retains native 256px field'],check=True)
