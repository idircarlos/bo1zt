#include "controller.h"
#include "controller/controller_internal.h"
#include "logic/cheat.h"
#include "logic/cheat/manager.h"
#include "logic/cheat/manager/handlers.h"
#include "logic/config.h"
#include "logic/game.h"
#include "logic/game/level.h"
#include "logic/game/round.h"
#include "logic/gsc.h"
#include "logic/server.h"
#include "win/process.h"
#include "api.h"
#include "logger.h"
#include "logic/state.h"
#include <stdlib.h>
#include <stdio.h>

Controller* controllerCreate() {
    Controller *controller = (Controller*)malloc(sizeof(Controller));
    if (!controller) return NULL;
    controller->config = configCreate();
    controller->process = NULL;
    controller->state = NULL;
    controller->api = NULL;
    controller->server = NULL;
    controller->gsc = NULL;
    controller->cheatManager = NULL;
    controllerAttachGame(controller);
    controller->cheatManager = cheatManagerCreate(controller);
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

bool controllerIsZombiesGameOngoing(Controller *controller) {
    (void)controller;
    return apiIsZombiesGameOngoing(controller->api);
}

bool controllerIsGameAttached(Controller *controller) {
    return controller != NULL && controller->process != NULL;
}

bool controllerLaunchGame(Controller *controller) {
    if (!controller) return false;
    char execPath[MAX_PATH];
    snprintf(execPath, MAX_PATH, "%s\\%s", controller->config->game.location, GAME_EXECUTABLE_NAME);
    return processExec(execPath);
}

bool controllerCloseGame(Controller *controller) {
    if (!controller) return false;
    return processTerminate(controller->process);
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
    if (!controller->server) controller->server = serverCreate(controller);
    if (!controller->gsc) controller->gsc = gscCreate(controller->server);
    controller->state->isGameAttached = controllerIsGameAttached(controller);
    controller->state->isTimRunning = controllerIsTimRunning(controller);
    controller->state->isZombiesGameOngoing = controllerIsZombiesGameOngoing(controller);
    controller->state->isZombiesGamePaused = apiIsZombiesGamePaused(controller->api);
    controller->state->gameResets = apiGetGameResets(controller->api);
    Game *activeGame = &controller->state->activeGame;
    if (levelIsMonitored(activeGame->levelName)) {
        activeGame->elapsed = controllerGetLevelElapsedTime(controller);
        activeGame->levelName = controllerGetLevelName(controller);
    }
    return true;
}

bool controllerDetachGame(Controller *controller) {
    if (!controllerIsGameAttached(controller)) {
        LOG_WARN("Cannot detach game since is not attached\n");
        return false;
    }
    LOG_INFO("Detaching game\n");
    
    // Clear AppliedState when game detaches
    if (controller->cheatManager) {
        cheatManagerHandleGameDetach(controller->cheatManager);
    }
    
    processClose(controller->process);
    stateGameClear(controller->state);
    controller->process = NULL;
    return true;
}

void controllerWaitUntilGameCloses(Controller *controller) {
    if (!controller) return;
    if (!controller->process) return;
    processWaitUntilExits(controller->process);
}

bool controllerIsGameWindowFocused(Controller *controller) {
    if (!controller || !controller->process) return false;
    return processIsWindowForeground(controller->process);
}

bool controllerIsGameWindowAttached(Controller *controller) {
    if (!controller) return false;
    if (!controller->process) return false;
    return processIsWindowAttached(controller->process);
}

bool controllerIsGameReady(Controller *controller) {
    if (!controller) return false;
    if (!controller->process) return false;
    return apiIsGameReady(controller->api);
}

int controllerGetLevelElapsedTime(Controller *controller) {
    if (!controller) return 0;
    if (!controller->process) return 0;
    return apiGetLevelElapsedTime(controller->api);
}

float controllerGetMovementSpeed(Controller *controller) {
    if (!controller) return 0;
    if (!controller->process) return 0;
    return apiGetMovementSpeed(controller->api);
}

Level controllerGetLevelName(Controller *controller) {
    if (!controller) return LEVEL_INVALID;
    if (!controller->process) return LEVEL_INVALID;
    return apiGetLevelName(controller->api);
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
        if (controller->cheatManager) {
            cheatManagerDestroy(controller->cheatManager);
            controller->cheatManager = NULL;
        }
        if (controller->process) {
            processClose(controller->process);
            controller->process = NULL;
        }
        free(controller);
    }
}

bool controllerIsCheatCheckboxChecked(Controller *controller, CheatName cheat) {
    if (!controller || !controller->config) return false;
    HacksConfig *hacks = &controller->config->hacks;
    switch (cheat) {
        case CHEAT_NAME_GOD_MODE:
            return hacks->godMode;
        case CHEAT_NAME_NO_CLIP:
            return hacks->noClip;
        case CHEAT_NAME_INVISIBLE:
            return hacks->invisible;
        case CHEAT_NAME_INFINITE_AMMO:
            return hacks->infiniteAmmo;
        case CHEAT_NAME_INSTANT_KILL:
            return hacks->instantKill;
        case CHEAT_NAME_NO_RECOIL:
            return hacks->noRecoil;
        case CHEAT_NAME_SMALL_CROSSHAIR:
            return hacks->smallCrosshair;
        case CHEAT_NAME_FAST_GAMEPLAY:
            return hacks->fastGameplay;
        case CHEAT_NAME_NO_SHELLSHOCK:
            return hacks->noShellshock;
        case CHEAT_NAME_INCREASE_KNIFE_RANGE:
            return hacks->increaseKnifeRange;
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return hacks->boxNeverMoves;
        case CHEAT_NAME_THIRD_PERSON:
            return hacks->thirdPerson;
        default:
            return false;
    }
}

int controllerUiGraphicsGetFpsCap(Controller *controller) {
    if (!controller || !controller->config) return 185; // default
    return controller->config->graphics.fpsCap;
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
    if (!controller) return NULL;
    return controller->state;
}

void controllerUpdateState(Controller *controller) {
    if (!controller || !controller->state) return;
    controller->state->isTimRunning = controllerIsTimRunning(controller);
    if (!controllerIsGameAttached(controller)) {
        stateGameClear(controller->state);
        return;
    };
    // General state
    controller->state->isGameAttached = controllerIsGameWindowAttached(controller);
    controller->state->isZombiesGameOngoing = controllerIsZombiesGameOngoing(controller);
    controller->state->isZombiesGamePaused = apiIsZombiesGamePaused(controller->api);
    controller->state->gameResets = apiGetGameResets(controller->api);
    
    // Active game
    Game *activeGame = &controller->state->activeGame;
    if (gameRunning(activeGame) && levelIsMonitored(activeGame->levelName)) {
        activeGame->movementSpeed = controllerGetMovementSpeed(controller);
        activeGame->currentEntities = apiGetCurrentSnapshotEntities(controller->api);
        activeGame->maxEntities = apiGetMaxSnapshotEntities(controller->api);
        int levelElapsed = controllerGetLevelElapsedTime(controller);
        gameUpdateElapsed(activeGame, levelElapsed);
        Round *round = &activeGame->currentRound;
        if (roundRunning(round)) {
            roundUpdateElapsed(round, levelElapsed);
        }
    }
}

void controllerInitTrainerConfig(Controller *controller) {
    if (!controllerIsGameAttached(controller)) return;
    Config *config = controller->config;
    CustomizerConfig *customizer = &config->customizer;
    
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_BACKGROUND, &customizer->scoreBackground);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P1, &customizer->scorePlayer1);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P2, &customizer->scorePlayer2);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P3, &customizer->scorePlayer3);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P4, &customizer->scorePlayer4);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_POINTS, &customizer->pointsTransparency);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_SCOREBOARD, &customizer->scoreboardTransparency);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_PRIMARY, &customizer->reloadWarnPrimary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_SECONDARY, &customizer->reloadWarnSecondary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_PRIMARY, &customizer->lowAmmoWarnPrimary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_SECONDARY, &customizer->lowAmmoWarnSecondary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_PRIMARY, &customizer->noAmmoWarnPrimary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_SECONDARY, &customizer->noAmmoWarnSecondary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_FREQUENCY, &customizer->warningTransitionsFrequency);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MIN, &customizer->warningTransitionsMin);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MAX, &customizer->warningTransitionsMax);

    if (controllerIsGameWindowAttached(controller) && !processIsBorderless(controller->process) && config->graphics.borderless) {
        controllerSetCheat(controller, CHEAT_NAME_MAKE_BORDERLESS, config->graphics.borderless);
    }

    // --- Other non-gui Config ---
    controllerSetCheat(controller, CHEAT_NAME_PATCH_CHAT, true);
    
    // --- HacksConfig ---
    // Use CheatManager to apply all enabled cheats from Config that meet conditions
    if (controller->cheatManager) {
        cheatManagerHandleGameAttach(controller->cheatManager);
    }
}

void controllerUpdateTrainerConfig(Controller *controller) {
    if (!controllerIsGameAttached(controller) || !controllerIsGameReady(controller)) return;
    
    // Use CheatManager for all cheats - handles conditions and applied state tracking
    if (controller->cheatManager) {
        cheatManagerHandleStateChange(controller->cheatManager);
    }
}

GameConfig controllerGetGameConfig(Controller *controller) {
    return controller->config->game;
}

GraphicsConfig controllerGetGraphicsConfig(Controller *controller) {
    return controller->config->graphics;
}

CustomizerConfig controllerGetCustomizerConfig(Controller *controller) {
    return controller->config->customizer;
}

WidgetConfig controllerGetWidgetConfig(Controller *controller, int index) {
    return controller->config->widgets[index];
}

BindsConfig controllerGetBindsConfig(Controller *controller) {
    return controller->config->binds;
}

Config *controllerGetConfig(Controller *controller) {
    return controller->config;
}

void controllerResetConfig(Controller *controller, ConfigType type) {
    if (!controller) return;
    switch (type) {
        case CONFIG_GRAPHICS:
            configResetGraphics(controller->config); return;
        case CONFIG_CUSTOMIZER:
            configResetCustomizer(controller->config); return;
        default:
            LOG_ERROR("Unknown Config Type %d\n", type);
    }
}

void controllerResetWidgetConfig(Controller *controller, int index) {
    if (!controller) return;
    configResetWidget(controller->config, index);
}

void controllerResetBindsConfig(Controller *controller) {
    if (!controller) return;
    configResetBinds(controller->config);
}

void controllerUpdateBindsConfig(Controller *controller, BindsConfig *bindsConfig) {
    if (!controller || !controller->config || !bindsConfig) return;
    controller->config->binds = *bindsConfig;
    configSave(controller->config);
}

Api *_controllerGetApi(Controller *controller) {
    if (!controller) return NULL;
    return controller->api;
}

GSC *_controllerGetGsc(Controller *controller) {
    if (!controller) return NULL;
    return controller->gsc;
}

Server *_controllerGetServer(Controller *controller) {
    if (!controller) return NULL;
    return controller->server;
}

CheatManager *controllerGetCheatManager(Controller *controller) {
    if (!controller) return NULL;
    return controller->cheatManager;
}

CheatManager *_controllerGetCheatManager(Controller *controller) {
    if (!controller) return NULL;
    return controller->cheatManager;
}
