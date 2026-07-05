#include "service/commands.h"
#include "service/service_internal.h"
#include "logic/command/manager.h"

#include <stddef.h>

typedef struct {
    ServiceCommandInfo *out;
    int max;
    int count;
} CollectCtx;

static void deriveName(const char *usage, char *buf, size_t bufSize) {
    buf[0] = '\0';
    if (!usage || bufSize == 0) return;
    const char *p = usage;
    if (*p == '/') p++;
    size_t i = 0;
    while (*p && *p != ' ' && i + 1 < bufSize) buf[i++] = *p++;
    buf[i] = '\0';
}

static void collect(const CommandEntry *entry, void *userData) {
    CollectCtx *ctx = (CollectCtx *)userData;
    if (!entry || ctx->count >= ctx->max) return;
    ServiceCommandInfo *info = &ctx->out[ctx->count];
    deriveName(entry->usage, info->name, sizeof(info->name));
    info->usage = entry->usage;
    info->description = entry->description;
    ctx->count++;
}

ServiceResult serviceCommandsList(Service *service, ServiceCommandInfo *out, int max, int *countOut) {
    if (!service || !out || max <= 0 || !countOut) return SERVICE_INVALID_PARAM;
    CommandManager *manager = controllerGetCommandManager(service->controller);
    if (!manager) return SERVICE_ENGINE_FAILED;
    CollectCtx ctx = { out, max, 0 };
    commandManagerForEach(manager, collect, &ctx);
    *countOut = ctx.count;
    return SERVICE_OK;
}
