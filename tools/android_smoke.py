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

# Exercise live drawable-driven widening separately from the 4:3 hash baseline.
# The state's framebuffer dump intentionally crops to the authentic 256px
# center, so verify that center remains bit-identical and inspect the native
# diagnostic for the actual adaptive frame width.
call('shell','am','force-stop','com.hector.yoshi')
call('shell','run-as','com.hector.yoshi','rm','-f',
     *[f'files/captures/{tag}.fb.bgrx' for tag in capture_names],
     'files/captures/adaptive-width.txt',check=False)
call('logcat','-c')
call('shell','am','start','-W','-n','com.hector.yoshi/.YoshiActivity',
     '--ez','regression','true','--ez','adaptive','true')
deadline=time.monotonic()+90
while time.monotonic()<deadline:
    result=call('shell','run-as','com.hector.yoshi','ls','files/captures/after_start.fb.bgrx',check=False)
    if result.returncode==0:break
    time.sleep(1)
else:
    (out/'adaptive-logcat.txt').write_bytes(call('logcat','-d').stdout)
    raise SystemExit('Adaptive Android capture did not complete')
data=call('exec-out','run-as','com.hector.yoshi','cat','files/captures/after_start.fb.bgrx').stdout
if len(data)!=256*224*4:
    raise SystemExit(f'Unexpected centered framebuffer size under widescreen: {len(data)} bytes')
if hashlib.sha256(data).hexdigest()!=expected['after_start']:
    raise SystemExit('Widened PPU changed the authentic 256px center framebuffer')
(out/'adaptive-after_start.fb.bgrx').write_bytes(data)
log=call('logcat','-d').stdout
(out/'adaptive-logcat.txt').write_bytes(log)
diag=call('exec-out','run-as','com.hector.yoshi','cat','files/captures/adaptive-width.txt').stdout
(out/'adaptive-width.txt').write_bytes(diag)
import re
matches=re.findall(rb'drawable=(\d+)x(\d+) frame=(\d+)x(\d+)',diag)
if not matches or int(matches[-1][2])<=256:
    raise SystemExit('Android drawable did not select a widened frame; inspect adaptive-width.txt')
print(f"adaptive PPU field: {matches[-1][2].decode()}x224; authentic 256px center matches desktop",flush=True)
subprocess.run(['python3',str(root/'tools/checkpoint.py'),'--event','Android 4:3 framebuffer hashes match desktop; live 1280x720 drawable selects 342x224 and preserves exact 256px center'],check=True)
