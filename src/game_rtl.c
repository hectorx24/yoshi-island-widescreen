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
#include "achievements.h"

#include "beam_frame_driver.h"
#include "display_geometry.h"
#include "yoshi_bg1_map.h"
#include "yoshi_scene_map.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "snes/superfx.h"
#include "snes/ws_shadow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* USA v1.1 RAM_Map_YI.asm: the four 16-bit layer origins begin at $7E:0039
 * and advance by four bytes per layer. They are pixel-space world positions;
 * captured tutorial state confirms BG1's X origin matches PPU H-scroll and
 * its Y origin matches the PPU's 10-bit vertical scroll modulo 1024. */
extern uint8_t g_ram[0x20000];
extern Ppu *g_ppu;
extern Snes *g_snes;
static int s_frame_width = 256;
static bool s_gsu_audit_hook_installed;
static int s_gsu_replay_audit_frame;
static uint8_t s_map_rows[256];
static uint8_t s_bg2_plane;
static uint8_t s_bg3_rows[32];
static bool s_boundary_extension;

static bool KnownPlane(unsigned layer, uint64_t expected) {
    if (!g_ppu || PPU_mode(g_ppu) != 1 || !PPU_bigTiles(g_ppu, layer) ||
        g_ppu->bgXsc[layer] != (layer == 1 ? 0x3a : 0x34) ||
        PPU_bgTileAdr(g_ppu, layer) != (layer == 1 ? 0x7000 : 0x2000))
        return false;
    const uint16_t *vram = g_ppu->renderVram ? g_ppu->renderVram : g_ppu->vram;
    const unsigned base = layer == 1 ? 0x3800 : 0x3400;
    const unsigned end = layer == 1 ? 0x4000 : 0x3800;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned i = base; i < end; ++i) {
        hash = (hash ^ (vram[i] & 255)) * UINT64_C(1099511628211);
        hash = (hash ^ (vram[i] >> 8)) * UINT64_C(1099511628211);
    }
    return hash == expected;
}

/* Separate original authored plane in the next reached gameplay scene.
 * Both maps retain their own 512px horizontal domain and actual PPU phase. */
static bool KnownBackground(void) {
    if (!g_ppu || !g_snes || !g_snes->cart || PPU_mode(g_ppu) != 1 ||
        !PPU_bigTiles(g_ppu, 1) || g_ppu->bgXsc[1] != 0x3a ||
        PPU_bgTileAdr(g_ppu, 1) != 0x7000) return false;
    const unsigned header = g_ram[0x13a] | (unsigned)g_ram[0x13b] << 8;
    /* Source verification generalizes to all32 original descriptors; scene
     * eligibility remains individually composed/tested rooms until
     * their foregrounds, effects and sprites are individually qualified. */
    if (header != 26 && header != 29) {
        if (header == 20) {
            /* Individually qualified 1-4 entrance hall and Level 2-4 (Bigger Boo's Fort).
             * The room's original GSU screen layout stays stable as blocks change.
             * A different room/encounter must qualify its own map and effects. */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21b] || g_ram[0x147] || !g_snes->cart->superfx) return false;
            bool is_1_4 = (g_ram[0x21a] == 3 && g_ram[0x146] == 7);
            bool is_2_4 = (g_ram[0x21a] == 15 && g_ram[0x146] == 0);
            if (!is_1_4 && !is_2_4) return false;
            uint64_t room = UINT64_C(14695981039346656037);
            for (unsigned i = 0; i < 128; ++i)
                room = (room ^ g_snes->cart->superfx->ram[0x0caa + i]) * UINT64_C(1099511628211);
            if (is_1_4 && room != UINT64_C(0x666c9488c384aca4)) return false;
            if (is_2_4 && room != UINT64_C(0x04cc798b969635ee)) return false;
        } else if (header == 14) {
            /* Individually qualified Level 1-2 (The Cave Of Chomp Rock). */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21a] != 1 ||
                g_ram[0x21b] || g_ram[0x146] != 0 || g_ram[0x147]) return false;
        } else if (header == 19) {
            /* Individually qualified Level 1-3 Crystal Cavern. */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21a] != 2 ||
                g_ram[0x21b] || g_ram[0x146] != 5 || g_ram[0x147]) return false;
        } else if (header == 8) {
            /* Individually qualified Level 1-7 (Touch Fuzzy, Get Dizzy), Level 2-3 (What's Gusty Taste Like?), and Level 3-2 (Jungle Rhythm...). */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21b] || g_ram[0x147]) return false;
            bool is_1_7 = (g_ram[0x21a] == 6 && g_ram[0x146] == 3);
            bool is_2_3 = (g_ram[0x21a] == 14 && g_ram[0x146] == 11);
            bool is_3_2 = (g_ram[0x21a] == 25 && g_ram[0x146] == 11);
            if (!is_1_7 && !is_2_3 && !is_3_2) return false;
        } else if (header == 6) {
            /* Individually qualified Level 1-8 (Salvo The Slime's Castle). */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21a] != 7 ||
                g_ram[0x21b] || g_ram[0x146] != 5 || g_ram[0x147]) return false;
        } else if (header == 4) {
            /* Individually qualified Level 2-1 (Visit Koopa And Para-Koopa) and Level 2-7 (Lakitu's Wall). */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21b] || g_ram[0x147]) return false;
            bool is_2_1 = (g_ram[0x21a] == 12 && g_ram[0x146] == 11);
            bool is_2_7 = (g_ram[0x21a] == 18 && g_ram[0x146] == 7);
            if (!is_2_1 && !is_2_7) return false;
        } else if (header == 21) {
            /* Individually qualified Level 2-2 (The Baseball Boys) and Extra 2 (Hit The Early Bird!). */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21b] || g_ram[0x147]) return false;
            bool is_2_2 = (g_ram[0x21a] == 13 && g_ram[0x146] == 0);
            bool is_extra_2 = (g_ram[0x21a] == 20 && g_ram[0x146] == 11);
            if (!is_2_2 && !is_extra_2) return false;
        } else if (header == 7) {
            /* Individually qualified Level 2-5 (Watch Out For Lakitu). */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21a] != 16 ||
                g_ram[0x21b] || g_ram[0x146] != 11 || g_ram[0x147]) return false;
        } else if (header == 11) {
            /* Individually qualified Level 2-6 (The Cave Of Mystery Maze). */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21a] != 17 ||
                g_ram[0x21b] || g_ram[0x146] || g_ram[0x147]) return false;
        } else if (header == 25) {
            /* Individually qualified Level 2-8 (The Potted Ghost's Castle). */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21a] != 19 ||
                g_ram[0x21b] || g_ram[0x146] != 11 || g_ram[0x147]) return false;
        } else if (header == 1) {
            /* Individually qualified Level 3-1 (Welcome To Monkey World!). */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21a] != 24 ||
                g_ram[0x21b] || g_ram[0x146] != 11 || g_ram[0x147]) return false;
        } else if (header == 10) {
            /* Individually qualified Level 3-3 (Nep-Enut's Domain). */
            if (g_ram[0x218] || g_ram[0x219] || g_ram[0x21a] != 26 ||
                g_ram[0x21b] || g_ram[0x146] != 11 || g_ram[0x147]) return false;
        } else {
            return false;
        }
    }
    static unsigned cached_header = 32;
    static uint8_t authored[4096];
    if (header != cached_header) {
        if (!yoshi_bg2_asset(g_snes->cart->rom, g_snes->cart->romSize,
                              header, authored)) return false;
        cached_header = header;
    }
    const uint16_t *vram = g_ppu->renderVram ? g_ppu->renderVram : g_ppu->vram;
    for (unsigned i = 0; i < 2048; ++i)
        if (vram[0x3800 + i] != (authored[i * 2] | (unsigned)authored[i * 2 + 1] << 8))
            return false;
    return true;
}

/* A decorative continuation only where 1-1's authored world ends at x=0.
 * Its original first32px ground strip has a continuous opaque silhouette at
 * the wrap seam. Exact MAP16 column provenance guards the entire strip;
 * there is no new collision/simulation space and no global edge-clamp rule. */
static bool KnownBoundaryExtension(void) {
    if (!g_snes || !g_snes->cart || !g_snes->cart->superfx ||
        g_ram[0x118] != 0x0f || g_ram[0x218] || g_ram[0x219] ||
        g_ram[0x21a] || g_ram[0x21b] || g_ram[0x136] != 12 ||
        g_ram[0x137] || g_ram[0x13a] != 29 || g_ram[0x13b]) return false;
    const uint8_t *gsu = g_snes->cart->superfx->ram;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned y = 0; y < 128; ++y) {
        const unsigned screen = gsu[0x0caa + (y >> 4) * 16] & 63;
        const unsigned map = 0x18000 + screen * 512 + (y & 15) * 32;
        for (unsigned byte = 0; byte < 4; ++byte)
            hash = (hash ^ g_ram[map + byte]) * UINT64_C(1099511628211);
    }
    return hash == UINT64_C(0x7944d8370f1fef5b);
}

uint16_t YoshiBg2MarginTile(uint16_t real_tile) {
    if (!s_bg2_plane)
        s_bg2_plane = KnownBackground() ? 1 : 2;
    /* Exact opening-scene 512x1024 authored plane. Its actual rendered scroll
     * owns both the tile key and pixel phase, independently of BG1. */
    WsShadowSetWorld(1, g_ppu->hScroll[1], g_ppu->vScroll[1]);
    WsShadowSetScroll(1, g_ppu->hScroll[1], g_ppu->vScroll[1]);
    return s_bg2_plane == 1 ? real_tile : 0x18ee;
}

uint16_t YoshiBg3MarginTile(uint16_t real_tile, uint32_t wrapped_y) {
    static const uint64_t lower_rows[8] = {
        UINT64_C(0xd973e4360957b865), UINT64_C(0xab29a56890591242),
        UINT64_C(0x497113f1e5ed2ec2), UINT64_C(0x18fb135bc08fde80),
        UINT64_C(0x809cd72d4eedbad1), UINT64_C(0x601436a2995304b4),
        UINT64_C(0xe45b4a4d8172cda3), UINT64_C(0x394d1bfb82ddbdef)};
    static const uint64_t s_bg3_cloud_rows[32] = {
        UINT64_C(0xc80e2646f19da6e7), UINT64_C(0xb08173430a3b2dcf),
        UINT64_C(0x0375a45c870a0ebc), UINT64_C(0x2d4bd9862e8aeb47),
        UINT64_C(0x4eaf505d1908e42f), UINT64_C(0x435859682b4a7767),
        UINT64_C(0xbcfae5a29400b6c5), UINT64_C(0x8a0268d40e4cf561),
        UINT64_C(0x811871fd275bfc46), UINT64_C(0xe106489b2743b464),
        UINT64_C(0xf3f50ccd8deb9eb9), UINT64_C(0x11da54f52a0b3b95),
        UINT64_C(0xabd5c5f24e9532fb), UINT64_C(0x7a41574f1ef7df03),
        UINT64_C(0x1d1e4ea6934a69e3), UINT64_C(0x93a06aa4110d16a5),
        UINT64_C(0x93a06aa4110d16a5), UINT64_C(0x398b564d9ed216e5),
        UINT64_C(0xb63425dd9712c4f0), UINT64_C(0xa2270cd857ac9185),
        UINT64_C(0xd299323e0a519c8c), UINT64_C(0x463d7ed62bbe3545),
        UINT64_C(0x0e6aefba03abc784), UINT64_C(0x141d38ac0309e84c),
        UINT64_C(0xb2d96f31baebd5dc), UINT64_C(0xa3f472fcac353567),
        UINT64_C(0xc45720941807d113), UINT64_C(0x10bf6f81d5eaf59f),
        UINT64_C(0xd0408ee70fcb4de9), UINT64_C(0xfea017de198b1011),
        UINT64_C(0x7cc7373e48acf535), UINT64_C(0x81fc0a1a1c54866d),
    };
    static const uint64_t s_bg3_sunset_rows[32] = {
        UINT64_C(0x50f40f1a8f24237b), UINT64_C(0xfdebb74d08afc7df),
        UINT64_C(0x0b199788a5083a27), UINT64_C(0xfcaac104f47cd4e0),
        UINT64_C(0xcac0060210056212), UINT64_C(0x1d4983c2e2c2cc5f),
        UINT64_C(0x334a521ad51f3e8b), UINT64_C(0x72a5f474ae279d19),
        UINT64_C(0x1bc671d66cd715ab), UINT64_C(0x90b48bf8e4a828c5),
        UINT64_C(0x62144830c89a987d), UINT64_C(0x7d7deeaed50ba16b),
        UINT64_C(0xf3aa2c48abfdbf25), UINT64_C(0x78ed0bea1d6a2a03),
        UINT64_C(0xc60345b812daf12b), UINT64_C(0xc152b04414866b0b),
        UINT64_C(0xd42b7eafffabb78a), UINT64_C(0x873fde481138169b),
        UINT64_C(0xdc392979d9f0ea2b), UINT64_C(0xe25e7d9d0304858d),
        UINT64_C(0xecc60c3d6f995f2b), UINT64_C(0x3365a6a41439ac3e),
        UINT64_C(0xbd80783d81d6e382), UINT64_C(0x67903410b7545305),
        UINT64_C(0x0fcea3e1e65045f3), UINT64_C(0x4d5f736d49c73459),
        UINT64_C(0x93a06aa4110d16a5), UINT64_C(0x93a06aa4110d16a5),
        UINT64_C(0x93a06aa4110d16a5), UINT64_C(0x93a06aa4110d16a5),
        UINT64_C(0x93a06aa4110d16a5), UINT64_C(0x93a06aa4110d16a5),
    };
    static const uint64_t s_bg3_jungle_rows[32] = {
        UINT64_C(0xc0fbd39666099435), UINT64_C(0x0e2416cc74e39c29),
        UINT64_C(0x8a986f511fbebd2d), UINT64_C(0xc06fb9852eff9907),
        UINT64_C(0x708af31e491996b7), UINT64_C(0x3807287d366b0d25),
        UINT64_C(0xe66e339956c824d5), UINT64_C(0x0dd7019a26b82f75),
        UINT64_C(0x596289417123ade5), UINT64_C(0x362bbfd2a6e9dce7),
        UINT64_C(0xa48fc72b006f8cab), UINT64_C(0x3807287d366b0d25),
        UINT64_C(0x9f4ab8c530f05d15), UINT64_C(0xf6c6461dd9969179),
        UINT64_C(0xc496128a4304e785), UINT64_C(0xb7b3c5628a374387),
        UINT64_C(0x25480bc76cd2b03d), UINT64_C(0x392b28a62dea4c43),
        UINT64_C(0xed642bb46cd80a63), UINT64_C(0x3025bc25fa455bc7),
        UINT64_C(0x4e9bd1b81c1381bf), UINT64_C(0x94dc6bb7273a3ebf),
        UINT64_C(0xf811567d4b0f6717), UINT64_C(0x2d525893ada2f963),
        UINT64_C(0x995835856ce6c595), UINT64_C(0x31aedb4ed3204393),
        UINT64_C(0x3eaca181b1677735), UINT64_C(0xcfe8ec77736ec165),
        UINT64_C(0xcfe8ec77736ec165), UINT64_C(0xcfe8ec77736ec165),
        UINT64_C(0xcfe8ec77736ec165), UINT64_C(0xcfe8ec77736ec165),
    };
    static const uint64_t s_bg3_jungle_rhythm_rows[32] = {
        UINT64_C(0xa13acd8bd3011da5), UINT64_C(0xa13acd8bd3011da5),
        UINT64_C(0xa13acd8bd3011da5), UINT64_C(0xa13acd8bd3011da5),
        UINT64_C(0xa13acd8bd3011da5), UINT64_C(0xa13acd8bd3011da5),
        UINT64_C(0xa13acd8bd3011da5), UINT64_C(0xa13acd8bd3011da5),
        UINT64_C(0xa13acd8bd3011da5), UINT64_C(0xa13acd8bd3011da5),
        UINT64_C(0xa13acd8bd3011da5), UINT64_C(0xa13acd8bd3011da5),
        UINT64_C(0xa13acd8bd3011da5), UINT64_C(0xa13acd8bd3011da5),
        UINT64_C(0x484e5c809521957f), UINT64_C(0x1e487e07ea096852),
        UINT64_C(0xc116c15ed9fe6f5d), UINT64_C(0xdfef98587b814f74),
        UINT64_C(0x8e230f9d09dc7934), UINT64_C(0x343a60cf7ebd8265),
        UINT64_C(0x343a60cf7ebd8265), UINT64_C(0xcc40ac3fe5ac7dbd),
        UINT64_C(0xedba7a04092b5de1), UINT64_C(0x941637e2d7bf7eb8),
        UINT64_C(0x8694bd885167a765), UINT64_C(0xf531e87f5074e836),
        UINT64_C(0xa16a207f7c3745ad), UINT64_C(0x361d28cc92e61f34),
        UINT64_C(0x343a60cf7ebd8265), UINT64_C(0x343a60cf7ebd8265),
        UINT64_C(0x343a60cf7ebd8265), UINT64_C(0x343a60cf7ebd8265),
    };
    static const uint64_t s_bg3_nep_enut_rows[32] = {
        UINT64_C(0xfa140632c28ef617), UINT64_C(0xd788d4804e9e6ebc),
        UINT64_C(0x96469e0db62d10dd), UINT64_C(0xc0a1d4462628b7a5),
        UINT64_C(0x8f46a9bd6e802f57), UINT64_C(0x38251b9f3c5a89d9),
        UINT64_C(0x34bf1e786ef726a1), UINT64_C(0x107672cac3149307),
        UINT64_C(0x6aa7780123499664), UINT64_C(0x95a133126fc0260f),
        UINT64_C(0x5a495bb5e02dd81f), UINT64_C(0x92f8f3dcbbb06cf1),
        UINT64_C(0xebbbd13b1bf3a07a), UINT64_C(0x759e08ed5026dcfd),
        UINT64_C(0x621faa996833a515), UINT64_C(0xe53085094ab98ef3),
        UINT64_C(0x6c233d2a729ab4fb), UINT64_C(0x92c0dcdbd07b42b3),
        UINT64_C(0xa02740c2331791f5), UINT64_C(0x6d229d7c42563847),
        UINT64_C(0x977cd7d4bbbc3274), UINT64_C(0xa40d709c8614ee33),
        UINT64_C(0x92b5795ad364767c), UINT64_C(0x5b8d69711007599a),
        UINT64_C(0xac4a272658236170), UINT64_C(0xfc72f8e32a88ab1a),
        UINT64_C(0x2cc173a3b9ad8cf4), UINT64_C(0xf11c33ae8a6ff852),
        UINT64_C(0x63480f323ad45ad0), UINT64_C(0xd8678c4ac21ac0f2),
        UINT64_C(0xe78fd99271432294), UINT64_C(0x3c212671e2a8f68a),
    };
    const unsigned row = (wrapped_y >> 4) & 31;
    if (!s_bg2_plane)
        s_bg2_plane = KnownBackground() ? 1 : 2;
    const unsigned bg3_ts = g_ram[0x138] | ((unsigned)g_ram[0x139] << 8);
    if (!s_bg3_rows[row]) {
        const uint16_t *vram = g_ppu->renderVram ? g_ppu->renderVram : g_ppu->vram;
        uint64_t hash = UINT64_C(14695981039346656037);
        for (unsigned i = 0; i < 32; ++i) {
            const uint16_t value = vram[0x3400 + row * 32 + i];
            hash = (hash ^ (value & 255)) * UINT64_C(1099511628211);
            hash = (hash ^ (value >> 8)) * UINT64_C(1099511628211);
        }
        uint64_t expected = 0;
        if (bg3_ts == 9)
            expected = row < 24 ? UINT64_C(0x8fd4ca6a34b4e865) : lower_rows[row - 24];
        else if (bg3_ts == 10)
            expected = s_bg3_cloud_rows[row];
        else if (bg3_ts == 15)
            expected = s_bg3_sunset_rows[row];
        else if (bg3_ts == 1)
            expected = s_bg3_jungle_rows[row];
        else if (bg3_ts == 27)
            expected = s_bg3_jungle_rhythm_rows[row];
        else if (bg3_ts == 5)
            expected = s_bg3_nep_enut_rows[row];
        s_bg3_rows[row] = s_bg2_plane == 1 && PPU_mode(g_ppu) == 1 &&
            PPU_bigTiles(g_ppu, 2) && g_ppu->bgXsc[2] == 0x34 &&
            PPU_bgTileAdr(g_ppu, 2) == 0x2000 && expected && hash == expected ? 1 : 2;
    }
    /* Original 512px foreground plane. Message boxes reuse this map
     * (CODE_01DF45); their modified rows must never wrap into margins. */
    WsShadowSetWorld(2, g_ppu->hScroll[2], g_ppu->vScroll[2]);
    WsShadowSetScroll(2, g_ppu->hScroll[2], g_ppu->vScroll[2]);
    const uint16_t fallback = bg3_ts == 15 ? 0x01db : (bg3_ts == 1 ? 0x052b : (bg3_ts == 27 ? 0x0143 : (bg3_ts == 5 ? 0x2100 : 0x0185)));
    return s_bg3_rows[row] == 1 ? real_tile : fallback;
}

/* Recover only the wrap epoch from WRAM. Fine phase comes from the PPU
 * register actually used to render this line, independently for each layer. */
static int WorldPhase(unsigned reference, unsigned ppu_phase) {
    int result = (int)(reference & ~1023u) + (int)(ppu_phase & 1023u);
    if (result + 512 < (int)reference) result += 1024;
    if (result > (int)reference + 512) result -= 1024;
    return result;
}

static bool VerifiedMapRow(unsigned wrapped_y, unsigned hscroll,
                           int *world_x, int *world_y) {
    const unsigned camera_x = g_ram[0x39] | ((unsigned)g_ram[0x3a] << 8);
    const unsigned camera_y = g_ram[0x3b] | ((unsigned)g_ram[0x3c] << 8);
    *world_x = WorldPhase(camera_x, hscroll);
    *world_y = WorldPhase(camera_y, wrapped_y);
    if (!g_snes || !g_snes->cart || !g_snes->cart->superfx ||
        (g_ram[0x118] != 0x0e && g_ram[0x118] != 0x0f) ||
        (PPU_mode(g_ppu) != 1 && PPU_mode(g_ppu) != 2) ||
        PPU_bigTiles(g_ppu, 0) ||
        (g_ppu->bgXsc[0] & 3) != 1 ||
        *world_x < 0 || *world_y < 0 || *world_y >= 2048) return false;
    const unsigned ty = (unsigned)*world_y / 8;
    if (s_map_rows[ty]) return s_map_rows[ty] == 1;
    const uint16_t *vram = g_ppu->renderVram ? g_ppu->renderVram : g_ppu->vram;
    bool valid = true;
    for (unsigned tx = ((unsigned)*world_x + 7) / 8;
         tx < ((unsigned)*world_x + 256) / 8; ++tx) {
        uint16_t decoded;
        const unsigned map = PPU_bgTilemapAdr(g_ppu, 0) + (tx & 31) +
                             ((ty & 31) << 5) + ((tx & 32) ? 0x400 : 0);
        if (!yoshi_bg1_tile(g_ram, g_snes->cart->superfx->ram,
                            g_snes->cart->rom, g_snes->cart->romSize,
                            tx, ty, &decoded) || decoded != vram[map & 0x7fff]) {
            valid = false;
            break;
        }
    }
    s_map_rows[ty] = valid ? 1 : 2;
    return valid;
}

/* Called by the pinned runner patch for BG1 margin cells ONLY. Never consult
 * the rolling VRAM/history fallback here. Verify this field's native row,
 * then read the corresponding original map source; otherwise stay blank. */
uint16_t YoshiBg1MarginTile(int screen_x, uint32_t wrapped_y,
                           uint16_t hscroll) {
    int x, y;
    uint16_t tile;
    if (VerifiedMapRow(wrapped_y, hscroll, &x, &y)) {
        int source_x = x + screen_x;
        if (source_x < 0 && s_boundary_extension)
            source_x = (source_x % 32 + 32) % 32;
        if (source_x >= 0 && yoshi_bg1_tile(g_ram, g_snes->cart->superfx->ram,
                g_snes->cart->rom, g_snes->cart->romSize,
                (unsigned)source_x / 8, (unsigned)y / 8, &tile)) return tile;
    }
    return 0x18ff;
}

static void BlackUnverifiedMargins(const Ppu *ppu, unsigned line, void *context) {
    (void)context;
    if (!(ppu->extraLeftCur || ppu->extraRightCur)) return;
    memset(s_bg3_rows, 0, sizeof(s_bg3_rows));
    if (!(g_snes_ppu_dbg_layer_mask & 1) ||
        !((ppu->screenEnabled[0] | ppu->screenEnabled[1]) & 1)) return;
    int x, y;
    const bool valid = VerifiedMapRow(ppu->vScroll[0] + line,
                                      ppu->hScroll[0], &x, &y);
    uint8_t *row = ppu->renderBuffer + (line - 1) * ppu->renderPitch;
    for (int sx = -ppu->extraLeftCur; sx < 256 + ppu->extraRightCur; ++sx) {
        if (sx >= 0 && sx < 256) continue;
        if (!valid || (x + sx < 0 && !s_boundary_extension) || x + sx >= 4096) {
            uint8_t *pixel = row + (sx + ppu->extraLeftRight) * 4;
            pixel[0] = pixel[1] = pixel[2] = 0;
        }
    }
    /* Raster writes may change a row between lines. Cache only within the
     * line, never across an HDMA/journal boundary. */
    memset(s_map_rows, 0, sizeof(s_map_rows));
}

/* Diagnostic private replay: verify that a copied GSU task reproduces the
 * native MAP16 conversion without changing the live Super FX state. */
static bool YoshiGsuReplayAuditPrepare(void *context, const SuperFx *source,
                                       uint8_t *private_ram)
{
    (void)context;
    s_gsu_replay_audit_frame = 0;
    if (!g_ppu || !(g_ppu->extraLeftCur || g_ppu->extraRightCur) ||
        (g_ram[0x0118] != 0x0e && g_ram[0x0118] != 0x0f))
        return false;
    extern int snes_frame_counter;
    s_gsu_replay_audit_frame = snes_frame_counter;
    return true;
}

static void YoshiGsuReplayAuditComplete(void *context,
                                        const SuperFx *replay)
{
    (void)context;
    const char *path = getenv("SNESRECOMP_WS_GSU_REPLAY_FILE");
    if (!path || !path[0] || !replay || !replay->ram || !g_snes ||
        !g_snes->cart || !g_snes->cart->superfx)
        return;
    const SuperFx *live = g_snes->cart->superfx;
    unsigned matches = 0;
    unsigned mismatches = 0;
    for (unsigned row = 0; row < 24; ++row) {
        for (unsigned side = 0; side < 2; ++side) {
            const unsigned address = (side ? 0x0deau : 0x0daau) + row * 2u;
            const uint16_t a = replay->ram[address] |
                               ((uint16_t)replay->ram[address + 1] << 8);
            const uint16_t b = live->ram[address] |
                               ((uint16_t)live->ram[address + 1] << 8);
            if (a == b) ++matches;
            else ++mismatches;
        }
    }
    FILE *file = fopen(path, "a");
    if (file) {
        fprintf(file, "%d,%ld,%u,%u", s_gsu_replay_audit_frame,
                0L, matches, mismatches);
        for (unsigned row = 0; row < 24; ++row) {
            for (unsigned side = 0; side < 2; ++side) {
                const unsigned address =
                    (side ? 0x0deau : 0x0daau) + row * 2u;
                const uint16_t value = replay->ram[address] |
                    ((uint16_t)replay->ram[address + 1] << 8);
                fprintf(file, ",%04x", value);
            }
        }
        fputc('\n', file);
        fclose(file);
    }
}

/* Capture the exact Yoshi BG1 expander input after its Super FX pass. This
 * runs at STOP without redirecting or changing the guest GSU state. Records
 * are [frame, pc, gsu_size, layer1_x, layer1_y, level_size, R0..R15], then
 * GSU RAM and the CPU LevelDataBuffer snapshot. */
static void YoshiGsuTaskAuditHook(SuperFx *fx, uint32_t pc24, void *context)
{
    (void)context;
    /* The v1.1 CODE_109089 -> FXCODE_09F9E8 column task fills one 16px
     * MAP16 column across the far 256px screen. Each MAP16 row becomes two
     * 8px PPU rows in the two halves at $0DAA/$0DEA. For a rightward column,
     * the VRAM map math in CODE_109089 places it 32 tiles past camera X; its
     * lower-screen rows begin at the next 256px boundary. Seed only while the
     * adaptive PPU is active. These are game-produced entries, and native
     * VRAM writes replace them if the game updates a cell later in the field. */
    if (fx && fx->ram && fx->ram_size >= 0x0e20 && g_ppu &&
        (g_ppu->extraLeftCur || g_ppu->extraRightCur) &&
        (g_ram[0x0118] == 0x0e || g_ram[0x0118] == 0x0f)) {
        const uint32_t camera_x = g_ram[0x39] | ((uint32_t)g_ram[0x3a] << 8);
        const uint32_t camera_y = g_ram[0x3b] | ((uint32_t)g_ram[0x3c] << 8);
        const uint32_t tile_x = (camera_x >> 3) + 32u;
        const uint32_t tile_y = ((camera_y + 255u) / 256u) * 32u;
        for (uint32_t row = 0; row < 24; ++row) {
            const uint32_t a = 0x0daau + row * 2u;
            const uint32_t b = 0x0deau + row * 2u;
            const uint16_t left = fx->ram[a] | ((uint16_t)fx->ram[a + 1] << 8);
            const uint16_t right = fx->ram[b] | ((uint16_t)fx->ram[b + 1] << 8);
            WsShadowPrefillTile(0, tile_x, tile_y + row, left);
            WsShadowPrefillTile(0, tile_x + 1u, tile_y + row, right);
        }
        /* Optional source/coordinate audit captured immediately after seed,
         * before the guest resumes and uploads its final tilemap to VRAM. */
        const char *audit_path = getenv("SNESRECOMP_WS_GSU_PREFILL_FILE");
        if (audit_path && audit_path[0]) {
            extern int snes_frame_counter;
            FILE *audit = fopen(audit_path, "a");
            if (audit) {
                for (uint32_t row = 0; row < 24; ++row) {
                    const uint32_t a = 0x0daau + row * 2u;
                    const uint32_t b = 0x0deau + row * 2u;
                    uint16_t entry = 0;
                    int state = WsShadowDebugCell(0, tile_x, tile_y + row,
                                                  &entry);
                    fprintf(audit, "%d,%u,%u,%u,%d,%04x,%04x\n",
                            snes_frame_counter, tile_x, tile_y, row, state,
                            (uint16_t)(fx->ram[a] | ((uint16_t)fx->ram[a + 1] << 8)),
                            entry);
                    state = WsShadowDebugCell(0, tile_x + 1u,
                                              tile_y + row, &entry);
                    fprintf(audit, "%d,%u,%u,%u,%d,%04x,%04x\n",
                            snes_frame_counter, tile_x + 1u, tile_y, row,
                            state,
                            (uint16_t)(fx->ram[b] | ((uint16_t)fx->ram[b + 1] << 8)),
                            entry);
                }
                fclose(audit);
            }
        }
    }

    const char *path = getenv("SNESRECOMP_WS_GSU_TASK_FILE");
    const char *range = getenv("SNESRECOMP_WS_GSU_TASK_RANGE");
    if (!path || !path[0] || !range) return;
    char *end = NULL;
    long first = strtol(range, &end, 10);
    if (end == range || *end != ',') return;
    const char *cursor = end + 1;
    long last = strtol(cursor, &end, 10);
    if (end == cursor || *end || first < 0 || last < first) return;

    extern int snes_frame_counter;
    if (snes_frame_counter < first || snes_frame_counter > last ||
        !fx || !fx->ram || !fx->ram_size)
        return;
    uint32_t record[22] = {
        (uint32_t)snes_frame_counter, pc24, fx->ram_size,
        g_ram[0x39] | ((uint32_t)g_ram[0x3a] << 8),
        g_ram[0x3b] | ((uint32_t)g_ram[0x3c] << 8), 0x8000};
    for (unsigned i = 0; i < 16; ++i)
        record[6 + i] = fx->r[i].data;
    FILE *file = fopen(path, "ab");
    if (!file) return;
    fwrite(record, sizeof(record), 1, file);
    fwrite(fx->ram, 1, fx->ram_size, file);
    fwrite(g_ram + 0x18000, 1, 0x8000, file);
    fclose(file);
}

static void InstallYoshiGsuAuditHook(void)
{
    if (s_gsu_audit_hook_installed || !g_snes ||
        !g_snes->cart || !g_snes->cart->superfx)
        return;
    if (superfx_set_pc_hook(g_snes->cart->superfx, 0x09fa26,
                            YoshiGsuTaskAuditHook, NULL))
        s_gsu_audit_hook_installed = true;
    if (getenv("SNESRECOMP_WS_GSU_REPLAY_FILE")) {
        SuperFx *fx = g_snes->cart->superfx;
        superfx_set_enhancement_mode(fx,
                                    kSuperFxEnhancement_PresentationReplay);
        superfx_set_presentation_replay(fx, 0x09, 0xf9e8,
                                        YoshiGsuReplayAuditPrepare,
                                        YoshiGsuReplayAuditComplete, NULL);
    }
}

/* The qualified scene family follows the usable drawable. Unknown scenes
 * and unauthored west edges retain native framing, except1-1's verified
 * original-art decorative boundary continuation.
 * Fixed-width overrides remain available for reproducible renderer tests. */
void GamePrepareFrame(int drawable_w, int drawable_h,
                      int *frame_w, int *frame_h)
{
    s_frame_width = *frame_w;
    const int fixed_extra = PpuWsExtraOverride();
    if (fixed_extra >= 0) {
        *frame_w = 256 + 2 * fixed_extra;
        *frame_h = 224;
    } else {
        *frame_w = 256;
        *frame_h = 224;
        YoshiDisplayGeometry geometry;
        if ((g_ram[0x118] == 0x0e || g_ram[0x118] == 0x0f) &&
            KnownBackground() &&
            yoshi_display_geometry(drawable_w, drawable_h, (YoshiInsets){0},
                                   7.0 / 6.0, &geometry)) {
            int extra = (geometry.logical_width - 256 + 1) / 2;
            if (extra > 95) extra = 95; /* Native nine-bit OAM range. */
            const unsigned x = g_ram[0x39] | ((unsigned)g_ram[0x3a] << 8);
            /* No playable BG1 world exists west of zero. Only1-1 has
             * a separately qualified original-art continuation there. */
            if (extra > 0 && (x >= (unsigned)extra || KnownBoundaryExtension()))
                *frame_w = 256 + 2 * extra;
        }
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
    YoshiAchievementsBegin();
    extern void YoshiInstallAndroidLifecycle(void);
    YoshiInstallAndroidLifecycle();
    InstallYoshiGsuAuditHook();
    if (!g_ppu || !(g_ppu->extraLeftCur || g_ppu->extraRightCur)) return;

    for (int layer = 0; layer < 4; ++layer) {
        const unsigned address = 0x39u + (unsigned)layer * 4u;
        const uint32_t world_x = g_ram[address] | ((uint32_t)g_ram[address + 1] << 8);
        const uint32_t world_y = g_ram[address + 2] | ((uint32_t)g_ram[address + 3] << 8);
        WsShadowSetWorld(layer, layer == 1 || layer == 2 ? g_ppu->hScroll[layer] : world_x,
                         layer == 1 || layer == 2 ? g_ppu->vScroll[layer] : world_y);
        WsShadowSetScroll(layer, g_ppu->hScroll[layer], g_ppu->vScroll[layer]);
        /* BG1 resolves through the original level map. BG2/BG3 have their own
         * PPU phase and authored-plane policies; no BG1 camera inheritance. */
        WsShadowSetRetainHistory(layer, layer > 2);
        WsShadowSetWestKeep(layer, 40);
        WsShadowSetEastKeep(layer, 40);
    }
    WsShadowFrame(g_ppu);
}

static bool AuditFrameSelected(const char *frames, unsigned number)
{
    if (!frames || !frames[0]) return false;
    const char *cursor = frames;
    while (*cursor) {
        char *end = NULL;
        unsigned long value = strtoul(cursor, &end, 10);
        if (end == cursor) {
            ++cursor;
            continue;
        }
        if (value == number) return true;
        cursor = end;
        while (*cursor && *cursor != ',') ++cursor;
        if (*cursor == ',') ++cursor;
    }
    return false;
}

/* Opt-in exact tile audit for routes where a former margin later enters the
 * native view. Region coordinates are world tile indices; the API is read-only
 * and reports whether each entry came from real PPU writes or a prefill guess. */
static void AuditShadowTiles(unsigned number)
{
    const char *frames = getenv("SNESRECOMP_WS_AUDIT_FRAMES");
    const char *rect = getenv("SNESRECOMP_WS_AUDIT_RECT");
    const char *path = getenv("SNESRECOMP_WS_AUDIT_FILE");
    if (!path || !path[0] || !AuditFrameSelected(frames, number) || !rect)
        return;

    char *end = NULL;
    long x0 = strtol(rect, &end, 10);
    if (end == rect || *end != ',') return;
    const char *cursor = end + 1;
    long x1 = strtol(cursor, &end, 10);
    if (end == cursor || *end != ',') return;
    cursor = end + 1;
    long y0 = strtol(cursor, &end, 10);
    if (end == cursor || *end != ',') return;
    cursor = end + 1;
    long y1 = strtol(cursor, &end, 10);
    if (end == cursor || *end || x0 < 0 || x1 < x0 || y0 < 0 || y1 < y0)
        return;

    const char *layer_text = getenv("SNESRECOMP_WS_AUDIT_LAYER");
    long layer = layer_text ? strtol(layer_text, NULL, 10) : 1;
    if (layer < 1 || layer > 4) return;
    FILE *file = fopen(path, "a");
    if (!file) return;
    for (long ty = y0; ty <= y1; ++ty) {
        for (long tx = x0; tx <= x1; ++tx) {
            uint16_t entry = 0;
            int state = WsShadowDebugCell((int)layer - 1, (uint32_t)tx,
                                          (uint32_t)ty, &entry);
            fprintf(file, "%u,%ld,%ld,%d,%04x\n", number, tx, ty, state,
                    entry);
        }
    }
    fclose(file);
}

void GameEndFrame(const uint8_t *field, unsigned number)
{
    YoshiAchievementsEnd();
    const char *directory = getenv("SNESRECOMP_WS_FRAME_DIR");
    if (directory && AuditFrameSelected(getenv("SNESRECOMP_WS_FRAME_NUMBERS"), number)) {
        char path[4096];
        snprintf(path, sizeof(path), "%s/%u.bgrx", directory, number);
        FILE *image = fopen(path, "wb");
        if (image) {
            fwrite(field, (size_t)s_frame_width * 224u * 4u, 1, image);
            fclose(image);
        }
    }
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
    AuditShadowTiles(number);
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
    memset(s_map_rows, 0, sizeof(s_map_rows));
    s_bg2_plane = 0;
    s_boundary_extension = KnownBoundaryExtension();
    memset(s_bg3_rows, 0, sizeof(s_bg3_rows));
    PpuSetWidescreenBg3Widen(g_ppu,
        (KnownPlane(1, UINT64_C(0x5f9149de60fa58c4)) ||
         KnownPlane(1, UINT64_C(0x096eb6f63784aef3)) ||
         KnownPlane(1, UINT64_C(0xbf3d1aeecd343863)) ||
         KnownPlane(1, UINT64_C(0x092f46b0b1817336)) ||
         KnownPlane(1, UINT64_C(0x651dd75517727c34)) ||
         KnownPlane(1, UINT64_C(0xa17e5ff8fbe84e98))) ? 1 : 0);
    if (g_ppu->extraLeftCur || g_ppu->extraRightCur)
        snes_beam_frame_driver_draw_ppu_frame_observed(BlackUnverifiedMargins, NULL);
    else
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
