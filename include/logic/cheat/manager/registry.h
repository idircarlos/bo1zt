#ifndef CHEAT_MANAGER_REGISTRY_H_
#define CHEAT_MANAGER_REGISTRY_H_

#include <stdbool.h>
#include "logic/cheat/manager.h"
#include "logic/cheat.h"

typedef struct {
    CheatName name;
    CheatCondition condition;
} CheatDefinition;

typedef struct {
    SimpleCheatName name;
    CheatCondition condition;
} SimpleCheatDefinition;

extern const CheatDefinition CHEAT_REGISTRY[];
extern const int NUM_CHEAT_REGISTRY;
extern const SimpleCheatDefinition SIMPLE_CHEAT_REGISTRY[];
extern const int NUM_SIMPLE_CHEAT_REGISTRY;

const CheatDefinition* findCheatDefinition(CheatName cheat);
const SimpleCheatDefinition* findSimpleCheatDefinition(SimpleCheatName cheat);
bool isManagedToggleCheat(CheatName cheat);

#endif // CHEAT_MANAGER_REGISTRY_H_
