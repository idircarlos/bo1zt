#include "service/camo.h"
#include "service/service_internal.h"
#include "controller.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *CAMO_FILE_TYPE_NAMES[CAMO_FILE_TYPE_COUNT] = {
    "spec", "color", "env", "normal",
};

const char *serviceCamoFileTypeName(CamoFileType type) {
    if ((int)type < 0 || (int)type >= CAMO_FILE_TYPE_COUNT) return NULL;
    return CAMO_FILE_TYPE_NAMES[type];
}

bool serviceCamoFileTypeFromName(const char *name, CamoFileType *typeOut) {
    if (!name || !typeOut) return false;
    for (int i = 0; i < CAMO_FILE_TYPE_COUNT; i++) {
        if (strcmp(CAMO_FILE_TYPE_NAMES[i], name) == 0) {
            *typeOut = (CamoFileType)i;
            return true;
        }
    }
    return false;
}

static CamoManager *managerOf(Service *service) {
    if (!service) return NULL;
    return controllerGetCamoManager(service->controller);
}

static ServiceResult translate(CamoResult result) {
    switch (result) {
        case CAMO_RESULT_OK:        return SERVICE_OK;
        case CAMO_RESULT_NOT_FOUND: return SERVICE_NOT_FOUND;
        case CAMO_RESULT_IN_USE:    return SERVICE_IN_USE;
        default:                    return SERVICE_INVALID_PARAM;
    }
}

static bool gameLocation(Service *service, char *out, size_t size) {
    GameConfig game = controllerGetGameConfig(service->controller);
    if (game.location[0] == '\0') return false;
    int n = snprintf(out, size, "%s", game.location);
    return n > 0 && (size_t)n < size;
}

static bool buildManagerFiles(const ServiceCamoFile *files, size_t fileCount,
                              CamoFile **out) {
    *out = NULL;
    if (fileCount == 0) return true;

    CamoFile *arr = (CamoFile *)calloc(fileCount, sizeof(CamoFile));
    if (!arr) return false;
    for (size_t i = 0; i < fileCount; ++i) {
        arr[i].type = files[i].type;
        arr[i].number = files[i].number;
        arr[i].fileName = (char *)files[i].source;
    }
    *out = arr;
    return true;
}

ServiceResult serviceCamoOverview(Service *service, ServiceCamoOverview *overviewOut) {
    CamoManager *manager = managerOf(service);
    if (!manager || !overviewOut) return SERVICE_INVALID_PARAM;

    camoManagerGetCamos(manager, &overviewOut->camoCount);
    camoManagerGetBundles(manager, &overviewOut->bundleCount);
    camoManagerGetWeapons(manager, &overviewOut->weaponCount);
    overviewOut->activeBundleId = camoManagerGetActiveBundleId(manager);
    return SERVICE_OK;
}

const CamoWeapon *serviceCamoWeaponList(Service *service, size_t *count) {
    return camoManagerGetWeapons(managerOf(service), count);
}

const CamoWeapon *serviceCamoWeaponFind(Service *service, const char *weaponId) {
    if (!weaponId) return NULL;
    size_t count = 0;
    const CamoWeapon *weapons = camoManagerGetWeapons(managerOf(service), &count);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(weapons[i].id, weaponId) == 0) return &weapons[i];
    }
    return NULL;
}

const Camo *serviceCamoList(Service *service, size_t *count) {
    return camoManagerGetCamos(managerOf(service), count);
}

const Camo *serviceCamoFind(Service *service, const char *camoId) {
    if (!camoId) return NULL;
    size_t count = 0;
    const Camo *camos = camoManagerGetCamos(managerOf(service), &count);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(camos[i].id, camoId) == 0) return &camos[i];
    }
    return NULL;
}

const CamoBundle *serviceCamoBundleList(Service *service, size_t *count) {
    return camoManagerGetBundles(managerOf(service), count);
}

const CamoBundle *serviceCamoBundleFind(Service *service, const char *bundleId) {
    if (!bundleId) return NULL;
    size_t count = 0;
    const CamoBundle *bundles = camoManagerGetBundles(managerOf(service), &count);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(bundles[i].id, bundleId) == 0) return &bundles[i];
    }
    return NULL;
}

bool serviceCamoBundleIsInstalled(Service *service, const char *bundleId) {
    const char *active = camoManagerGetActiveBundleId(managerOf(service));
    return active && bundleId && strcmp(active, bundleId) == 0;
}

ServiceResult serviceCamoCreate(Service *service, const char *name,
                                const ServiceCamoFile *files, size_t fileCount,
                                const char **idOut) {
    CamoManager *manager = managerOf(service);
    if (!manager || !name || name[0] == '\0') return SERVICE_INVALID_PARAM;
    if (fileCount > 0 && !files) return SERVICE_INVALID_PARAM;

    CamoFile *managerFiles = NULL;
    if (!buildManagerFiles(files, fileCount, &managerFiles)) return SERVICE_ENGINE_FAILED;

    CamoResult result = camoManagerCamoCreate(manager, name, managerFiles,
                                              fileCount, idOut);
    free(managerFiles);
    return translate(result);
}

static ServiceResult updateKeepingFiles(CamoManager *manager, const Camo *camo,
                                        const char *name) {
    size_t count = camo->fileCount;
    if (count == 0) {
        return translate(camoManagerCamoUpdate(manager, camo->id, name, NULL, 0));
    }

    CamoFile *files = (CamoFile *)calloc(count, sizeof(CamoFile));
    char (*paths)[MAX_PATH] = (char (*)[MAX_PATH])calloc(count, MAX_PATH);
    if (!files || !paths) {
        free(files);
        free(paths);
        return SERVICE_ENGINE_FAILED;
    }

    for (size_t i = 0; i < count; ++i) {
        if (!camoManagerCamoFilePath(manager, camo->id, camo->files[i].type,
                                     camo->files[i].number, paths[i], MAX_PATH)) {
            free(files);
            free(paths);
            return SERVICE_ENGINE_FAILED;
        }
        files[i].type = camo->files[i].type;
        files[i].number = camo->files[i].number;
        files[i].fileName = paths[i];
    }

    CamoResult result = camoManagerCamoUpdate(manager, camo->id, name, files, count);
    free(files);
    free(paths);
    return translate(result);
}

ServiceResult serviceCamoUpdate(Service *service, const char *camoId,
                                const ServiceCamoPatch *patch) {
    CamoManager *manager = managerOf(service);
    if (!manager || !camoId || !patch) return SERVICE_INVALID_PARAM;

    const Camo *camo = serviceCamoFind(service, camoId);
    if (!camo) return SERVICE_NOT_FOUND;

    const char *name = patch->hasName ? patch->name : camo->name;
    if (!name || name[0] == '\0') return SERVICE_INVALID_PARAM;

    if (!patch->hasFiles) return updateKeepingFiles(manager, camo, name);

    if (patch->fileCount > 0 && !patch->files) return SERVICE_INVALID_PARAM;

    CamoFile *managerFiles = NULL;
    if (!buildManagerFiles(patch->files, patch->fileCount, &managerFiles)) {
        return SERVICE_ENGINE_FAILED;
    }

    CamoResult result = camoManagerCamoUpdate(manager, camoId, name, managerFiles,
                                              patch->fileCount);
    free(managerFiles);
    return translate(result);
}

ServiceResult serviceCamoRemove(Service *service, const char *camoId,
                                bool removeReferences) {
    CamoManager *manager = managerOf(service);
    if (!manager || !camoId) return SERVICE_INVALID_PARAM;
    return translate(camoManagerCamoRemove(manager, camoId, removeReferences));
}

ServiceResult serviceCamoBundleCreate(Service *service, const char *name,
                                      const char **idOut) {
    CamoManager *manager = managerOf(service);
    if (!manager || !name || name[0] == '\0') return SERVICE_INVALID_PARAM;
    return translate(camoManagerBundleCreate(manager, name, idOut));
}

ServiceResult serviceCamoBundleRename(Service *service, const char *bundleId,
                                      const char *name) {
    CamoManager *manager = managerOf(service);
    if (!manager || !bundleId || !name || name[0] == '\0') return SERVICE_INVALID_PARAM;
    return translate(camoManagerBundleUpdate(manager, bundleId, name));
}

ServiceResult serviceCamoBundleRemove(Service *service, const char *bundleId) {
    CamoManager *manager = managerOf(service);
    if (!manager || !bundleId) return SERVICE_INVALID_PARAM;
    if (!serviceCamoBundleFind(service, bundleId)) return SERVICE_NOT_FOUND;

    if (serviceCamoBundleIsInstalled(service, bundleId)) {
        ServiceResult uninstalled = serviceCamoBundleUninstall(service, bundleId);
        if (uninstalled != SERVICE_OK) return uninstalled;
    }
    return translate(camoManagerBundleRemove(manager, bundleId));
}

ServiceResult serviceCamoBundleAssign(Service *service, const char *bundleId,
                                      const char *weaponId, const char *camoId) {
    CamoManager *manager = managerOf(service);
    if (!manager || !bundleId || !weaponId || !camoId) return SERVICE_INVALID_PARAM;
    return translate(camoManagerBundleAddCamo(manager, bundleId, weaponId, camoId));
}

ServiceResult serviceCamoBundleUnassign(Service *service, const char *bundleId,
                                        const char *weaponId) {
    CamoManager *manager = managerOf(service);
    if (!manager || !bundleId || !weaponId) return SERVICE_INVALID_PARAM;
    return translate(camoManagerBundleRemoveCamo(manager, bundleId, weaponId));
}

ServiceResult serviceCamoBundleInstall(Service *service, const char *bundleId) {
    CamoManager *manager = managerOf(service);
    if (!manager || !bundleId) return SERVICE_INVALID_PARAM;

    char location[MAX_PATH];
    if (!gameLocation(service, location, sizeof(location))) return SERVICE_INVALID_PARAM;
    return translate(camoManagerBundleInstall(manager, bundleId, location));
}

ServiceResult serviceCamoBundleUninstall(Service *service, const char *bundleId) {
    CamoManager *manager = managerOf(service);
    if (!manager || !bundleId) return SERVICE_INVALID_PARAM;
    if (!serviceCamoBundleFind(service, bundleId)) return SERVICE_NOT_FOUND;
    if (!serviceCamoBundleIsInstalled(service, bundleId)) return SERVICE_NOT_INSTALLED;

    char location[MAX_PATH];
    if (!gameLocation(service, location, sizeof(location))) return SERVICE_INVALID_PARAM;
    return translate(camoManagerBundleUninstall(manager, location));
}
