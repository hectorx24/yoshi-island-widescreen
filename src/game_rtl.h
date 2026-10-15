#ifndef GAME_RTL_H
#define GAME_RTL_H

#include "common_cpu_infra.h"

/* One frame of Yoshi's Island. See game_rtl.c — this is the port. */
void GameRunOneFrame(void);
void GameDrawPpuFrame(void);
void GameSessionReset(void);
void GamePrepareFrame(int drawable_w, int drawable_h,
                      int *frame_w, int *frame_h);

extern const RtlGameInfo kGameInfo;

#endif /* GAME_RTL_H */
