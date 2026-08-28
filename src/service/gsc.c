#include "service/gsc.h"
#include "service/service_internal.h"

#include <string.h>

const GSCMod *serviceGscModList(Service *service, size_t *count) {
    if (!service || !count) return NULL;

    gscModsReload();

    *count = gscModsCount();
    return gscModsAt(0);
}

ServiceResult serviceGscModCreate(Service *service, const char *name) {
    if (!service || !name) return SERVICE_INVALID_PARAM;

    gscModsReload();

    return gscModsCreate(name) ? SERVICE_OK : SERVICE_INVALID_PARAM;
}

ServiceResult serviceGscFolderCreate(Service *service, const char *path) {
    if (!service || !path) return SERVICE_INVALID_PARAM;

    gscModsReload();

    return gscModsCreateFolder(path) ? SERVICE_OK : SERVICE_INVALID_PARAM;
}

ServiceResult serviceGscRename(Service *service, const char *path, const char *name) {
    if (!service || !path || !name) return SERVICE_INVALID_PARAM;

    gscModsReload();

    return gscModsRename(path, name) ? SERVICE_OK : SERVICE_INVALID_PARAM;
}

ServiceResult serviceGscRemove(Service *service, const char *path) {
    if (!service || !path) return SERVICE_INVALID_PARAM;

    gscModsReload();

    return gscModsRemove(path) ? SERVICE_OK : SERVICE_NOT_FOUND;
}

ServiceResult serviceGscFolder(Service *service, char *out, size_t size) {
    if (!service || !out || size == 0) return SERVICE_INVALID_PARAM;
    return gscModsDir(out, size) ? SERVICE_OK : SERVICE_ENGINE_FAILED;
}

char *serviceGscScriptRead(Service *service, const char *path) {
    if (!service || !path) return NULL;
    return gscModsReadScript(path, NULL);
}

ServiceResult serviceGscScriptWrite(Service *service, const char *path, const char *content) {
    if (!service || !path || !content) return SERVICE_INVALID_PARAM;
    return gscModsWriteScript(path, content, strlen(content)) ? SERVICE_OK : SERVICE_NOT_FOUND;
}

ServiceResult serviceGscScriptCreate(Service *service, const char *path) {
    if (!service || !path) return SERVICE_INVALID_PARAM;

    gscModsReload();

    return gscModsCreateScript(path) ? SERVICE_OK : SERVICE_INVALID_PARAM;
}
