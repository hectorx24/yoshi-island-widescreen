# Yoshi Island Android port development

Unfinished SNESRecomp port with Mac ARM64 and Android ARM64 builds. The opening
scene has verified adaptive terrain and mountain rendering; other scenes remain
unqualified. Android emulator testing at2772x1272 selected420x224 while preserving
the original256px center exactly. Read PROJECT_STATUS.md for current coverage,
APK/build commands, and remaining work; no physical-phone test is claimed.

The owner-supplied USA v1.1 ROM is supported for this development target.
It is never modified or committed. Its headerless hash is recorded in
rom_identity.txt; code generation refuses a different image.

## Build on Mac

Requirements: Xcode Command Line Tools, Python 3.9+, Rust 1.97.1, CMake
4.4.4 and Ninja 1.13.2. This workspace has Rust under ~/.cargo/bin and
CMake/Ninja in local/venv; build scripts reuse them. SDL3 is fetched by
CMake with a pinned archive checksum on the first configure.

```sh
python3 tools/fetch_dependencies.py
python3 -m venv local/venv
local/venv/bin/python -m pip install cmake==4.4.4 ninja==1.13.2
sh tools/generate.sh '/absolute/path/to/owner-ROM.smc'
sh tools/build.sh
sh tools/test.sh
build/YoshiIsland --no-launcher --rom '/absolute/path/to/owner-ROM.smc'
```

Generated C remains private and ignored. Compiler/runtime attribution is in
upstream/snesrecomp/LICENSE and THIRD_PARTY_ATTRIBUTION.md; original template
comments are retained in the host integration files.

## Automated boot capture

```sh
mkdir -p local/scenes
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
SNESRECOMP_DUMP_DIR="$PWD/local/scenes" \
build/YoshiIsland --no-launcher --rom '/absolute/path/to/owner-ROM.smc' \
  --script "$PWD/tests/boot.script"
```

The script captures 600, 1200 and 1382-frame checkpoints and exits. A successful
exit alone does not validate game graphics or mechanics. Snapshots include
WRAM, SRAM, PPU state, original framebuffer, and Super FX activity.

## Independent oracle

```sh
make -C upstream/snes9x/libretro -j4
python3 tools/oracle.py --core upstream/snes9x/libretro/snes9x_libretro.dylib \
  --rom '/absolute/path/to/owner-ROM.smc' --script tests/boot.script \
  --output local/oracle-boot
```

This separate emulator is a testing oracle, not the application runtime.
Audio RMS indicates signal activity only; it does not establish audio fidelity.

## Native coverage experiment and regression

Capture into a private directory; profile JSON includes ROM-derived bytes.

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
SNESRECOMP_COVERAGE_DIR="$PWD/local/coverage" \
build/YoshiIsland --no-launcher --rom '/absolute/path/to/owner-ROM.smc' \
  --script "$PWD/tests/boot.script" --coverage-capture=on
sh tools/generate.sh '/absolute/path/to/owner-ROM.smc' '/absolute/path/to/capture.json'
sh tools/build.sh
python3 tools/regression_boot.py --rom '/absolute/path/to/owner-ROM.smc'
```

The framebuffer gate detects the known dispatcher promotion failure. It
does not certify hardware accuracy. tools/compare_captures.py reports CPU
and cartridge-memory differences and supports --strict-state. See
docs/ENGINEERING_NOTES.md for current differences and the version-specific
dispatcher correction.

Replay tests/tutorial.script using the same --script/SNESRECOMP_DUMP_DIR
pattern to reach the tutorial and exercise movement. Further mechanics,
bosses, transformations, controller devices, saves, Android lifecycle, and
widescreen remain unqualified.
