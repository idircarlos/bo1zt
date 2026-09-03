#include "service/game.h"
#include "service/service_internal.h"
#include "logic/config.h"
#include "logic/cheat.h"
#include "logic/cheat/manager/actions.h"
#include "logic/game/character.h"

#include <stdio.h>
#include <string.h>

ServiceGameStatus serviceGameStatus(Service *service) {
    ServiceGameStatus status = { false, false, false, false, false };
    if (!service) return status;
    Controller *c = service->controller;
    status.attached = controllerIsGameAttached(c);
    status.running = controllerIsGameRunning(c);
    status.ready = controllerIsGameReady(c);
    status.windowFocused = controllerIsGameWindowFocused(c);
    status.dllInjected = controllerIsDllInjected(c);
    return status;
}

ServiceResult serviceGameLaunch(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (controllerIsGameRunning(service->controller)) return SERVICE_OK;
    if (!controllerLaunchGame(service->controller)) return SERVICE_ENGINE_FAILED;
    return SERVICE_OK;
}

ServiceResult serviceGameClose(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (!controllerCloseGame(service->controller)) return SERVICE_ENGINE_FAILED;
    return SERVICE_OK;
}

ServiceResult serviceGameRestart(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (!controllerIsGameAttached(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    if (!controllerRestartMap(service->controller)) return SERVICE_ENGINE_FAILED;
    return SERVICE_OK;
}

ServiceResult serviceGameGetConfig(Service *service, ServiceGameConfig *out) {
    if (!service || !out) return SERVICE_INVALID_PARAM;
    GameConfig cfg = controllerGetGameConfig(service->controller);
    snprintf(out->location, sizeof(out->location), "%s", cfg.location);
    snprintf(out->hostname, sizeof(out->hostname), "%s", cfg.hostname);
    snprintf(out->character, sizeof(out->character), "%s", characterName((Character)cfg.character));
    return SERVICE_OK;
}

ServiceResult serviceGameUpdateConfig(Service *service, const ServiceGameConfigPatch *patch) {
    if (!service || !patch) return SERVICE_INVALID_PARAM;
    Controller *c = service->controller;

    if (patch->hasLocation) {
        Config *config = controllerGetConfig(c);
        if (!config) return SERVICE_ENGINE_FAILED;
        strncpy(config->game.location, patch->location, sizeof(config->game.location) - 1);
        config->game.location[sizeof(config->game.location) - 1] = '\0';
        configSave(config);
    }
    if (patch->hasHostname) {
        // Route through the cheat manager so it persists to config and applies
        // live when the game is ready (same as the old GUI hostname entry).
        CheatResult r = cheatManagerSetValue(controllerGetCheatManager(c),
                                             SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME,
                                             (void *)patch->hostname);
        if (r == CHEAT_RESULT_API_FAILED) return SERVICE_ENGINE_FAILED;
    }
    if (patch->hasCharacter) {
        // Character is daemon-owned config: persist (and apply live when ready)
        // through the cheat manager, readable/writable whether or not a game is
        // attached — unlike PATCH /player, which gates on an ongoing game.
        Character character = characterFromName(patch->character);
        if (character == CHARACTER_INVALID) return SERVICE_NOT_FOUND;
        int value = (int)character;
        CheatResult r = cheatManagerSetValue(controllerGetCheatManager(c),
                                             SIMPLE_CHEAT_NAME_CHARACTER, &value);
        if (r == CHEAT_RESULT_API_FAILED) return SERVICE_ENGINE_FAILED;
    }
    return SERVICE_OK;
}
