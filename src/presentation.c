#include "presentation.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __ANDROID__
#include <jni.h>
#endif

static atomic_int s_soft, s_lighting, s_configured;
static uint8_t *s_bright;
static size_t s_bright_capacity;
static int s_width, s_height, s_ambient[3] = {255, 255, 255};
static bool s_bright_ready;
static SDL_Texture *s_glow;
static SDL_Renderer *s_renderer;
static int s_glow_width, s_glow_height;
static uint32_t s_source_hash;
static int s_applied;
static uint8_t s_backdrop_pixels[32 * 28 * 4];
static bool s_backdrop_ready;
static SDL_Texture *s_backdrop_texture;
static SDL_Renderer *s_backdrop_renderer;
extern uint8_t g_ram[0x20000];

static void DownsampleBackdrop(const uint8_t *field, int width, int height) {
    if (width != 256 || height != 224 || !field) {
        s_backdrop_ready = false;
        return;
    }
    for (int by = 0; by < 28; ++by) {
        for (int bx = 0; bx < 32; ++bx) {
            uint32_t sum_b = 0, sum_g = 0, sum_r = 0;
            const int src_y = by * 8;
            const int src_x = bx * 8;
            for (int dy = 0; dy < 8; ++dy) {
                const uint8_t *row = field + ((src_y + dy) * 256 + src_x) * 4;
                for (int dx = 0; dx < 8; ++dx) {
                    sum_b += row[dx * 4 + 0];
                    sum_g += row[dx * 4 + 1];
                    sum_r += row[dx * 4 + 2];
                }
            }
            const int out_idx = (by * 32 + bx) * 4;
            s_backdrop_pixels[out_idx + 0] = (uint8_t)((sum_b * 115u) >> 14);
            s_backdrop_pixels[out_idx + 1] = (uint8_t)((sum_g * 115u) >> 14);
            s_backdrop_pixels[out_idx + 2] = (uint8_t)((sum_r * 115u) >> 14);
            s_backdrop_pixels[out_idx + 3] = 255;
        }
    }
    s_backdrop_ready = true;
}

static void Configure(int soft, int lighting) {
    atomic_store(&s_soft, soft != 0);
    atomic_store(&s_lighting, lighting != 0);
    atomic_store(&s_configured, 1);
}
static void EnsureSettings(void) {
    if (!atomic_load(&s_configured))
        Configure(getenv("SNESRECOMP_YOSHI_SOFT") != NULL,
                  getenv("SNESRECOMP_YOSHI_LIGHTING") != NULL);
}
#ifdef __ANDROID__
JNIEXPORT void JNICALL
Java_com_hector_yoshi_YoshiActivity_configurePresentation(
    JNIEnv *env, jclass type, jint soft, jboolean lighting) {
    (void)env; (void)type;
    Configure(soft, lighting);
}
#endif

/* The PPU field is never edited. Bright-pass extraction is small (one native
 * 224-line image); filtering, scaling, color blending and glow run on SDL's
 * GPU renderer after the original frame is composed. */
int YoshiComposeFrame(uint8_t *dst, size_t pitch, const uint8_t *field,
                       int width, int height, double alpha) {
    (void)dst; (void)pitch; (void)alpha;
    EnsureSettings();
    s_width = width; s_height = height;
    s_bright_ready = false;
    s_ambient[0] = s_ambient[1] = s_ambient[2] = 255;
    const size_t size = (size_t)width * height * 4;
    if (getenv("SNESRECOMP_PRESENT_CAPTURE")) {
        s_source_hash = 2166136261u;
        for (size_t i = 0; i < size; ++i)
            s_source_hash = (s_source_hash ^ field[i]) * 16777619u;
    }
    if (width == 256 && height == 224 && !getenv("SNESRECOMP_NO_THEMED_FALLBACK")) {
        DownsampleBackdrop(field, width, height);
    } else {
        s_backdrop_ready = false;
    }
    if ((!atomic_load(&s_lighting) && !atomic_load(&s_soft) &&
         !getenv("SNESRECOMP_PRESENT_TEST_CYCLE")) ||
        (g_ram[0x118] != 0x0e && g_ram[0x118] != 0x0f))
        return 0;
    if (size > s_bright_capacity) {
        uint8_t *next = realloc(s_bright, size);
        if (!next) return 0;
        s_bright = next; s_bright_capacity = size;
    }
    uint64_t red = 0, blue = 0;
    for (size_t i = 0; i < size; i += 4) {
        const unsigned b = field[i], g = field[i + 1], r = field[i + 2];
        const unsigned luminance = (54*r + 183*g + 19*b) >> 8;
        const unsigned light = luminance > 208 ? (luminance - 208) * 255 / 47 : 0;
        s_bright[i] = (uint8_t)(b * light / 255);
        s_bright[i + 1] = (uint8_t)(g * light / 255);
        s_bright[i + 2] = (uint8_t)(r * light / 255);
        s_bright[i + 3] = 0;
        red += r; blue += b;
    }
    const uint64_t pixels = size / 4;
    if (blue > red + pixels * 10) s_ambient[0] = s_ambient[1] = 253;
    else if (red > blue + pixels * 10) { s_ambient[1] = 254; s_ambient[2] = 252; }
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
            snesrecomp_sdl_set_texture_linear(s_backdrop_texture, true);
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
    /* The CC0 shared Warm Composite reference uses .18/.64/.18 horizontal
     * taps. Reduce to .10/.80/.10 subpixel taps for gentler softness; no scanlines,
     * curvature, phosphor mask or border. Texture alpha in SNES frames is 0,
     * so custom ONE+ONE blending is intentional. */
    if (soft) {
        Mod(texture, 208*r/255, 204*g/255, 198*b/255);
        Draw(renderer, texture, source, destination, 0, 0);
        SDL_SetTextureBlendMode(texture, add);
        Mod(texture, 26*r/255, 25*g/255, 25*b/255);
        Draw(renderer, texture, source, destination, -.45f*px, 0);
        Draw(renderer, texture, source, destination, .45f*px, 0);
        s_applied |= 1;
    } else {
        Mod(texture, r, g, b);
        Draw(renderer, texture, source, destination, 0, 0);
    }
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

/* One-shot GPU readback only when explicitly requested by the automated test.
 * Cycle all three choices on ONE completed PPU field: no simulation advances. */
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
