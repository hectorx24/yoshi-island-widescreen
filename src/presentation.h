#ifndef YOSHI_PRESENTATION_H
#define YOSHI_PRESENTATION_H
#include <stddef.h>
#include <stdint.h>
#include "desktop/sdl_compat.h"
int YoshiComposeFrame(uint8_t *dst, size_t pitch, const uint8_t *field,
                       int width, int height, double alpha);
bool YoshiRenderPresentation(SDL_Renderer *renderer, SDL_Texture *texture,
                              const SDL_Rect *source, const SDL_Rect *destination);
void YoshiCapturePresentation(SDL_Renderer *renderer, SDL_Texture *texture,
                               const SDL_Rect *source, const SDL_Rect *destination);
void YoshiPresentationReset(void);
#endif
