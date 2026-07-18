#include "logic/camo/manager/manifest.h"
#include "logic/camo/manager/manager_internal.h"
#include "logic/camo/manager/persistence.h"
#include "utils/json.h"
#include "win/resources.h"
#include "resource_ids.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

static bool isValidAssetName(const char *name) {
    if (!name || name[0] == '\0') return false;
    for (const char *p = name; *p; ++p) {
        if (*p == '/' || *p == '\\' || *p == ':') return false;
    }
    size_t len = strlen(name);
    if (len < 4) return false;
    return strcmp(name + (len - 4), ".iwi") == 0;
}

void camoManifestFreeWeapons(CamoWeapon *weapons, size_t weaponCount) {
    if (!weapons) return;
    for (size_t i = 0; i < weaponCount; ++i) {
        free(weapons[i].id);
        free(weapons[i].name);
        for (size_t j = 0; j < weapons[i].fileCount; ++j) {
            free(weapons[i].files[j].fileName);
        }
        free(weapons[i].files);
    }
    free(weapons);
}

static bool weaponIdSeen(const CamoWeapon *weapons, size_t count, const char *id) {
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(weapons[i].id, id) == 0) return true;
    }
    return false;
}

static bool parseWeaponFiles(const JsonValue *filesObj, CamoFile **outFiles,
                             size_t *outCount) {
    *outFiles = NULL;
    *outCount = 0;

    if (!filesObj) return true;
    if (jsonTypeOf(filesObj) != JSON_OBJECT) {
        LOG_ERROR("Weapon 'files' is not an object");
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
            LOG_ERROR("Invalid weapon file key '%s'", key ? key : "(null)");
            goto fail;
        }
        for (size_t j = 0; j < n; ++j) {
            if (files[j].type == type && files[j].number == number) {
                LOG_ERROR("Duplicate weapon file key '%s'", key);
                goto fail;
            }
        }
        const char *name = jsonGetString(val, NULL);
        if (!isValidAssetName(name)) {
            LOG_ERROR("Invalid weapon asset name for key '%s'", key);
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

static bool parseWeapons(const JsonValue *root, CamoWeapon **outWeapons,
                         size_t *outCount) {
    *outWeapons = NULL;
    *outCount = 0;

    JsonValue *arr = jsonObjectGet(root, "weapons");
    if (!arr || jsonTypeOf(arr) != JSON_ARRAY) {
        LOG_ERROR("Weapon manifest 'weapons' is missing or not an array");
        return false;
    }

    int count = jsonArrayCount(arr);
    if (count <= 0) {
        LOG_ERROR("Weapon manifest contains no weapons");
        return false;
    }

    CamoWeapon *weapons = (CamoWeapon *)calloc((size_t)count, sizeof(CamoWeapon));
    if (!weapons) return false;

    size_t n = 0;
    for (int i = 0; i < count; ++i) {
        JsonValue *entry = jsonArrayAt(arr, i);
        if (jsonTypeOf(entry) != JSON_OBJECT) {
            LOG_ERROR("Weapon entry %d is not an object", i);
            goto fail;
        }

        const char *id = jsonObjectGetString(entry, "id", NULL);
        const char *name = jsonObjectGetString(entry, "name", NULL);
        if (!camoPersistenceValidId(id)) {
            LOG_ERROR("Weapon entry %d has an invalid id", i);
            goto fail;
        }
        if (weaponIdSeen(weapons, n, id)) {
            LOG_ERROR("Duplicate weapon id '%s'", id);
            goto fail;
        }
        if (!name || name[0] == '\0') {
            LOG_ERROR("Weapon '%s' has an empty name", id);
            goto fail;
        }

        CamoFile *files = NULL;
        size_t fileCount = 0;
        if (!parseWeaponFiles(jsonObjectGet(entry, "files"), &files, &fileCount)) {
            goto fail;
        }

        CamoWeapon *weapon = &weapons[n++];
        weapon->id = _strdup(id);
        weapon->name = _strdup(name);
        weapon->files = files;
        weapon->fileCount = fileCount;
        if (!weapon->id || !weapon->name) goto fail;
    }

    *outWeapons = weapons;
    *outCount = n;
    return true;

fail:
    camoManifestFreeWeapons(weapons, n);
    return false;
}

bool camoManifestLoad(CamoManager *manager) {
    if (!manager) return false;
    if (manager->weapons != NULL || manager->weaponCount != 0) {
        LOG_ERROR("camoManifestLoad called with a non-empty weapon list");
        return false;
    }

    void *data = NULL;
    DWORD size = 0;
    if (!resourcesGetData(IDR_CAMO_WEAPONS, &data, &size)) {
        LOG_ERROR("Failed to load embedded weapon manifest resource");
        return false;
    }

    char *text = (char *)malloc((size_t)size + 1);
    if (!text) return false;
    memcpy(text, data, (size_t)size);
    text[size] = '\0';

    JsonValue *root = jsonParse(text);
    free(text);
    if (!root) {
        LOG_ERROR("Weapon manifest is corrupt");
        return false;
    }

    bool ok = false;
    CamoWeapon *weapons = NULL;
    size_t weaponCount = 0;
    int version;

    if (jsonTypeOf(root) != JSON_OBJECT) {
        LOG_ERROR("Weapon manifest root is not an object");
        goto done;
    }

    version = jsonObjectGetInt(root, "version", -1);
    if (version != CAMO_MANIFEST_VERSION) {
        LOG_ERROR("Unsupported weapon manifest version %d", version);
        goto done;
    }

    if (!parseWeapons(root, &weapons, &weaponCount)) goto done;

    manager->weapons = weapons;
    manager->weaponCount = weaponCount;
    weapons = NULL;
    ok = true;

done:
    camoManifestFreeWeapons(weapons, weaponCount);
    jsonFree(root);
    return ok;
}
