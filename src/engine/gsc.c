#include "engine/gsc.h"
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

struct GscBackend {
    Controller *controller;
};

typedef struct {
    GSC *gsc;
    GSCMethod method;
    GSCArgs args;
} GscBackendCallData;

static int _gscBackendAsyncHandler(void *data);
static bool _gscBackendCallAsync(GSC *gsc, GSCMethod method, GSCArgs args);
static bool _gscBackendCallPerks(GscBackend *gscBackend, GSCMethod method, List *perks);
static bool _gscBackendCallWeapons(GscBackend *gscBackend, List *weapons);
static GSCArgs _buildPerkArgs(List *perks);
static GSCArgs _buildWeaponArgs(List *weapons);

GscBackend *gscBackendCreate(Controller *controller) {
    if (!controller) return NULL;

    GscBackend *gscBackend = (GscBackend *)malloc(sizeof(GscBackend));
    if (!gscBackend) return NULL;

    gscBackend->controller = controller;
    return gscBackend;
}

void gscBackendDestroy(GscBackend *gscBackend) {
    if (gscBackend) {
        free(gscBackend);
    }
}

bool gscBackendAddPerks(GscBackend *gscBackend, List *perks) {
    return _gscBackendCallPerks(gscBackend, GSC_ADD_PERKS, perks);
}

bool gscBackendRemovePerks(GscBackend *gscBackend, List *perks) {
    return _gscBackendCallPerks(gscBackend, GSC_REMOVE_PERKS, perks);
}

static bool _gscBackendCallPerks(GscBackend *gscBackend, GSCMethod method, List *perks) {
    if (!gscBackend || !gscBackend->controller || listIsEmpty(perks)) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscBackend->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = _buildPerkArgs(perks);
    if (gscArgs.count == 0) {
        gscArgsFree(&gscArgs);
        return false;
    }

    return _gscBackendCallAsync(gsc, method, gscArgs);
}

static int _gscBackendAsyncHandler(void *data) {
    GscBackendCallData *callData = (GscBackendCallData *)data;
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

static bool _gscBackendCallAsync(GSC *gsc, GSCMethod method, GSCArgs args) {
    GscBackendCallData *callData = (GscBackendCallData *)malloc(sizeof(GscBackendCallData));
    if (!callData) {
        gscArgsFree(&args);
        return false;
    }

    callData->gsc = gsc;
    callData->method = method;
    callData->args = args;
    threadCreate(_gscBackendAsyncHandler, (void *)callData);

    return true;
}

static GSCArgs _buildPerkArgs(List *perks) {
    size_t count = listSize(perks);
    GSCArgs gscArgs = gscArgsCreate(count);

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
    GSCArgs gscArgs = gscArgsCreate(count);

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

static bool _gscBackendCallWeapons(GscBackend *gscBackend, List *weapons) {
    if (!gscBackend || !gscBackend->controller || listIsEmpty(weapons)) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscBackend->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = _buildWeaponArgs(weapons);
    if (gscArgs.count == 0) {
        gscArgsFree(&gscArgs);
        return false;
    }

    return _gscBackendCallAsync(gsc, GSC_GIVE_WEAPONS, gscArgs);
}

bool gscBackendGetStaticBox(GscBackend *gscBackend) {
    if (!gscBackend || !gscBackend->controller) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscBackend->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = gscArgsCreate(0);
    GSCResponse response = gscCall(gsc, GSC_STATIC_BOX, gscArgs);
    
    bool result = (response.status == GSC_STATUS_SUCCESS && strcmp(response.response, "1") == 0);
    return result;
}

bool gscBackendSetStaticBox(GscBackend *gscBackend, bool enabled) {
    if (!gscBackend || !gscBackend->controller) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscBackend->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = gscArgsCreate(1);
    if (!gscArgs.args) return false;
    gscArgs.args[0] = enabled ? "1" : "0";

    return _gscBackendCallAsync(gsc, GSC_STATIC_BOX, gscArgs);
}

bool gscBackendPlayEasterEggSong(GscBackend *gscBackend) {
    if (!gscBackend || !gscBackend->controller) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscBackend->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = gscArgsCreate(0);
    return _gscBackendCallAsync(gsc, GSC_PLAY_EASTER_EGG_SONG, gscArgs);
}

int gscBackendGetRound(GscBackend *gscBackend) {
    if (!gscBackend || !gscBackend->controller) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscBackend->controller);
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

bool gscBackendGiveWeapons(GscBackend *gscBackend, List *weapons) {
    return _gscBackendCallWeapons(gscBackend, weapons);
}

bool gscBackendTakeWeapons(GscBackend *gscBackend) {
    if (!gscBackend || !gscBackend->controller) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscBackend->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs gscArgs = gscArgsCreate(0);

    return _gscBackendCallAsync(gsc, GSC_TAKE_WEAPONS, gscArgs);
}
