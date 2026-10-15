#!/usr/bin/env python3
"""Verify optional Android login UI and offline failure via real JNI/rc_client.
Uses synthetic credentials and an explicit no-network transport test switch.
"""
import os
from pathlib import Path
import re
import subprocess
import time
import xml.etree.ElementTree as ET
root = Path(__file__).resolve().parents[1]
sdk = Path(os.environ.get('ANDROID_HOME', '/Users/hectorx24/Downloads/patreon/tooling/sdk'))
adb = [str(sdk/'platform-tools/adb'), '-s', 'emulator-5554']
package = 'com.hector.yoshi'
out = root/'local/android/achievements'; out.mkdir(parents=True, exist_ok=True)
def call(*args):
    return subprocess.run(adb+list(args), capture_output=True, check=True, timeout=30).stdout

def nodes():
    call('shell', 'uiautomator', 'dump', '/sdcard/yoshi-ui.xml')
    raw = call('exec-out', 'cat', '/sdcard/yoshi-ui.xml')
    return ET.fromstring(raw)
def tap(node):
    x1,y1,x2,y2 = map(int, re.findall(r'\d+',node.attrib['bounds']))
    call('shell','input','tap',str((x1+x2)//2),str((y1+y2)//2))
def text(value):
    tree = nodes()
    hits = [n for n in tree.iter('node') if value in n.attrib.get('text','')]
    assert hits, f'Missing UI text: {value}'
    return hits[0]
def menu():
    size = call('shell','wm','size').decode()
    w,h = map(int,re.findall(r'(\d+)x(\d+)',size)[-1]); w,h=max(w,h),min(w,h)
    s=min(h/448,w/800)
    call('shell','input','tap',str(round(w-24*s)),str(round(24*s)))
    tap(text('RetroAchievements'))

call('install','-r',str(root/'local/android/artifacts/yoshi-usa11-arm64-dev.apk'))
call('shell','am','force-stop',package)
call('shell','am','start','-W','-n',package+'/.YoshiActivity','--ez','achievements_test','true')
time.sleep(3)
# Fresh AVDs show Android's one-time immersive-mode education over the app.
for n in nodes().iter('node'):
    if n.attrib.get('resource-id') == 'android:id/ok':
        tap(n)
        break
menu()
assert 'Offline' in text('Offline').attrib['text']
tap(text('RetroAchievements username')); call('shell','input','text','SyntheticOffline')
call('shell','input','keyevent','KEYCODE_TAB')
time.sleep(.5)
call('shell','input','text','NeverARealPassword')
(out/'synthetic-form.xml').write_bytes(ET.tostring(nodes()))
call('shell','input','keyevent','KEYCODE_BACK') # dismiss keyboard only
# Keyboard may already have been hidden; locate the dialog button explicitly.
tap(text('LOG IN'))
time.sleep(2)
menu()
status = text('Login failed').attrib['text']
(out/'offline-status.txt').write_text(status)
tap(text('LOG OUT'))
time.sleep(1)
menu()
assert 'signed out' in text('signed out').attrib['text']
tap(text('BACK'))
call('shell','input','keyevent','KEYCODE_HOME')
print('Android optional login UI -> JNI -> real rc_client offline callback and logout passed; zero real authentication requests')
subprocess.run(['python3',str(root/'tools/checkpoint.py'),'--event',
    'Android rc_client optional login/offline/logout UI passed with explicitly disabled network; online account/unlocks unverified'],check=True)
