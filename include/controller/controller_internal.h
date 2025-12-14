#ifndef CONTROLLER_INTERNAL_H_
#define CONTROLLER_INTERNAL_H_

#include <windows.h>
#include "controller/controller.h"
#include "win/process.h"
#include "api/api.h"
#include "api/gsc.h"
#include "logic/gsc.h"
#include "logic/server.h"
#include "logic/state.h"
#include "logic/config.h"

struct Controller {
    Process *process;
    Api *api;
    ApiGsc *apiGsc;
    Server *server;
    GSC *gsc;
    State *state;
    Config *config;
};

Api *_controllerGetApi(Controller *controller);
ApiGsc *_controllerGetApiGsc(Controller *controller);
GSC *_controllerGetGsc(Controller *controller);
Server *_controllerGetServer(Controller *controller);

#endif // CONTROLLER_INTERNAL_H_
