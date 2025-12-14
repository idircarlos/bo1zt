#include "logic/game/round.h"
#include "logger/logger.h"
#include <stdbool.h>

#define MAX_ZOMBIES_PER_HORDE  24
#define ZOMBIES_PER_PLAYER  6

static const float roundEarlyScale[] = { 0.25f, 0.3f, 0.5f, 0.7f, 0.9f };

void roundInit(Round *round, int number, int players) {
    roundClear(round);
    round->number = number;
    round->players = players;
}

bool roundStart(Round *round, int startTimestamp) {
    round->startTimestamp = startTimestamp;
    round->isSpecial = false; // TODO
    round->zombiesLeft = roundZombieCount(round);
    round->drops = 0;
    return true;
}

bool roundClear(Round *round) {
    round->elapsed = 0;
    round->startTimestamp = 0;
    round->endTimestamp = 0;
    round->drops = 0;
    round->isSpecial = false;
    round->number = 0;
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

bool roundUpdateElapsed(Round *round, int levelElapsed) {
    round->elapsed = levelElapsed - round->startTimestamp;
    return true;
}

int roundZombieCount(Round *round) {
    int n = round->number;
    int p = round->players;
    float mult = (n < 5) ? 1.0f : (n / 5.0f);
    if (n >= 10) mult *= n * 0.15f;
    
    float playerFactor = (p == 1) ? 0.5f : (p - 1);
    int base = (int)(MAX_ZOMBIES_PER_HORDE + playerFactor * ZOMBIES_PER_PLAYER * mult);

    if (n >= 1 && n <= 5) {
        return (int)(base * roundEarlyScale[n - 1]);
    }
    return base;
}

float roundHordeCount(Round *round) {
    return roundZombieCount(round) / (float)MAX_ZOMBIES_PER_HORDE;
}

bool roundZombieKilled(Round *round) {
    round->zombiesLeft--;
    return true;
}

bool roundPowerupDropped(Round *round) {
    round->drops++;
    return true;
}

void roundPrint(Round *round) {
    LOG_INFO("Round { number: %d, elapsed: %d, start: %d, end: %d, drops: %d, zombiesLeft: %d, special: %s }\n",
        round->number, round->elapsed, round->startTimestamp, round->endTimestamp,
        round->drops, round->zombiesLeft, round->isSpecial ? "true" : "false");
}


