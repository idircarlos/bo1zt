#include "logic/game.h"
#include "logger/logger.h"
#include "logic/game/level.h"
#include "logic/game/round.h"
#include <stdlib.h>

Game *gameCreate() {
    Game *game = (Game*)malloc(sizeof(Game)); 
    if (!game) {
        LOG_ERROR("Couldn't create Game object\n");
        return NULL;
    }
    game->currentRound = roundCreate();
    game->levelName = LEVEL_INVALID;
    gameClear(game);
    return game;
}

bool gameStart(Game *game, Level level, int startTimestamp) {
    if (!game) {
        LOG_ERROR("Couldn't start game because Game is an invalid object\n");
        return NULL;
    }
    game->levelName = level;
    game->startTimestamp = startTimestamp;
    return true;
}

bool gameStarted(Game *game) {
    return game->startTimestamp != 0;
}

bool gameEnded(Game *game) {
    return game->endTimestamp != 0;
}

bool gameRunning(Game *game) {
    return gameStarted(game) && !gameEnded(game);
}

bool gameClear(Game *game) {
    game->elapsed = 0;
    game->startTimestamp = 0;
    game->endTimestamp = 0;
    game->quickRevivesDrunk = 0;
    roundClear(game->currentRound);
    return true;
}

bool gameEnd(Game *game, int endTimestamp) {
    game->endTimestamp = endTimestamp;
    return true;
}

void gameDestroy(Game *game) {
    if (game) {
        free(game);
    }
}
