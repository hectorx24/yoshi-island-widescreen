#!/usr/bin/env python3
"""Automated audio and rendering performance diagnostics for Android.
Measures Sharp, Authentic Soft, and Lighting across stable (1-1) and complex (1-4) scenes.
"""
import argparse
import atexit
import json
import os
from pathlib import Path
import re
import subprocess
import time

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--serial', default='emulator-5554')
p.add_argument('--display-size', default='2772x1272')
args = p.parse_args()

sdk = Path(os.environ.get('ANDROID_HOME', '/Users/hectorx24/Downloads/patreon/tooling/sdk'))
adb = [str(sdk / 'platform-tools/adb'), '-s', args.serial]
package = 'com.hector.yoshi'
out_dir = root / 'local/master-plan/06_audio-performance'
out_dir.mkdir(parents=True, exist_ok=True)

def call(*words, check=True):
    return subprocess.run(adb + list(words), capture_output=True, check=check, timeout=60)

def set_options(display_mode, lighting):
    # display_mode: 0 = Sharp, 1 = Authentic Soft
    # lighting: boolean
    xml = (
        '<?xml version="1.0" encoding="utf-8" standalone="yes" ?>\n'
        '<map>\n'
        f'    <int name="display" value="{display_mode}" />\n'
        f'    <boolean name="lighting" value="{"true" if lighting else "false"}" />\n'
        '</map>\n'
    )
    cmd = f"printf '{xml}' | run-as {package} tee shared_prefs/yoshi-options.xml"
    call('shell', cmd)

def parse_gfxinfo(raw_text):
    data = {}
    m_total = re.search(r'Total frames rendered:\s+(\d+)', raw_text)
    m_janky = re.search(r'Janky frames:\s+(\d+)\s+\(([\d\.]+)%\)', raw_text)
    m_p50 = re.search(r'50th percentile:\s+(\d+)ms', raw_text)
    m_p90 = re.search(r'90th percentile:\s+(\d+)ms', raw_text)
    m_p95 = re.search(r'95th percentile:\s+(\d+)ms', raw_text)
    m_p99 = re.search(r'99th percentile:\s+(\d+)ms', raw_text)
    m_missed_vsync = re.search(r'Number Missed Vsync:\s+(\d+)', raw_text)
    m_slow_ui = re.search(r'Number Slow UI thread:\s+(\d+)', raw_text)
    m_slow_draw = re.search(r'Number Slow issue draw commands:\s+(\d+)', raw_text)
    
    if m_total: data['total_frames'] = int(m_total.group(1))
    if m_janky:
        data['janky_frames'] = int(m_janky.group(1))
        data['janky_percent'] = float(m_janky.group(2))
    if m_p50: data['p50_ms'] = int(m_p50.group(1))
    if m_p90: data['p90_ms'] = int(m_p90.group(1))
    if m_p95: data['p95_ms'] = int(m_p95.group(1))
    if m_p99: data['p99_ms'] = int(m_p99.group(1))
    if m_missed_vsync: data['missed_vsync'] = int(m_missed_vsync.group(1))
    if m_slow_ui: data['slow_ui'] = int(m_slow_ui.group(1))
    if m_slow_draw: data['slow_draw'] = int(m_slow_draw.group(1))
    return data

def run_scene_test(scene_name, is_castle, mode_name, display_mode, lighting):
    print(f"\n--- Testing {scene_name} [{mode_name}] ---", flush=True)
    tag = 'castle03' if is_castle else 'step08'
    frame = 8331 if is_castle else 8020
    
    call('shell', 'am', 'force-stop', package)
    set_options(display_mode, lighting)
    
    # Clean output files
    call('shell', 'run-as', package, 'rm', '-f',
         f'files/captures/{tag}.fb.bgrx', f'files/captures/{tag}.info.json',
         'files/viewport.bgrx', 'files/viewport.bgrx.json')
    
    call('logcat', '-c')
    call('shell', 'dumpsys', 'gfxinfo', package, 'reset')
    
    # Launch with designated test route
    test_arg = 'castle_test' if is_castle else 'level_test'
    call('shell', 'am', 'start', '-W', '-n', f'{package}/.YoshiActivity',
         '--ez', 'regression', 'true', '--ez', 'adaptive', 'true',
         '--ez', test_arg, 'true')
    
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline:
        r_info = call('shell', 'run-as', package, 'test', '-e', f'files/captures/{tag}.info.json', check=False)
        r_img = call('shell', 'run-as', package, 'test', '-e', f'files/captures/{tag}.fb.bgrx', check=False)
        if r_info.returncode == 0 and r_img.returncode == 0:
            break
        time.sleep(1)
    else:
        print(f"Warning: test timed out waiting for {tag}")

    time.sleep(2)  # Allow remaining frames and logs to settle
    
    # Capture gfxinfo
    gfx_raw = call('shell', 'dumpsys', 'gfxinfo', package).stdout.decode('utf-8', errors='ignore')
    metrics = parse_gfxinfo(gfx_raw)
    
    # Capture relevant logcat
    log_raw = call('logcat', '-d').stdout.decode('utf-8', errors='ignore')
    audio_lines = [l for l in log_raw.splitlines() if any(k in l.lower() for k in ['audio', 'sound', 'spc', 'dsp', 'underflow', 'underrun', 'aaudio'])]
    
    call('shell', 'am', 'force-stop', package)
    
    result = {
        'scene': scene_name,
        'mode': mode_name,
        'display_val': display_mode,
        'lighting_val': lighting,
        'target_frame': frame,
        'gfxinfo': metrics,
        'audio_log_count': len(audio_lines),
        'audio_sample_logs': audio_lines[:15]
    }
    print(f"Result for {scene_name} [{mode_name}]: {json.dumps(metrics, indent=2)}", flush=True)
    return result

def main():
    previous = call('shell', 'wm', 'size').stdout.decode()
    override = re.search(r'Override size: (\d+x\d+)', previous)
    restore = override.group(1) if override else 'reset'
    call('shell', 'wm', 'size', args.display_size)
    atexit.register(lambda: call('shell', 'wm', 'size', restore, check=False))
    
    call('install', '-r', str(root / 'local/android/artifacts/yoshi-usa11-arm64-dev.apk'))
    
    modes = [
        ('sharp', 0, False),
        ('authentic_soft', 1, False),
        ('lighting', 0, True)
    ]
    
    all_results = []
    
    # 1. Stable scene: Level 1-1
    for mode_name, disp, light in modes:
        res = run_scene_test('level_1-1_stable', False, mode_name, disp, light)
        all_results.append(res)
        
    # 2. Complex scene: Castle 1-4
    for mode_name, disp, light in modes:
        res = run_scene_test('castle_1-4_complex', True, mode_name, disp, light)
        all_results.append(res)
        
    report_file = out_dir / 'performance_metrics.json'
    report_file.write_text(json.dumps(all_results, indent=2))
    print(f"\nAll metrics written to {report_file}", flush=True)

if __name__ == '__main__':
    main()
