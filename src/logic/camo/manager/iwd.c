#include "logic/camo/manager/iwd.h"
#include "logic/camo/manager/manager_internal.h"
#include "logic/camo/manager/persistence.h"
#include "win/file.h"
#include "win/resources.h"
#include "resource_ids.h"
#include "logger.h"

#include <windows.h>
#include "miniz.h"

#include <stdlib.h>
#include <string.h>

#define CAMO_IWD_IMAGES_DIR "images/"
#define CAMO_IWD_BLACK_NAME CAMO_IWD_IMAGES_DIR "$black.iwi"
#define CAMO_IWD_LEVEL MZ_DEFAULT_LEVEL

typedef struct {
    const char *assetName;
    char sourcePath[MAX_PATH];
} IwdSelection;

static const CamoWeapon *findWeapon(const CamoManager *manager, const char *id) {
    for (size_t i = 0; i < manager->weaponCount; ++i) {
        if (strcmp(manager->weapons[i].id, id) == 0) return &manager->weapons[i];
    }
    return NULL;
}

static const Camo *findCamo(const CamoManager *manager, const char *id) {
    for (size_t i = 0; i < manager->camoCount; ++i) {
        if (strcmp(manager->camos[i].id, id) == 0) return &manager->camos[i];
    }
    return NULL;
}

static const CamoFile *camoFileFor(const Camo *camo, CamoFileType type,
                                   unsigned int number) {
    for (size_t i = 0; i < camo->fileCount; ++i) {
        if (camo->files[i].type == type && camo->files[i].number == number) {
            return &camo->files[i];
        }
    }
    return NULL;
}

static size_t maxSelections(const CamoManager *manager, const CamoBundle *bundle) {
    size_t total = 0;
    for (size_t i = 0; i < bundle->entryCount; ++i) {
        const CamoWeapon *weapon = findWeapon(manager, bundle->entries[i].weaponId);
        if (weapon) total += weapon->fileCount;
    }
    return total;
}

static void safeStem(const char *name, char *out, size_t bufSize) {
    if (bufSize == 0) return;

    size_t n = 0;
    bool lastUnderscore = false;
    for (const char *p = name ? name : ""; *p && n + 1 < bufSize; ++p) {
        unsigned char c = (unsigned char)*p;
        bool alnum = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
                     (c >= 'A' && c <= 'Z');
        if (alnum) {
            if (c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
            out[n++] = (char)c;
            lastUnderscore = false;
        } else if (!lastUnderscore) {
            out[n++] = '_';
            lastUnderscore = true;
        }
    }
    if (n > 0 && out[n - 1] == '_') --n;
    out[n] = '\0';

    if (out[0] == '_') memmove(out, out + 1, strlen(out));

    if (out[0] == '\0') snprintf(out, bufSize, "%s", "bundle");
}

bool camoIwdFileName(char *out, size_t bufSize, const CamoBundle *bundle) {
    if (!out || bufSize == 0 || !bundle) return false;
    if (!bundle->id || bundle->id[0] == '\0') {
        LOG_ERROR("Bundle has no id");
        return false;
    }

    char stem[64];
    safeStem(bundle->name, stem, sizeof(stem));

    char id8[9];
    size_t idLen = strlen(bundle->id);
    size_t take = idLen < 8 ? idLen : 8;
    memcpy(id8, bundle->id, take);
    id8[take] = '\0';

    int n = snprintf(out, bufSize, "iw_bo1zt_%s_%s.iwd", stem, id8);
    if (n < 0 || (size_t)n >= bufSize) {
        LOG_ERROR("IWD filename too long");
        return false;
    }
    return true;
}

static bool gatherSelections(const CamoManager *manager, const CamoBundle *bundle,
                             IwdSelection *sel, size_t *outCount) {
    size_t count = 0;

    for (size_t i = 0; i < bundle->entryCount; ++i) {
        const CamoBundleEntry *entry = &bundle->entries[i];

        const CamoWeapon *weapon = findWeapon(manager, entry->weaponId);
        if (!weapon) {
            LOG_ERROR("Bundle references unknown weapon '%s'", entry->weaponId);
            return false;
        }
        const Camo *camo = findCamo(manager, entry->camoId);
        if (!camo) {
            LOG_ERROR("Bundle references unknown camo '%s'", entry->camoId);
            return false;
        }

        for (size_t f = 0; f < weapon->fileCount; ++f) {
            const CamoFile *target = &weapon->files[f];
            if (!camoFileFor(camo, target->type, target->number)) continue;

            if (!target->fileName || target->fileName[0] == '\0') {
                LOG_ERROR("Weapon '%s' target has no asset name", weapon->id);
                return false;
            }

            for (size_t j = 0; j < count; ++j) {
                if (strcmp(sel[j].assetName, target->fileName) == 0) {
                    LOG_ERROR("Duplicate target asset '%s' in bundle '%s'",
                              target->fileName, bundle->id);
                    return false;
                }
            }

            sel[count].assetName = target->fileName;
            if (!camoPersistenceCamoFilePath(sel[count].sourcePath,
                                             sizeof(sel[count].sourcePath),
                                             camo->id, target->type,
                                             target->number)) {
                return false;
            }
            ++count;
        }
    }

    *outCount = count;
    return true;
}

bool camoIwdBuild(const CamoManager *manager, const CamoBundle *bundle,
                  const char *outPath) {
    if (!manager || !bundle || !outPath || outPath[0] == '\0') {
        LOG_ERROR("camoIwdBuild called with invalid arguments");
        return false;
    }

    void *blackData = NULL;
    uint32_t blackSize = 0;
    if (!resourcesGetData(IDR_CAMO_BLACK_IWI, &blackData, &blackSize)) {
        LOG_ERROR("Failed to load embedded $black.iwi resource");
        return false;
    }

    size_t capacity = maxSelections(manager, bundle);
    IwdSelection *sel = NULL;
    if (capacity > 0) {
        sel = (IwdSelection *)calloc(capacity, sizeof(IwdSelection));
        if (!sel) {
            LOG_ERROR("Out of memory selecting IWD textures");
            return false;
        }
    }

    size_t count = 0;
    if (!gatherSelections(manager, bundle, sel, &count)) {
        free(sel);
        return false;
    }

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_writer_init_file(&zip, outPath, 0)) {
        LOG_ERROR("Failed to create IWD archive '%s'", outPath);
        free(sel);
        return false;
    }

    bool ok = true;

    if (!mz_zip_writer_add_mem(&zip, CAMO_IWD_BLACK_NAME, blackData,
                               (size_t)blackSize, CAMO_IWD_LEVEL)) {
        LOG_ERROR("Failed to add $black.iwi to IWD '%s'", outPath);
        ok = false;
    }

    for (size_t i = 0; ok && i < count; ++i) {
        char archiveName[MAX_PATH];
        int n = snprintf(archiveName, sizeof(archiveName), "%s%s",
                         CAMO_IWD_IMAGES_DIR, sel[i].assetName);
        if (n < 0 || (size_t)n >= sizeof(archiveName)) {
            LOG_ERROR("IWD archive member name too long for '%s'", sel[i].assetName);
            ok = false;
            break;
        }
        if (!mz_zip_writer_add_file(&zip, archiveName, sel[i].sourcePath, NULL, 0,
                                    CAMO_IWD_LEVEL)) {
            LOG_ERROR("Failed to add '%s' (from '%s') to IWD '%s'", archiveName,
                      sel[i].sourcePath, outPath);
            ok = false;
            break;
        }
    }

    if (ok && !mz_zip_writer_finalize_archive(&zip)) {
        LOG_ERROR("Failed to finalize IWD archive '%s'", outPath);
        ok = false;
    }

    if (!mz_zip_writer_end(&zip)) {
        LOG_ERROR("Failed to close IWD archive '%s'", outPath);
        ok = false;
    }

    free(sel);

    if (!ok) {
        fileDelete(outPath);
    }
    return ok;
}
