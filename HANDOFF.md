# Yoshi's Island Android port — handoff for Sol

Read `AGENTS.md` and `PROJECT_STATUS.md` first. Continue from the current checkout and verified evidence; do not restart research or treat 4:3 fallback as the widescreen deliverable. This file captures the broader user requirements and the latest renderer investigation.

## Goal and user decisions

Port the owner-supplied USA v1.1 Yoshi's Island ROM to a playable native Android ARM64 app, targeting the OPPO Find X9 Pro (2772×1272 landscape), while making the original game display naturally across the usable widescreen area. Preserve original simulation, graphics, audio, behavior, collision boundaries, transformations, and Super FX effects.

Widescreen priorities are: (1) true expanded gameplay showing authentic additional world content wherever feasible, including bosses; (2) when a particular scene cannot safely support expanded gameplay, a seamless scene-specific extension made from its own original graphics and tiles. Do not stretch, invent scenery, use generic bars, or make 4:3 the default unsupported-scene treatment. Native framing is currently a temporary corruption guard and must not be called completed widescreen. Investigate bosses individually; fixed camera or Super FX alone is not a reason to exclude them.

User asked for autonomous implementation, focused reproducible tests, stable incremental commits, and concise maintenance of `PROJECT_STATUS.md`. Do not repeatedly ask for experimental APK testing. Report only verified device tests; no physical OPPO or GameSir test has occurred. Preserve the ROM and extracted/copyrighted data outside Git.

Other requested eventual features: GameSir over Bluetooth/USB, optional touch input, reliable persistent saves, good audio/performance, correct safe-area scaling, selectable Sharp and subtle Authentic Soft composite filtering (SMWRecomp CRT Soft / Warm Composite as reference; no heavy scanlines/curvature), optional offline-safe RetroAchievements `rc_client` for game 558 with softcore support, and independently toggleable inexpensive scene-aware 2D lighting. Optional work must not delay a playable stable APK.

## ROM and architecture

- ROM: `Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc`, headerless 2 MiB, SHA-256 `bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06`, MD5 `ce1e3e33b6e39d37b43d7de599f9e785`. Verified against the Yoshifanatic v1.1 disassembly. Treat it as read-only.
- The project uses SNESRecomp generated C with the shared SNES/Super FX runtime and LLE for most CPU execution. This is a native port, not a bundled emulator substitution. `dependencies.json` pins upstreams.
- Main integration: `src/game_rtl.c` (frame boundary, GSU/widescreen hooks), `src/main.c` (host entry and callbacks), `src/display_geometry.c` (tested geometry helper), `config/bank00.cfg` (keeps unsafe dispatcher in LLE), `cmake/yoshi_widescreen_patch.cmake` (idempotent pinned-runner patches), `android/java/com/hector/yoshi/YoshiActivity.java`, `android/AndroidManifest.xml`, `tools/build_android.py`, `tools/android_smoke.py`.
- Generated sources, emulator/reference captures, APKs, local toolchains, and ROM-derived diagnostics are in ignored/local paths. Keep them out of commits.

## Implemented and verified

- Mac ARM64 desktop build launches the supplied ROM through title and into the playable opening tutorial. The 6,211-frame tutorial script exercises movement and level scrolling; it does not qualify all mechanics or levels.
- Android ARM64 development APK builds and passes package/signature/ABI/alignment checks. API 35 ARM64 emulator launch/replay matches desktop baseline hashes for `boot600`, `intro1200`, and `after_start`. The smoke test currently verifies safe native 256×224 tutorial framing only.
- APK path: `/Users/hectorx24/Downloads/ys/local/android/artifacts/yoshi-usa11-arm64-dev.apk`. Build: `python3 tools/build_android.py --rom "Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc"`.
- Android activity requests immersive landscape-friendly fullscreen, keeps the display awake, and applies platform system-bar/display-cutout insets (API 30+) while avoiding unsafe cutout layout. `src/display_geometry.c` unit tests include 2772×1272 with insets and explicit pixel aspect. The helper is not currently integrated into an adaptive game frame: `GamePrepareFrame` ignores drawable dimensions and selects 256×224 unless `SNESRECOMP_WS_EXTRA` is explicitly set. Thus full usable-area widescreen is not implemented.
- Built-in SRAM path exists, but durable Android save/relaunch behavior is unverified. SDL gamepad is enabled; GameSir Bluetooth/USB pairing and mapping are unverified. Audio signal and normal-speed pacing have a basic desktop measurement, not fidelity qualification.
- `sh tools/test.sh` passed (six Python tests plus C geometry cases). `python3 tools/regression_boot.py --rom "Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc"` matched the three baseline captures. Latest recorded desktop build, Android build, and emulator smoke all passed; see timestamped checkpoint in `PROJECT_STATUS.md`.
- No verified enhanced-lighting implementation is present in current source/history; lighting remains a requested optional feature, not a completed improvement. No CRT modes or RetroAchievements integration are implemented either.

## Widescreen inventory and latest visual evidence

**Production:** every scene currently renders at 256×224. No scene has production true expanded gameplay; no scene has an authentic visual side extension. This includes all normal levels, bosses, and special scenes. The temporary all-scene native framing exists because wide rendering previously showed repeated/stale VRAM and scene corruption, not because those scenes are inherently incompatible.

**Experimental renderer:** `SNESRECOMP_WS_EXTRA=43` produces a 342×224 field (43 extra pixels on each side) for deterministic experiments. World shadows map four v1.1 camera origins from WRAM `$7E0039 + 4*n`. Native PPU writes populate authoritative shadow cells. Missing cells may otherwise fall back to circular/live VRAM, which previously exposed stale repeated tiles. Keep production fallback in place until each scene/margin has verified source coverage and composed-frame inspection.

**GSU path:** a v1.1 hook at GSU stop `$09FA26` observes the game-generated BG1 conversion outputs at `$0DAA/$0DEA` (24 entries per half). For route frame 6033, same-field PPU write tracing confirmed the meaningful GSU entries at world X tiles 58/59, Y tiles 234–247. The hook maps the two 8px columns using `(cameraX >> 3) + 32` and `ceil(cameraY/256)*32`. This proves only those two native-generated columns in that measured field, not full margins or other scenes.

**Clipped mountains / foreground artifacts:** at 342×224, the center 256px view remains unchanged, but mountains, hills, and foreground geometry are cut at the native viewport edge. The right margin contains cyan/blue repeated or mismatched tile patterns and abrupt terrain/background seams; portions of foreground do not continue naturally. The left edge also lacks a qualified extension. These visible artifacts mean the scene is not widescreen-supported even when nonblack pixels appear.

Latest focused test: private GSU replay with MAP16 source shift +1, destination advanced by 16px. With `WsShadowPrefillTile`, the composed frame changed **zero** pixels versus the saved frame because prefill will not replace captured cells. A diagnostic `WsShadowForceTile` rerun changed 2,397 right-margin pixels, only at x=305–320 (y=40–215); the native center changed zero pixels. The rest of the 43px margin remained unresolved and the image still had obvious tile/scene artifacts. This does not qualify the shifted source as production-safe. Runtime source has been restored to `WsShadowPrefillTile`; do not leave the force experiment enabled. The replay seed is opt-in via `SNESRECOMP_WS_GSU_REPLAY_FILE`, `_SHIFT`, and `_SEED` and remains diagnostic/experimental.

The supported candidate `Yoshifanatic1/Yoshi-s-Island-Disassembly` explicitly has USA v1.1 references/release. `brunovalads/Yoshi-s-Island-Disassembly` targets v1.0 and may only supplement conceptual documentation; never transfer its ROM addresses without v1.1 confirmation. The relevant DKC3 widescreen guidance has already been reviewed: tile provenance, same-rendered-PPU-phase validation, sprite visibility, safe fallback. Its useful lesson is to use authentic world-keyed tiles and same-field writes; delayed VRAM snapshots after game updates are not proof. Related engine guidance is `upstream/snesrecomp/docs/WIDESCREEN_PATTERNS.md`.

## Technical references and failed approaches

- Yoshi USA v1.1 disassembly: <https://github.com/Yoshifanatic1/Yoshi-s-Island-Disassembly>
- Complementary v1.0-only documentation (addresses differ): <https://github.com/brunovalads/Yoshi-s-Island-Disassembly>
- DKC3 widescreen guide, only the requested provenance/prefill/sprite/fallback sections: <https://github.com/elliotttate/DKC3Recomp/blob/main/docs/WIDESCREEN_GUIDE.md>
- Requested later presentation references: <https://github.com/mstan/SuperMarioWorldRecomp> and <https://github.com/RetroAchievements/rcheevos> (game ID 558).
- Independent oracle: pinned Snes9x under `upstream/snes9x`; it is a test oracle only.
- Avoid: delayed VRAM snapshots as same-field truth; globally enabling echo rejection (changed margin pixels but left checkerboard); trusting shifted GSU replay merely because 44–48/48 outputs later matched after game state changed; `WsShadowForceTile` as a blanket fix; treating nonblack margins, native-center hashes, or tile counters as visual qualification; promoting the `$008150` dispatcher from LLE (it corrupts control flow).

## Phone display and configuration

The OPPO target is 2772×1272 landscape (about 2.18:1 before safe insets). Android hides system bars in immersive mode and applies the platform-reported cutout/system-bar safe insets. The geometry helper can calculate a wider logical field, but the runtime currently clamps its game frame to 256×224; the safe insets and phone aspect therefore have not yet produced true widescreen gameplay. No test has been performed on the physical OPPO. Emulator configuration and launch command are in `PROJECT_STATUS.md`.

## Current Git state and resume commands

At handoff, HEAD is `876bb86 Keep unqualified Yoshi scenes in native framing` (with preceding GSU commits `cbd450b` and `f6e22fd`). The tree has working changes in `PROJECT_STATUS.md`, `cmake/yoshi_widescreen_patch.cmake`, and `src/game_rtl.c`, plus untracked `last_run_report.json`; inspect `git status` before editing. The force-tile experiment was reverted to the original prefill calls. Do not discard other in-progress changes. No project source was intentionally changed after the user's handoff-only request.

Useful commands:

```sh
sh tools/build.sh
sh tools/test.sh
python3 tools/regression_boot.py --rom "Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc"
python3 tools/build_android.py --rom "Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc"
python3 tools/android_smoke.py
```

Desktop tutorial replay:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
SNESRECOMP_DUMP_DIR="$PWD/local/tutorial" build/YoshiIsland --no-launcher \
  --rom "$PWD/Super Mario World 2 - Yoshi's Island (U) (M3) (V1.1).smc" \
  --script "$PWD/tests/tutorial.script"
```

The last registered Codex quota observation in `PROJECT_STATUS.md` is 46% remaining for five-hour and 76% weekly at 2026-10-15 02:31 UTC; this is stale and must not be reported as current. Use the supported app quota API only at appropriate milestones, as `AGENTS.md` directs.

## Next work, in order

1. Requalify one composed level frame end-to-end: establish a correct BG1 world coordinate and phase, use only sources proven for that exact scene/field, render unknown margin cells black, and inspect full-frame seams, clipped mountains/foreground, sprites, and both margins. Make one measurable visual correction and repeat against a deterministic capture. Do not enable the current shifted replay seed in production based on the failed test above.
2. Build scene-specific coverage progressively. Expand authentic map data beyond the two confirmed GSU columns only after verifying the v1.1 map-buffer indexing and world destination. Prefer true gameplay; use original scene tiles for nonplayable side extension only when a specific encounter presents a demonstrated technical limit. Audit bosses individually in mode `$0F`, preserving behavior, collision, transformations, and effects.
3. Connect the tested usable-display geometry to runtime frame selection, then qualify orientation, cutout-safe scaling, Android lifecycle, saves, audio/performance, touch controls and GameSir over emulator/accessory tests. Keep the 4:3 guard only for cells/scenes that still lack authentic coverage.
4. After a stable playable wide APK, implement optional Sharp/Authentic Soft GPU filtering, offline-safe softcore achievements with verified SNES memory mapping, and independently toggleable subtle 2D lighting. Keep them optional and test separately.
5. Update `PROJECT_STATUS.md` at meaningful milestones and before ending; run focused tests, checkpoint, and commit stable tested source without ROMs/assets/saves.
