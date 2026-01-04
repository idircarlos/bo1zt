#include "logic/gsc.h"
#include "logic/gsc/pool.h"
#include "logger.h"
#include "logic/server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define GSC_DVAR_VALUE_MAX_LEN 1024
#define GSC_SLEEP_MS 10
#define GSC_TIMEOUT 3000

char *_gscMethodBuildDvarValue(const char *methodString, const char *argsString);

static const char *GSC_METHOD_NAMES[] = {
    "AddPerks",
    "RemovePerks",
    "StaticBox",
    "PlayEasterEggSong",
    "GetRound",
    "GiveWeapons",
    "TakeWeapons",
};

static int GSC_METHOD_NAMES_SIZE = sizeof(GSC_METHOD_NAMES)/sizeof(GSC_METHOD_NAMES[0]);

typedef struct GSC {
    GSCPool *pool;
    Server *server;
} GSC;

GSC* gscCreate(Server *sever) {
    GSC *gsc = (GSC*)malloc(sizeof(GSC));
    if (!gsc) return NULL;

    gsc->pool = poolCreate();
    if (!gsc->pool) {
        free(gsc);
        return NULL;
    }

    gsc->server = sever;
    return gsc;
}

void gscDestroy(GSC *gsc) {
    if (!gsc) return;

    if (gsc->pool) poolDestroy(gsc->pool);
    free(gsc);
}

GSCResponse gscCall(GSC *gsc, GSCMethod method, GSCArgs args) {
    GSCResponse result = { .status = GSC_STATUS_FAIL, .response = "" };
    
    if (!gsc) return result;

    const char *methodString = gscMethodToString(method);
    const char *argsString = gscArgsToString(args);
    LOG_INFO("Calling %s(%s)", methodString, argsString);

    int index = poolAcquire(gsc->pool);

    char dvarKey[256];
    snprintf(dvarKey, sizeof(dvarKey), "bo1zt_gsc_worker_%d", index);

    char *dvarValue = _gscMethodBuildDvarValue(methodString, argsString);
    if (!dvarValue) {
        poolRelease(gsc->pool, index);
        return result;
    }

    gsc->pool->responses[index] = NULL;
    LOG_DEBUG("Sending %s = %s", dvarKey, dvarValue);
    serverSetDVarString(gsc->server, dvarKey, dvarValue);
    free(dvarValue);

    int requestElapsedTime = 0;
    while (gsc->pool->responses[index] == NULL && requestElapsedTime < GSC_TIMEOUT) {
        Sleep(GSC_SLEEP_MS);
        requestElapsedTime = requestElapsedTime + GSC_SLEEP_MS;
    }

    if (requestElapsedTime >= GSC_TIMEOUT) {
        LOG_WARN("GSC call %s timed out after %dms", methodString, GSC_TIMEOUT);
        poolRelease(gsc->pool, index);
        result.status = GSC_STATUS_TIMEOUT;
        return result;
    }

    const char *resp = gsc->pool->responses[index];
    poolRelease(gsc->pool, index);

    result.status = GSC_STATUS_SUCCESS;
    if (resp) {
        strncpy(result.response, resp, sizeof(result.response) - 1);
        result.response[sizeof(result.response) - 1] = '\0';
    }

    LOG_INFO("Received %s -> [%s]", methodString, result.response);
    return result;
}

void gscWriteResponse(GSC *gsc, int index, const char *response) {
    if (!gsc || !gsc->pool) return;
    poolWriteResponseDirect(gsc->pool, index, response);
}

const char *gscMethodToString(GSCMethod method) {
    if (method < 0 || method >= GSC_METHOD_NAMES_SIZE)
        return "Unknown";
    return GSC_METHOD_NAMES[method] ? GSC_METHOD_NAMES[method] : "Unknown";
}

const char* gscArgsToString(GSCArgs args) {
    char *value = (char*)calloc(GSC_DVAR_VALUE_MAX_LEN, 1);
    if (!value) return "";

    for (int i = 0; i < args.count; i++) {
        if (args.args[i] != NULL) {
            strncat(value, args.args[i], GSC_DVAR_VALUE_MAX_LEN - strlen(value) - 1);
            if (i < args.count - 1) {
                strncat(value, ",", GSC_DVAR_VALUE_MAX_LEN - strlen(value) - 1);
            }
        }
    }

    return value;
}

GSCArgs gscArgsCreate(int count) {
    GSCArgs gscArgs;
    gscArgs.count = count;
    gscArgs.args = (count > 0) ? (const char**)malloc(count * sizeof(const char*)) : NULL;
    return gscArgs;
}

void gscArgsFree(GSCArgs *gscArgs) {
    if (gscArgs && gscArgs->args) {
        free(gscArgs->args);
        gscArgs->args = NULL;
        gscArgs->count = 0;
    }
}

char *_gscMethodBuildDvarValue(const char *methodString, const char *argsString) {
    char *value = (char*)calloc(GSC_DVAR_VALUE_MAX_LEN, 1);
    if (!value) return NULL;

    snprintf(value, GSC_DVAR_VALUE_MAX_LEN, "bo1zt::%s::%s", methodString, argsString);
    return value;
}
