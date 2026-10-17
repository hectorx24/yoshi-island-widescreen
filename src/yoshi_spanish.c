#include "yoshi_spanish.h"
#include "yoshi_spanish_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool s_spanish_enabled = true;
static uint8_t *s_english_backup[YOSHI_SPANISH_RECORD_COUNT];
static bool s_backup_initialized = false;
static uint8_t *s_active_rom = NULL;
static uint32_t s_active_rom_size = 0;

void YoshiApplySpanishTranslation(uint8_t *rom, uint32_t rom_size) {
    if (!rom || rom_size < 0x200000) return;
    s_active_rom = rom;
    s_active_rom_size = rom_size;

    const char *lang = getenv("SNESRECOMP_LANG");
    if (lang && (strcmp(lang, "en") == 0 || strcmp(lang, "off") == 0)) {
        s_spanish_enabled = false;
    } else {
        s_spanish_enabled = true;
    }

    if (!s_backup_initialized) {
        for (int i = 0; i < YOSHI_SPANISH_RECORD_COUNT; ++i) {
            const YoshiPatchRecord *rec = &s_spanish_records[i];
            if (rec->offset + rec->length <= rom_size) {
                s_english_backup[i] = (uint8_t *)malloc(rec->length);
                if (s_english_backup[i]) {
                    memcpy(s_english_backup[i], rom + rec->offset, rec->length);
                }
            } else {
                s_english_backup[i] = NULL;
            }
        }
        s_backup_initialized = true;
    }

    if (s_spanish_enabled) {
        for (int i = 0; i < YOSHI_SPANISH_RECORD_COUNT; ++i) {
            const YoshiPatchRecord *rec = &s_spanish_records[i];
            if (rec->offset + rec->length <= rom_size) {
                memcpy(rom + rec->offset, rec->data, rec->length);
            }
        }
        fprintf(stderr, "[yoshi-es] Spanish translation (Traducciones Wave) applied to in-memory ROM (%d records)\n",
                YOSHI_SPANISH_RECORD_COUNT);
    }
}

bool YoshiIsSpanishEnabled(void) {
    return s_spanish_enabled;
}

void YoshiSetSpanishEnabled(bool enabled) {
    if (s_spanish_enabled == enabled) return;
    s_spanish_enabled = enabled;
    if (!s_active_rom || !s_backup_initialized) return;

    if (s_spanish_enabled) {
        for (int i = 0; i < YOSHI_SPANISH_RECORD_COUNT; ++i) {
            const YoshiPatchRecord *rec = &s_spanish_records[i];
            if (rec->offset + rec->length <= s_active_rom_size) {
                memcpy(s_active_rom + rec->offset, rec->data, rec->length);
            }
        }
        fprintf(stderr, "[yoshi-es] Switched in-memory language to Spanish\n");
    } else {
        for (int i = 0; i < YOSHI_SPANISH_RECORD_COUNT; ++i) {
            const YoshiPatchRecord *rec = &s_spanish_records[i];
            if (s_english_backup[i] && rec->offset + rec->length <= s_active_rom_size) {
                memcpy(s_active_rom + rec->offset, s_english_backup[i], rec->length);
            }
        }
        fprintf(stderr, "[yoshi-es] Restored in-memory language to English\n");
    }
}
