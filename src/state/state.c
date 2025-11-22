#include "state.h"
#include "../timer/timer.h"
#include <stdlib.h>

struct State {
    bool isGameAttached;
    bool isTimRunning;
    bool isZombiesGameOngoing;
    bool isZombiesGamePaused;
    int gameResets;
    Timer *timer;
    Timer *roundTimer;
};

State *stateCreate() {
    State *state = (State*)malloc(sizeof(State));
    state->isGameAttached = false;
    state->isTimRunning = false;
    state->isZombiesGameOngoing = false;
    state->isZombiesGamePaused = false;
    state->gameResets = 0;
    state->timer = timerCreate();
    state->roundTimer = timerCreate();
    return state;
}

void stateGameClear(State *state) {
    // We don't clear TIM state because is not related to the game
    state->isGameAttached = false;
    state->isZombiesGameOngoing = false;
    state->isZombiesGamePaused = false;
    state->gameResets = 0;
    timerRestart(state->timer, true);
    timerRestart(state->roundTimer, true);
}

void stateDestroy(State *state) {
    if (!state) return;
    timerDestroy(state->timer);
    timerDestroy(state->roundTimer);
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

bool stateIsZombiesGameOngoing(State *state) {
    return state->isZombiesGameOngoing;
}

void stateSetZombiesGameOngoing(State *state, bool ongoing) {
    state->isZombiesGameOngoing = ongoing;
}

bool stateIsZombiesGamePaused(State *state) {
    return state->isZombiesGamePaused;
}

void stateSetZombiesGamePaused(State *state, bool paused) {
    state->isZombiesGamePaused = paused;
}

int stateGetGameResets(State *state) {
    return state->gameResets;
}

void stateSetGameResets(State *state, int resets) {
    state->gameResets = resets;
}

Timer *stateGetTimer(State *state) {
    if (!state) return NULL;
    return state->timer;
}

Timer *stateGetRoundTimer(State *state) {
    if (!state) return NULL;
    return state->roundTimer;
}
