#ifndef GAME_H_
#define GAME_H_

#include "logic/game/level.h"
#include "logic/game/round.h"

typedef struct Game {
    int elapsed;
    int startTimestamp;
    int endTimestamp;
    int players;
    int quickRevivesDrunk;
    int zombiesTotal;
    int drops; // TODO: Check how to recognize carpenter and sales drop to track cycles
    Level levelName;
    Round currentRound;
    Round rounds[MAX_ROUNDS];
} Game;

void gameInit(Game *game, int players);
bool gameStart(Game *game, Level level, int startTimestamp);
bool gameStarted(Game *game);
bool gameEnded(Game *game);
bool gameRunning(Game *game);
bool gameClear(Game *game);
bool gameEnd(Game *game, int endTimestamp);
bool gameUpdateElapsed(Game *game, int levelElapsed);
bool gameRoundStarted(Game *game, int startTimestamp);
bool gameRoundEnded(Game *game, int endTimestamp);
bool gameZombieKilled(Game *game);
bool gamePowerupDropped(Game *game);
void gamePrint(Game *game);

#endif // GAME_H_