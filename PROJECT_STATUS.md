# Yoshi Island — authoritative handoff (2026-10-14)

## Working state and decisions
- Continue this checkout; no re-cloning/research restart. Exact pins: dependencies.json. Technical findings: docs/ENGINEERING_NOTES.md. Commands: README.md.
- Owner ROM: headerless USA v1.1, 2 MiB; SHA256 bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06; MD5 ce1e3e33b6e39d37b43d7de599f9e785 matches Yoshifanatic v1.1. Source unchanged. brunovalads v1.0 addresses are not interchangeable.
- SNESRecomp hybrid generated C + shared SNES/Super FX runtime, beam-aligned driver. Current build: 70 AOT variants from local/boot-profile.json; fresh unprofiled: 6. Most CPU code still LLE. No silent emulator substitution.
- Mac ARM64 build/YoshiIsland boots through story/title and playable opening tutorial. SDL3 cache reused.
- Android ARM64 APK builds from current source and passes signature/ABI/export/alignment checks. The Java compile blocker was fixed by using an anonymous window-insets listener. API35 ARM64 emulator installed/launched the APK and matched all desktop regression hashes (`boot600`, `intro1200`, `after_start`) exactly. Smoke script clears old guest captures before each replay. APK: `/Users/hectorx24/Downloads/ys/local/android/artifacts/yoshi-usa11-arm64-dev.apk`. Lifecycle, durable saves and physical GameSir remain unverified.
- Java17/SDK35 reused from /Users/hectorx24/Downloads/patreon/tooling/{jdk-17.0.20.1+1/Contents/Home,sdk}. NDK r27d 27.3.13750724 copied to local/android-toolchain/ndk; official DMG SHA1 80f11292080fab4e869799f1d23caa88dcf3c709 verified. No mounted DMG needed.
- Adaptive native PPU width is now wired to the live drawable: a 1280x720 Android surface selects 342x224 (43 extra pixels per side, with the configured 7:6 SNES pixel aspect). The PPU renders into this wider field; simulation timing and collision are untouched. The regression smoke confirms the authentic centered 256x224 pixels remain exact. Margin tile/world authenticity, camera coverage, BG streaming, HUD behavior, and boss encounters remain unqualified; this is a renderer foundation, not a complete gameplay widescreen port.

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
- src/main.c: shared host identity/entry and native widescreen callback wiring; src/game_rtl.c: beam frame driver and aspect-driven frame sizing; config/bank00.cfg: dispatcher exclusion; src/display_geometry.c: tested geometry component.
- CMakeLists.txt: desktop executable/Android shared-library wiring; android/AndroidManifest.xml and android/java/com/hector/yoshi/YoshiActivity.java: direct-launch wrapper, ROM extraction, immersive mode, developer regression intent.
- tools/build_android.py: manual javac/d8/aapt/signing pipeline; tools/create_test_avd.py: isolated AVD; tools/android_smoke.py: verified 4:3 install/replay hashes plus adaptive width on emulator. Explicitly removes stale guest captures before each replay; wildcard cleanup through `adb shell` was unreliable.
- Latest commit f08465b (Android ARM64 port and adaptive PPU width); prior cf78b6e (handoff workflow), 83f52e0 (oracle/native promotion fix), d50b951 (initial integration). No ROMs, extracted assets, APKs, or signing keys are tracked.

## Commands and next concrete action
- Build/test: sh tools/build.sh; sh tools/test.sh; python3 tools/regression_boot.py --rom '<ROM>'.
- Generate: sh tools/generate.sh '<ROM>' ['<private coverage.json>']; preserve generated/ caches and local/boot-profile.json.
- APK: python3 tools/build_android.py --rom '<ROM>' (incremental, SDK/JDK/NDK env overrides supported).
- Replay: SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SNESRECOMP_DUMP_DIR="$PWD/local/<capture>" build/YoshiIsland --no-launcher --rom '<ROM>' --script "$PWD/tests/tutorial.script".
- Environment knobs require SNESRECOMP_ prefix; plain RUN_FRAMES is ignored. Hold menu inputs multiple frames; dialogs require input.
- Latest verified: `sh tools/test.sh` passed 6 Python tests and C geometry cases; `sh tools/build.sh` succeeded; `python3 tools/regression_boot.py --rom "Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc"` matched all 3 baseline captures; `python3 tools/build_android.py --rom "Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc"` and `python3 tools/android_smoke.py` succeeded. Android 4:3 hashes match desktop; adaptive 1280x720 selects 342x224 and preserves the centered hash. Evidence: `local/android/smoke/`, including `adaptive-width.txt`.
- NEXT priority 1: inspect captured 342x224 output and Yoshi v1.1 camera/VRAM streaming to establish which side-margin tiles are live level content versus stale/repeated data; add scene-specific validation. Do not call width expansion finished until playable scrolling coverage is demonstrated.
- Priority 2: add game-specific camera/tile-streaming policies, preserve HUD elements, and evaluate boss arenas individually, including Super FX transitions and collision. Use authentic background extension only for an encounter with a demonstrated limitation.
- Priority 3: resolve mode5->6 timing/GSU divergence and remaining AOT state differences with targeted oracle traces. Qualify Android pause/resume, durable saves and physical GameSir mapping; broad levels/bosses/transformations remain untested.
- Emulator setup is complete: official emulator 37.2.12 build16428233 (SHA1 3af4fe44ce82b3d88ae5678a53735f27ad729c15), API35 default ARM64 r02 image (SHA1 2026a06409db630b56711afdbffb457c1dbaed49), extracted in local/android-toolchain/. Hypervisor acceleration passed; isolated yoshi_test AVD booted in 22.459s and emulator-5554 was observed connected. No physical device tested. First boot failed a disk-space check; removing verified download ZIPs resolved it. Last observed free disk ~5.8 GiB: reuse caches, avoid large downloads.
- If emulator is no longer running: ANDROID_AVD_HOME="$PWD/local/android-toolchain/avd" ANDROID_HOME=/Users/hectorx24/Downloads/patreon/tooling/sdk local/android-toolchain/emulator/emulator -avd yoshi_test -port 5554 -no-window -gpu swiftshader_indirect -no-audio -no-snapshot -no-boot-anim -no-metrics > local/android/emulator.log 2>&1.
- Unfinished: timing/reference equivalence, native WRAM coverage, true widescreen, controller-device tests, durable background saves/lifecycle, broad levels/bosses/transformations and final APK qualification.
- Handoff workflow: tools/checkpoint.py atomically replaces this footer; build/test/APK/boot gates checkpoint automatically. Agents update substantive facts on discoveries and before ending. No quota polling daemon or guessed percentages; use app get_usage_limits at milestones, checkpoint immediately <=5%.

<!-- checkpoint:begin -->
## Automatic checkpoint
- Checked 2026-10-15T01:02:43+00:00; Committed f08465b; Android exact-hash boot regression and 342px adaptive PPU field verified
- Git base at checkpoint: f08465b; 1 changed/untracked paths (run git status).
- Last supported quota observation (2026-10-15T00:54:48+00:00): five_hour: 95% remaining; weekly: 84% remaining.
<!-- checkpoint:end -->
