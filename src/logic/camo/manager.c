#include "logic/camo/manager.h"
#include "logic/camo/manager/manager_internal.h"
#include "logic/camo/manager/persistence.h"
#include "logic/camo/manager/manifest.h"
#include "logic/camo/manager/iwd.h"
#include "logger.h"

#include <windows.h>
#include <objbase.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dupStr(const char *s) {
    return s ? _strdup(s) : NULL;
}

static void destroyFileArray(CamoFile *files, size_t count) {
    if (!files) return;
    for (size_t i = 0; i < count; ++i) {
        free(files[i].fileName);
    }
    free(files);
}

static void destroyCamo(Camo *camo) {
    free(camo->id);
    free(camo->name);
    destroyFileArray(camo->files, camo->fileCount);
}

static void destroyBundleEntry(CamoBundleEntry *entry) {
    free(entry->weaponId);
    free(entry->camoId);
}

static void destroyBundle(CamoBundle *bundle) {
    free(bundle->id);
    free(bundle->name);
    for (size_t i = 0; i < bundle->entryCount; ++i) {
        destroyBundleEntry(&bundle->entries[i]);
    }
    free(bundle->entries);
}

static void freeCamos(Camo *camos, size_t count) {
    if (!camos) return;
    for (size_t i = 0; i < count; ++i) {
        destroyCamo(&camos[i]);
    }
    free(camos);
}

static void freeBundles(CamoBundle *bundles, size_t count) {
    if (!bundles) return;
    for (size_t i = 0; i < count; ++i) {
        destroyBundle(&bundles[i]);
    }
    free(bundles);
}

static const char *fileBaseName(const char *path) {
    if (!path) return NULL;
    const char *base = path;
    for (const char *p = path; *p; ++p) {
        if (*p == '/' || *p == '\\' || *p == ':') base = p + 1;
    }
    return base;
}

static void removeManagedTree(const char *path) {
    char pattern[MAX_PATH];
    int n = snprintf(pattern, sizeof(pattern), "%s\\*", path);
    if (n < 0 || (size_t)n >= sizeof(pattern)) {
        RemoveDirectoryA(path);
        return;
    }

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) {
                continue;
            }
            char child[MAX_PATH];
            int cn = snprintf(child, sizeof(child), "%s\\%s", path, fd.cFileName);
            if (cn < 0 || (size_t)cn >= sizeof(child)) continue;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                removeManagedTree(child);
            } else {
                DeleteFileA(child);
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    if (!RemoveDirectoryA(path)) {
        DWORD err = GetLastError();
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
            LOG_ERROR("Failed to remove managed directory '%s' (error %lu)", path,
                      (unsigned long)err);
        }
    }
}

static void deleteManagedCamoDir(const char *camoId) {
    char dir[MAX_PATH];
    if (!camoPersistenceCamoDir(dir, sizeof(dir), camoId)) {
        LOG_ERROR("Could not resolve managed directory for camo '%s'", camoId);
        return;
    }
    removeManagedTree(dir);
}

static Camo *findCamo(CamoManager *manager, const char *id) {
    for (size_t i = 0; i < manager->camoCount; ++i) {
        if (strcmp(manager->camos[i].id, id) == 0) return &manager->camos[i];
    }
    return NULL;
}

static CamoBundle *findBundle(CamoManager *manager, const char *id) {
    for (size_t i = 0; i < manager->bundleCount; ++i) {
        if (strcmp(manager->bundles[i].id, id) == 0) {
            return &manager->bundles[i];
        }
    }
    return NULL;
}

static const CamoWeapon *findWeapon(const CamoManager *manager, const char *id) {
    for (size_t i = 0; i < manager->weaponCount; ++i) {
        if (strcmp(manager->weapons[i].id, id) == 0) return &manager->weapons[i];
    }
    return NULL;
}

static bool hasDuplicateFileKeys(const CamoFile *files, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            if (files[i].type == files[j].type &&
                files[i].number == files[j].number) {
                return true;
            }
        }
    }
    return false;
}

static bool buildFileMetadata(const CamoFile *src, size_t count,
                              CamoFile **out) {
    *out = NULL;
    if (count == 0) return true;

    CamoFile *arr = (CamoFile *)calloc(count, sizeof(CamoFile));
    if (!arr) return false;

    for (size_t i = 0; i < count; ++i) {
        const char *display = fileBaseName(src[i].fileName);
        if (!display || display[0] == '\0') {
            LOG_ERROR("Camo file %zu has no source path", i);
            destroyFileArray(arr, i);
            return false;
        }
        arr[i].type = src[i].type;
        arr[i].number = src[i].number;
        arr[i].fileName = dupStr(display);
        if (!arr[i].fileName) {
            destroyFileArray(arr, i);
            return false;
        }
    }

    *out = arr;
    return true;
}

static bool formatGuidId(char *out, size_t bufSize) {
    GUID guid;
    if (FAILED(CoCreateGuid(&guid))) {
        LOG_ERROR("Failed to generate a GUID");
        return false;
    }

    int n = snprintf(out, bufSize,
                     "%08lx%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
                     (unsigned long)guid.Data1, guid.Data2, guid.Data3,
                     guid.Data4[0], guid.Data4[1], guid.Data4[2],
                     guid.Data4[3], guid.Data4[4], guid.Data4[5],
                     guid.Data4[6], guid.Data4[7]);
    if (n < 0 || (size_t)n >= bufSize) {
        LOG_ERROR("Id buffer too small");
        return false;
    }
    return camoPersistenceValidId(out);
}

static bool generateCamoId(const CamoManager *manager, char *out,
                           size_t bufSize) {
    for (int attempt = 0; attempt < 32; ++attempt) {
        if (!formatGuidId(out, bufSize)) continue;

        bool collides = false;
        for (size_t i = 0; i < manager->camoCount; ++i) {
            if (strcmp(manager->camos[i].id, out) == 0) {
                collides = true;
                break;
            }
        }
        if (!collides) return true;
    }
    LOG_ERROR("Failed to generate a unique camo id");
    return false;
}

static bool generateBundleId(const CamoManager *manager, char *out,
                             size_t bufSize) {
    for (int attempt = 0; attempt < 32; ++attempt) {
        if (!formatGuidId(out, bufSize)) continue;

        bool collides = false;
        for (size_t i = 0; i < manager->bundleCount; ++i) {
            if (strcmp(manager->bundles[i].id, out) == 0) {
                collides = true;
                break;
            }
        }
        if (!collides) return true;
    }
    LOG_ERROR("Failed to generate a unique bundle id");
    return false;
}

CamoManager *camoManagerCreate(void) {
    CamoManager *manager = (CamoManager *)calloc(1, sizeof(CamoManager));
    if (!manager) {
        LOG_ERROR("Failed to allocate CamoManager");
        return NULL;
    }

    if (!camoManifestLoad(manager)) {
        LOG_ERROR("Failed to load embedded weapon manifest");
        free(manager);
        return NULL;
    }

    if (!camoPersistenceLoad(manager)) {
        LOG_ERROR("Failed to load camo-manager state");
        camoManifestFreeWeapons(manager->weapons, manager->weaponCount);
        free(manager);
        return NULL;
    }

    return manager;
}

void camoManagerDestroy(CamoManager *manager) {
    if (!manager) return;

    freeCamos(manager->camos, manager->camoCount);
    freeBundles(manager->bundles, manager->bundleCount);
    camoManifestFreeWeapons(manager->weapons, manager->weaponCount);
    free(manager->activeBundleId);
    free(manager->installedBundleFile);
    free(manager);
}

const Camo *camoManagerGetCamos(const CamoManager *manager, size_t *count) {
    if (!manager) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = manager->camoCount;
    return manager->camos;
}

const CamoBundle *camoManagerGetBundles(const CamoManager *manager, size_t *count) {
    if (!manager) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = manager->bundleCount;
    return manager->bundles;
}

const CamoWeapon *camoManagerGetWeapons(const CamoManager *manager, size_t *count) {
    if (!manager) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = manager->weaponCount;
    return manager->weapons;
}

const char *camoManagerGetActiveBundleId(const CamoManager *manager) {
    return manager ? manager->activeBundleId : NULL;
}

const char *camoManagerGetInstalledBundleFile(const CamoManager *manager) {
    return manager ? manager->installedBundleFile : NULL;
}

bool camoManagerCamoFilePath(const CamoManager *manager, const char *camoId,
                             CamoFileType type, unsigned int number,
                             char *out, size_t size) {
    if (!manager || !camoId) return false;
    return camoPersistenceCamoFilePath(out, size, camoId, type, number);
}

CamoResult camoManagerCamoCreate(CamoManager *manager, const char *name,
                                 const CamoFile *files, size_t fileCount) {
    if (!manager || !name || name[0] == '\0') return CAMO_RESULT_INVALID;
    if (fileCount > 0 && !files) return CAMO_RESULT_INVALID;
    if (hasDuplicateFileKeys(files, fileCount)) return CAMO_RESULT_INVALID;

    CamoFile *meta = NULL;
    if (!buildFileMetadata(files, fileCount, &meta)) return CAMO_RESULT_INVALID;

    char id[64];
    if (!generateCamoId(manager, id, sizeof(id))) {
        destroyFileArray(meta, fileCount);
        return CAMO_RESULT_INVALID;
    }

    if (!camoPersistenceImportCamoFiles(id, files, fileCount)) {
        destroyFileArray(meta, fileCount);
        return CAMO_RESULT_INVALID;
    }

    char *idCopy = dupStr(id);
    char *nameCopy = dupStr(name);
    if (!idCopy || !nameCopy) {
        free(idCopy);
        free(nameCopy);
        destroyFileArray(meta, fileCount);
        deleteManagedCamoDir(id);
        return CAMO_RESULT_INVALID;
    }

    Camo *grown = (Camo *)realloc(manager->camos,
                                  (manager->camoCount + 1) * sizeof(Camo));
    if (!grown) {
        free(idCopy);
        free(nameCopy);
        destroyFileArray(meta, fileCount);
        deleteManagedCamoDir(id);
        return CAMO_RESULT_INVALID;
    }
    manager->camos = grown;

    Camo *camo = &manager->camos[manager->camoCount];
    camo->id = idCopy;
    camo->name = nameCopy;
    camo->files = meta;
    camo->fileCount = fileCount;
    manager->camoCount += 1;

    if (!camoPersistenceSave(manager)) {
        manager->camoCount -= 1;
        destroyCamo(camo);
        camo->id = NULL;
        camo->name = NULL;
        camo->files = NULL;
        camo->fileCount = 0;
        deleteManagedCamoDir(id);
        return CAMO_RESULT_INVALID;
    }

    return CAMO_RESULT_OK;
}

CamoResult camoManagerCamoUpdate(CamoManager *manager, const char *camoId,
                                 const char *name, const CamoFile *files,
                                 size_t fileCount) {
    if (!manager || !camoId || !name || name[0] == '\0') return CAMO_RESULT_INVALID;
    if (fileCount > 0 && !files) return CAMO_RESULT_INVALID;

    Camo *camo = findCamo(manager, camoId);
    if (!camo) return CAMO_RESULT_NOT_FOUND;

    if (hasDuplicateFileKeys(files, fileCount)) return CAMO_RESULT_INVALID;

    CamoFile *meta = NULL;
    if (!buildFileMetadata(files, fileCount, &meta)) return CAMO_RESULT_INVALID;

    char *nameCopy = dupStr(name);
    if (!nameCopy) {
        destroyFileArray(meta, fileCount);
        return CAMO_RESULT_INVALID;
    }

    char *oldName = camo->name;
    CamoFile *oldFiles = camo->files;
    size_t oldCount = camo->fileCount;
    camo->name = nameCopy;
    camo->files = meta;
    camo->fileCount = fileCount;

    if (!camoPersistenceSave(manager)) {
        camo->name = oldName;
        camo->files = oldFiles;
        camo->fileCount = oldCount;
        free(nameCopy);
        destroyFileArray(meta, fileCount);
        return CAMO_RESULT_INVALID;
    }

    if (!camoPersistenceImportCamoFiles(camo->id, files, fileCount)) {
        camo->name = oldName;
        camo->files = oldFiles;
        camo->fileCount = oldCount;
        if (!camoPersistenceSave(manager)) {
            LOG_ERROR("Camo '%s' file import failed and its previous metadata "
                      "could not be restored", camo->id);
        }
        free(nameCopy);
        destroyFileArray(meta, fileCount);
        return CAMO_RESULT_INVALID;
    }

    free(oldName);
    destroyFileArray(oldFiles, oldCount);
    return CAMO_RESULT_OK;
}

typedef struct {
    size_t bundleIndex;
    CamoBundleEntry entry;
} CamoRemovedRef;

CamoResult camoManagerCamoRemove(CamoManager *manager, const char *camoId,
                                 bool removeReferences) {
    if (!manager || !camoId) return CAMO_RESULT_INVALID;

    size_t camoIndex = manager->camoCount;
    for (size_t i = 0; i < manager->camoCount; ++i) {
        if (strcmp(manager->camos[i].id, camoId) == 0) {
            camoIndex = i;
            break;
        }
    }
    if (camoIndex == manager->camoCount) return CAMO_RESULT_NOT_FOUND;

    size_t refCount = 0;
    for (size_t b = 0; b < manager->bundleCount; ++b) {
        const CamoBundle *bundle = &manager->bundles[b];
        for (size_t e = 0; e < bundle->entryCount; ++e) {
            if (strcmp(bundle->entries[e].camoId, camoId) == 0) ++refCount;
        }
    }

    if (refCount > 0 && !removeReferences) return CAMO_RESULT_IN_USE;

    CamoRemovedRef *removed = NULL;
    if (refCount > 0) {
        removed = (CamoRemovedRef *)calloc(refCount, sizeof(CamoRemovedRef));
        if (!removed) {
            LOG_ERROR("Out of memory removing camo references");
            return CAMO_RESULT_INVALID;
        }
    }

    size_t removedCount = 0;
    for (size_t b = 0; b < manager->bundleCount; ++b) {
        CamoBundle *bundle = &manager->bundles[b];
        size_t write = 0;
        for (size_t r = 0; r < bundle->entryCount; ++r) {
            if (strcmp(bundle->entries[r].camoId, camoId) == 0) {
                removed[removedCount].bundleIndex = b;
                removed[removedCount].entry = bundle->entries[r];
                ++removedCount;
            } else {
                bundle->entries[write++] = bundle->entries[r];
            }
        }
        bundle->entryCount = write;
    }

    Camo removedCamo = manager->camos[camoIndex];
    for (size_t i = camoIndex; i + 1 < manager->camoCount; ++i) {
        manager->camos[i] = manager->camos[i + 1];
    }
    manager->camoCount -= 1;

    if (!camoPersistenceSave(manager)) {
        manager->camoCount += 1;
        for (size_t i = manager->camoCount - 1; i > camoIndex; --i) {
            manager->camos[i] = manager->camos[i - 1];
        }
        manager->camos[camoIndex] = removedCamo;

        for (size_t k = 0; k < removedCount; ++k) {
            CamoBundle *bundle = &manager->bundles[removed[k].bundleIndex];
            bundle->entries[bundle->entryCount] = removed[k].entry;
            bundle->entryCount += 1;
        }

        free(removed);
        return CAMO_RESULT_INVALID;
    }

    deleteManagedCamoDir(camoId);

    for (size_t k = 0; k < removedCount; ++k) {
        destroyBundleEntry(&removed[k].entry);
    }
    free(removed);

    destroyCamo(&removedCamo);

    return CAMO_RESULT_OK;
}

CamoResult camoManagerBundleCreate(CamoManager *manager, const char *name) {
    if (!manager || !name || name[0] == '\0') return CAMO_RESULT_INVALID;

    char id[64];
    if (!generateBundleId(manager, id, sizeof(id))) return CAMO_RESULT_INVALID;

    char *idCopy = dupStr(id);
    char *nameCopy = dupStr(name);
    if (!idCopy || !nameCopy) {
        free(idCopy);
        free(nameCopy);
        return CAMO_RESULT_INVALID;
    }

    CamoBundle *grown =
        (CamoBundle *)realloc(manager->bundles,
                              (manager->bundleCount + 1) * sizeof(CamoBundle));
    if (!grown) {
        free(idCopy);
        free(nameCopy);
        return CAMO_RESULT_INVALID;
    }
    manager->bundles = grown;

    CamoBundle *bundle = &manager->bundles[manager->bundleCount];
    bundle->id = idCopy;
    bundle->name = nameCopy;
    bundle->entries = NULL;
    bundle->entryCount = 0;
    manager->bundleCount += 1;

    if (!camoPersistenceSave(manager)) {
        manager->bundleCount -= 1;
        free(bundle->id);
        free(bundle->name);
        bundle->id = NULL;
        bundle->name = NULL;
        return CAMO_RESULT_INVALID;
    }

    return CAMO_RESULT_OK;
}

CamoResult camoManagerBundleUpdate(CamoManager *manager, const char *bundleId,
                                       const char *name) {
    if (!manager || !bundleId || !name || name[0] == '\0') {
        return CAMO_RESULT_INVALID;
    }

    CamoBundle *bundle = findBundle(manager, bundleId);
    if (!bundle) return CAMO_RESULT_NOT_FOUND;

    char *nameCopy = dupStr(name);
    if (!nameCopy) return CAMO_RESULT_INVALID;

    char *oldName = bundle->name;
    bundle->name = nameCopy;

    if (!camoPersistenceSave(manager)) {
        bundle->name = oldName;
        free(nameCopy);
        return CAMO_RESULT_INVALID;
    }

    free(oldName);
    return CAMO_RESULT_OK;
}

CamoResult camoManagerBundleRemove(CamoManager *manager, const char *bundleId) {
    if (!manager || !bundleId) return CAMO_RESULT_INVALID;

    size_t bundleIndex = manager->bundleCount;
    for (size_t i = 0; i < manager->bundleCount; ++i) {
        if (strcmp(manager->bundles[i].id, bundleId) == 0) {
            bundleIndex = i;
            break;
        }
    }
    if (bundleIndex == manager->bundleCount) return CAMO_RESULT_NOT_FOUND;

    if (manager->activeBundleId &&
        strcmp(manager->activeBundleId, bundleId) == 0) {
        return CAMO_RESULT_IN_USE;
    }

    CamoBundle removedBundle = manager->bundles[bundleIndex];
    for (size_t i = bundleIndex; i + 1 < manager->bundleCount; ++i) {
        manager->bundles[i] = manager->bundles[i + 1];
    }
    manager->bundleCount -= 1;

    if (!camoPersistenceSave(manager)) {
        manager->bundleCount += 1;
        for (size_t i = manager->bundleCount - 1; i > bundleIndex; --i) {
            manager->bundles[i] = manager->bundles[i - 1];
        }
        manager->bundles[bundleIndex] = removedBundle;
        return CAMO_RESULT_INVALID;
    }

    destroyBundle(&removedBundle);
    return CAMO_RESULT_OK;
}

CamoResult camoManagerBundleAddCamo(CamoManager *manager, const char *bundleId,
                                        const char *weaponId, const char *camoId) {
    if (!manager || !bundleId || !weaponId || !camoId) return CAMO_RESULT_INVALID;

    CamoBundle *bundle = findBundle(manager, bundleId);
    if (!bundle) return CAMO_RESULT_NOT_FOUND;
    if (!findWeapon(manager, weaponId)) return CAMO_RESULT_NOT_FOUND;
    if (!findCamo(manager, camoId)) return CAMO_RESULT_NOT_FOUND;

    for (size_t e = 0; e < bundle->entryCount; ++e) {
        if (strcmp(bundle->entries[e].weaponId, weaponId) == 0) {
            if (strcmp(bundle->entries[e].camoId, camoId) == 0) {
                return CAMO_RESULT_OK;
            }

            char *newCamoId = dupStr(camoId);
            if (!newCamoId) return CAMO_RESULT_INVALID;

            char *oldCamoId = bundle->entries[e].camoId;
            bundle->entries[e].camoId = newCamoId;

            if (!camoPersistenceSave(manager)) {
                bundle->entries[e].camoId = oldCamoId;
                free(newCamoId);
                return CAMO_RESULT_INVALID;
            }

            free(oldCamoId);
            return CAMO_RESULT_OK;
        }
    }

    char *weaponCopy = dupStr(weaponId);
    char *camoCopy = dupStr(camoId);
    if (!weaponCopy || !camoCopy) {
        free(weaponCopy);
        free(camoCopy);
        return CAMO_RESULT_INVALID;
    }

    CamoBundleEntry *grown =
        (CamoBundleEntry *)realloc(bundle->entries,
                                   (bundle->entryCount + 1) * sizeof(CamoBundleEntry));
    if (!grown) {
        free(weaponCopy);
        free(camoCopy);
        return CAMO_RESULT_INVALID;
    }
    bundle->entries = grown;

    CamoBundleEntry *entry = &bundle->entries[bundle->entryCount];
    entry->weaponId = weaponCopy;
    entry->camoId = camoCopy;
    bundle->entryCount += 1;

    if (!camoPersistenceSave(manager)) {
        bundle->entryCount -= 1;
        free(entry->weaponId);
        free(entry->camoId);
        entry->weaponId = NULL;
        entry->camoId = NULL;
        return CAMO_RESULT_INVALID;
    }

    return CAMO_RESULT_OK;
}

CamoResult camoManagerBundleRemoveCamo(CamoManager *manager, const char *bundleId,
                                           const char *weaponId) {
    if (!manager || !bundleId || !weaponId) return CAMO_RESULT_INVALID;

    CamoBundle *bundle = findBundle(manager, bundleId);
    if (!bundle) return CAMO_RESULT_NOT_FOUND;

    size_t entryIndex = bundle->entryCount;
    for (size_t e = 0; e < bundle->entryCount; ++e) {
        if (strcmp(bundle->entries[e].weaponId, weaponId) == 0) {
            entryIndex = e;
            break;
        }
    }
    if (entryIndex == bundle->entryCount) return CAMO_RESULT_NOT_FOUND;

    CamoBundleEntry removedEntry = bundle->entries[entryIndex];
    for (size_t i = entryIndex; i + 1 < bundle->entryCount; ++i) {
        bundle->entries[i] = bundle->entries[i + 1];
    }
    bundle->entryCount -= 1;

    if (!camoPersistenceSave(manager)) {
        for (size_t i = bundle->entryCount; i > entryIndex; --i) {
            bundle->entries[i] = bundle->entries[i - 1];
        }
        bundle->entries[entryIndex] = removedEntry;
        bundle->entryCount += 1;
        return CAMO_RESULT_INVALID;
    }

    destroyBundleEntry(&removedEntry);
    return CAMO_RESULT_OK;
}

static CamoResult installBundleToMain(CamoManager *manager, CamoBundle *bundle,
                                      const char *gameLocation) {
    char mainDir[MAX_PATH];
    int n = snprintf(mainDir, sizeof(mainDir), "%s\\main", gameLocation);
    if (n < 0 || (size_t)n >= sizeof(mainDir)) {
        LOG_ERROR("Game main path too long for '%s'", gameLocation);
        return CAMO_RESULT_INVALID;
    }
    DWORD attrs = GetFileAttributesA(mainDir);
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        LOG_ERROR("Game main directory not found: '%s'", mainDir);
        return CAMO_RESULT_INVALID;
    }

    char leaf[MAX_PATH];
    if (!camoIwdFileName(leaf, sizeof(leaf), bundle)) {
        LOG_ERROR("Failed to compose IWD filename for bundle '%s'", bundle->id);
        return CAMO_RESULT_INVALID;
    }

    char bundleDir[MAX_PATH];
    if (!camoPersistenceBundleDir(bundleDir, sizeof(bundleDir), bundle->id)) {
        LOG_ERROR("Failed to resolve managed directory for bundle '%s'", bundle->id);
        return CAMO_RESULT_INVALID;
    }

    char builtPath[MAX_PATH];
    n = snprintf(builtPath, sizeof(builtPath), "%s\\%s", bundleDir, leaf);
    if (n < 0 || (size_t)n >= sizeof(builtPath)) {
        LOG_ERROR("Managed IWD path too long for bundle '%s'", bundle->id);
        return CAMO_RESULT_INVALID;
    }

    if (!camoIwdBuild(manager, bundle, builtPath)) {
        return CAMO_RESULT_INVALID;
    }

    char destPath[MAX_PATH];
    n = snprintf(destPath, sizeof(destPath), "%s\\%s", mainDir, leaf);
    if (n < 0 || (size_t)n >= sizeof(destPath)) {
        LOG_ERROR("Install destination path too long in '%s'", mainDir);
        return CAMO_RESULT_INVALID;
    }

    bool destWasTracked = manager->installedBundleFile &&
                          strcmp(manager->installedBundleFile, leaf) == 0;

    if (!camoPersistenceCopy(builtPath, destPath, true)) {
        LOG_ERROR("Failed to copy IWD into '%s'", destPath);
        return CAMO_RESULT_INVALID;
    }

    char *newActive = dupStr(bundle->id);
    char *newInstalled = dupStr(leaf);
    if (!newActive || !newInstalled) {
        free(newActive);
        free(newInstalled);
        if (!destWasTracked) camoPersistenceDelete(destPath);
        LOG_ERROR("Out of memory recording installed bundle");
        return CAMO_RESULT_INVALID;
    }

    char *oldActive = manager->activeBundleId;
    char *oldInstalled = manager->installedBundleFile;
    manager->activeBundleId = newActive;
    manager->installedBundleFile = newInstalled;

    if (!camoPersistenceSave(manager)) {
        manager->activeBundleId = oldActive;
        manager->installedBundleFile = oldInstalled;
        free(newActive);
        free(newInstalled);
        if (!destWasTracked) camoPersistenceDelete(destPath);
        return CAMO_RESULT_INVALID;
    }

    if (oldInstalled && strcmp(oldInstalled, leaf) != 0) {
        char oldPath[MAX_PATH];
        int on = snprintf(oldPath, sizeof(oldPath), "%s\\%s", mainDir, oldInstalled);
        if (on >= 0 && (size_t)on < sizeof(oldPath)) {
            if (!camoPersistenceDelete(oldPath)) {
                LOG_ERROR("Failed to remove previous installed IWD '%s'", oldPath);
            }
        }
    }

    free(oldActive);
    free(oldInstalled);
    return CAMO_RESULT_OK;
}

CamoResult camoManagerBundleInstall(CamoManager *manager, const char *bundleId,
                                        const char *gameLocation) {
    if (!manager || !bundleId || !gameLocation || gameLocation[0] == '\0') {
        return CAMO_RESULT_INVALID;
    }

    CamoBundle *bundle = findBundle(manager, bundleId);
    if (!bundle) return CAMO_RESULT_NOT_FOUND;

    return installBundleToMain(manager, bundle, gameLocation);
}

CamoResult camoManagerBundleUninstall(CamoManager *manager, const char *gameLocation) {
    if (!manager || !gameLocation || gameLocation[0] == '\0') return CAMO_RESULT_INVALID;

    if (!manager->installedBundleFile) {
        if (manager->activeBundleId) {
            char *oldActive = manager->activeBundleId;
            manager->activeBundleId = NULL;
            if (!camoPersistenceSave(manager)) {
                manager->activeBundleId = oldActive;
                return CAMO_RESULT_INVALID;
            }
            free(oldActive);
        }
        return CAMO_RESULT_OK;
    }

    char path[MAX_PATH];
    int n = snprintf(path, sizeof(path), "%s\\main\\%s", gameLocation,
                     manager->installedBundleFile);
    if (n < 0 || (size_t)n >= sizeof(path)) {
        LOG_ERROR("Uninstall path too long for '%s'", gameLocation);
        return CAMO_RESULT_INVALID;
    }

    if (!camoPersistenceDelete(path)) {
        LOG_ERROR("Failed to remove installed IWD '%s'", path);
        return CAMO_RESULT_INVALID;
    }

    char *oldActive = manager->activeBundleId;
    char *oldInstalled = manager->installedBundleFile;
    manager->activeBundleId = NULL;
    manager->installedBundleFile = NULL;

    if (!camoPersistenceSave(manager)) {
        manager->activeBundleId = oldActive;
        manager->installedBundleFile = oldInstalled;
        return CAMO_RESULT_INVALID;
    }

    free(oldActive);
    free(oldInstalled);
    return CAMO_RESULT_OK;
}
