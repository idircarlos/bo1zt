#ifndef COMMAND_MANAGER_INTERNAL_H_
#define COMMAND_MANAGER_INTERNAL_H_

#include "logic/command/manager/history.h"
#include <stdbool.h>

typedef struct Controller Controller;
typedef struct Server Server;
typedef struct Map Map;

typedef struct InputState {
    bool chatOpen;
    bool upKey;
    bool downKey;
} InputState;

typedef struct CommandManager {
    Controller *controller;
    Map *commandsMap;
    History *history;
    InputState prevState;
    bool submodulesInitialized;
} CommandManager;

#endif // COMMAND_MANAGER_INTERNAL_H_
