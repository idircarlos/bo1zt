#ifndef ROUND_H_
#define ROUND_H_

#include <stdbool.h>

typedef enum {
    SR_NONE = -1,
    SR_DOGS,
} SpecialRound;

typedef struct Round {
    int number;
    int elapsed;
    int startTimestamp;
    int endTimestamp;
    int drops;
    bool isSpecial;
} Round;

Round *roundCreate();
bool roundStart(Round *round, int startTimestamp);
bool roundClear(Round *round);
bool roundStarted(Round *round);
bool roundEnded(Round *round);
bool roundRunning(Round *round);
bool roundEnd(Round *round, int endTimestamp);
void roundDestroy(Round *round);

#endif // ROUND_H_