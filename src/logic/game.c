#include "logic/game.h"
#include "logger/logger.h"
#include "logic/game/level.h"
#include "logic/game/round.h"
#include <string.h>

void gameInit(Game *game, int players) {
    game->levelName = LEVEL_INVALID;
    game->players = players;
    gameClear(game);
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
    game->zombiesTotal = 0;
    game->quickRevivesDrunk = 0;
    game->drops = 0;
    memset(game->rounds, 0, sizeof(game->rounds));
    roundClear(&game->currentRound);
    return true;
}

bool gameEnd(Game *game, int endTimestamp) {
    game->endTimestamp = endTimestamp;
    return true;
}

bool gameUpdateElapsed(Game *game, int levelElapsed) {
    game->elapsed = levelElapsed - game->startTimestamp;
    return true;
}

bool gameRoundStarted(Game *game, int startTimestamp) {
    int nextNumber = game->currentRound.number + 1;
    roundInit(&game->currentRound, nextNumber, game->players);
    roundStart(&game->currentRound, startTimestamp);
    return true;
}

bool gameRoundEnded(Game *game, int endTimestamp) {
    roundEnd(&game->currentRound, endTimestamp);
    game->rounds[game->currentRound.number - 1] = game->currentRound;
    return true;
}

bool gameZombieKilled(Game *game) {
    game->zombiesTotal++;
    roundZombieKilled(&game->currentRound);
    return true;
}

bool gamePowerupDropped(Game *game) {
    game->drops++;
    roundPowerupDropped(&game->currentRound);
    return true;
}

void gamePrint(Game *game) {
    LOG_INFO("Game { elapsed: %d, start: %d, end: %d, players: %d, qr: %d, zombies: %d, drops: %d, level: %d }\n",
        game->elapsed, game->startTimestamp, game->endTimestamp, game->players,
        game->quickRevivesDrunk, game->zombiesTotal, game->drops, game->levelName);
}


