#ifndef LEVEL_H_
#define LEVEL_H_

#include "logic/game/round.h"
#include <stdbool.h>

typedef enum {
    LEVEL_INVALID = -1,
    LEVEL_MAIN_MENU,
    LEVEL_KINO_DER_TOTEN,
    LEVEL_ASCENSION,
    LEVEL_FIVE,
    LEVEL_CALL_OF_THE_DEAD,
    LEVEL_MOON,
    LEVEL_SHANGRI_LA,
    LEVEL_NACH_DER_UNTOTEN,
    LEVEL_DER_RIESE,
    LEVEL_VERRUCKT,
    LEVEL_SHI_NO_NUMA,
} Level;

RoundType levelGetSpecialRound(Level level);
Level levelGetFromId(const char *levelId);
bool levelIsMonitored(Level level);

#endif // LEVEL_H_