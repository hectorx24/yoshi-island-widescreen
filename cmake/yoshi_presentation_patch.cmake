# Title-owned SDL GPU post-processing; native PPU/captures remain untouched.
set(_yoshi_present_host "${SNESRECOMP_ROOT}/runner/src/desktop/host_main.c")
file(READ "${_yoshi_present_host}" _yoshi_present_source)
set(_yoshi_present_old [[  snesrecomp_sdl_render_texture(g_renderer, g_texture, &g_sdl_renderer_rect,
                                &g_sdl_present_rect);
  SDL_RenderPresent(g_renderer);]])
set(_yoshi_present_new [[  extern bool YoshiRenderPresentation(SDL_Renderer *, SDL_Texture *, const SDL_Rect *, const SDL_Rect *);
  extern void YoshiCapturePresentation(SDL_Renderer *, SDL_Texture *, const SDL_Rect *, const SDL_Rect *);
  if (!YoshiRenderPresentation(g_renderer, g_texture, &g_sdl_renderer_rect, &g_sdl_present_rect))
    snesrecomp_sdl_render_texture(g_renderer, g_texture, &g_sdl_renderer_rect,
                                  &g_sdl_present_rect);
  YoshiCapturePresentation(g_renderer, g_texture, &g_sdl_renderer_rect, &g_sdl_present_rect);
  SDL_RenderPresent(g_renderer);]])
string(FIND "${_yoshi_present_source}" "YoshiRenderPresentation" _yoshi_present_at)
if(_yoshi_present_at EQUAL -1)
  string(FIND "${_yoshi_present_source}" "${_yoshi_present_old}" _yoshi_present_at)
  if(_yoshi_present_at EQUAL -1)
    message(FATAL_ERROR "Pinned SDL presenter changed; review Yoshi GPU hook")
  endif()
  string(REPLACE "${_yoshi_present_old}" "${_yoshi_present_new}"
    _yoshi_present_source "${_yoshi_present_source}")
  string(REPLACE "static void SdlRenderer_Destroy(void) {"
    "static void SdlRenderer_Destroy(void) {\n  extern void YoshiPresentationReset(void);\n  YoshiPresentationReset();"
    _yoshi_present_source "${_yoshi_present_source}")
  file(WRITE "${_yoshi_present_host}" "${_yoshi_present_source}")
endif()
