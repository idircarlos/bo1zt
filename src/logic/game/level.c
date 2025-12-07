#include "logic/game/level.h"
#include <string.h>

SpecialRound levelGetSpecialRound(Level level) {
    switch (level) {
        case LEVEL_KINO_DER_TOTEN: return SR_DOGS;
        default: return SR_NONE;
    }
}

Level levelGetFromId(const char *levelId) {
    if (strcmp(levelId, "zombie_theater") == 0) return LEVEL_KINO_DER_TOTEN;
    if (strcmp(levelId, "frontend") == 0) return LEVEL_MAIN_MENU;
    return LEVEL_INVALID;
}

bool levelIsMonitored(Level level) {
    switch (level) {
        case LEVEL_KINO_DER_TOTEN: return true;
        case LEVEL_MAIN_MENU: return false;
        default: return false;
    }
}