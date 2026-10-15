#include "yoshi_bg1_map.h"
static uint16_t word(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
bool yoshi_bg1_tile(const uint8_t *wram, const uint8_t *gsu_ram,
                    const uint8_t *rom, size_t rom_size,
                    unsigned tx, unsigned ty, uint16_t *entry) {
    if (!wram || !gsu_ram || !rom || !entry || rom_size < 0x100000 ||
        tx >= 512 || ty >= 256) return false;
    const unsigned x = tx * 8, y = ty * 8;
    /* CPU $10:6CAA is the Super FX RAM window, NOT WRAM $7E:6CAA. */
    const unsigned screen = gsu_ram[0x0caa + (y >> 8) * 16 + (x >> 8)] & 63;
    const unsigned map = screen * 512 + (y & 0xf0) * 2 + ((x & 0xf0) >> 3);
    const uint16_t id = word(wram + 0x18000 + map);
    /* GSU UMULT uses the low byte only; HIB selects the high-byte group.
     * Bank $4C uses the linear Super FX ROM mapping (offset $0C0000). */
    const uint16_t offset = (uint16_t)(0x33f2 +
        word(rom + 0x0c32a4 + (id >> 8) * 2) + (id & 255) * 8 +
        (ty & 1) * 4 + (tx & 1) * 2);
    *entry = (uint16_t)(rom[0x0c0000 + offset] |
        ((uint16_t)rom[0x0c0000 + (uint16_t)(offset + 1)] << 8));
    return true;
}
