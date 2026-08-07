#include "controller.h"
#include "controller/controller_internal.h"
#include "logic/cheat.h"
#include "logic/cheat/manager.h"
#include "logic/cheat/manager/handlers.h"
#include "logic/command/manager.h"
#include "logic/config.h"
#include "logic/game.h"
#include "logic/game/level.h"
#include "logic/gsc.h"
#include "logic/server.h"
#include "win/process.h"
#include "engine.h"
#include "logger.h"
#include "logic/state.h"
#include "logic/widget/manager.h"
#include "logic/bind/manager.h"
#include "logic/camo/manager.h"
#include "logic/twitch/manager.h"
#include <stdlib.h>
#include <stdio.h>

Controller* controllerCreate() {
    Controller *controller = (Controller*)malloc(sizeof(Controller));
    if (!controller) return NULL;
    controller->config = configCreate();
    controller->process = NULL;
    controller->state = NULL;
    controller->engine = NULL;
    controller->server = NULL;
    controller->gsc = NULL;
    controller->cheatManager = NULL;
    controller->commandManager = NULL;
    controller->widgetManager = NULL;
    controller->bindManager = NULL;
    controller->camoManager = NULL;
    controller->twitchManager = NULL;
    controllerAttachGame(controller);
    return controller;
}

void controllerInitManagers(Controller *controller) {
    if (!controller) return;
    if (!controller->widgetManager) controller->widgetManager = widgetManagerCreate(controller);
    if (!controller->bindManager) controller->bindManager = bindManagerCreate(controller);
    if (!controller->camoManager) controller->camoManager = camoManagerCreate();
    if (!controller->twitchManager) controller->twitchManager = twitchManagerCreate(controller);
}

void controllerUpdateManagers(Controller *controller) {
    if (!controller) return;
    widgetManagerUpdate(controller->widgetManager);
    bindManagerUpdate(controller->bindManager);
}

WidgetManager *controllerGetWidgetManager(Controller *controller) {
    if (!controller) return NULL;
    return controller->widgetManager;
}

BindManager *controllerGetBindManager(Controller *controller) {
    if (!controller) return NULL;
    return controller->bindManager;
}

CamoManager *controllerGetCamoManager(Controller *controller) {
    if (!controller) return NULL;
    return controller->camoManager;
}

TwitchManager *controllerGetTwitchManager(Controller *controller) {
    if (!controller) return NULL;
    return controller->twitchManager;
}

Process* controllerGetProcess(Controller *controller) {
    if (!controller) return NULL;
    return controller->process;
}

bool controllerIsGameRunning(Controller *controller) {
    (void)controller;
    return processIsRunning(GAME_EXECUTABLE_NAME);
}

bool controllerIsZombiesGameOngoing(Controller *controller) {
    (void)controller;
    return engineIsZombiesGameOngoing(controller->engine);
}

bool controllerIsZombiesGamePaused(Controller *controller) {
    (void)controller;
    return engineIsZombiesGamePaused(controller->engine);
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
        LOG_WARN("Game is already attached. This souldn't happen. Omitting the operation");
        return true;
    }
    if (!controller->state) controller->state = stateCreate();
    if (!controller->cheatManager) controller->cheatManager = cheatManagerCreate(controller);
    if (!controller->commandManager) controller->commandManager = commandManagerCreate(controller);
    if (!controller->process) controller->process = processOpen(GAME_EXECUTABLE_NAME);
    if (!controller->process) return false;
    if (!controller->engine) controller->engine = engineCreate(controller);
    if (!controller->server) controller->server = serverCreate(controller);
    if (!controller->gsc) controller->gsc = gscCreate(controller->server);
    commandManagerInitSubmodules(controller->commandManager);
    if (!controllerIsGameRunning(controller)) return true;
    controller->state->isGameAttached = controllerIsGameAttached(controller);
    controller->state->isZombiesGameOngoing = controllerIsZombiesGameOngoing(controller);
    controller->state->isZombiesGamePaused = engineIsZombiesGamePaused(controller->engine);
    controller->state->gameResets = engineGetGameResets(controller->engine);

    if (controllerIsGameRunning(controller) && controllerIsZombiesGameOngoing(controller)) {
        serverChatMessage(controller->server, SERVER_BO1ZT_MSG_PREFIX "Zombies game is already in progress...");
        serverChatMessage(controller->server, SERVER_BO1ZT_MSG_PREFIX "Some functionalities may not work.");
        serverChatMessage(controller->server, SERVER_BO1ZT_MSG_PREFIX "You might want to /restart the run!");
        
        // Initialize activeGame with current game state when attaching mid-game
        Game *activeGame = &controller->state->activeGame;
        Level currentLevel = controllerGetLevelName(controller);
        if (levelIsMonitored(currentLevel)) {
            int elapsed = controllerGetLevelElapsedTime(controller);
            activeGame->levelName = currentLevel;
            activeGame->elapsed = elapsed;
            // Set startTimestamp so gameRunning() returns true
            // We use a symbolic non-real timestamp based on elapsed time
            activeGame->startTimestamp = elapsed > 0 ? 1 : 0;
        }
    }
    return true;
}

bool controllerDetachGame(Controller *controller) {
    if (!controllerIsGameAttached(controller)) {
        LOG_WARN("Cannot detach game since is not attached");
        return false;
    }
    LOG_INFO("Detaching game");
    
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
    return engineIsGameReady(controller->engine);
}

bool controllerIsChatOpen(Controller *controller) {
    if (!controller) return false;
    if (!controller->process) return false;
    return engineIsChatOpen(controller->engine);
}

bool controllerWriteToChatInput(Controller *controller, const char *text) {
    if (!controller) return false;
    if (!controller->process) return false;
    return engineWriteToChatInput(controller->engine, text);
}

int controllerGetLevelElapsedTime(Controller *controller) {
    if (!controller) return 0;
    if (!controller->process) return 0;
    return engineGetLevelElapsedTime(controller->engine);
}

float controllerGetMovementSpeed(Controller *controller) {
    if (!controller) return 0;
    if (!controller->process) return 0;
    return engineGetMovementSpeed(controller->engine);
}

Level controllerGetLevelName(Controller *controller) {
    if (!controller) return LEVEL_INVALID;
    if (!controller->process) return LEVEL_INVALID;
    return engineGetLevelName(controller->engine);
}

bool controllerTryAttachGameWindow(Controller *controller) {
    if (!controller) return false;
    if (!controller->process) return false;
    return processTryAttachWindow(controller->process, GAME_WINDOW_NAME_PREFIX);
}

bool controllerGetCheat(Controller *controller, CheatName cheat) {
    if (!controller || !controller->engine) return false;
    return engineIsCheatEnabled(controller->engine, cheat);
}

bool controllerSetCheat(Controller *controller, CheatName cheat, bool enabled) {
    if (!controller || !controller->engine) return false;
    return engineSetCheatEnabled(controller->engine, cheat, enabled);
}

bool controllerSetSimpleCheat(Controller *controller, SimpleCheatName cheat, void *value) {
    if (!controller || !controller->engine) return false;
    return engineSetSimpleCheat(controller->engine, cheat, value);
}

void controllerDestroy(Controller *controller) {
    if (controller) {
        if (controller->cheatManager) {
            cheatManagerDestroy(controller->cheatManager);
            controller->cheatManager = NULL;
        }
        if (controller->camoManager) {
            camoManagerDestroy(controller->camoManager);
            controller->camoManager = NULL;
        }
        if (controller->twitchManager) {
            twitchManagerDestroy(controller->twitchManager);
            controller->twitchManager = NULL;
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
    return engineGetPlayerCurrentCoords(controller->engine);
}

bool controllerGiveAmmo(Controller *controller) {
    if (!controller) return false;
    return engineGiveAmmo(controller->engine);
}

bool controllerGiveWeapons(Controller *controller, List *weapons) {
    if (!controller) return false;
    return engineGiveWeapons(controller->engine, weapons);
}

bool controllerAddPerks(Controller *controller, List *perks) {
    if (!controller || !controller->engine) return false;
    return engineAddPerks(controller->engine, perks);
}

bool controllerRemovePerks(Controller *controller, List *perks) {
    if (!controller || !controller->engine) return false;
    return engineRemovePerks(controller->engine, perks);
}

int controllerGetPerkCount(Controller *controller) {
    if (!controller || !controller->state) return 0;
    return controller->state->activeGame.numPerks;
}

int controllerGetPlayerHealth(Controller *controller) {
    if (!controller || !controller->engine) return 0;
    return engineGetSimpleCheatIntValue(controller->engine, SIMPLE_CHEAT_NAME_SET_HEALTH);
}

int controllerGetPlayerPoints(Controller *controller) {
    if (!controller || !controller->engine) return 0;
    return engineGetSimpleCheatIntValue(controller->engine, SIMPLE_CHEAT_NAME_SET_POINTS);
}

int controllerGetPlayerKills(Controller *controller) {
    if (!controller || !controller->engine) return 0;
    return engineGetSimpleCheatIntValue(controller->engine, SIMPLE_CHEAT_NAME_SET_KILLS);
}

int controllerGetPlayerHeadshots(Controller *controller) {
    if (!controller || !controller->engine) return 0;
    return engineGetSimpleCheatIntValue(controller->engine, SIMPLE_CHEAT_NAME_SET_HEADSHOTS);
}

bool controllerGetPlayerName(Controller *controller, char *out, size_t size) {
    if (!controller || !controller->engine) return false;
    return engineGetPlayerName(controller->engine, out, size);
}

bool controllerTakeWeapons(Controller *controller) {
    if (!controller) return false;
    return engineTakeWeapons(controller->engine);
}

bool controllerSetRound(Controller *controller, int round) {
    if (!controller || !controller->process) return false;
    return engineSetRound(controller->engine, round);
}

bool controllerRestartMap(Controller *controller) {
    if (!controller || !controller->server) return false;
    return serverExecuteCommand(controller->server, "map_restart");
}

bool controllerPlayEasterEggSong(Controller *controller) {
    if (!controller || !controller->engine) return false;
    return enginePlayEasterEggSong(controller->engine);
}

int controllerGetClaymoreCount(Controller *controller) {
    if (!controller || !controller->engine) return 0;
    return engineGetClaymoreCount(controller->engine);
}

bool controllerServerExecuteCommand(Controller *controller, const char *command) {
    if (!controller || !controller->server || !command) return false;
    return serverExecuteCommand(controller->server, command);
}

char *controllerServerGetDvar(Controller *controller, const char *name) {
    if (!controller || !controller->server || !name) return NULL;
    return serverGetDVarString(controller->server, name);
}

bool controllerServerSetDvar(Controller *controller, const char *name, const char *value) {
    if (!controller || !controller->server || !name || !value) return false;
    return serverSetDVarString(controller->server, name, value);
}

State *controllerGetState(Controller *controller) {
    if (!controller) return NULL;
    return controller->state;
}

void controllerUpdateState(Controller *controller) {
    if (!controller || !controller->state) return;
    if (!controllerIsGameAttached(controller) || !controllerIsGameRunning(controller) || !controllerIsGameReady(controller)) {
        stateGameClear(controller->state);
        return;
    };

    // General state
    controller->state->isGameAttached = controllerIsGameWindowAttached(controller);
    controller->state->isZombiesGameOngoing = controllerIsZombiesGameOngoing(controller);
    controller->state->isZombiesGamePaused = engineIsZombiesGamePaused(controller->engine);
    controller->state->gameResets = engineGetGameResets(controller->engine);
    
    // Active game
    Game *activeGame = &controller->state->activeGame;
    if (gameRunning(activeGame) && levelIsMonitored(activeGame->levelName)) {
        activeGame->movementSpeed = controllerGetMovementSpeed(controller);
        activeGame->currentEntities = engineGetCurrentSnapshotEntities(controller->engine);
        activeGame->maxEntities = engineGetMaxSnapshotEntities(controller->engine);
        int levelElapsed = controllerGetLevelElapsedTime(controller);
        if (levelElapsed > 0) gameUpdateElapsed(activeGame, levelElapsed);
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

    controllerSetCheat(controller, CHEAT_NAME_PATCH_CHAT, true);    
    cheatManagerHandleGameAttach(controller->cheatManager);
}

void controllerUpdateTrainerConfig(Controller *controller) {
    if (!controllerIsGameAttached(controller) || !controllerIsGameReady(controller)) return;
    
    cheatManagerHandleStateChange(controller->cheatManager);
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

TwitchConfig controllerGetTwitchConfig(Controller *controller) {
    return controller->config->twitch;
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
            LOG_ERROR("Unknown Config Type %d", type);
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

Engine *_controllerGetEngine(Controller *controller) {
    if (!controller) return NULL;
    return controller->engine;
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

CommandManager *_controllerGetCommandManager(Controller *controller) {
    if (!controller) return NULL;
    return controller->commandManager;
}

CommandManager *controllerGetCommandManager(Controller *controller) {
    if (!controller) return NULL;
    return controller->commandManager;
}
