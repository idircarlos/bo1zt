#include "logic/gsc.h"
#include "logic/gsc/pool.h"
#include "logger.h"
#include "logic/server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define GSC_DVAR_VALUE_MAX_LEN 1024

char *_gscMethodBuildDvarValue(GSCMethod method, int argc, const char **args);

static const char *GSC_METHOD_NAMES[] = {
    "AddPerks",
    "RemovePerks",
    "StaticBox",
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
    if (!gsc) return NULL;

    const char *methodName = gscMethodToString(method);
    LOG_INFO("Calling %s with %d args...\n", methodName, args.count);

    int index = poolAcquire(gsc->pool);

    char dvarKey[256];
    snprintf(dvarKey, sizeof(dvarKey), "bo1zt_gsc_worker_%d", index);

    char *dvarValue = _gscMethodBuildDvarValue(method, args.count, args.args);
    if (!dvarValue) {
        poolRelease(gsc->pool, index);
        return NULL;
    }

    gsc->pool->responses[index] = NULL;
    LOG_DEBUG("Sending %s = %s\n", dvarKey, dvarValue);
    serverSetDVarString(gsc->server, dvarKey, dvarValue);
    free(dvarValue);

    while (gsc->pool->responses[index] == NULL) {
        Sleep(10);
    }

    GSCResponse resp = gsc->pool->responses[index];
    poolRelease(gsc->pool, index);

    LOG_INFO("Received %s -> [%s]\n", methodName, resp);
    return resp;
}

void gscWriteResponse(GSC *gsc, int index, GSCResponse response) {
    if (!gsc || !gsc->pool) return;
    poolWriteResponseDirect(gsc->pool, index, response);
}

const char *gscMethodToString(GSCMethod method) {
    if (method < 0 || method >= GSC_METHOD_NAMES_SIZE)
        return "Unknown";
    return GSC_METHOD_NAMES[method] ? GSC_METHOD_NAMES[method] : "Unknown";
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

char *_gscMethodBuildDvarValue(GSCMethod method, int argc, const char **args) {
    char *value = (char*)calloc(GSC_DVAR_VALUE_MAX_LEN, 1);
    if (!value) return NULL;

    const char *methodName = gscMethodToString(method);

    snprintf(value, GSC_DVAR_VALUE_MAX_LEN, "bo1zt::%s::", methodName);

    for (int i = 0; i < argc; i++) {
        if (args[i] != NULL) {
            strncat(value, args[i], GSC_DVAR_VALUE_MAX_LEN - strlen(value) - 1);
            if (i < argc - 1) {
                strncat(value, ",", GSC_DVAR_VALUE_MAX_LEN - strlen(value) - 1);
            }
        }
    }

    return value;
}
