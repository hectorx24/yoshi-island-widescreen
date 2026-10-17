#include "presentation.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __ANDROID__
#include <jni.h>
#endif

typedef enum {
    SCENE_TYPE_A_WIDESCREEN = 0,
    SCENE_TYPE_B_BOSS,
    SCENE_TYPE_C_MAP,
    SCENE_TYPE_D_STORYBOOK,
    SCENE_TYPE_E_SYSTEM
} YoshiSceneType;

static atomic_int s_soft, s_lighting, s_configured;
static uint8_t *s_bright;
static size_t s_bright_capacity;
static int s_width, s_height, s_ambient[3] = {255, 252, 248};
static bool s_bright_ready;
static SDL_Texture *s_glow;
static SDL_Renderer *s_renderer;
static int s_glow_width, s_glow_height;
static uint32_t s_source_hash;
static int s_applied;
static uint8_t s_backdrop_pixels[32 * 28 * 4];
static bool s_backdrop_ready;
static bool s_backdrop_linear;
static YoshiSceneType s_current_scene = SCENE_TYPE_A_WIDESCREEN;
static SDL_Texture *s_backdrop_texture;
static SDL_Renderer *s_backdrop_renderer;
extern uint8_t g_ram[0x20000];

/* Scene Policy:
 * Type B: 12 Boss Arenas - nearest-neighbour ~1/8 mosaic of level palette with edge dark vignette (no HUD/boss ghost).
 * Type C: World Map - seamless parchment/ocean pastel extension matching scroll texture (NO blurred full-screen copy).
 * Type D: Storybook/Intro - warm illustrated storybook margin extension.
 * Type E: System screens - clean native black pillars. */
static void GenerateSceneBackdrop(const uint8_t *field, int width, int height, YoshiSceneType scene) {
    if (width != 256 || height != 224 || !field || scene == SCENE_TYPE_E_SYSTEM) {
        s_backdrop_ready = false;
        return;
    }

    if (scene == SCENE_TYPE_B_BOSS) {
        /* Nearest-neighbour ~1/8 mosaic from arena background; skip top 3 rows (HUD). */
        for (int by = 0; by < 28; ++by) {
            const int sample_by = (by < 3) ? 3 : by;
            for (int bx = 0; bx < 32; ++bx) {
                uint32_t sum_b = 0, sum_g = 0, sum_r = 0;
                const int src_y = sample_by * 8;
                const int src_x = bx * 8;
                for (int dy = 0; dy < 8; ++dy) {
                    const uint8_t *row = field + ((src_y + dy) * 256 + src_x) * 4;
                    for (int dx = 0; dx < 8; ++dx) {
                        sum_b += row[dx * 4 + 0];
                        sum_g += row[dx * 4 + 1];
                        sum_r += row[dx * 4 + 2];
                    }
                }
                /* Dark vignette factor toward outer edges for subtle atmospheric depth. */
                float dist_from_center = fabsf((float)bx - 15.5f) / 16.0f;
                float vignette = 0.95f - 0.45f * (dist_from_center * dist_from_center);
                if (vignette < 0.40f) vignette = 0.40f;

                const int out_idx = (by * 32 + bx) * 4;
                s_backdrop_pixels[out_idx + 0] = (uint8_t)(((uint32_t)((sum_b >> 6) * vignette * 210.0f)) >> 8);
                s_backdrop_pixels[out_idx + 1] = (uint8_t)(((uint32_t)((sum_g >> 6) * vignette * 210.0f)) >> 8);
                s_backdrop_pixels[out_idx + 2] = (uint8_t)(((uint32_t)((sum_r >> 6) * vignette * 210.0f)) >> 8);
                s_backdrop_pixels[out_idx + 3] = 255;
            }
        }
        s_backdrop_linear = false; /* Pixelated nearest-neighbour per Plan Maestro */
        s_backdrop_ready = true;
    } else if (scene == SCENE_TYPE_C_MAP) {
        /* World Map: Seamless watercolor ocean parchment extension.
         * Samples non-black scroll and ocean columns to form a natural vertical gradient. */
        uint32_t top_b = 0, top_g = 0, top_r = 0, top_n = 0;
        uint32_t bot_b = 0, bot_g = 0, bot_r = 0, bot_n = 0;
        for (int y = 16; y < 208; ++y) {
            for (int scan_x = 16; scan_x <= 32; scan_x += 8) {
                const uint8_t *l = field + (y * 256 + scan_x) * 4;
                const uint8_t *r = field + (y * 256 + (255 - scan_x)) * 4;
                if ((unsigned)l[0] + l[1] + l[2] > 40) {
                    if (y < 96) { top_b += l[0]; top_g += l[1]; top_r += l[2]; top_n++; }
                    else        { bot_b += l[0]; bot_g += l[1]; bot_r += l[2]; bot_n++; }
                }
                if ((unsigned)r[0] + r[1] + r[2] > 40) {
                    if (y < 96) { top_b += r[0]; top_g += r[1]; top_r += r[2]; top_n++; }
                    else        { bot_b += r[0]; bot_g += r[1]; bot_r += r[2]; bot_n++; }
                }
            }
        }
        const uint8_t t_b = top_n ? (uint8_t)(top_b / top_n) : 190;
        const uint8_t t_g = top_n ? (uint8_t)(top_g / top_n) : 220;
        const uint8_t t_r = top_n ? (uint8_t)(top_r / top_n) : 235;

        const uint8_t b_b = bot_n ? (uint8_t)(bot_b / bot_n) : 180;
        const uint8_t b_g = bot_n ? (uint8_t)(bot_g / bot_n) : 120;
        const uint8_t b_r = bot_n ? (uint8_t)(bot_r / bot_n) : 70;

        for (int by = 0; by < 28; ++by) {
            float t = (float)by / 27.0f;
            int row_b = (int)(t_b * (1.0f - t) + b_b * t);
            int row_g = (int)(t_g * (1.0f - t) + b_g * t);
            int row_r = (int)(t_r * (1.0f - t) + b_r * t);

            for (int bx = 0; bx < 32; ++bx) {
                const int out_idx = (by * 32 + bx) * 4;
                /* Subtle organic watercolor parchment grain */
                int wave = ((bx * 7 + by * 13) % 11) - 5;
                float dist_edge = fabsf((float)bx - 15.5f) / 16.0f;
                float fade = 1.0f - 0.22f * dist_edge;

                int b = (int)((row_b + wave) * fade);
                int g = (int)((row_g + wave) * fade);
                int r = (int)((row_r + wave) * fade);
                s_backdrop_pixels[out_idx + 0] = (uint8_t)(b < 0 ? 0 : (b > 255 ? 255 : b));
                s_backdrop_pixels[out_idx + 1] = (uint8_t)(g < 0 ? 0 : (g > 255 ? 255 : g));
                s_backdrop_pixels[out_idx + 2] = (uint8_t)(r < 0 ? 0 : (r > 255 ? 255 : r));
                s_backdrop_pixels[out_idx + 3] = 255;
            }
        }
        s_backdrop_linear = true;
        s_backdrop_ready = true;
    } else if (scene == SCENE_TYPE_D_STORYBOOK) {
        /* Storybook intro: Warm antique illustrated picture book framing. */
        uint32_t edge_b = 0, edge_g = 0, edge_r = 0, count = 0;
        for (int y = 20; y < 200; ++y) {
            const uint8_t *l = field + (y * 256 + 16) * 4;
            const uint8_t *r = field + (y * 256 + 239) * 4;
            if ((unsigned)l[0] + l[1] + l[2] > 30) {
                edge_b += l[0]; edge_g += l[1]; edge_r += l[2]; count++;
            }
            if ((unsigned)r[0] + r[1] + r[2] > 30) {
                edge_b += r[0]; edge_g += r[1]; edge_r += r[2]; count++;
            }
        }
        uint8_t avg_b = count ? (uint8_t)(edge_b / count) : 48;
        uint8_t avg_g = count ? (uint8_t)(edge_g / count) : 36;
        uint8_t avg_r = count ? (uint8_t)(edge_r / count) : 58;

        for (int by = 0; by < 28; ++by) {
            for (int bx = 0; bx < 32; ++bx) {
                const int out_idx = (by * 32 + bx) * 4;
                float dist_edge = fabsf((float)bx - 15.5f) / 16.0f;
                float fade = 1.0f - 0.35f * dist_edge;
                int wave = ((bx * 5 + by * 11) % 9) - 4;
                int b = (int)((avg_b + wave) * fade);
                int g = (int)((avg_g + wave) * fade);
                int r = (int)((avg_r + wave) * fade);
                s_backdrop_pixels[out_idx + 0] = (uint8_t)(b < 0 ? 0 : (b > 255 ? 255 : b));
                s_backdrop_pixels[out_idx + 1] = (uint8_t)(g < 0 ? 0 : (g > 255 ? 255 : g));
                s_backdrop_pixels[out_idx + 2] = (uint8_t)(r < 0 ? 0 : (r > 255 ? 255 : r));
                s_backdrop_pixels[out_idx + 3] = 255;
            }
        }
        s_backdrop_linear = true;
        s_backdrop_ready = true;
    }
}

static void Configure(int soft, int lighting) {
    atomic_store(&s_soft, soft != 0);
    atomic_store(&s_lighting, lighting != 0);
    atomic_store(&s_configured, 1);
}

static void EnsureSettings(void) {
    if (!atomic_load(&s_configured)) {
        /* "Island Cinema" is the single unified default preset:
         * .10/.80/.10 CRT horizontal blend + contextual 2D lighting + bloom */
        const char *env_soft = getenv("SNESRECOMP_YOSHI_SOFT");
        const char *env_light = getenv("SNESRECOMP_YOSHI_LIGHTING");
        int soft = env_soft ? (atoi(env_soft) != 0) : 1;
        int light = env_light ? (atoi(env_light) != 0) : 1;
        Configure(soft, light);
    }
}

#ifdef __ANDROID__
JNIEXPORT void JNICALL
Java_com_hector_yoshi_YoshiActivity_configurePresentation(
    JNIEnv *env, jclass type, jint soft, jboolean lighting) {
    (void)env; (void)type;
    Configure(soft, lighting);
}

JNIEXPORT void JNICALL
Java_com_hector_yoshi_YoshiActivity_setGamePaused(
    JNIEnv *env, jclass type, jboolean paused) {
    (void)env; (void)type;
    extern void YoshiSetHostPaused(bool paused);
    YoshiSetHostPaused(paused != 0);
}

JNIEXPORT jboolean JNICALL
Java_com_hector_yoshi_YoshiActivity_isGamePaused(
    JNIEnv *env, jclass type) {
    (void)env; (void)type;
    extern bool YoshiGetHostPaused(void);
    return (jboolean)YoshiGetHostPaused();
}

JNIEXPORT void JNICALL
Java_com_hector_yoshi_YoshiActivity_setSpanish(
    JNIEnv *env, jclass type, jboolean enabled) {
    (void)env; (void)type;
    extern void YoshiSetSpanishEnabled(bool enabled);
    YoshiSetSpanishEnabled(enabled != 0);
}

JNIEXPORT jboolean JNICALL
Java_com_hector_yoshi_YoshiActivity_isSpanish(
    JNIEnv *env, jclass type) {
    (void)env; (void)type;
    extern bool YoshiIsSpanishEnabled(void);
    return (jboolean)YoshiIsSpanishEnabled();
}
#endif

int YoshiComposeFrame(uint8_t *dst, size_t pitch, const uint8_t *field,
                       int width, int height, double alpha) {
    (void)dst; (void)pitch; (void)alpha;
    EnsureSettings();
    s_width = width; s_height = height;
    s_bright_ready = false;

    /* Classify scene type according to Plan Maestro */
    if (width > 256) {
        s_current_scene = SCENE_TYPE_A_WIDESCREEN;
        s_backdrop_ready = false;
    } else {
        uint8_t mode = g_ram[0x0118];
        if (mode == 0x03 || mode == 0x04) {
            s_current_scene = SCENE_TYPE_C_MAP;
        } else if (mode == 0x14 || mode == 0x01) {
            s_current_scene = SCENE_TYPE_D_STORYBOOK;
        } else if (mode == 0x0e || mode == 0x0f) {
            s_current_scene = SCENE_TYPE_B_BOSS;
        } else {
            s_current_scene = SCENE_TYPE_E_SYSTEM;
        }

        if (!getenv("SNESRECOMP_NO_THEMED_FALLBACK")) {
            GenerateSceneBackdrop(field, width, height, s_current_scene);
        } else {
            s_backdrop_ready = false;
        }
    }

    /* Contextual 2D Lighting by Scene Profile */
    s_ambient[0] = 255; s_ambient[1] = 252; s_ambient[2] = 248; /* Sunny baseline */
    if (g_ram[0x0118] == 0x0e || g_ram[0x0118] == 0x0f) {
        uint8_t lvl = g_ram[0x021a];
        /* Cave / Underground */
        if (lvl == 2 || lvl == 17 || lvl == 29 || lvl == 38 || lvl == 62) {
            s_ambient[0] = 234; s_ambient[1] = 238; s_ambient[2] = 252;
        }
        /* Castle / Fortress */
        else if (lvl == 3 || lvl == 7 || lvl == 15 || lvl == 19 || lvl == 27 ||
                 lvl == 31 || lvl == 39 || lvl == 43 || lvl == 51 || lvl == 55 ||
                 lvl == 63 || lvl == 67) {
            s_ambient[0] = 230; s_ambient[1] = 226; s_ambient[2] = 234;
        }
        /* Snow / Ice */
        else if (lvl == 48 || lvl == 49 || lvl == 50 || lvl == 65 || lvl == 66) {
            s_ambient[0] = 238; s_ambient[1] = 246; s_ambient[2] = 255;
        }
        /* Twilight / Night / Volcano */
        else if (lvl == 53 || lvl == 54 || lvl == 56 || lvl == 60) {
            s_ambient[0] = 246; s_ambient[1] = 228; s_ambient[2] = 238;
        }
    }

    const size_t size = (size_t)width * height * 4;
    if (getenv("SNESRECOMP_PRESENT_CAPTURE")) {
        s_source_hash = 2166136261u;
        for (size_t i = 0; i < size; ++i)
            s_source_hash = (s_source_hash ^ field[i]) * 16777619u;
    }

    if (!atomic_load(&s_lighting) && !atomic_load(&s_soft) &&
        !getenv("SNESRECOMP_PRESENT_TEST_CYCLE"))
        return 0;

    if (size > s_bright_capacity) {
        uint8_t *next = realloc(s_bright, size);
        if (!next) return 0;
        s_bright = next; s_bright_capacity = size;
    }

    /* Extract genuine high-luminance sources for localized bloom */
    for (size_t i = 0; i < size; i += 4) {
        const unsigned b = field[i], g = field[i + 1], r = field[i + 2];
        const unsigned luminance = (54*r + 183*g + 19*b) >> 8;
        const unsigned light = luminance > 208 ? (luminance - 208) * 255 / 47 : 0;
        s_bright[i] = (uint8_t)(b * light / 255);
        s_bright[i + 1] = (uint8_t)(g * light / 255);
        s_bright[i + 2] = (uint8_t)(r * light / 255);
        s_bright[i + 3] = 0;
    }
    s_bright_ready = true;
    return 0;
}

static void Draw(SDL_Renderer *renderer, SDL_Texture *texture,
                 const SDL_Rect *source, const SDL_Rect *destination,
                 float dx, float dy) {
#if SNESRECOMP_SDL3
    SDL_FRect src = {(float)source->x, (float)source->y, (float)source->w, (float)source->h};
    SDL_FRect dst = {(float)destination->x + dx, (float)destination->y + dy,
                    (float)destination->w, (float)destination->h};
    SDL_RenderTexture(renderer, texture, &src, &dst);
#else
    SDL_Rect dst = *destination;
    dst.x += (int)dx; dst.y += (int)dy;
    snesrecomp_sdl_render_texture(renderer, texture, source, &dst);
#endif
}

static void Mod(SDL_Texture *texture, int r, int g, int b) {
    SDL_SetTextureColorMod(texture, (Uint8)r, (Uint8)g, (Uint8)b);
}

bool YoshiRenderPresentation(SDL_Renderer *renderer, SDL_Texture *texture,
                              const SDL_Rect *source, const SDL_Rect *destination) {
    EnsureSettings();
    const bool soft = atomic_load(&s_soft) != 0;
    const bool lighting = atomic_load(&s_lighting) && s_bright_ready;
    const bool glow = (soft || lighting) && s_bright_ready;
    const int r = lighting ? s_ambient[0] : 255;
    const int g = lighting ? s_ambient[1] : 255;
    const int b = lighting ? s_ambient[2] : 255;
    s_applied = 0;

    int out_w = 0, out_h = 0;
    bool has_pillars = false;
    if (!getenv("SNESRECOMP_NO_THEMED_FALLBACK") && s_backdrop_ready &&
        snesrecomp_sdl_get_render_output_size(renderer, &out_w, &out_h)) {
        if (destination->x > 0 || (destination->x + destination->w) < out_w) {
            has_pillars = true;
        }
    }

    if (has_pillars) {
        if (s_backdrop_renderer != renderer || !s_backdrop_texture) {
            if (s_backdrop_texture) SDL_DestroyTexture(s_backdrop_texture);
            s_backdrop_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                                   SDL_TEXTUREACCESS_STREAMING, 32, 28);
            s_backdrop_renderer = renderer;
        }
        if (s_backdrop_texture) {
            SDL_UpdateTexture(s_backdrop_texture, NULL, s_backdrop_pixels, 32 * 4);
            snesrecomp_sdl_set_texture_linear(s_backdrop_texture, s_backdrop_linear);
#if SNESRECOMP_SDL3
            SDL_FRect bg_src = {0.0f, 0.0f, 32.0f, 28.0f};
            SDL_FRect bg_dst = {0.0f, 0.0f, (float)out_w, (float)out_h};
            SDL_RenderTexture(renderer, s_backdrop_texture, &bg_src, &bg_dst);
#else
            SDL_Rect bg_src = {0, 0, 32, 28};
            SDL_Rect bg_dst = {0, 0, out_w, out_h};
            snesrecomp_sdl_render_texture(renderer, s_backdrop_texture, &bg_src, &bg_dst);
#endif
            s_applied |= 4;
        }
    }

    snesrecomp_sdl_set_texture_linear(texture, soft);
    Mod(texture, 255, 255, 255);
    if (!soft && !lighting) {
        if (has_pillars) {
            Draw(renderer, texture, source, destination, 0, 0);
            return true;
        }
        return false;
    }

    const SDL_BlendMode add = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ZERO, SDL_BLENDOPERATION_ADD);
    if (!SDL_SetTextureBlendMode(texture, add)) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
        snesrecomp_sdl_set_texture_linear(texture, false);
        return false;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);

    const float px = (float)destination->w / source->w;
    const float py = (float)destination->h / source->h;

    /* Island Cinema Preset:
     * .10/.80/.10 horizontal subpixel CRT taps + contextual 2D lighting.
     * No artificial scanlines, phosphor mask, or screen curvature. */
    if (soft) {
        Mod(texture, 204*r/255, 204*g/255, 204*b/255);
        Draw(renderer, texture, source, destination, 0, 0);
        SDL_SetTextureBlendMode(texture, add);
        Mod(texture, 26*r/255, 26*g/255, 26*b/255);
        Draw(renderer, texture, source, destination, -.45f*px, 0);
        Draw(renderer, texture, source, destination, .45f*px, 0);
        s_applied |= 1;
    } else {
        Mod(texture, r, g, b);
        Draw(renderer, texture, source, destination, 0, 0);
    }

    /* Subtle localized bloom on bright light sources */
    if (glow) {
        if (s_renderer != renderer || s_glow_width != s_width || s_glow_height != s_height) {
            if (s_glow) SDL_DestroyTexture(s_glow);
            s_glow = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING, s_width, s_height);
            s_renderer = renderer; s_glow_width = s_width; s_glow_height = s_height;
        }
        if (s_glow && SDL_SetTextureBlendMode(s_glow, add)) {
            SDL_UpdateTexture(s_glow, NULL, s_bright, s_width * 4);
            snesrecomp_sdl_set_texture_linear(s_glow, true);
            Mod(s_glow, lighting ? 5 : 2, lighting ? 5 : 2, lighting ? 5 : 2);
            Draw(renderer, s_glow, source, destination, 0, 0);
            Mod(s_glow, lighting ? 2 : 1, lighting ? 2 : 1, lighting ? 2 : 1);
            Draw(renderer, s_glow, source, destination, -.8f*px, 0);
            Draw(renderer, s_glow, source, destination, .8f*px, 0);
            Draw(renderer, s_glow, source, destination, 0, -.8f*py);
            Draw(renderer, s_glow, source, destination, 0, .8f*py);
            if (lighting) s_applied |= 2;
        }
    }
    Mod(texture, 255, 255, 255);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
    return true;
}

void YoshiPresentationReset(void) {
    if (s_glow) SDL_DestroyTexture(s_glow);
    s_glow = NULL; s_renderer = NULL;
    if (s_backdrop_texture) SDL_DestroyTexture(s_backdrop_texture);
    s_backdrop_texture = NULL; s_backdrop_renderer = NULL;
    s_backdrop_ready = false;
    free(s_bright); s_bright = NULL; s_bright_capacity = 0;
    s_bright_ready = false;
}

#if SNESRECOMP_SDL3
static bool Capture(SDL_Renderer *renderer, const char *path, int width, int height, const SDL_Rect *destination) {
    extern int snes_frame_counter;
    SDL_Surface *read = SDL_RenderReadPixels(renderer, NULL);
    SDL_Surface *pixels = read ? SDL_ConvertSurface(read, SDL_PIXELFORMAT_ARGB8888) : NULL;
    if (!pixels) { if (read) SDL_DestroySurface(read); return false; }
    bool success = false;
    FILE *file = fopen(path, "wb");
    if (file) {
        for (int y = 0; y < pixels->h; ++y)
            fwrite((uint8_t *)pixels->pixels + y * pixels->pitch, (size_t)pixels->w * 4, 1, file);
        fclose(file);
        char metadata[4096];
        snprintf(metadata, sizeof(metadata), "%s.json", path);
        file = fopen(metadata, "w");
        if (file) {
            fprintf(file, "{\"frame\":%d,\"width\":%d,\"height\":%d,\"source_width\":%d,"
                "\"source_height\":%d,\"source_fnv\":\"%08x\",\"applied\":%d,\"viewport\":[%d,%d,%d,%d]}\n",
                snes_frame_counter, pixels->w, pixels->h, width, height, s_source_hash, s_applied,
                destination->x, destination->y, destination->w, destination->h);
            fclose(file);
            success = true;
        }
    }
    SDL_DestroySurface(pixels); SDL_DestroySurface(read);
    return success;
}
#endif

void YoshiCapturePresentation(SDL_Renderer *renderer, SDL_Texture *texture,
                               const SDL_Rect *source, const SDL_Rect *destination) {
#if SNESRECOMP_SDL3
    static bool captured;
    const char *path = getenv("SNESRECOMP_PRESENT_CAPTURE");
    const char *number = getenv("SNESRECOMP_PRESENT_FRAME");
    extern int snes_frame_counter;
    if (captured || !path || !number || snes_frame_counter < atoi(number)) return;
    if (!getenv("SNESRECOMP_PRESENT_TEST_CYCLE")) {
        captured = Capture(renderer, path, source->w, source->h, destination);
        return;
    }
    const int old_soft = atomic_load(&s_soft), old_lighting = atomic_load(&s_lighting);
    const char *names[] = {"sharp", "soft", "lighting"};
    bool success = true;
    for (int i = 0; i < 3; ++i) {
        Configure(i == 1, i == 2);
        SDL_RenderClear(renderer);
        if (!YoshiRenderPresentation(renderer, texture, source, destination))
            Draw(renderer, texture, source, destination, 0, 0);
        char variant[4096];
        snprintf(variant, sizeof(variant), "%s.%s", path, names[i]);
        success = Capture(renderer, variant, source->w, source->h, destination) && success;
    }
    Configure(old_soft, old_lighting);
    SDL_RenderClear(renderer);
    if (!YoshiRenderPresentation(renderer, texture, source, destination))
        Draw(renderer, texture, source, destination, 0, 0);
    captured = success;
#else
    (void)renderer; (void)texture; (void)source; (void)destination;
#endif
}
