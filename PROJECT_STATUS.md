# Yoshi Island — authoritative handoff (2026-10-14)

## Working state and decisions
- Continue this checkout; no re-cloning/research restart. Exact pins: dependencies.json. Technical findings: docs/ENGINEERING_NOTES.md. Commands: README.md.
- Owner ROM: headerless USA v1.1, 2 MiB; SHA256 bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06; MD5 ce1e3e33b6e39d37b43d7de599f9e785 matches Yoshifanatic v1.1. Source unchanged. brunovalads v1.0 addresses are not interchangeable.
- SNESRecomp hybrid generated C + shared SNES/Super FX runtime, beam-aligned driver. Current build: 70 AOT variants from local/boot-profile.json; fresh unprofiled: 6. Most CPU code still LLE. No silent emulator substitution.
- Mac ARM64 build/YoshiIsland boots through story/title and playable opening tutorial. SDL3 cache reused.
- Android ARM64 APK builds from current source and passes signature/ABI/export/alignment checks. The Java compile blocker was fixed by using an anonymous window-insets listener. API35 ARM64 emulator installed/launched the APK and matched all desktop regression hashes (`boot600`, `intro1200`, `after_start`) exactly. Smoke script clears old guest captures before each replay. APK: `/Users/hectorx24/Downloads/ys/local/android/artifacts/yoshi-usa11-arm64-dev.apk`. Lifecycle, durable saves and physical GameSir remain unverified.
- Java17/SDK35 reused from /Users/hectorx24/Downloads/patreon/tooling/{jdk-17.0.20.1+1/Contents/Home,sdk}. NDK r27d 27.3.13750724 copied to local/android-toolchain/ndk; official DMG SHA1 80f11292080fab4e869799f1d23caa88dcf3c709 verified. No mounted DMG needed.
- Adaptive native PPU width is wired only for verified game modes $0E/$0F; menus/narrative keep 256x224. A 1280x720 drawable selects 342x224 (43 extra pixels per side at 7:6 PAR). For widened gameplay, `GameBeginFrame` maps the four v1.1 layer origins at WRAM `$7E0039 + 4*n` into the runtime world-keyed BG shadow. This is presentation-only; collision and simulation timing are unchanged. Bosses in gameplay mode remain eligible with no blanket restriction.
- Widened 6,211-frame tutorial replay completed and produced nonblack margins; exact visual authenticity is NOT complete. At frame 6120 on desktop, BG1 world-shadow counters reported west hit 4,347,767 / miss 0, east hit 1,464,602 / miss 5,859,970; BG2 east hit 9,575,680 / miss 28,794,060. Misses fall back to raw tilemap data. The right gutter capture visibly has checkerboard/echo artifacts, so the next step is to correlate Yoshi's streamed map and GSU output with the exact margin tiles, then remove only demonstrated ghosts. Evidence: `local/wide-tutorial/wide-after_move.png`, `.bgrx`, `ws.log`; Android adaptive capture: `local/android/smoke/adaptive-after_move.png`.

## Verified tests and failures to avoid
- sh tools/test.sh: ROM intake + geometry and quota parsing pass (6 Python tests plus C cases).
- python3 tools/regression_boot.py --rom '<ROM>': exact baseline pixels at 600/1200/1382. This is a regression gate, NOT a hardware oracle.
- tests/tutorial.script: 6211 frames, original dialogs/scrolling, player SRAM X 136 -> 270. Evidence local/tutorial/. Jump/egg/collision/full levels unqualified.
- Audio: 600 normal-speed frames produce 9.938s nonzero S16LE stereo sink; RMS747.33, peak11712. Wall runtime10.619s. Fidelity/pacing still unqualified. Evidence local/audio/.
- Independent pinned Snes9x oracle: tools/oracle.py. Serve core option defaults or Super FX gets clock multiplier zero (black/silent). RGB565 green conversion corrected.
- Native promotion of $008150 stack-built RTL dispatcher corrupts flow. Keep force_lle 008150 in config/bank00.cfg. Corrected 70-AOT build matches baseline pixels; small stack/SRAM state differences remain.
- WRAM $7EC000..FFFF matches ROM offsets $4000..7FFF (16KiB); capped/unterminated profile snapshots do NOT establish native WRAM recompilation.
- Targeted reference diagnosis: native/oracle counters equal at frames80..260. First 3-tick divergence occurs in mode5->6 around280..290. At600/1200 pixel differences only logo box(69,64)..(186,100). Native600 pixels exactly match oracle594/595/604; oracle603 counter matches native600. Do not patch graphics to hide phase differences. Evidence local/{native-startup,oracle-startup,native-phase,oracle-phase,oracle-alignment}. Next timing trace: mode5 initialization/GSU jobs. No timing fix claimed.

## Source and recoverable Git state
- src/main.c: shared host identity/entry and native widescreen callback wiring; src/game_rtl.c: beam frame driver, aspect-driven frame sizing and Yoshi camera/world-shadow hooks; config/bank00.cfg: dispatcher exclusion; src/display_geometry.c: tested geometry component.
- CMakeLists.txt: desktop executable/Android shared-library wiring; android/AndroidManifest.xml and android/java/com/hector/yoshi/YoshiActivity.java: direct-launch wrapper, ROM extraction, immersive mode, developer regression intent.
- tools/build_android.py: manual javac/d8/aapt/signing pipeline; tools/create_test_avd.py: isolated AVD; tools/android_smoke.py: verified 4:3 install/replay hashes plus adaptive width on emulator. Explicitly removes stale guest captures before each replay; wildcard cleanup through `adb shell` was unreliable.
- Latest commit f08465b (Android ARM64 port and adaptive PPU width); prior cf78b6e (handoff workflow), 83f52e0 (oracle/native promotion fix), d50b951 (initial integration). No ROMs, extracted assets, APKs, or signing keys are tracked.

## Commands and next concrete action
- Build/test: sh tools/build.sh; sh tools/test.sh; python3 tools/regression_boot.py --rom '<ROM>'.
- Generate: sh tools/generate.sh '<ROM>' ['<private coverage.json>']; preserve generated/ caches and local/boot-profile.json.
- APK: python3 tools/build_android.py --rom '<ROM>' (incremental, SDK/JDK/NDK env overrides supported).
- Replay: SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SNESRECOMP_DUMP_DIR="$PWD/local/<capture>" build/YoshiIsland --no-launcher --rom '<ROM>' --script "$PWD/tests/tutorial.script".
- Environment knobs require SNESRECOMP_ prefix; plain RUN_FRAMES is ignored. Hold menu inputs multiple frames; dialogs require input.
- Latest verified: `sh tools/test.sh` passed 6 Python tests and C geometry cases; `sh tools/build.sh` succeeded; `python3 tools/regression_boot.py --rom "Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc"` matched all 3 baseline captures; Android APK build and `python3 tools/android_smoke.py` passed (4:3 hashes, then 6,211-frame adaptive tutorial at 342x224 with same-run 256px center equality). Desktop `tests/tutorial.script` also completed 6,211 frames at 342x224. Evidence under `local/android/smoke/` and `local/wide-tutorial/`.
- NEXT priority 1: fix the proven east-gutter tile echoes. The v1.1 routine `CODE_109089` streams the 64-column BG1 tilemap from `$7F8000` (`LevelDataBuffer`) into `$70409E` and GSU task `FXCODE_09F9E8`; inspect that address path and use the shadow lookup audit to match margin entries to their later native-view entries. Keep the observed source-specific limitation recorded; do not mask it by stretching or repeating edge pixels.
- Priority 2: check left/right margin sprites, HUD and GSU-generated elements during scrolling; validate resulting content against source map data. Investigate every boss separately in mode $0F, preserve Super FX transitions and collision; do not exclude fixed-camera bosses. Add encounter-specific tests before any background-only fallback.
- Priority 3: resolve mode5->6 timing/GSU divergence and remaining AOT state differences with targeted oracle traces. Qualify Android pause/resume, durable saves and physical GameSir mapping; broad levels/bosses/transformations remain untested.
- Emulator setup is complete: official emulator 37.2.12 build16428233 (SHA1 3af4fe44ce82b3d88ae5678a53735f27ad729c15), API35 default ARM64 r02 image (SHA1 2026a06409db630b56711afdbffb457c1dbaed49), extracted in local/android-toolchain/. Hypervisor acceleration passed; isolated yoshi_test AVD booted in 22.459s and emulator-5554 was observed connected. No physical device tested. First boot failed a disk-space check; removing verified download ZIPs resolved it. Last observed free disk ~5.8 GiB: reuse caches, avoid large downloads.
- If emulator is no longer running: ANDROID_AVD_HOME="$PWD/local/android-toolchain/avd" ANDROID_HOME=/Users/hectorx24/Downloads/patreon/tooling/sdk local/android-toolchain/emulator/emulator -avd yoshi_test -port 5554 -no-window -gpu swiftshader_indirect -no-audio -no-snapshot -no-boot-anim -no-metrics > local/android/emulator.log 2>&1.
- Unfinished: authentic BG margin data/streaming (right side has visible checkerboard echoes), broad level progression, boss/transformations and GSU margin elements, mode5->6 reference timing, native WRAM coverage, Android physical GameSir/save/lifecycle qualification.
- Handoff workflow: tools/checkpoint.py atomically replaces this footer; build/test/APK/boot gates checkpoint automatically. Agents update substantive facts on discoveries and before ending. No quota polling daemon or guessed percentages; use app get_usage_limits at milestones, checkpoint immediately <=5%.

<!-- checkpoint:begin -->
## Automatic checkpoint
- Checked 2026-10-15T01:10:38+00:00; Adaptive Android tutorial field 342x224 and 6211-frame desktop wide tutorial verified; BG1 east echoes remain open
- Git base at checkpoint: 7b19f4c; 7 changed/untracked paths (run git status).
- Last supported quota observation (2026-10-15T00:54:48+00:00): five_hour: 95% remaining; weekly: 84% remaining.
<!-- checkpoint:end -->
