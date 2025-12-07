#ifndef STATE_H_
#define STATE_H_

#include <stdbool.h>
#include "logic/game.h"

typedef struct State {
    Game *activeGame;
    bool isGameAttached;
    bool isTimRunning;
    bool isZombiesGameOngoing;
    bool isZombiesGamePaused;
    int gameResets;
} State;

State *stateCreate(void);
void stateGameClear(State *state);
void stateDestroy(State *state);

#endif // STATE_H_
