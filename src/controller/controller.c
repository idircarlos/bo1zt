#include "controller/controller.h"
#include "controller/controller_internal.h"
#include "logic/config.h"
#include "logic/game.h"
#include "logic/game/level.h"
#include "logic/game/round.h"
#include "logic/gsc.h"
#include "win/process.h"
#include "api/api.h"
#include "api/gsc.h"
#include "logger/logger.h"
#include "logic/state.h"
#include "gui/hacks.h"
#include "gui/graphics.h"
#include "gui/customizer.h"
#include "gui/game.h"
#include "gui/widgets.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Controller* controllerCreate() {
    Controller *controller = (Controller*)malloc(sizeof(Controller));
    if (!controller) return NULL;
    controller->config = configCreate();
    controller->process = NULL;
    controller->state = NULL;
    controller->api = NULL;
    controller->apiGsc = NULL;
    controller->server = NULL;
    controller->gsc = NULL;
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
    if (!controller->apiGsc) controller->apiGsc = apiGscCreate(controller->gsc);
    controller->state->isGameAttached = controllerIsGameAttached(controller);
    controller->state->isTimRunning = controllerIsTimRunning(controller);
    controller->state->isZombiesGameOngoing = apiIsZombiesGameOngoing(controller->api);
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
    controller->state->isTimRunning = controllerIsTimRunning(controller);
    if (!controllerIsGameAttached(controller)) {
        stateGameClear(controller->state);
        return;
    };
    // General state
    controller->state->isGameAttached = controllerIsGameWindowAttached(controller);
    controller->state->isZombiesGameOngoing = apiIsZombiesGameOngoing(controller->api);
    controller->state->isZombiesGamePaused = apiIsZombiesGamePaused(controller->api);
    controller->state->gameResets = apiGetGameResets(controller->api);
    
    // Active game
    Game *activeGame = &controller->state->activeGame;
    if (gameRunning(activeGame) && levelIsMonitored(activeGame->levelName)) {
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
    Color scoreBg = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_BACKGROUND);
    Color scoreP1 = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P1);
    Color scoreP2 = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P2);
    Color scoreP3 = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P3);
    Color scoreP4 = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P4);
    Color reloadWarnPrimary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_PRIMARY);
    Color reloadWarnSecondary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_SECONDARY);
    Color lowAmmoWarnPrimary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_PRIMARY);
    Color lowAmmoWarnSecondary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_SECONDARY);
    Color noAmmoWarnPrimary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_PRIMARY);
    Color noAmmoWarnSecondary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_SECONDARY);
    int transparencyPoints = uiCustomizerGetCheatInt(SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_POINTS);
    int transparencyScoreboard = uiCustomizerGetCheatInt(SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_SCOREBOARD);
    int warnFrequency = uiCustomizerGetCheatInt(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_FREQUENCY);
    int warnMin = uiCustomizerGetCheatInt(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MIN);
    int warnMax = uiCustomizerGetCheatInt(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MAX);
    
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_BACKGROUND, &scoreBg);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P1, &scoreP1);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P2, &scoreP2);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P3, &scoreP3);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P4, &scoreP4);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_POINTS, &transparencyPoints);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_SCOREBOARD, &transparencyScoreboard);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_PRIMARY, &reloadWarnPrimary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_SECONDARY, &reloadWarnSecondary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_PRIMARY, &lowAmmoWarnPrimary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_SECONDARY, &lowAmmoWarnSecondary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_PRIMARY, &noAmmoWarnPrimary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_SECONDARY, &noAmmoWarnSecondary);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_FREQUENCY, &warnFrequency);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MIN, &warnMin);
    controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MAX, &warnMax);

    bool makeBorderless = uiGraphicsIsChecked(CHEAT_NAME_MAKE_BORDERLESS);
    if (controllerIsGameWindowAttached(controller) && !processIsBorderless(controller->process) && makeBorderless) {
        controllerSetCheat(controller, CHEAT_NAME_MAKE_BORDERLESS, makeBorderless);
    }

    // --- Other non-gui Config ---
    controllerSetCheat(controller, CHEAT_NAME_PATCH_CHAT, true);
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

    // --- Game Config ---
    bool fixMovementSpeed = uiGameIsChecked(CHEAT_NAME_FIX_MOVEMENT_SPEED);
    bool showFps = uiGameIsChecked(CHEAT_NAME_SHOW_FPS);
    controllerSetCheat(controller, CHEAT_NAME_FIX_MOVEMENT_SPEED, fixMovementSpeed);
    controllerSetCheat(controller, CHEAT_NAME_SHOW_FPS, showFps);
    if (!controllerIsTimRunning(controller)) {
        // Only modify when there is an active game. This address gets mad when it gets modified outside a game.s
        if (controller->state->isZombiesGameOngoing) {
            char *hostname = uiGameGetHostname();
            controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME, hostname);            
            uiFreeText(hostname);
        }
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

void controllerUpdateConfig(Controller *controller, ConfigType type) {
    if (!controller) return;
    char *hostname;
    char *location;
    switch (type) {
        case CONFIG_GAME:
            controller->config->game.fixMovementSpeed = uiGameIsChecked(CHEAT_NAME_FIX_MOVEMENT_SPEED);
            controller->config->game.showFps = uiGameIsChecked(CHEAT_NAME_SHOW_FPS);
            hostname = uiGameGetHostname();
            location = uiGameGetLocation();
            strcpy(controller->config->game.hostname, hostname);
            strcpy(controller->config->game.location, location);
            uiFreeText(hostname);
            uiFreeText(location);
            break;
        case CONFIG_GRAPHICS:
            controller->config->graphics.fov = uiGraphicsGetFov();
            controller->config->graphics.fovScale = uiGraphicsGetFovScale();
            controller->config->graphics.fpsCap = uiGraphicsGetFpsCap();
            controller->config->graphics.borderless = uiGraphicsIsChecked(CHEAT_NAME_MAKE_BORDERLESS);
            controller->config->graphics.unlimitFps = uiGraphicsIsChecked(CHEAT_NAME_UNLIMIT_FPS);
            controller->config->graphics.disableHud = uiGraphicsIsChecked(CHEAT_NAME_DISABLE_HUD);
            controller->config->graphics.disableFog = uiGraphicsIsChecked(CHEAT_NAME_DISABLE_FOG);
            controller->config->graphics.fullbright = uiGraphicsIsChecked(CHEAT_NAME_FULLBRIGHT);
            controller->config->graphics.colorized = uiGraphicsIsChecked(CHEAT_NAME_COLORIZED);
            break;
        case CONFIG_CUSTOMIZER:
            controller->config->customizer.scoreBackground =  uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_BACKGROUND);
            controller->config->customizer.scorePlayer1 = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P1);
            controller->config->customizer.scorePlayer2 = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P2);
            controller->config->customizer.scorePlayer3 = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P3);
            controller->config->customizer.scorePlayer4 = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P4);
            controller->config->customizer.reloadWarnPrimary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_PRIMARY);
            controller->config->customizer.reloadWarnSecondary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_SECONDARY);
            controller->config->customizer.lowAmmoWarnPrimary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_PRIMARY);
            controller->config->customizer.lowAmmoWarnSecondary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_SECONDARY);
            controller->config->customizer.noAmmoWarnPrimary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_PRIMARY);
            controller->config->customizer.noAmmoWarnSecondary = uiCustomizerGetCheatColor(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_SECONDARY);
            controller->config->customizer.scoreboardTransparency = uiCustomizerGetCheatInt(SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_SCOREBOARD);
            controller->config->customizer.pointsTransparency = uiCustomizerGetCheatInt(SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_POINTS);
            controller->config->customizer.warningTransitionsFrequency = uiCustomizerGetCheatInt(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_FREQUENCY);
            controller->config->customizer.warningTransitionsMin = uiCustomizerGetCheatInt(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MIN);
            controller->config->customizer.warningTransitionsMax = uiCustomizerGetCheatInt(SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MAX);
            break;
        case CONFIG_WIDGETS:
            for (int i = 0; i < N_CONFIG_WIDGETS; i++) {
                controller->config->widgets[i].enabled = uiWidgetsIsEnabled(i);
                const char *font = uiWidgetsGetFont(i);
                strcpy(controller->config->widgets[i].font, font);
                controller->config->widgets[i].textColor = uiWidgetsGetTextColor(i);
                controller->config->widgets[i].hideOnDefault = uiWidgetsIsHideOnDefaultChecked(i); 
                controller->config->widgets[i].rect = uiWidgetsGetRect(i);
                controller->config->widgets[i].fontSize = uiWidgetsGetFontSize(i);  
            }
            break;
        default:
            LOG_ERROR("Unknown Config Type %d\n", type);
            return;
    }
    configSave(controller->config);
}

void controllerWidgetUpdateConfig(Controller *controller, int index) {
    if (!controller) return;
    controller->config->widgets[index].enabled = uiWidgetsIsEnabled(index);
    const char *font = uiWidgetsGetFont(index);
    strcpy(controller->config->widgets[index].font, font);
    controller->config->widgets[index].textColor = uiWidgetsGetTextColor(index);
    controller->config->widgets[index].hideOnDefault = uiWidgetsIsHideOnDefaultChecked(index);
    configSave(controller->config);
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

void controllerWidgetResetConfig(Controller *controller, int index) {
    if (!controller) return;
    configResetWidget(controller->config, index);
}

Api *_controllerGetApi(Controller *controller) {
    if (!controller) return NULL;
    return controller->api;
}

ApiGsc *_controllerGetApiGsc(Controller *controller) {
    if (!controller) return NULL;
    return controller->apiGsc;
}

GSC *_controllerGetGsc(Controller *controller) {
    if (!controller) return NULL;
    return controller->gsc;
}

Server *_controllerGetServer(Controller *controller) {
    if (!controller) return NULL;
    return controller->server;
}
