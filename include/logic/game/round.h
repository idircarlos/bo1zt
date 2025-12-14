#ifndef ROUND_H_
#define ROUND_H_

#include <stdbool.h>

#define MAX_ROUNDS 256

typedef enum {
    SR_NONE = -1,
    SR_DOGS,
} SpecialRound;

typedef struct Round {
    int number;
    int players;
    int elapsed;
    int startTimestamp;
    int endTimestamp;
    int drops;
    int zombiesLeft;
    bool isSpecial;
} Round;

void roundInit(Round *round, int number, int players);
bool roundStart(Round *round, int startTimestamp);
bool roundClear(Round *round);
bool roundStarted(Round *round);
bool roundEnded(Round *round);
bool roundRunning(Round *round);
bool roundEnd(Round *round, int endTimestamp);
bool roundUpdateElapsed(Round *round, int levelElapsed);
int roundZombieCount(Round *round);
float roundHordeCount(Round *round);
bool roundZombieKilled(Round *round);
bool roundPowerupDropped(Round *round);
void roundPrint(Round *round);

#endif // ROUND_H_