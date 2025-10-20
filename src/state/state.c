#include "state.h"
#include <stdlib.h>

struct State {
    bool isZombiesGameActive;
    int gameResets;
};

State *stateCreate(int isZombiesGameActive, int gameResets) {
    State *state = (State*)malloc(sizeof(State));
    state->isZombiesGameActive = isZombiesGameActive;
    state->gameResets = gameResets;
    return state;
}

void stateReset(State *state) {
    state->isZombiesGameActive = false;
    state->gameResets = 0;
}

void stateDestroy(State *state) {
    free(state);
}

bool stateIsZombiesGameActive(State *state) {
    return state->isZombiesGameActive;
}

int stateGetGameResets(State *state) {
    return state->gameResets;
}

void stateSetZombiesGameActive(State *state, bool active) {
    state->isZombiesGameActive = active;
}

void stateSetGameResets(State *state, int resets) {
    state->gameResets = resets;
}
