#include "api/gsc.h"
#include "controller.h"
#include "controller/controller_internal.h"
#include "logic/gsc.h"
#include "logic/gsc/misc.h"
#include "logic/game/perk.h"
#include "win/thread.h"
#include <stdlib.h>

struct GscApi {
    Controller *controller;
};

typedef struct {
    GSC *gsc;
    GSCMethod method;
    GSCArgs *args;
} GscApiCallData;

static int _apiThreadHandler(void *data);
static bool _gscApiCallPerks(GscApi *gscApi, GSCMethod method, List *perks);
static GSCArgs *_buildPerkArgs(List *perks);

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

// Aux
static bool _gscApiCallPerks(GscApi *gscApi, GSCMethod method, List *perks) {
    if (!gscApi || !gscApi->controller || listIsEmpty(perks)) {
        return false;
    }

    GSC *gsc = _controllerGetGsc(gscApi->controller);
    if (!gsc) {
        return false;
    }

    GSCArgs *gscArgs = _buildPerkArgs(perks);
    if (!gscArgs || gscArgs->count == 0) {
        if (gscArgs) {
            gscArgsFree(gscArgs);
            free(gscArgs);
        }
        return false;
    }

    GscApiCallData *callData = (GscApiCallData *)malloc(sizeof(GscApiCallData));
    if (!callData) {
        gscArgsFree(gscArgs);
        free(gscArgs);
        return false;
    }

    callData->gsc = gsc;
    callData->method = method;
    callData->args = gscArgs;
    threadCreate(_apiThreadHandler, (void *)callData);

    return true;
}

static GSCArgs *_buildPerkArgs(List *perks) {
    size_t count = listSize(perks);
    GSCArgs *gscArgs = (GSCArgs *)malloc(sizeof(GSCArgs));
    if (!gscArgs) return NULL;

    gscArgs->args = (const char **)malloc(count * sizeof(const char *));
    gscArgs->count = 0;

    for (size_t i = 0; i < count; i++) {
        Perk perk = (Perk)listGetInt(perks, i);
        const char *perkName = gscGetPerkName(perk);
        if (perkName != NULL) {
            gscArgs->args[gscArgs->count++] = perkName;
        }
    }
    return gscArgs;
}

static int _apiThreadHandler(void *data) {
    GscApiCallData *callData = (GscApiCallData *)data;
    gscCall(callData->gsc, callData->method, *callData->args);
    gscArgsFree(callData->args);
    free(callData->args);
    free(callData);
    return 1;
}
