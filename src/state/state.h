#ifndef STATE_H_
#define STATE_H_

#include <stdbool.h>
#include "../timer/timer.h"

typedef struct State State;

State *stateCreate();
void stateGameClear(State *state);
bool stateIsGameAttached(State *state);
void stateSetGameAttached(State *state, bool attached);
bool stateIsTimRunning(State *state);
void stateSetTimRunning(State *state, bool running);
bool stateIsZombiesGameOngoing(State *state);
void stateSetZombiesGameOngoing(State *state, bool ongoing);
bool stateIsZombiesGamePaused(State *state);
void stateSetZombiesGamePaused(State *state, bool paused);
void stateSetGameResets(State *state, int resets);
int stateGetGameResets(State *state);
Timer *stateGetTimer(State *state);
Timer *stateGetRoundTimer(State *state);
void stateDestroy(State *state);

#endif // STATE_H_