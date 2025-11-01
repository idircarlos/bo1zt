#include "state.h"
#include <stdlib.h>

struct State {
    bool isGameAttached;
    bool isTimRunning;
    bool isZombiesGameActive;
    int gameResets;
};

State *stateCreate() {
    State *state = (State*)malloc(sizeof(State));
    state->isGameAttached = false;
    state->isTimRunning = false;
    state->isZombiesGameActive = false;
    state->gameResets = 0;
    return state;
}

void stateGameClear(State *state) {
    // We don't clear TIM state because is not related to the game
    state->isGameAttached = false;
    state->isZombiesGameActive = false;
    state->gameResets = 0;
}

void stateDestroy(State *state) {
    free(state);
}

bool stateIsGameAttached(State *state) {
    return state->isGameAttached;
}

void stateSetGameAttached(State *state, bool attached) {
    state->isGameAttached = attached;
}

bool stateIsTimRunning(State *state) {
    return state->isTimRunning;
}

void stateSetTimRunning(State *state, bool running) {
    state->isTimRunning = running;
}

bool stateIsZombiesGameActive(State *state) {
    return state->isZombiesGameActive;
}

void stateSetZombiesGameActive(State *state, bool active) {
    state->isZombiesGameActive = active;
}

int stateGetGameResets(State *state) {
    return state->gameResets;
}

void stateSetGameResets(State *state, int resets) {
    state->gameResets = resets;
}
