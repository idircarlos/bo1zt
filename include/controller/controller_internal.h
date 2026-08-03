#ifndef CONTROLLER_INTERNAL_H_
#define CONTROLLER_INTERNAL_H_

#include <windows.h>
#include "controller.h"
#include "win/process.h"
#include "engine.h"
#include "logic/gsc.h"
#include "logic/server.h"
#include "logic/state.h"
#include "logic/config.h"
#include "logic/cheat/manager.h"
#include "logic/command/manager.h"

typedef struct WidgetManager WidgetManager;
typedef struct BindManager BindManager;
typedef struct CamoManager CamoManager;
typedef struct TwitchManager TwitchManager;

struct Controller {
    Process *process;
    Engine *engine;
    Server *server;
    GSC *gsc;
    State *state;
    Config *config;
    CheatManager *cheatManager;
    CommandManager *commandManager;
    WidgetManager *widgetManager;
    BindManager *bindManager;
    CamoManager *camoManager;
    TwitchManager *twitchManager;
};

Engine *_controllerGetEngine(Controller *controller);
GSC *_controllerGetGsc(Controller *controller);
Server *_controllerGetServer(Controller *controller);
CheatManager *_controllerGetCheatManager(Controller *controller);
CommandManager *_controllerGetCommandManager(Controller *controller);

#endif // CONTROLLER_INTERNAL_H_
