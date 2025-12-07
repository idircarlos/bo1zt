#include "logic/state.h"
#include "logic/game.h"
#include <stdlib.h>

State *stateCreate(void) {
    State *state = (State *)malloc(sizeof(State));
    state->activeGame = gameCreate();
    state->isGameAttached = false;
    state->isTimRunning = false;
    state->isZombiesGameOngoing = false;
    state->isZombiesGamePaused = false;
    state->gameResets = 0;
    return state;
}

void stateGameClear(State *state) {
    // We don't clear TIM state because is not related to the game
    state->isGameAttached = false;
    state->isZombiesGameOngoing = false;
    state->isZombiesGamePaused = false;
    state->gameResets = 0;
}

void stateDestroy(State *state) {
    if (!state) return;
    gameDestroy(state->activeGame);
    free(state);
}
