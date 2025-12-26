#ifndef GAME_H_
#define GAME_H_

#include "logic/game/level.h"
#include "logic/game/round.h"
#include "logic/game/powerup.h"
#include "logic/game/trade.h"

typedef struct Game {
    int elapsed;
    int startTimestamp;
    int endTimestamp;
    int powerOnTimestamp;
    int powerOnRound;
    int players;
    int quickRevivesDrunk;
    float movementSpeed;
    int totalZombies;
    int drops;
    int numPerks;
    int lastPerkAcquiredOnRound;
    int currentEntities;
    int maxEntities;
    Level levelName;
    Round currentRound;
    Round rounds[MAX_ROUNDS];
    Trade currentTrade;
    Trade trades[MAX_TRADES];
    int tradeCount;
} Game;

void gameInit(Game *game, int players);
bool gameStart(Game *game, Level level, int startTimestamp);
bool gameStarted(Game *game);
bool gameEnded(Game *game);
bool gameRunning(Game *game);
bool gameClear(Game *game);
bool gameEnd(Game *game, int endTimestamp);
bool gameUpdateElapsed(Game *game, int levelElapsed);
bool gameRoundStarted(Game *game, int startTimestamp, bool special);
bool gameRoundEnded(Game *game, int endTimestamp);
bool gameZombieKilled(Game *game);
bool gamePowerupDropped(Game *game, Powerup powerup);
bool gamePowerupNewCycle(Game *game);
bool gamePowerOn(Game *game, int timestamp);
bool gameSetNumPerks(Game *game, int numPerks);
bool gamePerkAcquired(Game *game);
bool gamePerkLost(Game *game);
int gameNumSpecialRounds(Game *game);
const char* gameNextPotentialSpecialRounds(Game *game);
bool gameTradeHit(Game *game);
void gamePrint(Game *game);

#endif // GAME_H_