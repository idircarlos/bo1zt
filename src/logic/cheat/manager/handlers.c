#include "logic/cheat/manager/handlers.h"
#include "logic/cheat/manager/manager_internal.h"
#include "logic/cheat/manager/state.h"
#include "logic/cheat/manager/registry.h"
#include "controller.h"
#include "controller/controller_internal.h"
#include "logic/config.h"
#include "logic/server.h"
#include "engine.h"
#include <string.h>

static void notifyCheatFailed(CheatManager *manager) {
    if (!manager || !manager->controller) return;
    Server *server = _controllerGetServer(manager->controller);
    if (server) {
        serverChatMessage(server, SERVER_BO1ZT_MSG_PREFIX "Cheat failed to apply");
    }
}

static bool checkConditions(CheatManager *manager, CheatCondition conditions) {
    if (!manager || !manager->controller) return false;
    if (conditions == CHEAT_COND_NONE) return true;
    
    if ((conditions & CHEAT_COND_GAME_ONGOING) && !controllerIsZombiesGameOngoing(manager->controller)) {
        return false;
    }
    
    if ((conditions & CHEAT_COND_GAME_READY) && !controllerIsGameReady(manager->controller)) {
        return false;
    }
    
    return true;
}

static bool checkSimpleCheatConditions(CheatManager *manager, SimpleCheatName cheat) {
    const SimpleCheatDefinition *def = findSimpleCheatDefinition(cheat);
    if (!def) return false;
    return checkConditions(manager, def->condition);
}

static bool applyCharacter(CheatManager *manager, int character) {
    Server *server = _controllerGetServer(manager->controller);
    if (!server) return false;
    return serverSetDVarInt(server, "bo1zt_character", character);
}

void cheatManagerHandleGamePreStart(CheatManager *manager) {
    if (!manager || !manager->config) return;
    
    Engine *engine = _controllerGetEngine(manager->controller);
    if (!engine) return;
    
    for (int i = 0; i < NUM_CHEAT_REGISTRY; i++) {        
        CheatName cheat = CHEAT_REGISTRY[i].name;
        bool configValue = getConfigToggleValue(manager->config, cheat);
        
        // Skip disabled cheats
        if (!configValue) continue;
        
        // Force apply (ignore applied state, always re-apply)
        bool engineResult = engineSetCheatEnabled(engine, cheat, true);
        if (engineResult) {
            setAppliedToggleValue(&manager->applied, cheat, true);
        } else {
            notifyCheatFailed(manager);
        }
    }
    
    // Apply hostname if set (force apply on game start)
    GameConfig *game = &manager->config->game;
    if (strlen(game->hostname) > 0) {
        if (engineSetSimpleCheat(engine, SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME, game->hostname)) {
            strncpy(manager->applied.hostname, game->hostname, sizeof(manager->applied.hostname) - 1);
            manager->applied.hostname[sizeof(manager->applied.hostname) - 1] = '\0';
        }
    }
}

void cheatManagerHandleGameStart(CheatManager *manager) {
    if (!manager || !manager->config) return;
    
    Engine *engine = _controllerGetEngine(manager->controller);
    if (!engine) return;
    
    // Nothing as of now
}

void cheatManagerHandleStateChange(CheatManager *manager) {
    if (!manager || !manager->config) return;
    
    Engine *engine = _controllerGetEngine(manager->controller);
    if (!engine) return;
    
    // Handle value cheats that have conditions (FOV, FOVScale, FPSCap, Hostname)
    GraphicsConfig *graphics = &manager->config->graphics;
    GameConfig *game = &manager->config->game;
    
    // We do not "cache" since FOV is a special value that gets reset after each map restart.
    // Just force always a restart. 
    if (checkSimpleCheatConditions(manager, SIMPLE_CHEAT_NAME_FOV)) {
        if (engineSetSimpleCheat(engine, SIMPLE_CHEAT_NAME_FOV, &graphics->fov)) {
            manager->applied.fov = graphics->fov;
        }
    }
    
    if (manager->applied.fovScale != graphics->fovScale && checkSimpleCheatConditions(manager, SIMPLE_CHEAT_NAME_FOV_SCALE)) {
        if (engineSetSimpleCheat(engine, SIMPLE_CHEAT_NAME_FOV_SCALE, &graphics->fovScale)) {
            manager->applied.fovScale = graphics->fovScale;
        }
    }
    
    if (!graphics->unlimitFps && manager->applied.fpsCap != graphics->fpsCap && checkSimpleCheatConditions(manager, SIMPLE_CHEAT_NAME_FPS_CAP)) {
        if (engineSetSimpleCheat(engine, SIMPLE_CHEAT_NAME_FPS_CAP, &graphics->fpsCap)) {
            manager->applied.fpsCap = graphics->fpsCap;
        }
    }
    
    if (strcmp(manager->applied.hostname, game->hostname) != 0 && checkSimpleCheatConditions(manager, SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME)) {
        if (engineSetSimpleCheat(engine, SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME, game->hostname)) {
            strncpy(manager->applied.hostname, game->hostname, sizeof(manager->applied.hostname) - 1);
            manager->applied.hostname[sizeof(manager->applied.hostname) - 1] = '\0';
        }
    }
    
    // Character
    if (manager->applied.character != game->character && checkSimpleCheatConditions(manager, SIMPLE_CHEAT_NAME_CHARACTER)) {
        if (applyCharacter(manager, game->character)) {
            manager->applied.character = game->character;
        }
    }
}

void cheatManagerHandleGameAttach(CheatManager *manager) {
    if (!manager || !manager->config) return;
    
    Engine *engine = _controllerGetEngine(manager->controller);
    if (!engine) return;
    
    // Apply all enabled toggle cheats from Config that meet conditions
    for (int i = 0; i < NUM_CHEAT_REGISTRY; i++) {
        CheatName cheat = CHEAT_REGISTRY[i].name;
        bool configValue = getConfigToggleValue(manager->config, cheat);
        
        // Skip disabled cheats
        if (!configValue) continue;
        
        // Check conditions
        if (!checkConditions(manager, CHEAT_REGISTRY[i].condition)) continue;
        
        // Apply the cheat
        bool engineResult = engineSetCheatEnabled(engine, cheat, configValue);
        if (engineResult) {
            setAppliedToggleValue(&manager->applied, cheat, configValue);
        } else {
            notifyCheatFailed(manager);
        }
    }
    
    // Apply value cheats that meet conditions
    GraphicsConfig *graphics = &manager->config->graphics;
    GameConfig *game = &manager->config->game;
    
    // FOV
    if (checkSimpleCheatConditions(manager, SIMPLE_CHEAT_NAME_FOV)) {
        if (engineSetSimpleCheat(engine, SIMPLE_CHEAT_NAME_FOV, &graphics->fov)) {
            manager->applied.fov = graphics->fov;
        }
    }
    
    // FOV Scale
    if (checkSimpleCheatConditions(manager, SIMPLE_CHEAT_NAME_FOV_SCALE)) {
        if (engineSetSimpleCheat(engine, SIMPLE_CHEAT_NAME_FOV_SCALE, &graphics->fovScale)) {
            manager->applied.fovScale = graphics->fovScale;
        }
    }
    
    // FPS Cap
    if (checkSimpleCheatConditions(manager, SIMPLE_CHEAT_NAME_FPS_CAP)) {
        if (engineSetSimpleCheat(engine, SIMPLE_CHEAT_NAME_FPS_CAP, &graphics->fpsCap)) {
            manager->applied.fpsCap = graphics->fpsCap;
        }
    }
    
    // Hostname
    if (checkSimpleCheatConditions(manager, SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME)) {
        if (engineSetSimpleCheat(engine, SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME, game->hostname)) {
            strncpy(manager->applied.hostname, game->hostname, sizeof(manager->applied.hostname) - 1);
            manager->applied.hostname[sizeof(manager->applied.hostname) - 1] = '\0';
        }
    }
    
    // Character
    if (checkSimpleCheatConditions(manager, SIMPLE_CHEAT_NAME_CHARACTER)) {
        if (applyCharacter(manager, game->character)) {
            manager->applied.character = game->character;
        }
    }
}

void cheatManagerHandleGameDetach(CheatManager *manager) {
    if (!manager) return;
    appliedStateClear(&manager->applied);
}

void cheatManagerHandleGameEnd(CheatManager *manager) {
    if (!manager || !manager->config) return;
    
    Engine *engine = _controllerGetEngine(manager->controller);
    if (!engine) return;
    
    // Disable all cheats that require GAME_ONGOING condition
    for (int i = 0; i < NUM_CHEAT_REGISTRY; i++) {
        CheatName cheat = CHEAT_REGISTRY[i].name;
        CheatCondition condition = CHEAT_REGISTRY[i].condition;
        
        if (!(condition & CHEAT_COND_GAME_ONGOING)) continue;
        
        if (!getAppliedToggleValue(&manager->applied, cheat)) continue;
        
        bool engineResult = engineSetCheatEnabled(engine, cheat, false);
        if (engineResult) {
            setAppliedToggleValue(&manager->applied, cheat, false);
        }
    }
}
