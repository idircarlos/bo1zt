#include "logic/state.h"
#include "logic/game.h"
#include <stdlib.h>

State *stateCreate(void) {
    State *state = (State *)malloc(sizeof(State));
    gameInit(&state->activeGame, 1);
    state->isGameAttached = false;
    state->isZombiesGameOngoing = false;
    state->isZombiesGamePaused = false;
    state->gameResets = 0;
    return state;
}

void stateGameClear(State *state) {
    state->isGameAttached = false;
    state->isZombiesGameOngoing = false;
    state->isZombiesGamePaused = false;
    state->gameResets = 0;
}

void stateDestroy(State *state) {
    if (!state) return;
    free(state);
}
