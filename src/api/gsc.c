#include "api/gsc.h"
#include "logic/gsc.h"
#include "logic/gsc/misc.h"
#include "logic/game/perk.h"
#include "win/thread.h"
#include <stdlib.h>

struct ApiGsc {
    GSC *gsc;
};

typedef struct {
    GSC *gsc;
    GSCMethod method;
    GSCArgs *args;
} ApiGscCallData;

static int _apiThreadHandler(void *data);
static bool _apiGscCallPerks(ApiGsc *apiGsc, GSCMethod method, List *perks);
static GSCArgs *_buildPerkArgs(List *perks);

ApiGsc *apiGscCreate(GSC *gsc) {
    if (!gsc) return NULL;

    ApiGsc *apiGsc = (ApiGsc *)malloc(sizeof(ApiGsc));
    if (!apiGsc) return NULL;

    apiGsc->gsc = gsc;
    return apiGsc;
}

void apiGscDestroy(ApiGsc *apiGsc) {
    if (apiGsc) {
        free(apiGsc);
    }
}

bool apiGscAddPerks(ApiGsc *apiGsc, List *perks) {
    return _apiGscCallPerks(apiGsc, GSC_ADD_PERKS, perks);
}

bool apiGscRemovePerks(ApiGsc *apiGsc, List *perks) {
    return _apiGscCallPerks(apiGsc, GSC_REMOVE_PERKS, perks);
}

// Aux
static bool _apiGscCallPerks(ApiGsc *apiGsc, GSCMethod method, List *perks) {
    if (!apiGsc || !apiGsc->gsc || listIsEmpty(perks)) {
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

    ApiGscCallData *callData = (ApiGscCallData *)malloc(sizeof(ApiGscCallData));
    if (!callData) {
        gscArgsFree(gscArgs);
        free(gscArgs);
        return false;
    }

    callData->gsc = apiGsc->gsc;
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
    ApiGscCallData *callData = (ApiGscCallData *)data;
    gscCall(callData->gsc, callData->method, *callData->args);
    gscArgsFree(callData->args);
    free(callData->args);
    free(callData);
    return 1;
}
