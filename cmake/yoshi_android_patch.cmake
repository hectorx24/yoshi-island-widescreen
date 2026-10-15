# Android save-load diagnostics; runtime lifecycle handling uses SDL event watches.
# Opt-in load fingerprint for the emulator persistence test. No save bytes
# are logged and no emulated state changes.
set(_yoshi_rtl "${SNESRECOMP_ROOT}/runner/src/common_rtl.c")
file(READ "${_yoshi_rtl}" _yoshi_rtl_source)
set(_yoshi_sram_old [[    if (fread(g_sram, 1, g_sram_size, f) != g_sram_size)
      fprintf(stderr, "Error reading %s\n", path);
    fclose(f);]])
set(_yoshi_sram_new [[    if (fread(g_sram, 1, g_sram_size, f) != g_sram_size)
      fprintf(stderr, "Error reading %s\n", path);
    else if (getenv("SNESRECOMP_SAVE_DIAG")) {
      uint32_t hash = 2166136261u;
      for (int i = 0; i < g_sram_size; ++i)
        hash = (hash ^ g_sram[i]) * 16777619u;
      fprintf(stderr, "[yoshi-save] loaded bytes=%d fnv=%08x\n", g_sram_size, hash);
    }
    fclose(f);]])
string(FIND "${_yoshi_rtl_source}" "[yoshi-save] loaded" _yoshi_sram_at)
string(FIND "${_yoshi_rtl_source}" "YoshiSaveDiagnostic" _yoshi_sram_current)
if(_yoshi_sram_at EQUAL -1 AND _yoshi_sram_current EQUAL -1)
  string(FIND "${_yoshi_rtl_source}" "${_yoshi_sram_old}" _yoshi_sram_at)
  if(_yoshi_sram_at EQUAL -1)
    message(FATAL_ERROR "Pinned SRAM loader changed; review persistence diagnostic")
  endif()
  string(REPLACE "${_yoshi_sram_old}" "${_yoshi_sram_new}"
    _yoshi_rtl_source "${_yoshi_rtl_source}")
  file(WRITE "${_yoshi_rtl}" "${_yoshi_rtl_source}")
endif()
set(_yoshi_diag_v1 [[    else if (getenv("SNESRECOMP_SAVE_DIAG")) {
      uint32_t hash = 2166136261u;
      for (int i = 0; i < g_sram_size; ++i)
        hash = (hash ^ g_sram[i]) * 16777619u;
      fprintf(stderr, "[yoshi-save] loaded bytes=%d fnv=%08x\n", g_sram_size, hash);
    }]])
set(_yoshi_diag_v2 [[    else {
      extern void YoshiSaveDiagnostic(const char *, int);
      YoshiSaveDiagnostic("loaded", 1);
    }]])
string(REPLACE "${_yoshi_diag_v1}" "${_yoshi_diag_v2}"
  _yoshi_rtl_source "${_yoshi_rtl_source}")
file(WRITE "${_yoshi_rtl}" "${_yoshi_rtl_source}")
