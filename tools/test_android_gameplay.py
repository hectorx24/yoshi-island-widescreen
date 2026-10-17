#!/usr/bin/env python3
"""Automated gameplay navigation and verification tool for Android emulator with accurate SNES boot timings."""
import subprocess
import time
from pathlib import Path

ADB = "/Users/hectorx24/Downloads/patreon/tooling/sdk/platform-tools/adb"
OUT_DIR = Path("local/emulator_captures")
OUT_DIR.mkdir(parents=True, exist_ok=True)

def adb_cmd(*args):
    cmd = [ADB] + list(args)
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return res.stdout

def tap(x, y, duration_ms=250):
    adb_cmd("shell", "input", "swipe", str(x), str(y), str(x), str(y), str(duration_ms))

def screencap(name):
    path = OUT_DIR / f"{name}.png"
    subprocess.run(f"{ADB} exec-out screencap -p > {path}", shell=True)
    print(f"Captured: {path}")
    return path

def main():
    print("Restarting app...")
    adb_cmd("shell", "am", "force-stop", "com.hector.yoshi")
    time.sleep(0.5)
    adb_cmd("shell", "am", "start", "-n", "com.hector.yoshi/.YoshiActivity")

    print("Waiting for boot Nintendo screen (8s)...")
    time.sleep(8.5)

    print("Title screen active, pressing START...")
    tap(710, 665, 250) # Start
    time.sleep(2.0)

    print("Story screen active, pressing START to skip...")
    tap(710, 665, 250) # Start
    time.sleep(2.5)

    screencap("stage_01_file_selection")

    print("Moving cursor to File 2 (D-pad Right)...")
    tap(160, 601, 250)
    time.sleep(1.0)
    screencap("stage_02_file2_highlighted")

    print("Selecting File 2 (Button A)...")
    tap(1210, 598, 250)
    time.sleep(1.5)
    screencap("stage_03_file2_selected")

    print("Confirming and entering World Map (Button A)...")
    tap(1210, 598, 250)
    time.sleep(3.5)
    screencap("stage_04_world_map")

    print("Entering World 2 Level 1 (Button A)...")
    tap(1210, 598, 250)
    time.sleep(4.0)
    screencap("stage_05_level_2_1_entry")

    print("Walking right in World 2-1...")
    # Hold D-pad Right for 2 seconds
    adb_cmd("shell", "input", "swipe", "160", "601", "160", "601", "2000")
    time.sleep(0.5)
    screencap("stage_06_level_2_1_gameplay")

    print("Jumping in World 2-1 (Button B)...")
    tap(1152, 656, 300)
    time.sleep(0.3)
    screencap("stage_07_level_2_1_jump_shadow")

if __name__ == "__main__":
    main()
