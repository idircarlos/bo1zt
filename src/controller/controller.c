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

Controller* controllerCreate(const char *executableName) {
    Controller *controller = (Controller*)malloc(sizeof(Controller));
    if (!controller) return NULL;
    controller->ph = memoryOpenProcess(executableName);
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

bool controllerSetCheat(Controller *controller, Cheat cheat, bool enabled) {
    if (!controller || !controller->api) return false;
    switch (cheat) {
        case CHEAT_GOD_MODE:
            return apiSetGodMode(controller->api, enabled);
        case CHEAT_NO_CLIP:
            return apiSetNoClip(controller->api, enabled);
        case CHEAT_INVISIBLE:
            return apiSetInvisible(controller->api, enabled);
        default:
            fprintf(stderr, "Unknown cheat %d\n", cheat);
            return false;
    }
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

bool controllerIsCheckboxChecked(Controller *controller, Cheat cheat) {
    if (!controller || !controller->ph) return false;
    return guiIsCheatChecked(controller, cheat);
}
