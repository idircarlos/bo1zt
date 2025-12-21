#ifndef CHEAT_MANAGER_ACTIONS_H_
#define CHEAT_MANAGER_ACTIONS_H_

#include <stdbool.h>
#include "logic/cheat.h"
#include "logic/cheat/manager.h"

CheatResult cheatManagerSetToggle(CheatManager *manager, CheatName cheat, bool enabled);
CheatResult cheatManagerToggle(CheatManager *manager, CheatName cheat);
bool cheatManagerGetToggle(CheatManager *manager, CheatName cheat);
CheatResult cheatManagerSetValue(CheatManager *manager, SimpleCheatName cheat, void *value);
bool cheatManagerIsApplied(CheatManager *manager, CheatName cheat);

#endif // CHEAT_MANAGER_ACTIONS_H_
