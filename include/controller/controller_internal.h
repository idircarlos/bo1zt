#ifndef CONTROLLER_INTERNAL_H_
#define CONTROLLER_INTERNAL_H_

#include <windows.h>
#include "controller.h"
#include "win/process.h"
#include "api.h"
#include "logic/gsc.h"
#include "logic/server.h"
#include "logic/state.h"
#include "logic/config.h"
#include "logic/cheat/manager.h"

struct Controller {
    Process *process;
    Api *api;
    Server *server;
    GSC *gsc;
    State *state;
    Config *config;
    CheatManager *cheatManager;
};

Api *_controllerGetApi(Controller *controller);
GSC *_controllerGetGsc(Controller *controller);
Server *_controllerGetServer(Controller *controller);
CheatManager *_controllerGetCheatManager(Controller *controller);

#endif // CONTROLLER_INTERNAL_H_
