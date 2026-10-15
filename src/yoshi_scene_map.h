#ifndef YOSHI_SCENE_MAP_H
#define YOSHI_SCENE_MAP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Original v1.1 Super FX CODE_08A980 bounded asset decompression. */
bool yoshi_asset_decode(const uint8_t *source, size_t source_size,
                         uint8_t *output, size_t capacity, size_t *written);
/* Exact v1.1 CODE_01E814 background descriptor. Returns the entire original
 * authored BG2 plane, never a VRAM snapshot or guessed missing tile. */
bool yoshi_bg2_asset(const uint8_t *rom, size_t rom_size, unsigned header,
                      uint8_t output[4096]);
#endif
