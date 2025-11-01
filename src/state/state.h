#ifndef STATE_H_
#define STATE_H_

#include <stdbool.h>

typedef struct State State;

State *stateCreate();
void stateGameClear(State *state);
bool stateIsGameAttached(State *state);
void stateSetGameAttached(State *state, bool attached);
bool stateIsTimRunning(State *state);
void stateSetTimRunning(State *state, bool running);
bool stateIsZombiesGameActive(State *state);
void stateSetZombiesGameActive(State *state, bool active);
void stateSetGameResets(State *state, int resets);
int stateGetGameResets(State *state);
void stateDestroy(State *state);

#endif // STATE_H_