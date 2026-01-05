#include "api/gsc.h"
#include "controller.h"
#include "controller/controller_internal.h"
#include "logger.h"
#include "logic/gsc.h"
#include "logic/gsc/method.h"
#include "logic/gsc/misc.h"
#include "logic/game/perk.h"
#include "logic/game/weapon.h"
#include "win/thread.h"
#include <stdlib.h>
#include <string.h>

struct GscApi {
    Controller *controller;
};

typedef struct {
    GSC *gsc;
    GSCMethod method;
    GSCArgs args;
} GscApiCallData;

static int _gscApiAsyncHandler(void *data);
static bool _gscApiCallAsync(GSC *gsc, GSCMethod method, GSCArgs args);
static bool _gscApiCallPerks(GscApi *gscApi, GSCMethod method, List *perks);
static bool _gscApiCallWeapons(GscApi *gscApi, List *weapons);
static GSCArgs _buildPerkArgs(List *perks);
static GSCArgs _buildWeaponArgs(List *weapons);

GscApi *gscApiCreate(Controller *controller) {
    if (!controller) return NULL;

    GscApi *gscApi = (GscApi *)malloc(sizeof(GscApi));
    if (!gscApi) return NULL;

    gscApi->controller = controller;
    return gscApi;
}

void gscApiDestroy(GscApi *gscApi) {
    if (gscApi) {
        free(gscApi);
    }
}

bool gscApiAddPerks(GscApi *gscApi, List *perks) {
    return _gscApiCallPerks(gscApi, GSC_ADD_PERKS, perks);
}

bool gscApiRemovePerks(GscApi *gscApi, List *perks) {
    return _gscApiCallPerks(gscApi, GSC_REMOVE_PERKS, perks);
}

static bool _gscApiCallPerks(GscApi *gscApi, GSCMethod method, List *perks) {
    if (!gscApi || !gscApi->controller || listIsEmpty(perks)) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscApi->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = _buildPerkArgs(perks);
    if (gscArgs.count == 0) {
        gscArgsFree(&gscArgs);
        return false;
    }

    return _gscApiCallAsync(gsc, method, gscArgs);
}

static int _gscApiAsyncHandler(void *data) {
    GscApiCallData *callData = (GscApiCallData *)data;
    GSCResponse response = gscCall(callData->gsc, callData->method, callData->args);
    if (response.status == GSC_STATUS_FAIL) {
        LOG_ERROR("Async GSC call to %s(%s) failed.", gscMethodToString(callData->method), gscArgsToString(callData->args));
    } else if (response.status == GSC_STATUS_TIMEOUT) {
        LOG_ERROR("Async GSC call to %s(%s) timed out.", gscMethodToString(callData->method), gscArgsToString(callData->args));
    }
    gscArgsFree(&callData->args);
    free(callData);
    return 1;
}

static bool _gscApiCallAsync(GSC *gsc, GSCMethod method, GSCArgs args) {
    GscApiCallData *callData = (GscApiCallData *)malloc(sizeof(GscApiCallData));
    if (!callData) {
        gscArgsFree(&args);
        return false;
    }

    callData->gsc = gsc;
    callData->method = method;
    callData->args = args;
    threadCreate(_gscApiAsyncHandler, (void *)callData);

    return true;
}

static GSCArgs _buildPerkArgs(List *perks) {
    size_t count = listSize(perks);
    GSCArgs gscArgs = {0};

    gscArgs.args = (const char **)malloc(count * sizeof(const char *));
    if (!gscArgs.args) return gscArgs;
    gscArgs.count = 0;

    for (size_t i = 0; i < count; i++) {
        Perk perk = (Perk)listGetInt(perks, i);
        const char *perkName = gscGetPerkName(perk);
        if (perkName != NULL) {
            gscArgs.args[gscArgs.count++] = perkName;
        }
    }
    return gscArgs;
}

static GSCArgs _buildWeaponArgs(List *weapons) {
    size_t count = listSize(weapons);
    GSCArgs gscArgs = {0};

    gscArgs.args = (const char **)malloc(count * sizeof(const char *));
    if (!gscArgs.args) return gscArgs;
    gscArgs.count = 0;

    for (size_t i = 0; i < count; i++) {
        Weapon weapon = (Weapon)listGetInt(weapons, i);
        const char *weaponName = gscGetWeaponName(weapon);
        if (weaponName != NULL) {
            gscArgs.args[gscArgs.count++] = weaponName;
        }
    }
    return gscArgs;
}

static bool _gscApiCallWeapons(GscApi *gscApi, List *weapons) {
    if (!gscApi || !gscApi->controller || listIsEmpty(weapons)) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscApi->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = _buildWeaponArgs(weapons);
    if (gscArgs.count == 0) {
        gscArgsFree(&gscArgs);
        return false;
    }

    return _gscApiCallAsync(gsc, GSC_GIVE_WEAPONS, gscArgs);
}

bool gscApiGetStaticBox(GscApi *gscApi) {
    if (!gscApi || !gscApi->controller) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscApi->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = gscArgsCreate(0);
    GSCResponse response = gscCall(gsc, GSC_STATIC_BOX, gscArgs);
    
    bool result = (response.status == GSC_STATUS_SUCCESS && strcmp(response.response, "1") == 0);
    return result;
}

bool gscApiSetStaticBox(GscApi *gscApi, bool enabled) {
    if (!gscApi || !gscApi->controller) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscApi->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = gscArgsCreate(1);
    if (!gscArgs.args) return false;
    gscArgs.args[0] = enabled ? "1" : "0";

    return _gscApiCallAsync(gsc, GSC_STATIC_BOX, gscArgs);
}

bool gscApiPlayEasterEggSong(GscApi *gscApi) {
    if (!gscApi || !gscApi->controller) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscApi->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = gscArgsCreate(0);
    return _gscApiCallAsync(gsc, GSC_PLAY_EASTER_EGG_SONG, gscArgs);
}

int gscApiGetRound(GscApi *gscApi) {
    if (!gscApi || !gscApi->controller) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscApi->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = gscArgsCreate(0);
    GSCResponse response = gscCall(gsc, GSC_GET_ROUND, gscArgs);
    
    bool success = (response.status == GSC_STATUS_SUCCESS && strcmp(response.response, "error") != 0);
    if (!success) {
        LOG_WARN("Received an error on GSC GetRound. Returning 1 as fallback");
        return 1;
    }
    int round = atoi(response.response);
    if (round == 0) {
        LOG_WARN("Invalid response from GSC GetRound. Returning 1 as fallback");
        return 1;
    }
    return round;
}

bool gscApiGiveWeapons(GscApi *gscApi, List *weapons) {
    return _gscApiCallWeapons(gscApi, weapons);
}

bool gscApiTakeWeapons(GscApi *gscApi) {
    if (!gscApi || !gscApi->controller) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscApi->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = gscArgsCreate(0);

    return _gscApiCallAsync(gsc, GSC_TAKE_WEAPONS, gscArgs);
}
