#include "yoshi_scene_map.h"
#include <string.h>
static unsigned Word(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
bool yoshi_asset_decode(const uint8_t *source, size_t source_size,
                         uint8_t *output, size_t capacity, size_t *written) {
    size_t input = 0, n = 0;
    if (written) *written = 0;
    if (!source || !output || !written) return false;
    while (input < source_size) {
        unsigned command = source[input++];
        if (command == 255) { *written = n; return true; }
        unsigned operation = command >> 5;
        size_t count = (command & 31) + 1;
        if (operation == 7) {
            if (input >= source_size) return false;
            operation = (command >> 2) & 7;
            count = ((command & 3) << 8) + source[input++] + 1;
        }
        if (count > capacity - n) return false;
        if (operation == 0) {
            if (count > source_size - input) return false;
            memcpy(output + n, source + input, count); input += count; n += count;
        } else if (operation == 1 || operation == 3) {
            if (input >= source_size) return false;
            unsigned value = source[input++];
            for (size_t i = 0; i < count; ++i)
                output[n++] = (uint8_t)(value + (operation == 3 ? i : 0));
        } else if (operation == 2) {
            if (source_size - input < 2) return false;
            for (size_t i = 0; i < count; ++i) output[n++] = source[input + (i & 1)];
            input += 2;
        } else if (operation >= 4 && operation <= 6) {
            /* All negative command selectors use the same forward copy in
             * this game's GSU routine. Offsets are big-endian, output-relative.
             * Overlap is intentional, but unread/out-of-buffer RAM is rejected. */
            if (source_size - input < 2) return false;
            size_t offset = (size_t)source[input] * 256 + source[input + 1]; input += 2;
            if (offset >= n) return false;
            for (size_t i = 0; i < count; ++i) {
                output[n] = output[offset + i]; ++n;
            }
        } else return false;
    }
    return false;
}
bool yoshi_bg2_asset(const uint8_t *rom, size_t rom_size, unsigned header,
                      uint8_t output[4096]) {
    if (!rom || !output || rom_size != 0x200000 || header >= 32) return false;
    /* USA v1.1: exact 32-entry descriptor index at linear0E71E, descriptors
     * immediately after at0E75E (v1.0 label addresses differ by13 bytes).
     * Original compressed-resource pointer table06:F95E is linear03795E. */
    unsigned descriptor = Word(rom + 0x0e71e + header * 2);
    if (descriptor > 0xaf) return false;
    unsigned resource = rom[0x0e75e + descriptor + 1];
    const uint8_t *pointer = rom + 0x03795e + resource * 3;
    uint32_t bank = pointer[2];
    if (bank < 0x40 || bank > 0x5f) return false;
    size_t offset = ((bank & 0x3f) << 16) | Word(pointer);
    if (offset >= rom_size) return false;
    size_t written;
    return yoshi_asset_decode(rom + offset, rom_size - offset, output, 4096, &written) && written == 4096;
}
