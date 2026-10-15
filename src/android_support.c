#include "common_rtl.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __ANDROID__
#include "desktop/sdl_compat.h"
#endif

void YoshiSaveDiagnostic(const char *event, int ok);

#ifdef __ANDROID__
static bool SDLCALL YoshiLifecycleWatch(void *context, SDL_Event *event) {
    (void)context;
    if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND) {
        /* SDL Android queues this on its native event-pump thread and then
         * blocks immediately. A queued-event switch cannot save until resume;
         * SDL explicitly requires lifecycle handling in a filter/watch. */
        YoshiSaveDiagnostic("background", RtlTryWriteSram());
    }
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
        const char *path = getenv("SNESRECOMP_INPUT_DIAG");
        FILE *file = path && path[0] ? fopen(path, "a") : NULL;
        if (file) {
            fprintf(file, "%s key=%08x\n", event->type == SDL_EVENT_KEY_DOWN ? "down" : "up",
                    (unsigned)event->key.key);
            fclose(file);
        }
    }
    return true;
}
#endif

void YoshiInstallAndroidLifecycle(void) {
#ifdef __ANDROID__
    static bool installed;
    if (!installed) {
        const char *log = getenv("SNESRECOMP_TEST_LOG");
        if (log && log[0]) {
            freopen(log, "w", stderr);
            setvbuf(stderr, NULL, _IONBF, 0);
        }
        installed = SDL_AddEventWatch(YoshiLifecycleWatch, NULL);
        const char *input = getenv("SNESRECOMP_INPUT_DIAG");
        FILE *ready = installed && input && input[0] ? fopen(input, "a") : NULL;
        if (ready) { fputs("ready\n", ready); fclose(ready); }
    }
#endif
}

/* Opt-in metadata for automated lifecycle tests. Called only on the native
 * emulation thread at load/save boundaries, never from the Java UI thread. */
void YoshiSaveDiagnostic(const char *event, int ok) {
    const char *path = getenv("SNESRECOMP_SAVE_DIAG");
    if (!path || !path[0] || !g_sram || g_sram_size <= 0) return;
    uint32_t hash = 2166136261u;
    for (int i = 0; i < g_sram_size; ++i)
        hash = (hash ^ g_sram[i]) * 16777619u;
    FILE *file = fopen(path, "a");
    if (file) {
        fprintf(file, "%s status=%s bytes=%d fnv=%08x\n",
                event, ok ? "ok" : "FAILED", g_sram_size, hash);
        fclose(file);
    }
}
