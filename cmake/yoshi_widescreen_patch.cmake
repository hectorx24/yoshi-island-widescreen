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
  if (!layer->active || screenX < 256)
    return realTile;]])
string(FIND "${_yoshi_ws_source}" "${_yoshi_ws_old}" _yoshi_ws_old_at)
if(NOT _yoshi_ws_old_at EQUAL -1)
  string(REPLACE "${_yoshi_ws_old}" "${_yoshi_ws_new}" _yoshi_ws_patched
         "${_yoshi_ws_source}")
  file(WRITE "${_yoshi_ws_shadow}" "${_yoshi_ws_patched}")
else()
  string(FIND "${_yoshi_ws_source}" "${_yoshi_ws_new}" _yoshi_ws_new_at)
  if(_yoshi_ws_new_at EQUAL -1)
    message(FATAL_ERROR "Pinned SNESRecomp ws_shadow.c changed; review Yoshi's native-edge patch")
  endif()
endif()
unset(_yoshi_ws_shadow)
unset(_yoshi_ws_source)
unset(_yoshi_ws_old)
unset(_yoshi_ws_new)
unset(_yoshi_ws_patched)
unset(_yoshi_ws_old_at)
unset(_yoshi_ws_new_at)
