#include "logic/camo/manager/persistence.h"
#include "logic/camo/manager/manager_internal.h"
#include "utils/json.h"
#include "win/file.h"
#include "logger.h"

#include <windows.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAMO_ROOT_FOLDER  "camo-manager"
#define CAMO_CAMOS_FOLDER "camos"
#define CAMO_BUNDLES_FOLDER "bundles"
#define CAMO_STATE_FILE   "camo-manager.json"

#define CAMO_STAGE_SUFFIX  ".stage"
#define CAMO_BACKUP_SUFFIX ".old"

static const char *fileTypeName(CamoFileType type) {
    switch (type) {
    case CAMO_FILE_SPEC:   return "spec";
    case CAMO_FILE_COLOR:  return "color";
    case CAMO_FILE_ENV:    return "env";
    case CAMO_FILE_NORMAL: return "normal";
    default:               return NULL;
    }
}

bool camoPersistenceValidId(const char *id) {
    if (!id || id[0] == '\0') return false;
    if (strcmp(id, ".") == 0 || strcmp(id, "..") == 0) return false;

    for (const char *p = id; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c < 0x20) return false;
        switch (c) {
        case '/':
        case '\\':
        case ':':
        case '*':
        case '?':
        case '"':
        case '<':
        case '>':
        case '|':
            return false;
        default:
            break;
        }
    }
    return true;
}

static bool camoPersistenceBaseDir(char *out, size_t bufSize) {
    if (!out || bufSize == 0) return false;

    if (!fileAppFolderPath(out, bufSize, CAMO_ROOT_FOLDER)) {
        LOG_ERROR("Failed to resolve %%APPDATA%%");
        return false;
    }
    return fileCreateFolder(out);
}

static bool camoPersistenceJsonPath(char *out, size_t bufSize) {
    if (!out || bufSize == 0) return false;

    char base[MAX_PATH];
    if (!camoPersistenceBaseDir(base, sizeof(base))) return false;

    int n = snprintf(out, bufSize, "%s\\%s", base, CAMO_STATE_FILE);
    if (n < 0 || (size_t)n >= bufSize) {
        LOG_ERROR("camo-manager.json path too long");
        return false;
    }
    return true;
}

static bool composeOwnerDir(char *out, size_t bufSize, const char *category,
                            const char *id) {
    if (!out || bufSize == 0) return false;
    if (!camoPersistenceValidId(id)) {
        LOG_ERROR("Invalid camo/bundle id");
        return false;
    }

    char base[MAX_PATH];
    if (!camoPersistenceBaseDir(base, sizeof(base))) return false;

    char categoryDir[MAX_PATH];
    int n = snprintf(categoryDir, sizeof(categoryDir), "%s\\%s", base, category);
    if (n < 0 || (size_t)n >= sizeof(categoryDir)) {
        LOG_ERROR("Managed category path too long");
        return false;
    }
    if (!fileCreateFolder(categoryDir)) return false;

    n = snprintf(out, bufSize, "%s\\%s", categoryDir, id);
    if (n < 0 || (size_t)n >= bufSize) {
        LOG_ERROR("Managed owner path too long");
        return false;
    }
    return fileCreateFolder(out);
}

bool camoPersistenceCamoDir(char *out, size_t bufSize, const char *camoId) {
    return composeOwnerDir(out, bufSize, CAMO_CAMOS_FOLDER, camoId);
}

bool camoPersistenceBundleDir(char *out, size_t bufSize, const char *bundleId) {
    return composeOwnerDir(out, bufSize, CAMO_BUNDLES_FOLDER, bundleId);
}

static bool camoPersistenceFileLeaf(char *out, size_t bufSize, CamoFileType type,
                             unsigned int number) {
    if (!out || bufSize == 0) return false;

    const char *typeName = fileTypeName(type);
    if (!typeName) {
        LOG_ERROR("Invalid camo file type %d", (int)type);
        return false;
    }

    int n;
    if (number == 0) {
        n = snprintf(out, bufSize, "%s.iwi", typeName);
    } else {
        n = snprintf(out, bufSize, "%s_%u.iwi", typeName, number);
    }
    if (n < 0 || (size_t)n >= bufSize) {
        LOG_ERROR("Managed file leaf too long");
        return false;
    }
    return true;
}

bool camoPersistenceCamoFilePath(char *out, size_t bufSize, const char *camoId,
                                 CamoFileType type, unsigned int number) {
    if (!out || bufSize == 0) return false;

    char dir[MAX_PATH];
    if (!camoPersistenceCamoDir(dir, sizeof(dir), camoId)) return false;

    char leaf[64];
    if (!camoPersistenceFileLeaf(leaf, sizeof(leaf), type, number)) return false;

    int n = snprintf(out, bufSize, "%s\\%s", dir, leaf);
    if (n < 0 || (size_t)n >= bufSize) {
        LOG_ERROR("Managed camo file path too long");
        return false;
    }
    return true;
}

bool camoPersistenceCopy(const char *src, const char *dst, bool replace) {
    if (!src || src[0] == '\0' || !dst || dst[0] == '\0') {
        LOG_ERROR("Copy requires non-empty source and destination paths");
        return false;
    }

    if (!fileCopy(src, dst, replace)) {
        LOG_ERROR("Failed to copy '%s' -> '%s' (error %lu)", src, dst,
                  (unsigned long)GetLastError());
        return false;
    }
    return true;
}

bool camoPersistenceDelete(const char *path) {
    if (!path || path[0] == '\0') {
        LOG_ERROR("Delete requires a non-empty path");
        return false;
    }

    if (fileDelete(path)) return true;

    LOG_ERROR("Failed to delete '%s' (error %lu)", path, (unsigned long)GetLastError());
    return false;
}

bool camoPersistenceImportCamoFiles(const char *camoId, const CamoFile *files,
                                    size_t fileCount) {
    if (fileCount > 0 && !files) {
        LOG_ERROR("Import called with null file list");
        return false;
    }

    char camoDir[MAX_PATH];
    if (!camoPersistenceCamoDir(camoDir, sizeof(camoDir), camoId)) return false;

    char stageDir[MAX_PATH];
    char backupDir[MAX_PATH];
    int ns = snprintf(stageDir, sizeof(stageDir), "%s%s", camoDir, CAMO_STAGE_SUFFIX);
    int nb = snprintf(backupDir, sizeof(backupDir), "%s%s", camoDir, CAMO_BACKUP_SUFFIX);
    if (ns < 0 || (size_t)ns >= sizeof(stageDir) ||
        nb < 0 || (size_t)nb >= sizeof(backupDir)) {
        LOG_ERROR("Import staging path too long");
        return false;
    }

    fileDelete(stageDir);
    if (!fileCreateFolder(stageDir)) return false;

    for (size_t i = 0; i < fileCount; ++i) {
        const CamoFile *f = &files[i];
        if (!f->fileName || f->fileName[0] == '\0') {
            LOG_ERROR("Camo file %zu has no source path", i);
            fileDelete(stageDir);
            return false;
        }

        char leaf[64];
        if (!camoPersistenceFileLeaf(leaf, sizeof(leaf), f->type, f->number)) {
            fileDelete(stageDir);
            return false;
        }

        char stagePath[MAX_PATH];
        int n = snprintf(stagePath, sizeof(stagePath), "%s\\%s", stageDir, leaf);
        if (n < 0 || (size_t)n >= sizeof(stagePath)) {
            LOG_ERROR("Staged file path too long");
            fileDelete(stageDir);
            return false;
        }

        if (!camoPersistenceCopy(f->fileName, stagePath, true)) {
            fileDelete(stageDir);
            return false;
        }
    }

    fileDelete(backupDir);

    if (!fileMove(camoDir, backupDir, false)) {
        LOG_ERROR("Failed to set aside camo directory '%s' (error %lu)", camoDir,
                  (unsigned long)GetLastError());
        fileDelete(stageDir);
        return false;
    }

    if (!fileMove(stageDir, camoDir, false)) {
        LOG_ERROR("Failed to swap in staged camo directory '%s' (error %lu)",
                  camoDir, (unsigned long)GetLastError());
        fileMove(backupDir, camoDir, false);
        fileDelete(stageDir);
        return false;
    }

    fileDelete(backupDir);
    return true;
}

static bool camoPersistenceFileKey(char *out, size_t bufSize, CamoFileType type,
                            unsigned int number) {
    if (!out || bufSize == 0) return false;

    const char *typeName = fileTypeName(type);
    if (!typeName) {
        LOG_ERROR("Invalid camo file type %d", (int)type);
        return false;
    }

    int n;
    if (number == 0) {
        n = snprintf(out, bufSize, "%s", typeName);
    } else {
        n = snprintf(out, bufSize, "%s_%u", typeName, number);
    }
    if (n < 0 || (size_t)n >= bufSize) {
        LOG_ERROR("Camo file key too long");
        return false;
    }
    return true;
}

bool camoPersistenceParseFileKey(const char *key, CamoFileType *type,
                                 unsigned int *number) {
    if (!key || key[0] == '\0') return false;

    static const struct {
        const char *name;
        CamoFileType type;
    } kTypes[] = {
        { "spec",   CAMO_FILE_SPEC },
        { "color",  CAMO_FILE_COLOR },
        { "env",    CAMO_FILE_ENV },
        { "normal", CAMO_FILE_NORMAL },
    };

    for (size_t i = 0; i < sizeof(kTypes) / sizeof(kTypes[0]); ++i) {
        size_t len = strlen(kTypes[i].name);
        if (strncmp(key, kTypes[i].name, len) != 0) continue;

        const char *rest = key + len;
        if (rest[0] == '\0') {
            if (type) *type = kTypes[i].type;
            if (number) *number = 0;
            return true;
        }
        if (rest[0] != '_') return false;
        const char *digits = rest + 1;
        if (digits[0] < '1' || digits[0] > '9') return false;

        unsigned int value = 0;
        for (const char *p = digits; *p; ++p) {
            if (*p < '0' || *p > '9') return false;
            if (value > (UINT_MAX - (unsigned)(*p - '0')) / 10u) return false;
            value = value * 10u + (unsigned)(*p - '0');
        }
        if (type) *type = kTypes[i].type;
        if (number) *number = value;
        return true;
    }
    return false;
}

static bool isBareFileName(const char *name) {
    if (!name || name[0] == '\0') return false;
    for (const char *p = name; *p; ++p) {
        if (*p == '/' || *p == '\\' || *p == ':') return false;
    }
    return true;
}

static void freeCamoArray(Camo *camos, size_t count) {
    if (!camos) return;
    for (size_t i = 0; i < count; ++i) {
        free(camos[i].id);
        free(camos[i].name);
        for (size_t j = 0; j < camos[i].fileCount; ++j) {
            free(camos[i].files[j].fileName);
        }
        free(camos[i].files);
    }
    free(camos);
}

static void freeBundleArray(CamoBundle *bundles, size_t count) {
    if (!bundles) return;
    for (size_t i = 0; i < count; ++i) {
        free(bundles[i].id);
        free(bundles[i].name);
        for (size_t j = 0; j < bundles[i].entryCount; ++j) {
            free(bundles[i].entries[j].weaponId);
            free(bundles[i].entries[j].camoId);
        }
        free(bundles[i].entries);
    }
    free(bundles);
}

static bool camoIdExists(const Camo *camos, size_t count, const char *id) {
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(camos[i].id, id) == 0) return true;
    }
    return false;
}

static bool weaponIdExists(const CamoManager *manager, const char *id) {
    for (size_t i = 0; i < manager->weaponCount; ++i) {
        if (strcmp(manager->weapons[i].id, id) == 0) return true;
    }
    return false;
}

static bool parseCamoFiles(const JsonValue *filesObj, CamoFile **outFiles,
                           size_t *outCount) {
    *outFiles = NULL;
    *outCount = 0;

    if (!filesObj) return true;
    if (jsonTypeOf(filesObj) != JSON_OBJECT) {
        LOG_ERROR("Camo 'files' is not an object");
        return false;
    }

    int count = jsonObjectCount(filesObj);
    if (count <= 0) return true;

    CamoFile *files = (CamoFile *)calloc((size_t)count, sizeof(CamoFile));
    if (!files) return false;

    size_t n = 0;
    for (int i = 0; i < count; ++i) {
        const char *key = jsonObjectKeyAt(filesObj, i);
        JsonValue *val = jsonObjectValueAt(filesObj, i);

        CamoFileType type;
        unsigned int number;
        if (!key || !camoPersistenceParseFileKey(key, &type, &number)) {
            LOG_ERROR("Invalid camo file key '%s'", key ? key : "(null)");
            goto fail;
        }
        for (size_t j = 0; j < n; ++j) {
            if (files[j].type == type && files[j].number == number) {
                LOG_ERROR("Duplicate camo file key '%s'", key);
                goto fail;
            }
        }
        const char *name = jsonGetString(val, NULL);
        if (!isBareFileName(name)) {
            LOG_ERROR("Invalid camo file name for key '%s'", key);
            goto fail;
        }

        files[n].type = type;
        files[n].number = number;
        files[n].fileName = _strdup(name);
        if (!files[n].fileName) goto fail;
        ++n;
    }

    *outFiles = files;
    *outCount = n;
    return true;

fail:
    for (size_t j = 0; j < n; ++j) free(files[j].fileName);
    free(files);
    return false;
}

static bool parseCamos(const JsonValue *root, Camo **outCamos, size_t *outCount) {
    *outCamos = NULL;
    *outCount = 0;

    JsonValue *arr = jsonObjectGet(root, "camos");
    if (!arr) return true;
    if (jsonTypeOf(arr) != JSON_ARRAY) {
        LOG_ERROR("'camos' is not an array");
        return false;
    }

    int count = jsonArrayCount(arr);
    if (count <= 0) return true;

    Camo *camos = (Camo *)calloc((size_t)count, sizeof(Camo));
    if (!camos) return false;

    size_t n = 0;
    for (int i = 0; i < count; ++i) {
        JsonValue *entry = jsonArrayAt(arr, i);
        if (jsonTypeOf(entry) != JSON_OBJECT) {
            LOG_ERROR("Camo entry %d is not an object", i);
            goto fail;
        }

        const char *id = jsonObjectGetString(entry, "id", NULL);
        const char *name = jsonObjectGetString(entry, "name", NULL);
        if (!camoPersistenceValidId(id)) {
            LOG_ERROR("Camo entry %d has an invalid id", i);
            goto fail;
        }
        if (camoIdExists(camos, n, id)) {
            LOG_ERROR("Duplicate camo id '%s'", id);
            goto fail;
        }
        if (!name || name[0] == '\0') {
            LOG_ERROR("Camo '%s' has an empty name", id);
            goto fail;
        }

        CamoFile *files = NULL;
        size_t fileCount = 0;
        if (!parseCamoFiles(jsonObjectGet(entry, "files"), &files, &fileCount)) {
            goto fail;
        }

        camos[n].id = _strdup(id);
        camos[n].name = _strdup(name);
        camos[n].files = files;
        camos[n].fileCount = fileCount;
        if (!camos[n].id || !camos[n].name) {
            free(camos[n].id);
            free(camos[n].name);
            for (size_t j = 0; j < fileCount; ++j) free(files[j].fileName);
            free(files);
            goto fail;
        }
        ++n;
    }

    *outCamos = camos;
    *outCount = n;
    return true;

fail:
    freeCamoArray(camos, n);
    return false;
}

static bool parseBundleEntries(const JsonValue *entriesArr,
                               const CamoManager *manager, const Camo *camos,
                               size_t camoCount, CamoBundleEntry **outEntries,
                               size_t *outCount) {
    *outEntries = NULL;
    *outCount = 0;

    if (!entriesArr) return true;
    if (jsonTypeOf(entriesArr) != JSON_ARRAY) {
        LOG_ERROR("Bundle 'entries' is not an array");
        return false;
    }

    int count = jsonArrayCount(entriesArr);
    if (count <= 0) return true;

    CamoBundleEntry *entries =
        (CamoBundleEntry *)calloc((size_t)count, sizeof(CamoBundleEntry));
    if (!entries) return false;

    size_t n = 0;
    for (int i = 0; i < count; ++i) {
        JsonValue *entry = jsonArrayAt(entriesArr, i);
        if (jsonTypeOf(entry) != JSON_OBJECT) {
            LOG_ERROR("Bundle entry %d is not an object", i);
            goto fail;
        }

        const char *weaponId = jsonObjectGetString(entry, "weaponId", NULL);
        const char *camoId = jsonObjectGetString(entry, "camoId", NULL);
        if (!weaponId || !weaponIdExists(manager, weaponId)) {
            LOG_ERROR("Bundle entry references unknown weapon '%s'",
                      weaponId ? weaponId : "(null)");
            goto fail;
        }
        if (!camoId || !camoIdExists(camos, camoCount, camoId)) {
            LOG_ERROR("Bundle entry references unknown camo '%s'",
                      camoId ? camoId : "(null)");
            goto fail;
        }
        for (size_t j = 0; j < n; ++j) {
            if (strcmp(entries[j].weaponId, weaponId) == 0) {
                LOG_ERROR("Bundle assigns weapon '%s' more than once", weaponId);
                goto fail;
            }
        }

        entries[n].weaponId = _strdup(weaponId);
        entries[n].camoId = _strdup(camoId);
        if (!entries[n].weaponId || !entries[n].camoId) {
            free(entries[n].weaponId);
            free(entries[n].camoId);
            goto fail;
        }
        ++n;
    }

    *outEntries = entries;
    *outCount = n;
    return true;

fail:
    for (size_t j = 0; j < n; ++j) {
        free(entries[j].weaponId);
        free(entries[j].camoId);
    }
    free(entries);
    return false;
}

static bool parseBundles(const JsonValue *root, const CamoManager *manager,
                         const Camo *camos, size_t camoCount,
                         CamoBundle **outBundles, size_t *outCount) {
    *outBundles = NULL;
    *outCount = 0;

    JsonValue *arr = jsonObjectGet(root, "bundles");
    if (!arr) return true;
    if (jsonTypeOf(arr) != JSON_ARRAY) {
        LOG_ERROR("'bundles' is not an array");
        return false;
    }

    int count = jsonArrayCount(arr);
    if (count <= 0) return true;

    CamoBundle *bundles = (CamoBundle *)calloc((size_t)count, sizeof(CamoBundle));
    if (!bundles) return false;

    size_t n = 0;
    for (int i = 0; i < count; ++i) {
        JsonValue *entry = jsonArrayAt(arr, i);
        if (jsonTypeOf(entry) != JSON_OBJECT) {
            LOG_ERROR("Bundle entry %d is not an object", i);
            goto fail;
        }

        const char *id = jsonObjectGetString(entry, "id", NULL);
        const char *name = jsonObjectGetString(entry, "name", NULL);
        if (!camoPersistenceValidId(id)) {
            LOG_ERROR("Bundle entry %d has an invalid id", i);
            goto fail;
        }
        for (size_t j = 0; j < n; ++j) {
            if (strcmp(bundles[j].id, id) == 0) {
                LOG_ERROR("Duplicate bundle id '%s'", id);
                goto fail;
            }
        }
        if (!name || name[0] == '\0') {
            LOG_ERROR("Bundle '%s' has an empty name", id);
            goto fail;
        }

        CamoBundleEntry *entries = NULL;
        size_t entryCount = 0;
        if (!parseBundleEntries(jsonObjectGet(entry, "entries"), manager, camos,
                                camoCount, &entries, &entryCount)) {
            goto fail;
        }

        bundles[n].id = _strdup(id);
        bundles[n].name = _strdup(name);
        bundles[n].entries = entries;
        bundles[n].entryCount = entryCount;
        if (!bundles[n].id || !bundles[n].name) {
            free(bundles[n].id);
            free(bundles[n].name);
            for (size_t j = 0; j < entryCount; ++j) {
                free(entries[j].weaponId);
                free(entries[j].camoId);
            }
            free(entries);
            goto fail;
        }
        ++n;
    }

    *outBundles = bundles;
    *outCount = n;
    return true;

fail:
    freeBundleArray(bundles, n);
    return false;
}

static bool bundleIdExists(const CamoBundle *bundles, size_t count,
                           const char *id) {
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(bundles[i].id, id) == 0) return true;
    }
    return false;
}

bool camoPersistenceLoad(CamoManager *manager) {
    if (!manager) return false;

    char path[MAX_PATH];
    if (!camoPersistenceJsonPath(path, sizeof(path))) return false;

    if (!fileExists(path)) return true;

    char *text = fileReadAll(path, NULL);
    if (!text) {
        LOG_ERROR("Failed to read '%s'", path);
        return false;
    }

    JsonValue *root = jsonParse(text);
    free(text);
    if (!root) {
        LOG_ERROR("camo-manager.json is corrupt; leaving it unchanged");
        return false;
    }

    bool ok = false;
    Camo *camos = NULL;
    size_t camoCount = 0;
    CamoBundle *bundles = NULL;
    size_t bundleCount = 0;
    char *activeId = NULL;
    char *installedFile = NULL;
    int version = -1;
    const char *activeStr = NULL;
    const char *installedStr = NULL;

    if (jsonTypeOf(root) != JSON_OBJECT) {
        LOG_ERROR("camo-manager.json root is not an object");
        goto done;
    }

    version = jsonObjectGetInt(root, "version", -1);
    if (version != CAMO_SCHEMA_VERSION) {
        LOG_ERROR("Unsupported camo-manager.json schema version %d", version);
        goto done;
    }

    if (!parseCamos(root, &camos, &camoCount)) goto done;
    if (!parseBundles(root, manager, camos, camoCount, &bundles, &bundleCount)) {
        goto done;
    }

    activeStr = jsonObjectGetString(root, "activeCamoBundleId", NULL);
    installedStr = jsonObjectGetString(root, "installedCamoBundleFile", NULL);

    if ((activeStr != NULL) != (installedStr != NULL)) {
        LOG_ERROR("activeCamoBundleId and installedCamoBundleFile must be set "
                  "together");
        goto done;
    }
    if (activeStr) {
        if (!bundleIdExists(bundles, bundleCount, activeStr)) {
            LOG_ERROR("activeCamoBundleId '%s' does not reference a bundle",
                      activeStr);
            goto done;
        }
        if (!isBareFileName(installedStr)) {
            LOG_ERROR("installedCamoBundleFile is not a bare file name");
            goto done;
        }
        activeId = _strdup(activeStr);
        installedFile = _strdup(installedStr);
        if (!activeId || !installedFile) goto done;
    }

    manager->camos = camos;
    manager->camoCount = camoCount;
    manager->bundles = bundles;
    manager->bundleCount = bundleCount;
    manager->activeBundleId = activeId;
    manager->installedBundleFile = installedFile;
    camos = NULL;
    bundles = NULL;
    activeId = NULL;
    installedFile = NULL;
    ok = true;

done:
    freeCamoArray(camos, camoCount);
    freeBundleArray(bundles, bundleCount);
    free(activeId);
    free(installedFile);
    jsonFree(root);
    return ok;
}

static JsonValue *buildCamoFilesJson(const Camo *camo) {
    JsonValue *files = jsonNewObject();
    if (!files) return NULL;
    for (size_t i = 0; i < camo->fileCount; ++i) {
        char key[64];
        if (!camoPersistenceFileKey(key, sizeof(key), camo->files[i].type,
                                    camo->files[i].number)) {
            jsonFree(files);
            return NULL;
        }
        jsonObjectSetString(files, key,
                            camo->files[i].fileName ? camo->files[i].fileName : "");
    }
    return files;
}

bool camoPersistenceSave(const CamoManager *manager) {
    if (!manager) return false;

    char path[MAX_PATH];
    if (!camoPersistenceJsonPath(path, sizeof(path))) return false;

    JsonValue *root = jsonNewObject();
    if (!root) return false;

    jsonObjectSetInt(root, "version", CAMO_SCHEMA_VERSION);

    JsonValue *camosArr = jsonNewArray();
    if (!camosArr) { jsonFree(root); return false; }
    jsonObjectSet(root, "camos", camosArr);
    for (size_t i = 0; i < manager->camoCount; ++i) {
        const Camo *camo = &manager->camos[i];
        JsonValue *obj = jsonNewObject();
        if (!obj) { jsonFree(root); return false; }
        jsonObjectSetString(obj, "id", camo->id);
        jsonObjectSetString(obj, "name", camo->name);
        JsonValue *files = buildCamoFilesJson(camo);
        if (!files) { jsonFree(obj); jsonFree(root); return false; }
        jsonObjectSet(obj, "files", files);
        jsonArrayAppend(camosArr, obj);
    }

    JsonValue *bundlesArr = jsonNewArray();
    if (!bundlesArr) { jsonFree(root); return false; }
    jsonObjectSet(root, "bundles", bundlesArr);
    for (size_t i = 0; i < manager->bundleCount; ++i) {
        const CamoBundle *bundle = &manager->bundles[i];
        JsonValue *obj = jsonNewObject();
        if (!obj) { jsonFree(root); return false; }
        jsonObjectSetString(obj, "id", bundle->id);
        jsonObjectSetString(obj, "name", bundle->name);
        JsonValue *entries = jsonNewArray();
        if (!entries) { jsonFree(obj); jsonFree(root); return false; }
        jsonObjectSet(obj, "entries", entries);
        for (size_t j = 0; j < bundle->entryCount; ++j) {
            JsonValue *entryObj = jsonNewObject();
            if (!entryObj) { jsonFree(root); return false; }
            jsonObjectSetString(entryObj, "weaponId", bundle->entries[j].weaponId);
            jsonObjectSetString(entryObj, "camoId", bundle->entries[j].camoId);
            jsonArrayAppend(entries, entryObj);
        }
        jsonArrayAppend(bundlesArr, obj);
    }

    if (manager->activeBundleId) {
        jsonObjectSetString(root, "activeCamoBundleId", manager->activeBundleId);
    } else {
        jsonObjectSetNull(root, "activeCamoBundleId");
    }
    if (manager->installedBundleFile) {
        jsonObjectSetString(root, "installedCamoBundleFile",
                            manager->installedBundleFile);
    } else {
        jsonObjectSetNull(root, "installedCamoBundleFile");
    }
    char *serialized = jsonSerializePretty(root, 2);
    jsonFree(root);
    if (!serialized) {
        LOG_ERROR("Failed to serialize camo-manager.json");
        return false;
    }

    char tmpPath[MAX_PATH];
    int tn = snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);
    if (tn < 0 || (size_t)tn >= sizeof(tmpPath)) {
        LOG_ERROR("camo-manager.json temp path too long");
        free(serialized);
        return false;
    }

    bool wrote = fileWriteAll(tmpPath, serialized, strlen(serialized));
    free(serialized);

    if (!wrote) {
        LOG_ERROR("Failed to write camo-manager.json");
        fileDelete(tmpPath);
        return false;
    }

    if (!fileMove(tmpPath, path, true)) {
        LOG_ERROR("Failed to replace '%s' (error %lu)", path,
                  (unsigned long)GetLastError());
        fileDelete(tmpPath);
        return false;
    }
    return true;
}
