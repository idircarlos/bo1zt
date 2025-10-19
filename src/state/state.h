#ifndef STATE_H_
#define STATE_H_

#include <stdbool.h>

typedef struct State State;

State *stateCreate(int isZombiesGameActive, int gameResets);
void stateReset(State *state);
bool stateIsZombiesGameActive(State *state);
bool stateSetZombiesGameActive(State *state, bool active);
void stateSetGameResets(State *state, int resets);
int stateGetGameResets(State *state);
void stateDestroy(State *state);

#endif // STATE_H_