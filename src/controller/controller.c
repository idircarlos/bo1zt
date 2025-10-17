#include "controller.h"
#include "../memory/memory.h"
#include "../api/api.h"
#include "../gui/gui.h"
#include <stdlib.h>
#include <stdio.h>

struct Controller {
    ProcessHandle *ph;
    Api *api;
};

Controller* controllerCreate() {
    Controller *controller = (Controller*)malloc(sizeof(Controller));
    if (!controller) return NULL;
    controller->ph = memoryOpenProcess("BlackOps.exe");
    controller->api = apiCreate(controller);
    if (!controller->ph) {
        free(controller);
        return NULL;
    }
    return controller;
}

ProcessHandle* controllerGetProcessHandle(Controller *controller) {
    if (!controller) return NULL;
    return controller->ph;
}

bool controllerGetCheat(Controller *controller, CheatName cheat) {
    if (!controller || !controller->api) return false;
    return apiIsCheatEnabled(controller->api, cheat);
}

bool controllerSetCheat(Controller *controller, CheatName cheat, bool enabled) {
    if (!controller || !controller->api) return false;
    return apiSetCheatEnabled(controller->api, cheat, enabled);
}

bool controllerSetSimpleCheat(Controller *controller, SimpleCheatName cheat, void *value) {
    if (!controller || !controller->api) return false;
    return apiSetSimpleCheat(controller->api, cheat, value);
}

void controllerDestroy(Controller *controller) {
    if (controller) {
        if (controller->ph) {
            memoryCloseProcess(controller->ph);
            controller->ph = NULL;
        }
        free(controller);
    }
}

bool controllerIsCheckboxChecked(Controller *controller, CheatName cheat) {
    if (!controller || !controller->ph) return false;
    return guiIsCheatChecked(controller, cheat);
}
