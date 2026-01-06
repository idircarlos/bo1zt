#include "logic/game/level.h"
#include "logic/game/round.h"
#include <string.h>

RoundType levelGetSpecialRound(Level level) {
    switch (level) {
        case LEVEL_FIVE: return RT_THIEF;
        case LEVEL_ASCENSION: return RT_MONKEYS;
        case LEVEL_CALL_OF_THE_DEAD: return RT_GEORGE;
        case LEVEL_KINO_DER_TOTEN:
        case LEVEL_DER_RIESE:
        case LEVEL_SHI_NO_NUMA:
            return RT_DOGS;
        default: return RT_NORMAL;
    }
}

Level levelGetFromId(const char *levelId) {
    if (strcmp(levelId, "zombie_theater") == 0) return LEVEL_KINO_DER_TOTEN;
    if (strcmp(levelId, "zombie_cosmodrome") == 0) return LEVEL_ASCENSION;
    if (strcmp(levelId, "zombie_pentagon") == 0) return LEVEL_FIVE;
    if (strcmp(levelId, "zombie_coast") == 0) return LEVEL_CALL_OF_THE_DEAD;
    if (strcmp(levelId, "zombie_moon") == 0) return LEVEL_MOON;
    if (strcmp(levelId, "zombie_temple") == 0) return LEVEL_SHANGRI_LA;
    if (strcmp(levelId, "zombie_cod5_prototype") == 0) return LEVEL_NACH_DER_UNTOTEN;
    if (strcmp(levelId, "zombie_cod5_factory") == 0) return LEVEL_DER_RIESE;
    if (strcmp(levelId, "zombie_cod5_asylum") == 0) return LEVEL_VERRUCKT;
    if (strcmp(levelId, "zombie_cod5_sumpf") == 0) return LEVEL_SHI_NO_NUMA;
    if (strcmp(levelId, "frontend") == 0) return LEVEL_MAIN_MENU;
    return LEVEL_INVALID;
}

bool levelIsMonitored(Level level) {
    switch (level) {
        case LEVEL_KINO_DER_TOTEN:
        case LEVEL_ASCENSION:
        case LEVEL_FIVE:
        case LEVEL_CALL_OF_THE_DEAD:
        case LEVEL_MOON:
        case LEVEL_SHANGRI_LA:
        case LEVEL_NACH_DER_UNTOTEN:
        case LEVEL_DER_RIESE:
        case LEVEL_VERRUCKT:
        case LEVEL_SHI_NO_NUMA:
            return true;
        case LEVEL_MAIN_MENU:        
        default:
            return false;
    }
}