#include <oat.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logic/assets.h"
#include "logger.h"
#include "utils/json.h"
#include "win/file.h"
#include "win/resources.h"
#include "resource_ids.h"

static const char *const ASSETS_ZONES[] = {
    "common_zombie.ff",
    "zombie_cod5_asylum.ff",
    "zombie_cod5_factory.ff",
    "zombie_cod5_prototype.ff",
    "zombie_cod5_sumpf.ff",
};

static bool assetsDir(char *out, size_t bufSize) {
    if (!fileAppDataPath(out, bufSize, "bo1zt\\assets")) {
        LOG_ERROR("Failed to resolve %%APPDATA%%");
        return false;
    }
    return true;
}

bool assetsModelExportDir(char *out, size_t size) {
    char dir[MAX_PATH];
    if (!assetsDir(dir, sizeof(dir))) return false;
    snprintf(out, size, "%s\\model_export", dir);
    return true;
}

static int assetsCountLod0(const char *dir) {
    char pattern[MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*_lod0.xmodel_export", dir);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;

    int count = 0;
    do {
        count++;
    } while (FindNextFileA(h, &fd));

    FindClose(h);
    return count;
}

int assetsZoneCount(void) {
    return (int)(sizeof(ASSETS_ZONES) / sizeof(ASSETS_ZONES[0]));
}

const char *assetsZoneName(int index) {
    return ASSETS_ZONES[index];
}

bool assetsInstalled(void) {
    char modelExport[MAX_PATH];
    if (!assetsModelExportDir(modelExport, sizeof(modelExport))) return false;
    return fileExists(modelExport);
}

static void assetsModelName(const char *model, char *out, size_t size) {
    snprintf(out, size, "%s", model);

    char *ext = strstr(out, ".xmodel_export");
    if (ext) *ext = '\0';

    size_t len = strlen(out);
    for (size_t i = len; i-- > 0;) {
        if (strncmp(&out[i], "_lod", 4) != 0) continue;

        bool allDigits = out[i + 4] != '\0';
        for (size_t j = i + 4; out[j]; ++j) {
            if (out[j] < '0' || out[j] > '9') { allDigits = false; break; }
        }
        if (allDigits) out[i] = '\0';
        return;
    }
}

static bool assetsNameSeen(char **names, int count, const char *name) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(names[i], name) == 0) return true;
    }
    return false;
}

static void assetsFreeNames(char **names, int count) {
    for (int i = 0; i < count; ++i) free(names[i]);
    free(names);
}

static char **assetsWeaponModels(int *outCount) {
    *outCount = 0;

    void *data = NULL;
    DWORD size = 0;
    if (!resourcesGetData(IDR_CAMO_WEAPONS, &data, &size)) {
        LOG_ERROR("Failed to load embedded weapon manifest resource");
        return NULL;
    }

    char *text = (char *)malloc((size_t)size + 1);
    if (!text) return NULL;
    memcpy(text, data, (size_t)size);
    text[size] = '\0';

    JsonValue *root = jsonParse(text);
    free(text);
    if (!root) {
        LOG_ERROR("Weapon manifest is corrupt");
        return NULL;
    }

    const JsonValue *weapons = jsonObjectGet(root, "weapons");
    const int weaponCount = jsonArrayCount(weapons);
    char **names = weaponCount > 0 ? (char **)calloc((size_t)weaponCount, sizeof(char *)) : NULL;
    if (!names) {
        jsonFree(root);
        return NULL;
    }

    int n = 0;
    for (int i = 0; i < weaponCount; ++i) {
        const char *model = jsonObjectGetString(jsonArrayAt(weapons, i), "model", NULL);
        if (!model || model[0] == '\0') continue;

        char name[256];
        assetsModelName(model, name, sizeof(name));
        if (name[0] == '\0' || assetsNameSeen(names, n, name)) continue;

        names[n] = _strdup(name);
        if (!names[n]) {
            assetsFreeNames(names, n);
            jsonFree(root);
            return NULL;
        }
        n++;
    }

    jsonFree(root);
    *outCount = n;
    return names;
}

int assetsModelTotal(void) {
    static int total = -1;
    if (total < 0) {
        int count = 0;
        char **models = assetsWeaponModels(&count);
        assetsFreeNames(models, count);
        total = count;
    }
    return total;
}

int assetsExtractedCount(void) {
    char modelExport[MAX_PATH];
    if (!assetsModelExportDir(modelExport, sizeof(modelExport))) return 0;
    return assetsCountLod0(modelExport);
}

bool assetsExtractZone(const char *gameLocation, int index) {
    char dir[MAX_PATH];
    if (!assetsDir(dir, sizeof(dir))) return false;

    char zoneDir[MAX_PATH];
    snprintf(zoneDir, sizeof(zoneDir), "%s\\zone\\Common", gameLocation);

    char fastFile[MAX_PATH];
    snprintf(fastFile, sizeof(fastFile), "%s\\%s", zoneDir, ASSETS_ZONES[index]);

    int modelCount = 0;
    char **models = assetsWeaponModels(&modelCount);
    if (!models || modelCount == 0) {
        assetsFreeNames(models, modelCount);
        LOG_ERROR("No weapon models to extract");
        return false;
    }

    LOG_INFO("Extracting %d models from '%s'...", modelCount, fastFile);
    const int result = oatDumpXModelExport(fastFile, zoneDir, dir, (const char *const *)models, modelCount);
    assetsFreeNames(models, modelCount);

    if (result != 0) {
        LOG_ERROR("Model extraction failed for '%s'", fastFile);
        return false;
    }
    return true;
}

void assetsCleanup(void) {
    char dir[MAX_PATH];
    if (!assetsDir(dir, sizeof(dir))) return;

    char sub[MAX_PATH];
    snprintf(sub, sizeof(sub), "%s\\xmodel", dir);
    fileDelete(sub);
    snprintf(sub, sizeof(sub), "%s\\zone_source", dir);
    fileDelete(sub);
}
