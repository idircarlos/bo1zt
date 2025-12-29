#include "logic/game/level.h"
#include "logic/game/round.h"
#include <string.h>

RoundType levelGetSpecialRound(Level level) {
    switch (level) {
        case LEVEL_FIVE: return RT_THIEF;
        case LEVEL_ASCENSION: return RT_MONKEYS;
        case LEVEL_CALL_OF_THE_DEAD: return RT_GEORGE;
        case LEVEL_KINO_DER_TOTEN: return RT_DOGS;
        default: return RT_NORMAL;
    }
}

Level levelGetFromId(const char *levelId) {
    if (strcmp(levelId, "zombie_theater") == 0) return LEVEL_KINO_DER_TOTEN;
    if (strcmp(levelId, "zombie_cosmodrome") == 0) return LEVEL_ASCENSION;
    if (strcmp(levelId, "zombie_pentagon") == 0) return LEVEL_FIVE;
    if (strcmp(levelId, "zombie_coast") == 0) return LEVEL_CALL_OF_THE_DEAD;
    if (strcmp(levelId, "frontend") == 0) return LEVEL_MAIN_MENU;
    return LEVEL_INVALID;
}

bool levelIsMonitored(Level level) {
    switch (level) {
        case LEVEL_KINO_DER_TOTEN: return true;
        case LEVEL_FIVE: return true;
        case LEVEL_ASCENSION: return true;
        default: return false;
    }
}