#include "logic/game/round.h"
#include "logger/logger.h"
#include <stdlib.h>
#include <stdbool.h>

Round *roundCreate() {
    Round *round = (Round*)malloc(sizeof(Round));
    if (!round) {
        LOG_ERROR("Couldn't create Round object\n");
        return NULL;
    }
    roundClear(round);
    return round;
}

bool roundStart(Round *round, int startTimestamp) {
    if (!round) {
        LOG_ERROR("Couldn't start Round because is an invalid object\n");
        return false;
    }
    round->startTimestamp = startTimestamp;
    round->isSpecial = false; // TODO
    round->drops = 0;
    return true;
}

bool roundClear(Round *round) {
    round->number = 0;
    round->elapsed = 0;
    round->startTimestamp = 0;
    round->endTimestamp = 0;
    round->drops = 0;
    round->isSpecial = false;
    return true;
}

bool roundStarted(Round *round) {
    return round->startTimestamp != 0;
}

bool roundEnded(Round *round) {
    return round->endTimestamp != 0;
}

bool roundRunning(Round *round) {
    return roundStarted(round) && !roundEnded(round);
}


bool roundEnd(Round *round, int endTimestamp) {
    round->endTimestamp = endTimestamp;
    return true;
}

void roundDestroy(Round *round) {
    if (round) {
        free(round);
    }
}
