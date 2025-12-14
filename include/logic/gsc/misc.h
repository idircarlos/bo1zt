#ifndef GSC_MISC_H
#define GSC_MISC_H

#include "logic/game/perk.h"

#define GSC_PERK_QUICK_REVIVE   "specialty_quickrevive"
#define GSC_PERK_JUGGERNAUT     "specialty_armorvest"
#define GSC_PERK_SPEED_COLA     "specialty_fastreload"
#define GSC_PERK_DOUBLE_TAP     "specialty_rof"
#define GSC_PERK_STAMINA_PLUS   "specialty_longersprint"
#define GSC_PERK_MULE_KICK      "specialty_additionalprimaryweapon"

const char* gscGetPerkName(Perk perk);

#endif // GSC_MISC_H