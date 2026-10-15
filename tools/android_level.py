#!/usr/bin/env python3
"""Compare Android level1-1 at phone aspect to a previous native desktop capture."""
import argparse
import atexit
import json
import os
from pathlib import Path
import subprocess
import time
root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--native-prefix', type=Path, required=True)
p.add_argument('--initial-native-prefix', type=Path)
p.add_argument('--display-size', default='2772x1272')
a = p.parse_args()
import re
assert re.fullmatch(r'[1-9][0-9]*x[1-9][0-9]*', a.display_size)
native = Path(str(a.native_prefix)+'.fb.bgrx').read_bytes()
assert json.loads(Path(str(a.native_prefix)+'.info.json').read_text())['frame'] == 8020
assert len(native) == 256*224*4
sdk = Path(os.environ.get('ANDROID_HOME', '/Users/hectorx24/Downloads/patreon/tooling/sdk'))
adb = [str(sdk/'platform-tools/adb'), '-s', 'emulator-5554']
package = 'com.hector.yoshi'
out = root/'local/android/level'; out.mkdir(parents=True,exist_ok=True)
def call(*words, check=True):
    return subprocess.run(adb+list(words),capture_output=True,check=check,timeout=30)
def private(name):
    return call('exec-out','run-as',package,'cat','files/captures/'+name).stdout
previous=call('shell','wm','size').stdout.decode()
override=re.search(r'Override size: (\d+x\d+)', previous)
restore=override.group(1) if override else 'reset'
call('shell','wm','size',a.display_size)
atexit.register(lambda: call('shell','wm','size',restore,check=False))
call('install','-r',str(root/'local/android/artifacts/yoshi-usa11-arm64-dev.apk'))
call('shell','am','force-stop',package)
call('shell','run-as',package,'rm','-f','files/captures/step08.fb.bgrx',
     'files/captures/wide-after_move.bgrx','files/captures/7819.bgrx','files/captures/adaptive-width.txt')
call('shell','run-as',package,'rm','-f','files/viewport.bgrx','files/viewport.bgrx.json')
call('shell','am','start','-W','-n',package+'/.YoshiActivity','--ez','regression','true',
     '--ez','adaptive','true','--ez','level_test','true')
deadline=time.monotonic()+150
while time.monotonic()<deadline:
    r=call('shell','run-as',package,'test','-e','files/captures/step08.info.json',check=False)
    # A prior .info file is insufficient; also require the freshly removed image.
    image=call('shell','run-as',package,'test','-e','files/captures/step08.fb.bgrx',check=False)
    presented=call('shell','run-as',package,'test','-e','files/viewport.bgrx.json',check=False)
    if r.returncode == image.returncode == presented.returncode == 0:break
    time.sleep(1)
else:raise SystemExit('Android level-route timed out')
wide=private('wide-after_move.bgrx'); info=json.loads(private('step08.info.json'))
assert info['frame']==8020
width=len(wide)//(224*4); assert len(wide)==width*224*4
assert width==420, f'Phone-aspect level scene did not expand: {width}'
extra=(width-256)//2
center=b''.join(wide[(y*width+extra)*4:(y*width+extra+256)*4] for y in range(224))
assert center==native, 'Android wide level differs from independently captured desktop native center'
meta=call('exec-out','run-as',package,'cat','files/viewport.bgrx.json').stdout
viewport=json.loads(meta)
x,y,w,h=viewport['viewport']
assert x<=0 and y==0 and x+w>=2772 and h==1272, f'Unfilled phone viewport: {viewport}'
assert w==2783 and x==-5, f'Unexpected SNES pixel-aspect crop: {viewport}'
(out/'viewport.json').write_bytes(meta)
if a.initial_native_prefix:
    reference=Path(str(a.initial_native_prefix)+'.fb.bgrx').read_bytes()
    initial=private('7819.bgrx')
    assert len(initial)==420*224*4, 'Initial world boundary did not extend'
    initial_center=b''.join(initial[(y*420+82)*4:(y*420+338)*4] for y in range(224))
    assert initial_center==reference, 'Authentic boundary extension changed native center'
    (out/'initial-wide.bgrx').write_bytes(initial)
    print('Initial1-1: genuine world east, authenticated decorative ground continuation west; independent native center exact',flush=True)
(out/'wide.bgrx').write_bytes(wide)
(out/'adaptive-width.txt').write_bytes(private('adaptive-width.txt'))
(out/'verified.json').write_text(json.dumps(dict(frame=8020,width=width,height=224,
    native_center_different_pixels=0,native_reference=str(a.native_prefix)),indent=2)+'\n')
call('shell','input','keyevent','KEYCODE_HOME')
print('Android level1-1: 420x224 at2772x1272; center byte-identical to independent native desktop replay; GPU viewport fills display at original pixel aspect')
subprocess.run(['python3',str(root/'tools/checkpoint.py'),'--event',
    'Android level1-1 phone-aspect420x224 matches independent desktop native center exactly'],check=True)
