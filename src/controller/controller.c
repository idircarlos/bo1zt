#include "controller.h"
#include "../process/process.h"
#include "../api/api.h"
#include "../gui/gui.h"
#include "../logger/logger.h"
#include "../state/state.h"
#include "../gui/hacks/hacks.h"
#include "../gui/graphics/graphics.h"
#include "../gui/game/game.h"
#include <stdlib.h>
#include <stdio.h>

#define GAME_EXECUTABLE_NAME "BlackOps.exe"
#define TIM_EXECUTABLE_NAME "Black Ops TIM.exe"
#define GAME_WINDOW_NAME_PREFIX "Call of Duty"

struct Controller {
    Process *process;
    Api *api;
    State *state;
};

Controller* controllerCreate() {
    Controller *controller = (Controller*)malloc(sizeof(Controller));
    if (!controller) return NULL;
    controller->process = NULL;
    controller->state = NULL;
    controller->api = NULL;
    controllerAttachGame(controller);
    return controller;
}

Process* controllerGetProcess(Controller *controller) {
    if (!controller) return NULL;
    return controller->process;
}

bool controllerIsGameRunning(Controller *controller) {
    (void)controller;
    return processIsRunning(GAME_EXECUTABLE_NAME);
}

bool controllerIsTimRunning(Controller *controller) {
    (void)controller;
    return processIsRunning(TIM_EXECUTABLE_NAME);
}

bool controllerIsGameAttached(Controller *controller) {
    return controller != NULL && controller->process != NULL;
}

bool controllerAttachGame(Controller *controller) {
    if (!controller) return false;
    if (controller->process) {
        LOG_WARN("Game is already attached. This souldn't happen. Omitting the operation\n");
        return true;
    }
    if (!controller->state) controller->state = stateCreate();
    if (!controller->process) controller->process = processOpen(GAME_EXECUTABLE_NAME);
    if (!controller->process) return false;
    if (!controller->api) controller->api = apiCreate(controller);
    bool isGameAttached = controllerIsGameAttached(controller);
    bool isTimRunning = controllerIsTimRunning(controller);
    bool isZombiesActive = apiIsZombiesGameRunning(controller->api);
    int gameResets = apiGetGameResets(controller->api);
    stateSetGameAttached(controller->state, isGameAttached);
    stateSetTimRunning(controller->state, isTimRunning);
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
    processClose(controller->process);
    stateGameClear(controller->state);
    controller->process = NULL;
    return true;
}

void controllerWaitUntilGameCloses(Controller *controller) {
    if (!controller) return;
    if (!controller->process) return;
    processWaitUntilCloses(controller->process);
}

bool controllerIsGameWindowAttached(Controller *controller) {
    if (!controller) return false;
    if (!controller->process) return false;
    return processIsWindowAttached(controller->process);
}

bool controllerTryAttachGameWindow(Controller *controller) {
    if (!controller) return false;
    if (!controller->process) return false;
    return processTryAttachWindow(controller->process, GAME_WINDOW_NAME_PREFIX);
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
        if (controller->process) {
            processClose(controller->process);
            controller->process = NULL;
        }
        free(controller);
    }
}

bool controllerIsCheatCheckboxChecked(Controller *controller, CheatName cheat) {
    if (!controller || !controller->process) return false;
    return uiHacksIsChecked(cheat);
}

int controllerUiGraphicsGetFpsCap(Controller *controller) {
    if (!controller || !controller->process) return false;
    return uiGraphicsGetFpsCap();
}

TeleportCoords *controllerGetPlayerCurrentCoords(Controller *controller) {
    if (!controller || !controller->process) return NULL;
    return apiGetPlayerCurrentCoords(controller->api);
}

WeaponName controllerGetPlayerCurrentWeapon(Controller *controller) {
    if (!controller || !controller->process) return WEAPON_UNKNOWNWEAPON;
    return apiGetPlayerCurrentWeapon(controller->api);
}

WeaponName controllerGetPlayerWeapon(Controller *controller, int slot) {
    if (!controller || !controller->process) return WEAPON_UNKNOWNWEAPON;
    return apiGetPlayerWeapon(controller->api, slot);
}

bool controllerSetPlayerWeapon(Controller *controller, WeaponName weapon, int slot) {
    if (!controller || !controller->process) return false;
    return apiSetPlayerWeapon(controller->api, weapon, slot);
}

bool controllerGivePlayerAmmo(Controller *controller) {
    if (!controller || !controller->process) return false;
    return apiGivePlayerAmmo(controller->api);
}

bool controllerSetRound(Controller *controller, int currentRound, int nextRound) {
    if (!controller || !controller->process) return false;
    return apiSetRound(controller->api, currentRound, nextRound);
}

State *controllerGetState(Controller *controller) {
    return controller->state;
}

void controllerUpdateState(Controller *controller) {
    stateSetTimRunning(controller->state, controllerIsTimRunning(controller));
    if (!controllerIsGameAttached(controller)) {
        stateGameClear(controller->state);
        return;
    };
    stateSetGameAttached(controller->state, controllerIsGameAttached(controller));
    stateSetZombiesGameActive(controller->state, apiIsZombiesGameRunning(controller->api));
    stateSetGameResets(controller->state, apiGetGameResets(controller->api));
}

void controllerUpdateTrainerConfig(Controller *controller) {
    if (!controllerIsGameAttached(controller)) return;
    // --- Graphics Config ---
    // Only update these cheats if TIM is NOT running. Fight is over... He is stronger...
    if (!controllerIsTimRunning(controller)) {
        int fov = uiGraphicsGetFov();
        int fovScale = uiGraphicsGetFovScale();
        controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_FOV, &fov);
        controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_FOV_SCALE, &fovScale);
        bool unlimitFps = uiGraphicsIsChecked(CHEAT_NAME_UNLIMIT_FPS);
        if (!unlimitFps) {
            int fpsCap = uiGraphicsGetFpsCap();
            controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_FPS_CAP, &fpsCap);
        }
    }
    
    bool disableHud = uiGraphicsIsChecked(CHEAT_NAME_DISABLE_HUD);
    bool disableFog = uiGraphicsIsChecked(CHEAT_NAME_DISABLE_FOG);
    bool fullbright = uiGraphicsIsChecked(CHEAT_NAME_FULLBRIGHT);
    bool colorized = uiGraphicsIsChecked(CHEAT_NAME_COLORIZED);
    controllerSetCheat(controller, CHEAT_NAME_DISABLE_HUD, disableHud);
    controllerSetCheat(controller, CHEAT_NAME_DISABLE_FOG, disableFog);
    controllerSetCheat(controller, CHEAT_NAME_COLORIZED, colorized);
    controllerSetCheat(controller, CHEAT_NAME_FULLBRIGHT, fullbright);
    // Only make borderless once
    bool makeBorderless = uiGraphicsIsChecked(CHEAT_NAME_MAKE_BORDERLESS);
    if (controllerIsGameWindowAttached(controller) && !processIsBorderless(controller->process) && makeBorderless) {
        controllerSetCheat(controller, CHEAT_NAME_MAKE_BORDERLESS, makeBorderless);
    }

    // --- Game Config ---
    bool fixMovementSpeed = uiGameIsChecked(CHEAT_NAME_FIX_MOVEMENT_SPEED);
    bool showFps = uiGameIsChecked(CHEAT_NAME_SHOW_FPS);
    controllerSetCheat(controller, CHEAT_NAME_FIX_MOVEMENT_SPEED, fixMovementSpeed);
    controllerSetCheat(controller, CHEAT_NAME_SHOW_FPS, showFps);
}
