#include "achievement_memory.h"
#include <assert.h>
#include <stdio.h>
static uint8_t wram[0x20000], sram[0x8000];
int main(void) {
    uint8_t result[4] = {0};
    wram[0] = 0x17; wram[0x1fffe] = 0x28; wram[0x1ffff] = 0x39;
    sram[0] = 0x4a; sram[1] = 0x5b; sram[0x7fff] = 0x6c;
    assert(yoshi_achievement_read(0, result, 1, wram, sram, sizeof(sram)) == 1 && result[0] == 0x17);
    assert(yoshi_achievement_read(0x1fffe, result, 4, wram, sram, sizeof(sram)) == 4);
    assert(result[0] == 0x28 && result[1] == 0x39 && result[2] == 0x4a && result[3] == 0x5b);
    assert(yoshi_achievement_read(0x27fff, result, 2, wram, sram, sizeof(sram)) == 1 && result[0] == 0x6c);
    assert(yoshi_achievement_read(0x28000, result, 1, wram, sram, sizeof(sram)) == 0);
    assert(yoshi_achievement_read(0xa0000, result, 1, wram, sram, sizeof(sram)) == 0);
    assert(yoshi_achievement_read(0x20000, result, 1, wram, NULL, 0) == 0);
    puts("RetroAchievements SNES WRAM/Super FX RAM boundaries verified");
}
