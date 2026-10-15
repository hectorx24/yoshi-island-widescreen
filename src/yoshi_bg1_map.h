#ifndef YOSHI_BG1_MAP_H
#define YOSHI_BG1_MAP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* USA v1.1 CODE_109089 screen lookup + FXCODE_09FA28 MAP16 expansion.
 * Read-only: coordinates are absolute 8px world tiles, not VRAM ring cells. */
bool yoshi_bg1_tile(const uint8_t *wram, const uint8_t *gsu_ram,
                    const uint8_t *rom, size_t rom_size,
                    unsigned tx, unsigned ty, uint16_t *entry);
#endif
