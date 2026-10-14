# Yoshi Island — authoritative handoff (2026-10-14)

## Working state and decisions
- Continue this checkout; no re-cloning/research restart. Exact pins: dependencies.json. Technical findings: docs/ENGINEERING_NOTES.md. Commands: README.md.
- Owner ROM: headerless USA v1.1, 2 MiB; SHA256 bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06; MD5 ce1e3e33b6e39d37b43d7de599f9e785 matches Yoshifanatic v1.1. Source unchanged. brunovalads v1.0 addresses are not interchangeable.
- SNESRecomp hybrid generated C + shared SNES/Super FX runtime, beam-aligned driver. Current build: 70 AOT variants from local/boot-profile.json; fresh unprofiled: 6. Most CPU code still LLE. No silent emulator substitution.
- Mac ARM64 build/YoshiIsland boots through story/title and playable opening tutorial. SDL3 cache reused.
- Android ARM64 libYoshiIsland.so + libSDL3.so build. Private dev APK: local/android/artifacts/yoshi-usa11-arm64-dev.apk. Signature, arm64 ABI, SDL_main export, 16 KiB ELF/ZIP alignment verified. Java wrapper compiles; direct launch, owner ROM asset, immersive landscape/safe cutout policy, standard SDL gamepad path. Device execution/lifecycle/GameSir NOT verified yet.
- Java17/SDK35 reused from /Users/hectorx24/Downloads/patreon/tooling/{jdk-17.0.20.1+1/Contents/Home,sdk}. NDK r27d 27.3.13750724 copied to local/android-toolchain/ndk; official DMG SHA1 80f11292080fab4e869799f1d23caa88dcf3c709 verified. No mounted DMG needed.
- True widescreen/background extensions NOT implemented. src/display_geometry.c passes sizing/inset tests, not integrated. Bosses require individual true-widescreen solutions; fixed camera/Super FX alone never justify fallback.

## Verified tests and failures to avoid
- sh tools/test.sh: ROM intake + geometry and quota parsing pass (6 Python tests plus C cases).
- python3 tools/regression_boot.py --rom '<ROM>': exact baseline pixels at 600/1200/1382. This is a regression gate, NOT a hardware oracle.
- tests/tutorial.script: 6211 frames, original dialogs/scrolling, player SRAM X 136 -> 270. Evidence local/tutorial/. Jump/egg/collision/full levels unqualified.
- Audio: 600 normal-speed frames produce 9.938s nonzero S16LE stereo sink; RMS747.33, peak11712. Wall runtime10.619s. Fidelity/pacing still unqualified. Evidence local/audio/.
- Independent pinned Snes9x oracle: tools/oracle.py. Serve core option defaults or Super FX gets clock multiplier zero (black/silent). RGB565 green conversion corrected.
- Native promotion of $008150 stack-built RTL dispatcher corrupts flow. Keep force_lle 008150 in config/bank00.cfg. Corrected 70-AOT build matches baseline pixels; small stack/SRAM state differences remain.
- WRAM $7EC000..FFFF matches ROM offsets $4000..7FFF (16KiB); capped/unterminated profile snapshots do NOT establish native WRAM recompilation.
- Targeted reference diagnosis: native/oracle counters equal at frames80..260. First 3-tick divergence occurs in mode5->6 around280..290. At600/1200 pixel differences only logo box(69,64)..(186,100). Native600 pixels exactly match oracle594/595/604; oracle603 counter matches native600. Do not patch graphics to hide phase differences. Evidence local/{native-startup,oracle-startup,native-phase,oracle-phase,oracle-alignment}. Next timing trace: mode5 initialization/GSU jobs. No timing fix claimed.

## Commands and next concrete action
- Build/test: sh tools/build.sh; sh tools/test.sh; python3 tools/regression_boot.py --rom '<ROM>'.
- Generate: sh tools/generate.sh '<ROM>' ['<private coverage.json>']; preserve generated/ caches and local/boot-profile.json.
- APK: python3 tools/build_android.py --rom '<ROM>' (incremental, SDK/JDK/NDK env overrides supported).
- Replay: SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SNESRECOMP_DUMP_DIR="$PWD/local/<capture>" build/YoshiIsland --no-launcher --rom '<ROM>' --script "$PWD/tests/tutorial.script".
- Environment knobs require SNESRECOMP_ prefix; plain RUN_FRAMES is ignored. Hold menu inputs multiple frames; dialogs require input.
- NEXT: finish headless Android emulator setup currently downloading local/android-toolchain/{emulator.zip,system-image.zip}; verify official hashes (commands in active shell), create local AVD, install/launch private dev APK and inspect logcat/screenshot. No ADB device currently connected. Then lifecycle/save qualification, mode5 timing diagnosis, and genuine camera/streaming/GSU widescreen including bosses.
- Unfinished: timing/reference equivalence, native WRAM coverage, true widescreen, controller-device tests, durable background saves/lifecycle, broad levels/bosses/transformations and final APK qualification.
- Handoff workflow: tools/checkpoint.py atomically replaces this footer; build/test/APK/boot gates checkpoint automatically. Agents update substantive facts on discoveries and before ending. No quota polling daemon or guessed percentages; use app get_usage_limits at milestones, checkpoint immediately <=5%.

<!-- checkpoint:begin -->
## Automatic checkpoint
- Checked 2026-10-14T22:54:50+00:00; Authoritative handoff compacted; Android emulator downloads in progress
- Git base at checkpoint: 83f52e0; 11 changed/untracked paths (run git status).
- Last supported quota observation (2026-10-14T22:54:20+00:00): five_hour: 10% remaining; weekly: 86% remaining.
<!-- checkpoint:end -->
