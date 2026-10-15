#include "achievement_memory.h"
#include <string.h>
size_t yoshi_achievement_read(uint32_t address, uint8_t *buffer, size_t bytes,
                              const uint8_t *wram, const uint8_t *sram, size_t sram_size) {
    size_t copied = 0;
    if (!buffer) return 0;
    while (copied < bytes) {
        const uint8_t *source;
        size_t available;
        if (address < 0x20000 && wram) {
            source = wram + address; available = 0x20000 - address;
        } else if (address >= 0x20000 && address - 0x20000 < sram_size && sram) {
            source = sram + address - 0x20000;
            available = sram_size - (address - 0x20000);
        } else break;
        size_t length = bytes - copied;
        if (length > available) length = available;
        memcpy(buffer + copied, source, length);
        copied += length;
        if (length > UINT32_MAX - address) break;
        address += (uint32_t)length;
    }
    return copied;
}
