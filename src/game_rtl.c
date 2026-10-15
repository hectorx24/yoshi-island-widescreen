/*
 * Yoshi's Island — per-game runtime glue.
 *
 * THIS FILE IS THE PORT. Everything else the scaffold produced is layout,
 * build wiring, and packaging; this is where the actual work happens.
 *
 * The framework does not, and cannot, drive an arbitrary SNES game on its
 * own. Two responsibilities always land on the game host:
 *
 *   1. Deciding what "one frame" means for THIS title, and returning from
 *      run_frame() at that boundary. A real ROM's reset vector never
 *      returns — it enters a main loop that waits on vblank — so somebody
 *      has to choose the yield point.
 *   2. Delivering NMI/IRQ at the hardware edge (see
 *      snesrecomp/docs/FRAME_MODEL_HOSTS.md).
 *
 * A new port starts on the framework's beam-aligned driver
 * (runner/src/beam_frame_driver.h): one frame is one PPU field, cut at the
 * beam's vblank edge; interrupts -- raster splits and a coprocessor's IRQ
 * line -- are taken where the beam latches them; parked time still runs the
 * beam, APU and coprocessors; and the field is rasterized from the raster
 * journal with real HDMA. It is the framework's, so a fix to it reaches this
 * port on a submodule pull.
 *
 * This file used to hold its own copy of a simpler driver: frames measured
 * from wherever the CPU last stopped, NMI delivered at an arbitrary beam
 * position, the field drawn with end-of-frame register state. A game that
 * uploads in vblank between INIDISP=$80 and $0F flickered black, and one
 * that chains raster splits fell apart. Replace these calls only for what
 * the driver does not do; MetalWarriorsSNESRecomp's src/mw_rtl.c and
 * GundamWingEndlessDuelSNESRecomp's src/game_rtl.c are developed examples of
 * title-specific drivers.
 */

#include "game_rtl.h"

#include "beam_frame_driver.h"
#include "display_geometry.h"
#include "snes/ppu.h"
#include "snes/ws_shadow.h"
#include <stdio.h>
#include <stdlib.h>

/* USA v1.1 RAM_Map_YI.asm: the four 16-bit layer origins begin at $7E:0039
 * and advance by four bytes per layer. They are pixel-space world positions;
 * captured tutorial state confirms BG1's X origin matches PPU H-scroll and
 * its Y origin matches the PPU's 10-bit vertical scroll modulo 1024. */
extern uint8_t g_ram[0x20000];
extern Ppu *g_ppu;
static int s_frame_width = 256;

/* Derive the native widened PPU field from the live landscape drawable only
 * while the USA v1.1 game-mode byte identifies tutorial/gameplay (0E/0F).
 * Menus and narrative scenes retain their original framing. Keep 224 scanlines
 * and guest timing unchanged; a fixed PPU override supports reproducible tests. */
void GamePrepareFrame(int drawable_w, int drawable_h,
                      int *frame_w, int *frame_h)
{
    s_frame_width = *frame_w;
    const int fixed_extra = PpuWsExtraOverride();
    if (fixed_extra >= 0) {
        *frame_w = 256 + 2 * fixed_extra;
        *frame_h = 224;
    } else if (g_ram[0x0118] == 0x0e || g_ram[0x0118] == 0x0f) {
        YoshiDisplayGeometry geometry;
        if (yoshi_display_geometry(drawable_w, drawable_h,
                                   (YoshiInsets){0, 0, 0, 0}, 7.0 / 6.0,
                                   &geometry)) {
            int width = geometry.logical_width;
            if (width < 256) width = 256;
            if (width > kPpuBufWidth) width = kPpuBufWidth;
            /* The native PPU allocates symmetric per-side margins. */
            width = 256 + ((width - 256 + 1) & ~1);
            if (width > kPpuBufWidth) width = kPpuBufWidth & ~1;
            *frame_w = width;
            *frame_h = 224;
        }
    } else {
        *frame_w = 256;
        *frame_h = 224;
    }
    s_frame_width = *frame_w;

    /* Opt-in diagnostic used by the Android smoke test to assert that the
     * live drawable, rather than a pinned test override, selected the field. */
    if (getenv("SNESRECOMP_WS_DIAG")) {
        static int last_width;
        if (last_width != *frame_w) {
            fprintf(stderr, "Yoshi adaptive field: drawable=%dx%d frame=%dx%d\n",
                    drawable_w, drawable_h, *frame_w, *frame_h);
            const char *path = getenv("SNESRECOMP_WS_DIAG_FILE");
            if (path && path[0]) {
                FILE *file = fopen(path, "a");
                if (file) {
                    fprintf(file, "drawable=%dx%d frame=%dx%d\n",
                            drawable_w, drawable_h, *frame_w, *frame_h);
                    fclose(file);
                }
            }
            last_width = *frame_w;
        }
    }
}

void GameBeginFrame(unsigned number)
{
    (void)number;
    if (!g_ppu || !(g_ppu->extraLeftCur || g_ppu->extraRightCur)) return;

    for (int layer = 0; layer < 4; ++layer) {
        const unsigned address = 0x39u + (unsigned)layer * 4u;
        const uint32_t world_x = g_ram[address] | ((uint32_t)g_ram[address + 1] << 8);
        const uint32_t world_y = g_ram[address + 2] | ((uint32_t)g_ram[address + 3] << 8);
        WsShadowSetWorld(layer, world_x, world_y);
        WsShadowSetScroll(layer, g_ppu->hScroll[layer], g_ppu->vScroll[layer]);
        WsShadowSetRetainHistory(layer, true);
        WsShadowSetWestKeep(layer, 40);
        WsShadowSetEastKeep(layer, 40);
    }
    WsShadowFrame(g_ppu);
}

void GameEndFrame(const uint8_t *field, unsigned number)
{
    const char *frame_path = getenv("SNESRECOMP_WS_FRAME_FILE");
    const char *frame_number = getenv("SNESRECOMP_WS_FRAME_NUMBER");
    if (frame_path && frame_path[0] && frame_number &&
        strtoul(frame_number, NULL, 10) == number) {
        FILE *image = fopen(frame_path, "wb");
        if (image) {
            fwrite(field, (size_t)s_frame_width * 224u * 4u, 1, image);
            fclose(image);
        }
    }
    if (!getenv("SNESRECOMP_WS_DIAG") || number % 120 != 0) return;
    const char *path = getenv("SNESRECOMP_WS_DIAG_FILE");
    if (!path || !path[0]) return;
    FILE *file = fopen(path, "a");
    if (!file) return;
    for (int layer = 0; layer < 4; ++layer) {
        WsShadowMarginStat stats;
        WsShadowGetMarginStats(layer, &stats);
        fprintf(file, "frame=%u bg=%d west_hit=%llu west_miss=%llu east_hit=%llu east_miss=%llu\n",
                number, layer + 1,
                (unsigned long long)stats.westHit,
                (unsigned long long)stats.westMiss,
                (unsigned long long)stats.eastHit,
                (unsigned long long)stats.eastMiss);
    }
    fclose(file);
}

void GameRunOneFrame(void)
{
    snes_beam_frame_driver_run_frame();
}

void GameDrawPpuFrame(void)
{
    snes_beam_frame_driver_draw_ppu_frame();
}

void GameSessionReset(void)
{
    /* Rematch / soft-return: clear anything that must not survive a new
     * session. recomp-ai-rules/NETPLAY.md §3 — sticky state that "has always
     * been fine" is the usual desync culprit, because single-player never
     * re-enters the boot path twice in one process. */
    snes_beam_frame_driver_reset();
    WsShadowReset();
}

const RtlGameInfo kGameInfo = {
    .title = "yoshi-island-usa11",
    .initialize = NULL,
    .run_frame = &GameRunOneFrame,
    .draw_ppu_frame = &GameDrawPpuFrame,
    .save_name_prefix = "save",
    .session_reset = &GameSessionReset,
};
