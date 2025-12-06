#ifndef CONTROLLER_INTERNAL_H_
#define CONTROLLER_INTERNAL_H_

#include <windows.h>
#include "controller/controller.h"
#include "win/process.h"
#include "api/api.h"
#include "logic/server.h"
#include "logic/state.h"
#include "logic/config.h"

struct Controller {
    Process *process;
    Api *api;
    Server *server;
    State *state;
    Config *config;
};

Api *_controllerGetApi(Controller *controller);
Server *_controllerGetServer(Controller *controller);

#endif // CONTROLLER_INTERNAL_H_
