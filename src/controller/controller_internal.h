#ifndef CONTROLLER_INTERNAL_H_
#define CONTROLLER_INTERNAL_H_

#include <windows.h>
#include "controller.h"
#include "../process/process.h"
#include "../api/api.h"
#include "../server/server.h"
#include "../state/state.h"
#include "../config/config.h"

struct Controller {
    Process *process;
    Api *api;
    Server *server;
    State *state;
    Config *config;
};

Api *_controllerGetApi(Controller *controller);

#endif // CONTROLLER_INTERNAL_H_