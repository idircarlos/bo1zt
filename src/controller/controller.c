#include "controller.h"
#include "../memory/memory.h"
#include "../api/api.h"
#include "../gui/gui.h"
#include "../logger/logger.h"
#include <stdlib.h>
#include <stdio.h>

struct Controller {
    ProcessHandle *ph;
    Api *api;
};

ProcessHandle *_controllerAttachProcess(Controller *controller);

Controller* controllerCreate() {
    Controller *controller = (Controller*)malloc(sizeof(Controller));
    if (!controller) return NULL;
    controller->ph = _controllerAttachProcess(controller);
    controller->api = apiCreate(controller);
    if (!controller->ph) {
        LOG_WARN("Could not create ProcessHandle. Game is not running.\n");
    }
    return controller;
}

ProcessHandle* controllerGetProcessHandle(Controller *controller) {
    if (!controller) return NULL;
    return controller->ph;
}

bool controllerIsGameRunning(Controller *controller) {
    if (!controller) return false;
    controller->ph = _controllerAttachProcess(controller);
    if (!controller->ph) return false;
    return true;
}

void controllerWaitUntilGameCloses(Controller *controller) {
    if (!controller) return;
    if (!controller->ph) return;
    memoryWaitUntilProcessCloses(controller->ph);
    memoryCloseProcess(controller->ph);
    controller->ph = NULL;
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

TeleportCoords *controllerGetPlayerCurrentCoords(Controller *controller) {
    if (!controller || !controller->ph) return NULL;
    return apiGetPlayerCurrentCoords(controller->api);
}

WeaponName controllerGetPlayerCurrentWeapon(Controller *controller) {
    if (!controller || !controller->ph) return WEAPON_UNKNOWNWEAPON;
    return apiGetPlayerCurrentWeapon(controller->api);
}

WeaponName controllerGetPlayerWeapon(Controller *controller, int slot) {
    if (!controller || !controller->ph) return WEAPON_UNKNOWNWEAPON;
    return apiGetPlayerWeapon(controller->api, slot);
}

bool controllerSetPlayerWeapon(Controller *controller, WeaponName weapon, int slot) {
    if (!controller || !controller->ph) return false;
    return apiSetPlayerWeapon(controller->api, weapon, slot);
}

bool controllerGivePlayerAmmo(Controller *controller) {
    if (!controller || !controller->ph) return false;
    return apiGivePlayerAmmo(controller->api);
}

bool controllerSetRound(Controller *controller, int currentRound, int nextRound) {
    if (!controller || !controller->ph) return false;
    return apiSetRound(controller->api, currentRound, nextRound);
}

ProcessHandle *_controllerAttachProcess(Controller *controller) {
    if (!controller) return NULL;
    ProcessHandle *ph = memoryOpenProcess("BlackOps.exe");
    if (!ph) {
        LOG_WARN("Game is not running\n");
        return NULL;
    }
    return ph;
}
