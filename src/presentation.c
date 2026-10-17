#include "presentation.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __ANDROID__
#include <jni.h>
#include <android/log.h>
#define LOG_25D(...) __android_log_print(ANDROID_LOG_INFO, "Yoshi25D", __VA_ARGS__)
#else
#define LOG_25D(...) fprintf(stderr, __VA_ARGS__)
#endif

extern uint8_t g_yoshi_layer_mask[224][512];
extern int g_yoshi_layer_mask_w;

/* Lighting proof counters (Lighting Proof L1) */
static uint32_t s_lighting_pass_count = 0;
static uint32_t s_shadow_pass_count = 0;
static uint32_t s_bloom_pass_count = 0;
static uint32_t s_dof_pass_count = 0;
static uint32_t s_particle_pass_count = 0;
static uint32_t s_last_log_frame = 0;

typedef enum {
    MAT_NONE = 0,
    MAT_SKY,
    MAT_MOUNTAIN,
    MAT_TERRAIN_GRASS,
    MAT_TERRAIN_ROCK,
    MAT_WATER,
    MAT_LAVA,
    MAT_ACTOR,
    MAT_HUD
} MaterialType;

static uint8_t s_mat_buf[224 * 512];
static float s_nx_buf[224 * 512];
static float s_ny_buf[224 * 512];
static float s_nz_buf[224 * 512];

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
static uint8_t s_raw_field[512 * 224 * 4];
static uint8_t s_composed_frame[512 * 224 * 4];
static uint8_t s_backdrop_pixels[32 * 28 * 4];
static bool s_backdrop_ready;
static bool s_backdrop_linear;
static YoshiSceneType s_current_scene = SCENE_TYPE_A_WIDESCREEN;
static SDL_Texture *s_backdrop_texture;
static SDL_Renderer *s_backdrop_renderer;
extern uint8_t g_ram[0x20000];

#define MAX_VISUAL_PARTICLES 64
typedef enum {
    PARTICLE_DUST = 0,
    PARTICLE_WATER_SPLASH,
    PARTICLE_WATER_RIPPLE,
    PARTICLE_EMBER,
    PARTICLE_SPARKLE,
    PARTICLE_SNOW
} ParticleType;

typedef struct {
    float x, y;
    float vx, vy;
    float life, max_life;
    uint8_t r, g, b, a;
    float size;
    ParticleType type;
    bool active;
} VisualParticle;

static VisualParticle s_particles[MAX_VISUAL_PARTICLES];
static uint32_t s_visual_rng = 0x1337BEEF;
static uint8_t s_prev_action = 0;
static uint8_t s_prev_yspeed = 0;
static uint8_t s_prev_stars = 0;
static uint8_t s_prev_coins = 0;
static unsigned s_particle_frame = 0;

static inline float VisualRand(void) {
    s_visual_rng = s_visual_rng * 1664525u + 1013904223u;
    return (float)(s_visual_rng & 0xFFFF) / 65535.0f;
}

static void SpawnParticle(float x, float y, float vx, float vy, float life,
                          uint8_t r, uint8_t g, uint8_t b, uint8_t a, float size, ParticleType type) {
    for (int i = 0; i < MAX_VISUAL_PARTICLES; ++i) {
        if (!s_particles[i].active) {
            s_particles[i].x = x;
            s_particles[i].y = y;
            s_particles[i].vx = vx;
            s_particles[i].vy = vy;
            s_particles[i].life = life;
            s_particles[i].max_life = life;
            s_particles[i].r = r;
            s_particles[i].g = g;
            s_particles[i].b = b;
            s_particles[i].a = a;
            s_particles[i].size = size;
            s_particles[i].type = type;
            s_particles[i].active = true;
            break;
        }
    }
}

/* Scene Policy:
 * Type B: 12 Boss Arenas - nearest-neighbour ~1/8 mosaic of level palette with edge dark vignette (no HUD/boss ghost).
 * Type C: World Map - seamless parchment/ocean pastel extension matching scroll texture (NO blurred full-screen copy).
 * Type D: Storybook/Intro/Title - warm illustrated storybook margin extension with vertical gradient tracking.
 * Type E: System screens - clean native black pillars. */
static void GenerateSceneBackdrop(const uint8_t *field, int width, int height, YoshiSceneType scene) {
    if (width != 256 || height != 224 || !field || scene == SCENE_TYPE_E_SYSTEM) {
        s_backdrop_ready = false;
        return;
    }

    if (scene == SCENE_TYPE_B_BOSS) {
        /* Nearest-neighbour ~1/8 mosaic exclusively from outer arena walls (0..31 on left, 224..255 on right).
         * This strictly guarantees ZERO boss, player, or sprite pixels ever bleed into lateral margins. */
        for (int by = 0; by < 28; ++by) {
            const int sample_by = (by < 3) ? 3 : by;
            for (int bx = 0; bx < 32; ++bx) {
                uint32_t sum_b = 0, sum_g = 0, sum_r = 0;
                const int src_y = sample_by * 8;
                const int src_x = (bx < 16) ? ((bx % 4) * 8) : (224 + ((bx - 16) % 4) * 8);
                for (int dy = 0; dy < 8; ++dy) {
                    const uint8_t *row = field + ((src_y + dy) * 256 + src_x) * 4;
                    for (int dx = 0; dx < 8; ++dx) {
                        sum_b += row[dx * 4 + 0];
                        sum_g += row[dx * 4 + 1];
                        sum_r += row[dx * 4 + 2];
                    }
                }
                /* Dark vignette factor toward outer edges for atmospheric dungeon depth. */
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
        s_backdrop_linear = false; /* Pixelated nearest-neighbour 16-bit mosaic */
        s_backdrop_ready = true;
    } else if (scene == SCENE_TYPE_C_MAP) {
        /* World Map: Authentic 16-bit pixelated ocean parchment extension.
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
        s_backdrop_linear = false; /* Pixelated nearest-neighbour 16-bit mosaic */
        s_backdrop_ready = true;
    } else if (scene == SCENE_TYPE_D_STORYBOOK) {
        /* Storybook intro / Title / Cinematic / Menus: Warm illustrated margin extension in 16-bit pixel style. */
        uint32_t total_b = 0, total_g = 0, total_r = 0, total_n = 0;
        for (int y = 16; y < 208; y += 4) {
            const uint8_t *l = field + (y * 256 + 16) * 4;
            const uint8_t *r = field + (y * 256 + 239) * 4;
            if ((unsigned)l[0] + l[1] + l[2] > 20) {
                total_b += l[0]; total_g += l[1]; total_r += l[2]; total_n++;
            }
            if ((unsigned)r[0] + r[1] + r[2] > 20) {
                total_b += r[0]; total_g += r[1]; total_r += r[2]; total_n++;
            }
        }
        const uint8_t fallback_b = total_n ? (uint8_t)(total_b / total_n) : 48;
        const uint8_t fallback_g = total_n ? (uint8_t)(total_g / total_n) : 36;
        const uint8_t fallback_r = total_n ? (uint8_t)(total_r / total_n) : 58;

        for (int by = 0; by < 28; ++by) {
            uint32_t row_b = 0, row_g = 0, row_r = 0, row_n = 0;
            const int sample_y = by * 8 + 4;
            for (int dx = 12; dx <= 24; dx += 6) {
                const uint8_t *l = field + (sample_y * 256 + dx) * 4;
                const uint8_t *r = field + (sample_y * 256 + (255 - dx)) * 4;
                if ((unsigned)l[0] + l[1] + l[2] > 24) {
                    row_b += l[0]; row_g += l[1]; row_r += l[2]; row_n++;
                }
                if ((unsigned)r[0] + r[1] + r[2] > 24) {
                    row_b += r[0]; row_g += r[1]; row_r += r[2]; row_n++;
                }
            }
            const uint8_t cur_b = row_n ? (uint8_t)(row_b / row_n) : fallback_b;
            const uint8_t cur_g = row_n ? (uint8_t)(row_g / row_n) : fallback_g;
            const uint8_t cur_r = row_n ? (uint8_t)(row_r / row_n) : fallback_r;

            for (int bx = 0; bx < 32; ++bx) {
                const int out_idx = (by * 32 + bx) * 4;
                float dist_edge = fabsf((float)bx - 15.5f) / 16.0f;
                float fade = 1.0f - 0.28f * dist_edge;
                int wave = ((bx * 5 + by * 11) % 9) - 4;
                int b = (int)((cur_b + wave) * fade);
                int g = (int)((cur_g + wave) * fade);
                int r = (int)((cur_r + wave) * fade);
                s_backdrop_pixels[out_idx + 0] = (uint8_t)(b < 0 ? 0 : (b > 255 ? 255 : b));
                s_backdrop_pixels[out_idx + 1] = (uint8_t)(g < 0 ? 0 : (g > 255 ? 255 : g));
                s_backdrop_pixels[out_idx + 2] = (uint8_t)(r < 0 ? 0 : (r > 255 ? 255 : r));
                s_backdrop_pixels[out_idx + 3] = 255;
            }
        }
        s_backdrop_linear = false; /* Pixelated nearest-neighbour 16-bit mosaic */
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

static inline uint8_t ClampU8(int v) {
    return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

static inline int PixelLuminance(uint8_t r, uint8_t g, uint8_t b) {
    return (54 * (int)r + 183 * (int)g + 19 * (int)b) >> 8;
}

int YoshiComposeFrame(uint8_t *dst, size_t pitch, const uint8_t *field,
                       int width, int height, double alpha) {
    (void)alpha;
    EnsureSettings();
    s_width = width; s_height = height;
    s_bright_ready = false;

    if ((size_t)width * height * 4 <= sizeof(s_raw_field)) {
        memcpy(s_raw_field, field, (size_t)width * height * 4);
    }

    /* Classify scene type according to Plan Maestro */
    if (width > 256) {
        s_current_scene = SCENE_TYPE_A_WIDESCREEN;
        s_backdrop_ready = false;
    } else {
        uint8_t mode = g_ram[0x0118];
        if (mode == 0x03 || mode == 0x04) {
            s_current_scene = SCENE_TYPE_C_MAP;
        } else if (mode == 0x14 || mode == 0x01 || mode == 0x00 || mode == 0x02 || mode == 0x05) {
            s_current_scene = SCENE_TYPE_D_STORYBOOK;
        } else if (mode == 0x0e || mode == 0x0f) {
            s_current_scene = SCENE_TYPE_B_BOSS;
        } else {
            s_current_scene = SCENE_TYPE_D_STORYBOOK;
        }

        if (!getenv("SNESRECOMP_NO_THEMED_FALLBACK")) {
            GenerateSceneBackdrop(field, width, height, s_current_scene);
        } else {
            s_backdrop_ready = false;
        }
    }

    /* Directional Light Vector and Ambient per Biome */
    float lx = 0.55f, ly = -0.65f, lz = 0.52f; /* Sunny baseline */
    s_ambient[0] = 255; s_ambient[1] = 252; s_ambient[2] = 248;
    uint8_t lvl = g_ram[0x021a];

    if (g_ram[0x0118] == 0x0e || g_ram[0x0118] == 0x0f) {
        /* Cave / Underground */
        if (lvl == 2 || lvl == 17 || lvl == 29 || lvl == 38 || lvl == 62) {
            lx = 0.10f; ly = -0.40f; lz = 0.91f;
            s_ambient[0] = 218; s_ambient[1] = 224; s_ambient[2] = 245;
        }
        /* Castle / Fortress / Torchlit */
        else if (lvl == 3 || lvl == 7 || lvl == 15 || lvl == 19 || lvl == 27 ||
                 lvl == 31 || lvl == 39 || lvl == 43 || lvl == 51 || lvl == 55 ||
                 lvl == 63 || lvl == 67) {
            lx = 0.35f; ly = -0.55f; lz = 0.76f;
            s_ambient[0] = 245; s_ambient[1] = 220; s_ambient[2] = 195;
        }
        /* Snow / Ice */
        else if (lvl == 48 || lvl == 49 || lvl == 50 || lvl == 65 || lvl == 66) {
            lx = -0.45f; ly = -0.65f; lz = 0.61f;
            s_ambient[0] = 235; s_ambient[1] = 245; s_ambient[2] = 255;
        }
        /* Forest / Jungle / Rain */
        else if (lvl == 12 || lvl == 14 || lvl == 25 || lvl == 40) {
            lx = 0.40f; ly = -0.60f; lz = 0.69f;
            s_ambient[0] = 232; s_ambient[1] = 248; s_ambient[2] = 236;
        }
        /* Twilight / Volcano / Lava (lit upwards from molten magma) */
        else if (lvl == 53 || lvl == 54 || lvl == 56 || lvl == 60) {
            lx = 0.0f; ly = 0.70f; lz = 0.71f;
            s_ambient[0] = 255; s_ambient[1] = 215; s_ambient[2] = 185;
        }

        /* 6 Families of Presentation Particles */
        s_particle_frame++;
        uint8_t action = g_ram[0x0378];
        uint8_t yspeed = g_ram[0x0388];
        float yx = (float)g_ram[0x00a0];
        float yy = (float)g_ram[0x00a2] + 28.0f;

        // 1. Landing dust puff
        if (s_prev_yspeed > 2 && yspeed == 0 && action == 0) {
            float spread = (float)s_prev_yspeed * 0.12f;
            SpawnParticle(yx - 4.0f, yy, -0.5f - spread, -0.3f, 0.35f, 220, 210, 190, 180, 2.0f, PARTICLE_DUST);
            SpawnParticle(yx + 4.0f, yy,  0.5f + spread, -0.3f, 0.35f, 220, 210, 190, 180, 2.0f, PARTICLE_DUST);
        }

        // 2. Jump takeoff dust puff
        if (s_prev_action == 0 && action == 2) {
            SpawnParticle(yx - 3.0f, yy, -0.4f, -0.2f, 0.25f, 215, 205, 185, 150, 1.6f, PARTICLE_DUST);
            SpawnParticle(yx + 3.0f, yy,  0.4f, -0.2f, 0.25f, 215, 205, 185, 150, 1.6f, PARTICLE_DUST);
        }

        // 3. Running sprint dust
        if (action == 1 && (s_particle_frame % 16 == 0)) {
            SpawnParticle(yx + (VisualRand() - 0.5f) * 6.0f, yy, (VisualRand() - 0.5f) * 0.4f, -0.2f, 0.25f, 215, 205, 185, 130, 1.4f, PARTICLE_DUST);
        }

        // 4. Water splashes & ripples
        if (lvl == 12 || lvl == 14 || lvl == 25 || lvl == 40 || lvl == 44) {
            if (action == 4 || (s_particle_frame % 30 == 0 && yy > 160.0f)) {
                SpawnParticle(yx, yy > 170.0f ? yy - 6.0f : yy, 0.0f, 0.0f, 0.6f, 180, 225, 255, 140, 3.0f, PARTICLE_WATER_RIPPLE);
            }
            if (s_prev_action != 4 && action == 4) {
                for (int d = -2; d <= 2; ++d) {
                    SpawnParticle(yx + (float)d * 3.0f, yy - 4.0f, (float)d * 0.35f, -0.8f - VisualRand() * 0.4f,
                                  0.45f, 160, 220, 255, 200, 1.8f, PARTICLE_WATER_SPLASH);
                }
            }
        }

        // 5. Floating embers in volcanic stages
        if ((lvl == 53 || lvl == 54 || lvl == 56 || lvl == 60) && (s_particle_frame % 20 == 0)) {
            float ex = VisualRand() * (float)width;
            SpawnParticle(ex, 220.0f, (VisualRand() - 0.5f) * 0.35f, -0.7f - VisualRand() * 0.5f,
                          1.3f, 255, 130 + (uint8_t)(VisualRand() * 60), 30, 210, 1.6f, PARTICLE_EMBER);
        }

        // 6. Snowflakes in snowy mountains (World 5)
        if ((lvl == 48 || lvl == 49 || lvl == 50 || lvl == 65 || lvl == 66) && (s_particle_frame % 8 == 0)) {
            float sx = VisualRand() * (float)width;
            SpawnParticle(sx, 0.0f, -0.3f + VisualRand() * 0.2f, 0.7f + VisualRand() * 0.4f,
                          2.5f, 240, 248, 255, 190, 1.5f, PARTICLE_SNOW);
        }

        // Collectible star & coin sparkles
        uint8_t cur_stars = g_ram[0x03b6];
        uint8_t cur_coins = g_ram[0x037b];
        if (s_prev_stars != 0 && cur_stars > s_prev_stars) {
            for (int s = 0; s < 4; ++s) {
                float angle = (float)s * 1.570796f + VisualRand() * 0.5f;
                SpawnParticle(yx, yy - 14.0f, cosf(angle) * 0.6f, sinf(angle) * 0.6f,
                              0.4f, 255, 230, 60, 220, 2.2f, PARTICLE_SPARKLE);
            }
        }
        if (s_prev_coins != 0 && cur_coins > s_prev_coins) {
            SpawnParticle(yx + (VisualRand() - 0.5f) * 8.0f, yy - 12.0f, 0.0f, -0.6f,
                          0.35f, 255, 220, 80, 200, 1.8f, PARTICLE_SPARKLE);
        }

        s_prev_action = action;
        s_prev_yspeed = yspeed;
        s_prev_stars = cur_stars;
        s_prev_coins = cur_coins;
    }

    const size_t size = (size_t)width * height * 4;
    if (getenv("SNESRECOMP_PRESENT_CAPTURE")) {
        s_source_hash = 2166136261u;
        for (size_t i = 0; i < size; ++i)
            s_source_hash = (s_source_hash ^ field[i]) * 16777619u;
    }

    /* Extract emissive sources for bloom */
    if (size > s_bright_capacity) {
        uint8_t *next = realloc(s_bright, size);
        if (next) { s_bright = next; s_bright_capacity = size; }
    }
    if (s_bright) {
        for (size_t i = 0; i < size; i += 4) {
            const unsigned b = field[i], g = field[i + 1], r = field[i + 2];
            const unsigned luminance = (54*r + 183*g + 19*b) >> 8;
            unsigned light = 0;
            if (r > 195 && r > (b + 35) && luminance > 135) {
                light = (r - 180) * 255 / 75;
            } else if (r > 200 && g > 170 && b < 140) {
                light = (luminance > 175) ? (luminance - 175) * 255 / 80 : 0;
            } else if (luminance > 250) {
                light = (luminance - 250) * 255 / 25;
            }
            if (light > 255) light = 255;
            s_bright[i] = (uint8_t)(b * light / 255);
            s_bright[i + 1] = (uint8_t)(g * light / 255);
            s_bright[i + 2] = (uint8_t)(r * light / 255);
            s_bright[i + 3] = 0;
        }
        s_bright_ready = true;
    }

    /* Check if remaster lighting is enabled */
    if (!atomic_load(&s_lighting)) {
        /* Base unenhanced fallback: direct copy into texture surface */
        for (int y = 0; y < height; ++y) {
            memcpy(dst + (size_t)y * pitch, field + (size_t)y * width * 4, (size_t)width * 4);
        }
        return 0;
    }

    /* Increment execution proof counters (Lighting Proof L1) */
    s_lighting_pass_count++;
    s_shadow_pass_count++;
    s_bloom_pass_count++;
    s_dof_pass_count++;
    s_particle_pass_count++;

    /* Pass 1: Classify Materials and Compute Normals */
    int safe_w = width > 512 ? 512 : width;
    for (int y = 0; y < height; ++y) {
        const uint8_t *row_field = field + y * width * 4;
        for (int x = 0; x < safe_w; ++x) {
            int idx = y * safe_w + x;
            const uint8_t b = row_field[x * 4 + 0];
            const uint8_t g = row_field[x * 4 + 1];
            const uint8_t r = row_field[x * 4 + 2];
            uint8_t layer = (x < g_yoshi_layer_mask_w) ? g_yoshi_layer_mask[y][x] : 0;

            MaterialType mat = MAT_TERRAIN_ROCK;
            if (y < 32 && (layer == 2 || (layer == 4 && x > safe_w - 75))) {
                mat = MAT_HUD;
            } else if (layer == 5) {
                mat = MAT_SKY;
            } else if (layer == 2) {
                mat = (y < 110) ? MAT_SKY : MAT_MOUNTAIN;
            } else if (layer == 1) {
                mat = MAT_MOUNTAIN;
            } else if (layer == 4) {
                if (r > 210 && g > 180 && b < 80) mat = MAT_LAVA; /* Collectible star/coin emissive */
                else mat = MAT_ACTOR;
            } else {
                if (r > 190 && r > (b + 60) && g < 140) mat = MAT_LAVA;
                else if (b > r + 30 && b > g) mat = MAT_WATER;
                else if (g > r + 10 && g > b + 10) mat = MAT_TERRAIN_GRASS;
                else mat = MAT_TERRAIN_ROCK;
            }
            s_mat_buf[idx] = (uint8_t)mat;

            /* Compute Normals */
            if (mat == MAT_TERRAIN_GRASS || mat == MAT_TERRAIN_ROCK) {
                int ym1 = (y > 0) ? y - 1 : 0;
                int yp1 = (y < height - 1) ? y + 1 : height - 1;
                int xm1 = (x > 0) ? x - 1 : 0;
                int xp1 = (x < safe_w - 1) ? x + 1 : safe_w - 1;

                int lt = PixelLuminance(field[(ym1 * width + xm1) * 4 + 2], field[(ym1 * width + xm1) * 4 + 1], field[(ym1 * width + xm1) * 4 + 0]);
                int t  = PixelLuminance(field[(ym1 * width + x  ) * 4 + 2], field[(ym1 * width + x  ) * 4 + 1], field[(ym1 * width + x  ) * 4 + 0]);
                int rt = PixelLuminance(field[(ym1 * width + xp1) * 4 + 2], field[(ym1 * width + xp1) * 4 + 1], field[(ym1 * width + xp1) * 4 + 0]);
                int l  = PixelLuminance(field[(y   * width + xm1) * 4 + 2], field[(y   * width + xm1) * 4 + 1], field[(y   * width + xm1) * 4 + 0]);
                int r_ = PixelLuminance(field[(y   * width + xp1) * 4 + 2], field[(y   * width + xp1) * 4 + 1], field[(y   * width + xp1) * 4 + 0]);
                int lb = PixelLuminance(field[(yp1 * width + xm1) * 4 + 2], field[(yp1 * width + xm1) * 4 + 1], field[(yp1 * width + xm1) * 4 + 0]);
                int b_ = PixelLuminance(field[(yp1 * width + x  ) * 4 + 2], field[(yp1 * width + x  ) * 4 + 1], field[(yp1 * width + x  ) * 4 + 0]);
                int rb = PixelLuminance(field[(yp1 * width + xp1) * 4 + 2], field[(yp1 * width + xp1) * 4 + 1], field[(yp1 * width + xp1) * 4 + 0]);

                int gx = (rt + 2 * r_ + rb) - (lt + 2 * l + lb);
                int gy = (lb + 2 * b_ + rb) - (lt + 2 * t + rt);

                float nx = -gx * 0.0075f;
                float ny = -gy * 0.0075f;
                if (nx < -0.85f) nx = -0.85f; else if (nx > 0.85f) nx = 0.85f;
                if (ny < -0.85f) ny = -0.85f; else if (ny > 0.85f) ny = 0.85f;
                float nz2 = 1.0f - nx * nx - ny * ny;
                s_nx_buf[idx] = nx;
                s_ny_buf[idx] = ny;
                s_nz_buf[idx] = (nz2 > 0.1f) ? sqrtf(nz2) : 0.316f;
            } else if (mat == MAT_ACTOR) {
                float u = ((float)(x % 16) - 7.5f) / 8.0f;
                if (u < -1.0f) u = -1.0f; else if (u > 1.0f) u = 1.0f;
                float nx = u * 0.80f;
                s_nx_buf[idx] = nx;
                s_ny_buf[idx] = 0.0f;
                float nz2 = 1.0f - nx * nx;
                s_nz_buf[idx] = (nz2 > 0.1f) ? sqrtf(nz2) : 0.316f;
            } else if (mat == MAT_MOUNTAIN) {
                s_nx_buf[idx] = 0.20f;
                s_ny_buf[idx] = -0.15f;
                s_nz_buf[idx] = 0.96f;
            } else {
                s_nx_buf[idx] = 0.0f;
                s_ny_buf[idx] = 0.0f;
                s_nz_buf[idx] = 1.0f;
            }
        }
    }

    /* Pass 2: Trace Contact Shadow for Yoshi directly onto solid terrain */
    int yoshi_raw_x = g_ram[0x00a0];
    int yoshi_raw_y = g_ram[0x00a2];
    int extra_margin = (width - 256) / 2;
    int yoshi_cx = yoshi_raw_x + extra_margin + 8;
    int yoshi_feet_y = yoshi_raw_y + 28;
    int yoshi_ground_dist = 0;
    bool has_ground_shadow = false;
    int ground_y = yoshi_feet_y;

    if (yoshi_raw_x > 0 && yoshi_raw_x < 248 && yoshi_raw_y > 0 && yoshi_raw_y < 200 && yoshi_cx < safe_w) {
        for (int sy = yoshi_feet_y - 2; sy < yoshi_feet_y + 36 && sy < height; ++sy) {
            uint8_t m = s_mat_buf[sy * safe_w + yoshi_cx];
            if (m == MAT_TERRAIN_GRASS || m == MAT_TERRAIN_ROCK) {
                ground_y = sy;
                has_ground_shadow = true;
                break;
            }
        }
        yoshi_ground_dist = ground_y - yoshi_feet_y;
    }

    /* Pass 3: Cel-Shading, Shadow Occlusion, and Depth Haze */
    for (int y = 0; y < height; ++y) {
        const uint8_t *src_row = field + y * width * 4;
        uint8_t *dst_row = dst + y * pitch;

        for (int x = 0; x < width; ++x) {
            int sx = (x < safe_w) ? x : safe_w - 1;
            int idx = y * safe_w + sx;
            MaterialType mat = (MaterialType)s_mat_buf[idx];
            uint8_t b = src_row[x * 4 + 0];
            uint8_t g = src_row[x * 4 + 1];
            uint8_t r = src_row[x * 4 + 2];

            if (mat == MAT_HUD) {
                dst_row[x * 4 + 0] = b;
                dst_row[x * 4 + 1] = g;
                dst_row[x * 4 + 2] = r;
                dst_row[x * 4 + 3] = 255;
                continue;
            }

            if (mat == MAT_SKY) {
                int out_b = (b * s_ambient[2]) / 255;
                int out_g = (g * s_ambient[1]) / 255;
                int out_r = (r * s_ambient[0]) / 255;
                dst_row[x * 4 + 0] = ClampU8(out_b);
                dst_row[x * 4 + 1] = ClampU8(out_g);
                dst_row[x * 4 + 2] = ClampU8(out_r);
                dst_row[x * 4 + 3] = 255;
                continue;
            }

            if (mat == MAT_LAVA) {
                dst_row[x * 4 + 0] = ClampU8((b * 13) / 10);
                dst_row[x * 4 + 1] = ClampU8((g * 13) / 10);
                dst_row[x * 4 + 2] = ClampU8((r * 13) / 10);
                dst_row[x * 4 + 3] = 255;
                continue;
            }

            /* Quantized dot(N, L) */
            float dot = s_nx_buf[idx] * lx + s_ny_buf[idx] * ly + s_nz_buf[idx] * lz;
            float factor = 1.0f;
            if (dot >= 0.52f) factor = 1.15f;
            else if (dot >= 0.22f) factor = 1.00f;
            else if (dot >= -0.08f) factor = 0.86f;
            else factor = 0.72f;

            int out_r = (int)((float)r * factor * (float)s_ambient[0] / 255.0f);
            int out_g = (int)((float)g * factor * (float)s_ambient[1] / 255.0f);
            int out_b = (int)((float)b * factor * (float)s_ambient[2] / 255.0f);

            /* Atmospheric depth haze on distant mountains */
            if (mat == MAT_MOUNTAIN) {
                out_r = (out_r * 88 + s_ambient[0] * 12) / 100;
                out_g = (out_g * 88 + s_ambient[1] * 12) / 100;
                out_b = (out_b * 88 + s_ambient[2] * 12) / 100;
            }

            /* Contact Shadow Occlusion on Terrain */
            if (has_ground_shadow && (mat == MAT_TERRAIN_GRASS || mat == MAT_TERRAIN_ROCK)) {
                float dx = (float)(x - yoshi_cx);
                float dy = (float)(y - ground_y);
                float rx = 9.5f + (float)yoshi_ground_dist * 0.10f;
                float ry = 3.5f;
                float dist_sq = (dx * dx) / (rx * rx) + (dy * dy) / (ry * ry);
                if (dist_sq <= 1.0f) {
                    float alpha_s = 0.45f * (1.0f - (float)yoshi_ground_dist / 64.0f);
                    if (alpha_s < 0.12f) alpha_s = 0.12f;
                    float shadow_mult = 1.0f - alpha_s * (1.0f - dist_sq * 0.35f);
                    out_r = (int)((float)out_r * shadow_mult);
                    out_g = (int)((float)out_g * shadow_mult);
                    out_b = (int)((float)out_b * shadow_mult);
                }
            }

            dst_row[x * 4 + 0] = ClampU8(out_b);
            dst_row[x * 4 + 1] = ClampU8(out_g);
            dst_row[x * 4 + 2] = ClampU8(out_r);
            dst_row[x * 4 + 3] = 255;
        }
    }

    /* Proof heartbeat every 60 frames */
    extern int snes_frame_counter;
    if (snes_frame_counter - s_last_log_frame >= 60) {
        s_last_log_frame = (uint32_t)snes_frame_counter;
        LOG_25D("[2.5D Remaster] Frame: %u | LightPass: %u, ShadowPass: %u, BloomPass: %u, DoFPass: %u, PartPass: %u | YoshiGroundDist: %d px\n",
                (unsigned)snes_frame_counter, (unsigned)s_lighting_pass_count, (unsigned)s_shadow_pass_count,
                (unsigned)s_bloom_pass_count, (unsigned)s_dof_pass_count, (unsigned)s_particle_pass_count,
                yoshi_ground_dist);
    }

    if ((size_t)width * height * 4 <= sizeof(s_composed_frame)) {
        for (int cy = 0; cy < height; ++cy) {
            memcpy(s_composed_frame + (size_t)cy * width * 4, dst + (size_t)cy * pitch, (size_t)width * 4);
        }
    }

    return 1;
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
        if (destination->x > 0 || (destination->x + destination->w) < out_w ||
            destination->y > 0 || (destination->y + destination->h) < out_h) {
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

    const SDL_BlendMode add = SDL_BLENDMODE_ADD;
    bool add_ok = SDL_SetTextureBlendMode(texture, add);
    if (!add_ok) {
        /* Fall back gracefully without additive bloom instead of aborting all presentation */
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
            Mod(s_glow, lighting ? 45 : 20, lighting ? 42 : 18, lighting ? 32 : 15);
            Draw(renderer, s_glow, source, destination, 0, 0);
            Mod(s_glow, lighting ? 24 : 12, lighting ? 22 : 10, lighting ? 18 : 8);
            Draw(renderer, s_glow, source, destination, -1.0f*px, 0);
            Draw(renderer, s_glow, source, destination,  1.0f*px, 0);
            Draw(renderer, s_glow, source, destination, 0, -1.0f*py);
            Draw(renderer, s_glow, source, destination, 0,  1.0f*py);
            if (lighting) s_applied |= 2;
        }
    }
    /* Render active contextual presentation particles */
    for (int i = 0; i < MAX_VISUAL_PARTICLES; ++i) {
        if (s_particles[i].active) {
            VisualParticle *p = &s_particles[i];
            p->x += p->vx;
            p->y += p->vy;
            if (p->type == PARTICLE_EMBER) {
                p->x += sinf(p->life * 7.0f) * 0.45f;
            } else if (p->type == PARTICLE_SNOW) {
                p->x += sinf(p->life * 4.5f) * 0.35f;
            }
            p->life -= 0.016667f;
            if (p->life <= 0.0f) {
                p->active = false;
                continue;
            }
            float alpha_frac = p->life / p->max_life;
            uint8_t cur_a = (uint8_t)(p->a * alpha_frac);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, p->r, p->g, p->b, cur_a);
            float screen_x = destination->x + (p->x / 256.0f) * destination->w;
            float screen_y = destination->y + (p->y / 224.0f) * destination->h;

            if (p->type == PARTICLE_WATER_RIPPLE) {
                float ripple_w = p->size * (1.2f + (1.0f - alpha_frac) * 4.0f) * px;
                float ripple_h = ripple_w * 0.40f;
#if SNESRECOMP_SDL3
                SDL_FRect rp = {screen_x - ripple_w * 0.5f, screen_y - ripple_h * 0.5f, ripple_w, ripple_h};
                SDL_RenderRect(renderer, &rp);
#else
                SDL_Rect rp = {(int)(screen_x - ripple_w * 0.5f), (int)(screen_y - ripple_h * 0.5f), (int)ripple_w, (int)ripple_h};
                SDL_RenderRect(renderer, &rp);
#endif
            } else {
                float p_size = p->size * px * 1.6f;
#if SNESRECOMP_SDL3
                SDL_FRect pr = {screen_x - p_size * 0.5f, screen_y - p_size * 0.5f, p_size, p_size};
                SDL_RenderFillRect(renderer, &pr);
#else
                SDL_Rect pr = {(int)(screen_x - p_size * 0.5f), (int)(screen_y - p_size * 0.5f), (int)p_size, (int)p_size};
                SDL_RenderFillRect(renderer, &pr);
#endif
            }
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
    for (int i = 0; i < MAX_VISUAL_PARTICLES; ++i) s_particles[i].active = false;
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
        if (i == 0) {
            /* Authentic unmodified base SNES PPU output */
            SDL_UpdateTexture(texture, NULL, s_raw_field, source->w * 4);
            snesrecomp_sdl_set_texture_linear(texture, false);
            Mod(texture, 255, 255, 255);
            Draw(renderer, texture, source, destination, 0, 0);
        } else {
            /* Definitive 2.5D layer-aware remaster */
            SDL_UpdateTexture(texture, NULL, s_composed_frame, source->w * 4);
            if (!YoshiRenderPresentation(renderer, texture, source, destination))
                Draw(renderer, texture, source, destination, 0, 0);
        }
        char variant[4096];
        snprintf(variant, sizeof(variant), "%s.%s", path, names[i]);
        success = Capture(renderer, variant, source->w, source->h, destination) && success;
    }
    Configure(old_soft, old_lighting);
    SDL_UpdateTexture(texture, NULL, s_composed_frame, source->w * 4);
    SDL_RenderClear(renderer);
    if (!YoshiRenderPresentation(renderer, texture, source, destination))
        Draw(renderer, texture, source, destination, 0, 0);
    captured = success;
#else
    (void)renderer; (void)texture; (void)source; (void)destination;
#endif
}
