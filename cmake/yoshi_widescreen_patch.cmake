# Keep the original 256px center byte-identical when an 8px BG tile straddles
# its boundary. The native tile also supplies the first pixels in the margin;
# world-keyed extension starts at the next tile boundary.
set(_yoshi_ws_shadow "${SNESRECOMP_ROOT}/runner/src/snes/ws_shadow.c")
file(READ "${_yoshi_ws_shadow}" _yoshi_ws_source)
set(_yoshi_ws_old [[  /*
   * The 8x8 background renderers call once per tile chunk. A hardware window
   * can make the final chunk start inside the native viewport but extend into
   * the right margin (for example x=255..262). Treat that whole straddling
   * chunk as world-keyed shadow data; returning the circular VRAM tile inserts
   * an exact one-tile seam before margin rendering begins.
   */
  const int tile_pixels = 1 << (layer->tileShift ? layer->tileShift : 3);
  if (!layer->active ||
      (screenX >= 0 && screenX + tile_pixels <= 256))
    return realTile;]])
set(_yoshi_ws_new [[  /* Preserve the native framebuffer through a tile chunk
   * that crosses x=256; its real VRAM entry is the same tile used at the
   * viewport edge. Consult world-keyed margin data at the next chunk. */
  if (!layer->active || (screenX < 256 &&
      screenX + (1 << (layer->tileShift ? layer->tileShift : 3)) > 0))
    return realTile;]])
# Migrate the earlier center guard, which accidentally bypassed ALL west tiles.
string(REPLACE "if (!layer->active || screenX < 256)"
  "if (!layer->active || (screenX < 256 &&\n      screenX + (1 << (layer->tileShift ? layer->tileShift : 3)) > 0))"
  _yoshi_ws_source "${_yoshi_ws_source}")
string(FIND "${_yoshi_ws_source}" "${_yoshi_ws_old}" _yoshi_ws_old_at)
if(NOT _yoshi_ws_old_at EQUAL -1)
  string(REPLACE "${_yoshi_ws_old}" "${_yoshi_ws_new}" _yoshi_ws_patched
         "${_yoshi_ws_source}")
  set(_yoshi_ws_source "${_yoshi_ws_patched}")
else()
  string(FIND "${_yoshi_ws_source}" "${_yoshi_ws_new}" _yoshi_ws_new_at)
  if(_yoshi_ws_new_at EQUAL -1)
    message(FATAL_ERROR "Pinned SNESRecomp ws_shadow.c changed; review Yoshi's native-edge patch")
  endif()
endif()
set(_yoshi_write_old [[    uint32_t ty = WorldRowForMapRow(layer, row);
    SetEntry(layer, tx, ty, value);]])
set(_yoshi_write_new [[    uint32_t ty = WorldRowForMapRow(layer, row);
    const char *trace_path = getenv("SNESRECOMP_WS_VRAM_WRITE_TRACE");
    if (trace_path && trace_path[0]) {
      const char *frame_text = getenv("SNESRECOMP_WS_VRAM_WRITE_FRAME");
      extern int snes_frame_counter;
      if (!frame_text || strtoul(frame_text, NULL, 10) ==
                             (unsigned long)snes_frame_counter) {
        FILE *trace = fopen(trace_path, "a");
        if (trace) {
          fprintf(trace, "%d,%d,%04x,%04x,%d,%d,%u,%u\n",
                  snes_frame_counter, i, wordAdr, value, col, row, tx, ty);
          fclose(trace);
        }
      }
    }
    SetEntry(layer, tx, ty, value);]])
string(FIND "${_yoshi_ws_source}" "${_yoshi_write_old}" _yoshi_write_old_at)
if(NOT _yoshi_write_old_at EQUAL -1)
  string(REPLACE "${_yoshi_write_old}" "${_yoshi_write_new}"
         _yoshi_ws_patched "${_yoshi_ws_source}")
  set(_yoshi_ws_source "${_yoshi_ws_patched}")
else()
  string(FIND "${_yoshi_ws_source}" "${_yoshi_write_new}" _yoshi_write_new_at)
  if(_yoshi_write_new_at EQUAL -1)
    message(FATAL_ERROR "Pinned SNESRecomp ws_shadow.c changed; review Yoshi's PPU write trace patch")
  endif()
endif()
set(_yoshi_margin_anchor [[  /* Layered margin sources, exact-first:]])
set(_yoshi_margin_resolver [[  /* Yoshi: BG1 may only use its current-field verified level-map source. */
  if (layerIndex == 0) {
    extern uint16_t YoshiBg1MarginTile(int, uint32_t, uint16_t);
    return YoshiBg1MarginTile(screenX, wrappedY, hScroll);
  }

  /* Layered margin sources, exact-first:]])
string(FIND "${_yoshi_ws_source}" "YoshiBg1MarginTile" _yoshi_margin_at)
if(_yoshi_margin_at EQUAL -1)
  string(FIND "${_yoshi_ws_source}" "${_yoshi_margin_anchor}" _yoshi_margin_at)
  if(_yoshi_margin_at EQUAL -1)
    message(FATAL_ERROR "Pinned ws_shadow.c changed; review BG1 strict source gate")
  endif()
  string(REPLACE "${_yoshi_margin_anchor}" "${_yoshi_margin_resolver}"
    _yoshi_ws_source "${_yoshi_ws_source}")
endif()
file(WRITE "${_yoshi_ws_shadow}" "${_yoshi_ws_source}")
unset(_yoshi_ws_shadow)
unset(_yoshi_ws_source)
unset(_yoshi_ws_old)
unset(_yoshi_ws_new)
unset(_yoshi_ws_patched)
unset(_yoshi_ws_old_at)
unset(_yoshi_ws_new_at)
unset(_yoshi_write_old)
unset(_yoshi_write_new)
unset(_yoshi_write_old_at)
unset(_yoshi_write_new_at)
