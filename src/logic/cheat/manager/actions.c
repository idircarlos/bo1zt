#include "logic/cheat/manager/actions.h"
#include "logger.h"
#include "logic/cheat/manager/manager_internal.h"
#include "logic/cheat/manager/state.h"
#include "logic/cheat/manager/registry.h"
#include "controller.h"
#include "controller/controller_internal.h"
#include "logic/config.h"
#include "logic/server.h"
#include "api.h"
#include <string.h>

static void notifyCheatFailed(CheatManager *manager) {
    if (!manager || !manager->controller) return;
    Server *server = _controllerGetServer(manager->controller);
    if (server) {
        serverChatMessage(server, "Cheat failed to apply");
    }
}

static bool checkConditions(CheatManager *manager, CheatCondition conditions) {
    if (!manager || !manager->controller) return false;
    if (conditions == CHEAT_COND_NONE) return true;
    
    // Check GAME_ONGOING condition - zombies game must be ongoing
    if ((conditions & CHEAT_COND_GAME_ONGOING) && !controllerIsZombiesGameOngoing(manager->controller)) {
        return false;
    }
    
    // Check GAME_RUNNING condition - game is attached
    if ((conditions & CHEAT_COND_GAME_READY) && !controllerIsGameReady(manager->controller)) {
        return false;
    }
    
    return true;
}

static bool checkToggleConditions(CheatManager *manager, CheatName cheat) {
    const CheatDefinition *def = findCheatDefinition(cheat);
    if (!def) return false;
    return checkConditions(manager, def->condition);
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

CheatResult cheatManagerSetToggle(CheatManager *manager, CheatName cheat, bool enabled) {
    if (!manager || !manager->config) return CHEAT_RESULT_API_FAILED;
    
    // Only handle managed toggle cheats
    if (!isManagedToggleCheat(cheat)) return CHEAT_RESULT_API_FAILED;
    
    // Check if value is unchanged
    bool currentConfig = getConfigToggleValue(manager->config, cheat);
    if (currentConfig == enabled) {
        return CHEAT_RESULT_NO_CHANGE;
    }
    
    // Update Config with desired value (always update Config regardless of conditions)
    setConfigToggleValue(manager->config, cheat, enabled);
    
    // Persist to INI immediately after Config change
    configSave(manager->config);
    
    // Check conditions
    if (!checkToggleConditions(manager, cheat)) {
        return CHEAT_RESULT_CONDITION_NOT_MET;
    }
    
    // Check if already applied with same value
    bool currentApplied = getAppliedToggleValue(&manager->applied, cheat);
    if (currentApplied == enabled) {
        return CHEAT_RESULT_OK; // Config changed and saved, but already applied
    }
    
    // Call API to apply the cheat
    Api *api = _controllerGetApi(manager->controller);
    if (!api) return CHEAT_RESULT_API_FAILED;
    
    bool apiResult = apiSetCheatEnabled(api, cheat, enabled);
    if (!apiResult) {
        notifyCheatFailed(manager);
        return CHEAT_RESULT_API_FAILED;
    }
    
    // Update AppliedState on success
    setAppliedToggleValue(&manager->applied, cheat, enabled);
    
    return CHEAT_RESULT_OK;
}

bool cheatManagerGetToggle(CheatManager *manager, CheatName cheat) {
    if (!manager || !manager->config) return false;
    
    if (!isManagedToggleCheat(cheat)) return false;
    
    return getConfigToggleValue(manager->config, cheat);
}

CheatResult cheatManagerToggle(CheatManager *manager, CheatName cheat) {
    bool current = cheatManagerGetToggle(manager, cheat);
    return cheatManagerSetToggle(manager, cheat, !current);
}

static bool isValueCheatUnchanged(CheatManager *manager, SimpleCheatName cheat, void *value) {
    if (!manager || !value) return false;
    
    switch (cheat) {
        case SIMPLE_CHEAT_NAME_FOV:
            return manager->applied.fov == *(int *)value;
        case SIMPLE_CHEAT_NAME_FOV_SCALE:
            return manager->applied.fovScale == *(int *)value;
        case SIMPLE_CHEAT_NAME_FPS_CAP:
            return manager->applied.fpsCap == *(int *)value;
        case SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME:
            return strcmp(manager->applied.hostname, (const char *)value) == 0;
        case SIMPLE_CHEAT_NAME_CHARACTER:
            return manager->applied.character == *(int *)value;
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_CHAT_NAME:
            return manager->applied.chatNameColor == *(ChatColor *)value;
        default:
            return false;
    }
}

static void updateAppliedValueCheat(CheatManager *manager, SimpleCheatName cheat, void *value) {
    if (!manager || !value) return;
    
    switch (cheat) {
        case SIMPLE_CHEAT_NAME_FOV:
            manager->applied.fov = *(int *)value;
            break;
        case SIMPLE_CHEAT_NAME_FOV_SCALE:
            manager->applied.fovScale = *(int *)value;
            break;
        case SIMPLE_CHEAT_NAME_FPS_CAP:
            manager->applied.fpsCap = *(int *)value;
            break;
        case SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME:
            strncpy(manager->applied.hostname, (const char *)value, sizeof(manager->applied.hostname) - 1);
            manager->applied.hostname[sizeof(manager->applied.hostname) - 1] = '\0';
            break;
        case SIMPLE_CHEAT_NAME_CHARACTER:
            manager->applied.character = *(int *)value;
            break;
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_CHAT_NAME:
            manager->applied.chatNameColor = *(ChatColor *)value;
            break;
        default:
            break;
    }
}

static bool hasTrackedAppliedState(SimpleCheatName cheat) {
    switch (cheat) {
        case SIMPLE_CHEAT_NAME_FOV:
        case SIMPLE_CHEAT_NAME_FOV_SCALE:
        case SIMPLE_CHEAT_NAME_FPS_CAP:
        case SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME:
        case SIMPLE_CHEAT_NAME_CHARACTER:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_CHAT_NAME:
            return true;
        default:
            return false;
    }
}

static void updateConfigValueCheat(Config *config, SimpleCheatName cheat, void *value) {
    if (!config || !value) return;
    
    switch (cheat) {
        case SIMPLE_CHEAT_NAME_FOV:
            config->graphics.fov = *(int *)value;
            break;
        case SIMPLE_CHEAT_NAME_FOV_SCALE:
            config->graphics.fovScale = *(int *)value;
            break;
        case SIMPLE_CHEAT_NAME_FPS_CAP:
            config->graphics.fpsCap = *(int *)value;
            break;
        case SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME:
            strncpy(config->game.hostname, (const char *)value, sizeof(config->game.hostname) - 1);
            config->game.hostname[sizeof(config->game.hostname) - 1] = '\0';
            break;
        case SIMPLE_CHEAT_NAME_CHARACTER:
            config->game.character = *(int *)value;
            break;
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_CHAT_NAME:
            config->customizer.chatName = *(ChatColor *)value;
            break;
        default:
            break;
    }
}

CheatResult cheatManagerSetValue(CheatManager *manager, SimpleCheatName cheat, void *value) {
    if (!manager || !value) return CHEAT_RESULT_API_FAILED;
    
    // For value cheats with tracked applied state, check if unchanged
    if (hasTrackedAppliedState(cheat) && isValueCheatUnchanged(manager, cheat, value)) {
        return CHEAT_RESULT_NO_CHANGE;
    }
    
    // Update Config with desired value (always update Config regardless of conditions)
    updateConfigValueCheat(manager->config, cheat, value);
    
    // Persist to INI immediately after Config change
    configSave(manager->config);
    
    // Check conditions for this value cheat
    if (!checkSimpleCheatConditions(manager, cheat)) {
        return CHEAT_RESULT_CONDITION_NOT_MET;
    }
    
    bool apiResult = false;
    
    // Character uses serverSetDVarInt instead of API
    if (cheat == SIMPLE_CHEAT_NAME_CHARACTER) {
        apiResult = applyCharacter(manager, *(int *)value);
    } else if (cheat == SIMPLE_CHEAT_NAME_CUSTOMIZER_CHAT_NAME) {
        Api *api = _controllerGetApi(manager->controller);
        if (!api) return CHEAT_RESULT_API_FAILED;
        apiResult = apiSetChatNameColor(api, *(ChatColor *)value);
    } else {
        Api *api = _controllerGetApi(manager->controller);
        if (!api) return CHEAT_RESULT_API_FAILED;
        apiResult = apiSetSimpleCheat(api, cheat, value);
    }
    
    if (!apiResult) {
        notifyCheatFailed(manager);
        return CHEAT_RESULT_API_FAILED;
    }
    
    // Update AppliedState on success (for tracked value cheats)
    updateAppliedValueCheat(manager, cheat, value);
    
    return CHEAT_RESULT_OK;
}

bool cheatManagerIsApplied(CheatManager *manager, CheatName cheat) {
    if (!manager) return false;
    if (!isManagedToggleCheat(cheat)) return false;
    return getAppliedToggleValue(&manager->applied, cheat);
}
