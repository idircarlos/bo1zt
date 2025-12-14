#include "api.h"
#include "api/raw.h"
#include "api/gsc.h"
#include "controller.h"
#include "logger.h"
#include <stdlib.h>

struct Api {
    Controller *controller;
    RawApi *raw;
    GscApi *gsc;
};

Api *apiCreate(Controller *controller) {
    Api *api = (Api*)malloc(sizeof(Api));
    if (!api) return NULL;
    
    api->controller = controller;
    api->raw = rawApiCreate(controller);
    api->gsc = gscApiCreate(controller);
    
    if (!api->raw) {
        LOG_ERROR("Couldn't create Raw API backend\n");
        free(api);
        return NULL;
    }
    
    if (!api->gsc) {
        LOG_ERROR("Couldn't create GSC API backend\n");
        rawApiDestroy(api->raw);
        free(api);
        return NULL;
    }
    
    return api;
}

void apiDestroy(Api *api) {
    if (api) {
        if (api->raw) rawApiDestroy(api->raw);
        if (api->gsc) gscApiDestroy(api->gsc);
        free(api);
    }
}

// Raw backend forwards

bool apiIsCheatEnabled(Api *api, CheatName cheatName) {
    if (!api) return false;
    return rawApiIsCheatEnabled(api->raw, cheatName);
}

bool apiSetCheatEnabled(Api *api, CheatName cheatName, bool enabled) {
    if (!api) return false;
    return rawApiSetCheatEnabled(api->raw, cheatName, enabled);
}

bool apiSetSimpleCheat(Api *api, SimpleCheatName simpleCheatName, void *value) {
    if (!api) return false;
    return rawApiSetSimpleCheat(api->raw, simpleCheatName, value);
}

TeleportCoords *apiGetPlayerCurrentCoords(Api *api) {
    if (!api) return NULL;
    return rawApiGetPlayerCurrentCoords(api->raw);
}

WeaponName apiGetPlayerCurrentWeapon(Api *api) {
    if (!api) return WEAPON_UNKNOWNWEAPON;
    return rawApiGetPlayerCurrentWeapon(api->raw);
}

WeaponName apiGetPlayerWeapon(Api *api, int slot) {
    if (!api) return WEAPON_UNKNOWNWEAPON;
    return rawApiGetPlayerWeapon(api->raw, slot);
}

bool apiSetPlayerWeapon(Api *api, WeaponName weapon, int slot) {
    if (!api) return false;
    return rawApiSetPlayerWeapon(api->raw, weapon, slot);
}

bool apiGivePlayerAmmo(Api *api) {
    if (!api) return false;
    return rawApiGivePlayerAmmo(api->raw);
}

bool apiSetRound(Api *api, int currentRound, int nextRound) {
    if (!api) return false;
    return rawApiSetRound(api->raw, currentRound, nextRound);
}

bool apiIsGameReady(Api *api) {
    if (!api) return false;
    return rawApiIsGameReady(api->raw);
}

Level apiGetLevelName(Api *api) {
    if (!api) return LEVEL_INVALID;
    return rawApiGetLevelName(api->raw);
}

double apiGetLevelElapsedTime(Api *api) {
    if (!api) return 0;
    return rawApiGetLevelElapsedTime(api->raw);
}

bool apiIsZombiesGameOngoing(Api *api) {
    if (!api) return false;
    return rawApiIsZombiesGameOngoing(api->raw);
}

bool apiIsZombiesGamePaused(Api *api) {
    if (!api) return false;
    return rawApiIsZombiesGamePaused(api->raw);
}

int apiGetGameResets(Api *api) {
    if (!api) return 0;
    return rawApiGetGameResets(api->raw);
}

bool apiSVSendServerCommand(Api *api, int commandType, int clientNumber, const char *commands) {
    if (!api) return false;
    return rawApiSVSendServerCommand(api->raw, commandType, clientNumber, commands);
}

bool apiCBuffAddText(Api *api, const char *commands) {
    if (!api) return false;
    return rawApiCBuffAddText(api->raw, commands);
}

uintptr_t apiGetDVarPointer(Api *api, const char *dVar) {
    if (!api) return 0;
    return rawApiGetDVarPointer(api->raw, dVar);
}

// GSC backend forwards

bool apiAddPerks(Api *api, List *perks) {
    if (!api || !api->gsc) return false;
    return gscApiAddPerks(api->gsc, perks);
}

bool apiRemovePerks(Api *api, List *perks) {
    if (!api || !api->gsc) return false;
    return gscApiRemovePerks(api->gsc, perks);
}
