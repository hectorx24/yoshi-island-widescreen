#ifndef YOSHI_ACHIEVEMENTS_H
#define YOSHI_ACHIEVEMENTS_H
/* Called only by the emulation thread, once per original field. */
void YoshiAchievementsBegin(void);
void YoshiAchievementsEnd(void);
void YoshiAchievementsReset(void);
#endif
