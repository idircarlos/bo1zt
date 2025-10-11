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
    switch (cheat) {
        case CHEAT_NAME_GOD_MODE:
            return apiGetGodMode(controller->api);
        case CHEAT_NAME_INVISIBLE:
            return apiGetInvisible(controller->api);
        case CHEAT_NAME_NO_CLIP:
            return apiGetNoClip(controller->api);
        case CHEAT_NAME_NO_RECOIL:
            return apiGetNoRecoil(controller->api);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return apiGetBoxNeverMoves(controller->api);
        case CHEAT_NAME_THIRD_PERSON:
            return apiGetThirdPerson(controller->api);
        case CHEAT_NAME_INFINITE_AMMO:
            return apiGetInfiniteAmmo(controller->api);
        case CHEAT_NAME_INSTANT_KILL:
            return apiGetInstantKill(controller->api);
        
        default:
            fprintf(stderr, "Unknown cheat %d\n", cheat);
            return false;
    }
}

bool controllerSetCheat(Controller *controller, CheatName cheat, bool enabled) {
    if (!controller || !controller->api) return false;
    switch (cheat) {
        case CHEAT_NAME_GOD_MODE:
            return apiSetGodMode(controller->api, enabled);
        case CHEAT_NAME_INVISIBLE:
            return apiSetInvisible(controller->api, enabled);
        case CHEAT_NAME_NO_CLIP:
            return apiSetNoClip(controller->api, enabled);
        case CHEAT_NAME_NO_RECOIL:
            return apiSetNoRecoil(controller->api, enabled);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return apiSetBoxNeverMoves(controller->api, enabled);
        case CHEAT_NAME_THIRD_PERSON:
            return apiSetThirdPerson(controller->api, enabled);
        case CHEAT_NAME_INFINITE_AMMO:
            return apiSetInfiniteAmmo(controller->api, enabled);
        case CHEAT_NAME_INSTANT_KILL:
            return apiSetInstantKill(controller->api, enabled);
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

bool controllerIsCheckboxChecked(Controller *controller, CheatName cheat) {
    if (!controller || !controller->ph) return false;
    return guiIsCheatChecked(controller, cheat);
}
