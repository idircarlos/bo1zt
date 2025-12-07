#ifndef GAME_H_
#define GAME_H_

#include "logic/game/level.h"
#include "logic/game/round.h"

typedef struct Game {
    int elapsed;
    int startTimestamp;
    int endTimestamp;
    Level levelName;
    Round *currentRound;
    int quickRevivesDrunk;
} Game;

Game *gameCreate();
bool gameStart(Game *game, Level level, int startTimestamp);
bool gameStarted(Game *game);
bool gameEnded(Game *game);
bool gameRunning(Game *game);
bool gameClear(Game *game);
bool gameEnd(Game *game, int endTimestamp);
void gameDestroy(Game *game);

#endif // GAME_H_