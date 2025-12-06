#include "state.h"
#include <stdlib.h>

State *stateCreate(void) {
    State *state = (State *)malloc(sizeof(State));
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
