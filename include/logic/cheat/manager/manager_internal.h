#ifndef CHEAT_MANAGER_INTERNAL_H_
#define CHEAT_MANAGER_INTERNAL_H_

#include "logic/cheat/manager/state.h"

typedef struct Controller Controller;

typedef struct CheatManager {
    Controller *controller;
    Config *config;
    AppliedState applied;
} CheatManager;

#endif // CHEAT_MANAGER_INTERNAL_H_
