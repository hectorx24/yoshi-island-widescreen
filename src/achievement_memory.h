#ifndef YOSHI_ACHIEVEMENT_MEMORY_H
#define YOSHI_ACHIEVEMENT_MEMORY_H
#include <stddef.h>
#include <stdint.h>
/* rcheevos SNES logical map: $000000-$01FFFF WRAM; $020000+ cartridge
 * RAM. No SA-1 I-RAM and no invented mirroring beyond this cartridge's RAM. */
size_t yoshi_achievement_read(uint32_t address, uint8_t *buffer, size_t bytes,
                              const uint8_t *wram, const uint8_t *sram, size_t sram_size);
#endif
