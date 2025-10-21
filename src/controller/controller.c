#include "controller.h"
#include "../memory/memory.h"
#include "../api/api.h"
#include "../gui/gui.h"
#include "../logger/logger.h"
#include "../state/state.h"
#include "../gui/cheats/cheats.h"
#include <stdlib.h>
#include <stdio.h>

#define GAME_EXECUTABLE_NAME "BlackOps.exe"

struct Controller {
    ProcessHandle *ph;
    Api *api;
    State *state;
};

Controller* controllerCreate() {
    Controller *controller = (Controller*)malloc(sizeof(Controller));
    if (!controller) return NULL;
    controller->ph = NULL;
    controller->state = NULL;
    controller->api = NULL;
    controllerAttachGame(controller);
    return controller;
}

ProcessHandle* controllerGetProcessHandle(Controller *controller) {
    if (!controller) return NULL;
    return controller->ph;
}

bool controllerIsGameRunning(Controller *controller) {
    (void)controller;
    return memoryIsProcessRunning(GAME_EXECUTABLE_NAME);
}

bool controllerIsGameAttached(Controller *controller) {
    return controller != NULL && controller->ph != NULL;
}

bool controllerAttachGame(Controller *controller) {
    if (!controller) return false;
    if (controller->ph) {
        LOG_WARN("Game is already attached. This souldn't happen. Omitting the operation\n");
        return true;
    }
    if (!controller->ph) controller->ph = memoryOpenProcess(GAME_EXECUTABLE_NAME);
    if (!controller->ph) return false;
    if (!controller->api) controller->api = apiCreate(controller);
    if (!controller->state) controller->state = stateCreate(0, 0);
    bool isZombiesActive = controllerIsZombiesGameActive(controller);
    int gameResets = controllerGetGameResets(controller);
    stateSetZombiesGameActive(controller->state, isZombiesActive);
    stateSetGameResets(controller->state, gameResets);
    return true;
}

bool controllerDetachGame(Controller *controller) {
    if (!controllerIsGameAttached(controller)) {
        LOG_WARN("Cannot detach game since is not attached\n");
        return false;
    }
    LOG_INFO("Detaching game\n");
    memoryCloseProcess(controller->ph);
    stateReset(controller->state);
    controller->ph = NULL;
    return true;
}

void controllerWaitUntilGameCloses(Controller *controller) {
    if (!controller) return;
    if (!controller->ph) return;
    memoryWaitUntilProcessCloses(controller->ph);
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

bool controllerIsCheatCheckboxChecked(Controller *controller, CheatName cheat) {
    if (!controller || !controller->ph) return false;
    return uiCheatsIsCheatChecked(cheat);
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

bool controllerIsZombiesGameActive(Controller *controller) {
    if (!controller || !controller->ph) return false;
    return apiIsZombiesGameRunning(controller->api);
}

int controllerGetGameResets(Controller *controller) {
    if (!controller || !controller->ph) return false;
    return apiGetGameResets(controller->api);
}

