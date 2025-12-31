#include "api.h"
#include "api/raw.h"
#include "api/gsc.h"
#include "controller.h"
#include "logic/cheat.h"
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
    
    // Use GSC backend for BOX_NEVER_MOVES
    if (cheatName == CHEAT_NAME_BOX_NEVER_MOVES) {
        return apiGetStaticBox(api);
    }
    
    return rawApiIsCheatEnabled(api->raw, cheatName);
}

bool apiSetCheatEnabled(Api *api, CheatName cheatName, bool enabled) {
    if (!api) return false;
    
    // Use GSC backend for BOX_NEVER_MOVES
    if (cheatName == CHEAT_NAME_BOX_NEVER_MOVES) {
        return apiSetStaticBox(api, enabled);
    }
    
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

bool apiSetRound(Api *api, int round) {
    if (!api) return false;
    int currentRound = gscApiGetRound(api->gsc);
    return rawApiSetRound(api->raw, currentRound, round);
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

float apiGetMovementSpeed(Api *api) {
    if (!api) return 0;
    return rawApiGetMovementSpeed(api->raw);
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

int apiGetClaymoreCount(Api *api) {
    if (!api) return 0;
    return rawApiGetClaymoreCount(api->raw);
}

int apiGetCurrentSnapshotEntities(Api *api) {
    if (!api) return 0;
    return rawApiGetCurrentSnapshotEntities(api->raw);
}

int apiGetMaxSnapshotEntities(Api *api) {
    if (!api) return 0;
    return rawApiGetMaxSnapshotEntities(api->raw);
}

bool apiIsChatOpen(Api *api) {
    if (!api) return false;
    return rawApiIsChatOpen(api->raw);
}

bool apiWriteToChatInput(Api *api, const char *text) {
    if (!api) return false;
    return rawApiWriteToChatInput(api->raw, text);
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

bool apiGetStaticBox(Api *api) {
    if (!api || !api->gsc) return false;
    return gscApiGetStaticBox(api->gsc);
}

bool apiSetStaticBox(Api *api, bool enabled) {
    if (!api || !api->gsc) return false;
    return gscApiSetStaticBox(api->gsc, enabled);
}

bool apiPlayEasterEggSong(Api *api) {
    if (!api || !api->gsc) return false;
    return gscApiPlayEasterEggSong(api->gsc);
}

int apiGetRound(Api *api) {
    if (!api || !api->gsc) return false;
    return gscApiGetRound(api->gsc);
}

bool apiGiveWeapons(Api *api, List *weapons) {
    if (!api || !api->gsc) return false;
    return gscApiGiveWeapons(api->gsc, weapons);
}

bool apiTakeWeapons(Api *api) {
    if (!api || !api->gsc) return false;
    return gscApiTakeWeapons(api->gsc);
}
