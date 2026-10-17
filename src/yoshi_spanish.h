#ifndef YOSHI_SPANISH_H
#define YOSHI_SPANISH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void YoshiApplySpanishTranslation(uint8_t *rom, uint32_t rom_size);
bool YoshiIsSpanishEnabled(void);
void YoshiSetSpanishEnabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* YOSHI_SPANISH_H */
