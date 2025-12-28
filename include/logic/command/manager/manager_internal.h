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
    Server *server;
    Map *commandsMap;
    History *history;
    InputState prevState;
} CommandManager;

#endif // COMMAND_MANAGER_INTERNAL_H_
