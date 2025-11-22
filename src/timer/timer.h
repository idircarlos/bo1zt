#ifndef TIMER_H_
#define TIMER_H_

#include <stdbool.h>

typedef struct Timer Timer;

Timer *timerCreate();
void timerStart(Timer *timer);
void timerPause(Timer *timer);
void timerRestart(Timer *timer, bool pause);
double timerGetElapsedMillis(Timer *timer);
void timerDestroy(Timer *timer);

#endif // TIMER_H_