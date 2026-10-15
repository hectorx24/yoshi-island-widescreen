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
#include <stdio.h>
#include <stdlib.h>

/* Derive the native widened PPU field from the live landscape drawable. Keep
 * 224 scanlines and guest timing unchanged. A fixed PPU override keeps
 * reference captures reproducible. */
void GamePrepareFrame(int drawable_w, int drawable_h,
                      int *frame_w, int *frame_h)
{
    const int fixed_extra = PpuWsExtraOverride();
    if (fixed_extra >= 0) {
        *frame_w = 256 + 2 * fixed_extra;
        *frame_h = 224;
    } else {
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
    }

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
}

const RtlGameInfo kGameInfo = {
    .title = "yoshi-island-usa11",
    .initialize = NULL,
    .run_frame = &GameRunOneFrame,
    .draw_ppu_frame = &GameDrawPpuFrame,
    .save_name_prefix = "save",
    .session_reset = &GameSessionReset,
};
