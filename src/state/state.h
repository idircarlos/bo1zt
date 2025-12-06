#ifndef STATE_H_
#define STATE_H_

#include <stdbool.h>
#include "../timer/timer.h"

typedef struct State {
    bool isGameAttached;
    bool isTimRunning;
    bool isZombiesGameOngoing;
    bool isZombiesGamePaused;
    int gameResets;
    Timer *timer;
    Timer *roundTimer;
} State;

State *stateCreate(void);
void stateGameClear(State *state);
void stateDestroy(State *state);

#endif // STATE_H_
