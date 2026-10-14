# Verified state — 2026-10-14

- Target: supplied USA v1.1, explicitly authorized. Source ROM remains unchanged.
- Headerless 2 MiB; region=1, revision=1, checksum/complement valid; calculated checksum matches.
- SHA-256: bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06.
- MD5: ce1e3e33b6e39d37b43d7de599f9e785, exact match to Yoshifanatic's USA v1.1 README reference.
- brunovalads reference targets v1.0 SHA-256 9b4957466798bbdb5b43a450bbb60b2591ae81d95b891430f62d53ca62e8bc7b; do not reuse its ROM addresses blindly.
- Exact repository/tool revisions: dependencies.json. Upstream checkouts and all ROM-derived output are ignored.
- Architecture: SNESRecomp generated C + shared SNES/Super FX runtime, beam-aligned frame driver. Hybrid execution currently has only 6 AOT-eligible variants; most game code runs via interpreter fallback. Not a completed static native port.
- Built Mac ARM64 desktop executable: build/YoshiIsland. SDL3 built locally; incremental cache retained.
- Verified automated 1382-frame boot replay exited normally; captured and visually inspected Nintendo opening, story, animated title. Super FX trace present. Evidence: local/scenes/, local/scenes.log.
- ROM intake: 3 unit tests pass. C geometry tests pass for 16:9, 19.5:9, 20:9, 21:9, 2772x1272 and insets. Geometry is not yet integrated into presentation.
- No true widescreen, background extension, Android APK, full audio qualification, or gameplay validation yet.
- Boss policy: prioritize actual world expansion individually; fixed camera/Super FX do not automatically disqualify encounters. Fallback needs a demonstrated encounter limitation.
- Installed Rust minimal toolchain and project-local CMake/Ninja venv. Java, Android SDK/NDK absent from usual locations.
- Build: sh tools/build.sh. Tests: sh tools/test.sh.
- Boot replay: SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SNESRECOMP_DUMP_DIR="$PWD/local/scenes" build/YoshiIsland --no-launcher --rom '<owner ROM path>' --script "$PWD/tests/boot.script".
- Environment controls require SNESRECOMP_ prefix (e.g. SNESRECOMP_RUN_FRAMES); plain RUN_FRAMES is ignored.
- Next: inspect gameplay replay, compare original viewport against oracle, expand revision-specific AOT roots and investigate camera/tile streaming/GSU rendering. Then Android integration.
