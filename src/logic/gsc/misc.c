#include <stdlib.h>
#include "logic/gsc/misc.h"
#include "logic/game/perk.h"

const char* gscGetPerkName(Perk perk) {
    switch (perk) {
        case PERK_QUICK_REVIVE: return GSC_PERK_QUICK_REVIVE;
        case PERK_JUGGERNAUT: return GSC_PERK_JUGGERNAUT;
        case PERK_SPEED_COLA: return GSC_PERK_SPEED_COLA;
        case PERK_DOUBLE_TAP: return GSC_PERK_DOUBLE_TAP;
        case PERK_MULE_KICK: return GSC_PERK_MULE_KICK;
        default: return NULL;
    }
}
