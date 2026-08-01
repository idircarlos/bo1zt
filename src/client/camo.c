#include "client/camo.h"
#include "client/client_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CLIENT_CAMO_PATH_SIZE 192

static const char *CAMO_FILE_TYPE_NAMES[CAMO_FILE_TYPE_COUNT] = {
    "spec", "color", "env", "normal",
};

const char *clientCamoFileTypeName(CamoFileType type) {
    if ((int)type < 0 || (int)type >= CAMO_FILE_TYPE_COUNT) return NULL;
    return CAMO_FILE_TYPE_NAMES[type];
}

static bool fileTypeFromName(const char *name, CamoFileType *typeOut) {
    if (!name) return false;
    for (int i = 0; i < CAMO_FILE_TYPE_COUNT; i++) {
        if (strcmp(CAMO_FILE_TYPE_NAMES[i], name) == 0) {
            *typeOut = (CamoFileType)i;
            return true;
        }
    }
    return false;
}

static void copyField(char *out, size_t size, const char *value) {
    snprintf(out, size, "%s", value ? value : "");
}

static bool parseFiles(const JsonValue *arr, ClientCamoFile **filesOut, size_t *countOut) {
    *filesOut = NULL;
    *countOut = 0;
    if (jsonTypeOf(arr) != JSON_ARRAY) return false;

    int count = jsonArrayCount(arr);
    if (count <= 0) return true;

    ClientCamoFile *files = (ClientCamoFile *)calloc((size_t)count, sizeof(ClientCamoFile));
    if (!files) return false;
    for (int i = 0; i < count; i++) {
        const JsonValue *item = jsonArrayAt(arr, i);
        if (!fileTypeFromName(jsonObjectGetString(item, "type", NULL), &files[i].type)) {
            free(files);
            return false;
        }
        files[i].number = (unsigned int)jsonObjectGetInt(item, "number", 0);
    }
    *filesOut = files;
    *countOut = (size_t)count;
    return true;
}

static bool parseCamo(const JsonValue *obj, ClientCamo *out) {
    if (jsonTypeOf(obj) != JSON_OBJECT) return false;
    copyField(out->id, sizeof(out->id), jsonObjectGetString(obj, "id", NULL));
    copyField(out->name, sizeof(out->name), jsonObjectGetString(obj, "name", NULL));
    return parseFiles(jsonObjectGet(obj, "files"), &out->files, &out->fileCount);
}

static bool parseWeapon(const JsonValue *obj, ClientCamoWeapon *out) {
    if (jsonTypeOf(obj) != JSON_OBJECT) return false;
    copyField(out->id, sizeof(out->id), jsonObjectGetString(obj, "id", NULL));
    copyField(out->name, sizeof(out->name), jsonObjectGetString(obj, "name", NULL));
    copyField(out->model, sizeof(out->model), jsonObjectGetString(obj, "model", NULL));
    return parseFiles(jsonObjectGet(obj, "files"), &out->files, &out->fileCount);
}

static bool parseBundle(const JsonValue *obj, ClientCamoBundle *out) {
    if (jsonTypeOf(obj) != JSON_OBJECT) return false;
    copyField(out->id, sizeof(out->id), jsonObjectGetString(obj, "id", NULL));
    copyField(out->name, sizeof(out->name), jsonObjectGetString(obj, "name", NULL));
    out->installed = jsonObjectGetBool(obj, "installed", false);
    out->entries = NULL;
    out->entryCount = 0;

    const JsonValue *arr = jsonObjectGet(obj, "entries");
    if (jsonTypeOf(arr) != JSON_ARRAY) return false;
    int count = jsonArrayCount(arr);
    if (count <= 0) return true;

    ClientCamoEntry *entries = (ClientCamoEntry *)calloc((size_t)count, sizeof(ClientCamoEntry));
    if (!entries) return false;
    for (int i = 0; i < count; i++) {
        const JsonValue *item = jsonArrayAt(arr, i);
        copyField(entries[i].weaponId, sizeof(entries[i].weaponId),
                  jsonObjectGetString(item, "weapon-id", NULL));
        copyField(entries[i].camoId, sizeof(entries[i].camoId),
                  jsonObjectGetString(item, "camo-id", NULL));
    }
    out->entries = entries;
    out->entryCount = (size_t)count;
    return true;
}

void clientFreeCamo(ClientCamo *camo) {
    if (!camo) return;
    free(camo->files);
    camo->files = NULL;
    camo->fileCount = 0;
}

void clientFreeCamos(ClientCamo *camos, size_t count) {
    if (!camos) return;
    for (size_t i = 0; i < count; i++) clientFreeCamo(&camos[i]);
    free(camos);
}

void clientFreeCamoWeapon(ClientCamoWeapon *weapon) {
    if (!weapon) return;
    free(weapon->files);
    weapon->files = NULL;
    weapon->fileCount = 0;
}

void clientFreeCamoWeaponFiles(ClientCamoWeaponFile *files) {
    free(files);
}

void clientFreeCamoWeapons(ClientCamoWeapon *weapons, size_t count) {
    if (!weapons) return;
    for (size_t i = 0; i < count; i++) clientFreeCamoWeapon(&weapons[i]);
    free(weapons);
}

void clientFreeCamoBundle(ClientCamoBundle *bundle) {
    if (!bundle) return;
    free(bundle->entries);
    bundle->entries = NULL;
    bundle->entryCount = 0;
}

void clientFreeCamoBundles(ClientCamoBundle *bundles, size_t count) {
    if (!bundles) return;
    for (size_t i = 0; i < count; i++) clientFreeCamoBundle(&bundles[i]);
    free(bundles);
}

static ClientResult requestArray(Client *client, const char *path, JsonValue **bodyOut) {
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", path, NULL, &body);
    if (r != CLIENT_OK) return r;
    if (jsonTypeOf(body) != JSON_ARRAY) {
        jsonFree(body);
        return CLIENT_ERR_PROTOCOL;
    }
    *bodyOut = body;
    return CLIENT_OK;
}

static ClientResult requestCamo(Client *client, const char *method, const char *path,
                                const char *reqBody, ClientCamo *out) {
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, method, path, reqBody, out ? &body : NULL);
    if (r != CLIENT_OK) return r;
    if (!out) return CLIENT_OK;
    bool parsed = parseCamo(body, out);
    jsonFree(body);
    return parsed ? CLIENT_OK : CLIENT_ERR_PROTOCOL;
}

static ClientResult requestBundle(Client *client, const char *method, const char *path,
                                  const char *reqBody, ClientCamoBundle *out) {
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, method, path, reqBody, out ? &body : NULL);
    if (r != CLIENT_OK) return r;
    if (!out) return CLIENT_OK;
    bool parsed = parseBundle(body, out);
    jsonFree(body);
    return parsed ? CLIENT_OK : CLIENT_ERR_PROTOCOL;
}

static JsonValue *fileSourcesJson(const ClientCamoFileSource *files, size_t fileCount) {
    JsonValue *arr = jsonNewArray();
    for (size_t i = 0; i < fileCount; i++) {
        JsonValue *obj = jsonNewObject();
        jsonObjectSetString(obj, "type", clientCamoFileTypeName(files[i].type));
        jsonObjectSetInt(obj, "number", files[i].number);
        jsonObjectSetString(obj, "source", files[i].source);
        jsonArrayAppend(arr, obj);
    }
    return arr;
}

ClientResult clientGetCamoOverview(Client *client, ClientCamoOverview *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/camo-manager", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (jsonTypeOf(body) != JSON_OBJECT) {
        jsonFree(body);
        return CLIENT_ERR_PROTOCOL;
    }

    out->camoCount = (size_t)jsonObjectGetInt(body, "camo-count", 0);
    out->bundleCount = (size_t)jsonObjectGetInt(body, "bundle-count", 0);
    out->weaponCount = (size_t)jsonObjectGetInt(body, "weapon-count", 0);
    copyField(out->activeBundleId, sizeof(out->activeBundleId),
              jsonObjectGetString(body, "active-bundle-id", NULL));
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientGetCamoWeapons(Client *client, ClientCamoWeapon **outWeapons, size_t *outCount) {
    if (!outWeapons || !outCount) return CLIENT_ERR_INVALID_PARAM;
    *outWeapons = NULL;
    *outCount = 0;

    JsonValue *body = NULL;
    ClientResult r = requestArray(client, CLIENT_API_BASE "/camo-manager/weapons", &body);
    if (r != CLIENT_OK) return r;

    int count = jsonArrayCount(body);
    ClientCamoWeapon *weapons = NULL;
    if (count > 0) {
        weapons = (ClientCamoWeapon *)calloc((size_t)count, sizeof(ClientCamoWeapon));
        if (!weapons) {
            jsonFree(body);
            return CLIENT_ERR_PROTOCOL;
        }
        for (int i = 0; i < count; i++) {
            if (parseWeapon(jsonArrayAt(body, i), &weapons[i])) continue;
            clientFreeCamoWeapons(weapons, (size_t)i);
            jsonFree(body);
            return CLIENT_ERR_PROTOCOL;
        }
    }
    jsonFree(body);
    *outWeapons = weapons;
    *outCount = (size_t)count;
    return CLIENT_OK;
}

ClientResult clientGetCamoWeapon(Client *client, const char *weaponId, ClientCamoWeapon *out) {
    if (!weaponId || !out) return CLIENT_ERR_INVALID_PARAM;
    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/weapons/%s", weaponId);

    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", path, NULL, &body);
    if (r != CLIENT_OK) return r;
    bool parsed = parseWeapon(body, out);
    jsonFree(body);
    return parsed ? CLIENT_OK : CLIENT_ERR_PROTOCOL;
}

ClientResult clientGetCamoWeaponFiles(Client *client, const char *weaponId,
                                     ClientCamoWeaponFile **outFiles, size_t *outCount) {
    if (!weaponId || !outFiles || !outCount) return CLIENT_ERR_INVALID_PARAM;
    *outFiles = NULL;
    *outCount = 0;

    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/weapons/%s/files", weaponId);

    JsonValue *body = NULL;
    ClientResult r = requestArray(client, path, &body);
    if (r != CLIENT_OK) return r;

    int count = jsonArrayCount(body);
    ClientCamoWeaponFile *files = NULL;
    if (count > 0) {
        files = (ClientCamoWeaponFile *)calloc((size_t)count, sizeof(ClientCamoWeaponFile));
        if (!files) {
            jsonFree(body);
            return CLIENT_ERR_PROTOCOL;
        }
        for (int i = 0; i < count; i++) {
            const JsonValue *item = jsonArrayAt(body, i);
            if (!fileTypeFromName(jsonObjectGetString(item, "type", NULL), &files[i].type)) {
                free(files);
                jsonFree(body);
                return CLIENT_ERR_PROTOCOL;
            }
            files[i].number = (unsigned int)jsonObjectGetInt(item, "number", 0);
            copyField(files[i].fileName, sizeof(files[i].fileName),
                      jsonObjectGetString(item, "name", NULL));
        }
    }
    jsonFree(body);
    *outFiles = files;
    *outCount = (size_t)count;
    return CLIENT_OK;
}

ClientResult clientGetCamos(Client *client, ClientCamo **outCamos, size_t *outCount) {
    if (!outCamos || !outCount) return CLIENT_ERR_INVALID_PARAM;
    *outCamos = NULL;
    *outCount = 0;

    JsonValue *body = NULL;
    ClientResult r = requestArray(client, CLIENT_API_BASE "/camo-manager/camos", &body);
    if (r != CLIENT_OK) return r;

    int count = jsonArrayCount(body);
    ClientCamo *camos = NULL;
    if (count > 0) {
        camos = (ClientCamo *)calloc((size_t)count, sizeof(ClientCamo));
        if (!camos) {
            jsonFree(body);
            return CLIENT_ERR_PROTOCOL;
        }
        for (int i = 0; i < count; i++) {
            if (parseCamo(jsonArrayAt(body, i), &camos[i])) continue;
            clientFreeCamos(camos, (size_t)i);
            jsonFree(body);
            return CLIENT_ERR_PROTOCOL;
        }
    }
    jsonFree(body);
    *outCamos = camos;
    *outCount = (size_t)count;
    return CLIENT_OK;
}

ClientResult clientGetCamo(Client *client, const char *camoId, ClientCamo *out) {
    if (!camoId || !out) return CLIENT_ERR_INVALID_PARAM;
    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/camos/%s", camoId);
    return requestCamo(client, "GET", path, NULL, out);
}

ClientResult clientCreateCamo(Client *client, const char *name,
                              const ClientCamoFileSource *files, size_t fileCount,
                              ClientCamo *out) {
    if (!name || (fileCount > 0 && !files)) return CLIENT_ERR_INVALID_PARAM;

    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "name", name);
    if (fileCount > 0) jsonObjectSet(obj, "files", fileSourcesJson(files, fileCount));
    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;

    ClientResult r = requestCamo(client, "POST", CLIENT_API_BASE "/camo-manager/camos",
                                 reqBody, out);
    free(reqBody);
    return r;
}

ClientResult clientUpdateCamo(Client *client, const char *camoId,
                              const ClientCamoPatch *patch, ClientCamo *out) {
    if (!camoId || !patch) return CLIENT_ERR_INVALID_PARAM;
    if (patch->hasFiles && patch->fileCount > 0 && !patch->files) return CLIENT_ERR_INVALID_PARAM;

    JsonValue *obj = jsonNewObject();
    if (patch->hasName) jsonObjectSetString(obj, "name", patch->name);
    if (patch->hasFiles) {
        jsonObjectSet(obj, "files", fileSourcesJson(patch->files, patch->fileCount));
    }
    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;

    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/camos/%s", camoId);
    ClientResult r = requestCamo(client, "PATCH", path, reqBody, out);
    free(reqBody);
    return r;
}

ClientResult clientDeleteCamo(Client *client, const char *camoId, bool removeReferences) {
    if (!camoId) return CLIENT_ERR_INVALID_PARAM;
    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/camos/%s%s", camoId,
             removeReferences ? "?remove-references=true" : "");
    return clientRequest(client, "DELETE", path, NULL, NULL);
}

ClientResult clientGetCamoBundles(Client *client, ClientCamoBundle **outBundles, size_t *outCount) {
    if (!outBundles || !outCount) return CLIENT_ERR_INVALID_PARAM;
    *outBundles = NULL;
    *outCount = 0;

    JsonValue *body = NULL;
    ClientResult r = requestArray(client, CLIENT_API_BASE "/camo-manager/bundles", &body);
    if (r != CLIENT_OK) return r;

    int count = jsonArrayCount(body);
    ClientCamoBundle *bundles = NULL;
    if (count > 0) {
        bundles = (ClientCamoBundle *)calloc((size_t)count, sizeof(ClientCamoBundle));
        if (!bundles) {
            jsonFree(body);
            return CLIENT_ERR_PROTOCOL;
        }
        for (int i = 0; i < count; i++) {
            if (parseBundle(jsonArrayAt(body, i), &bundles[i])) continue;
            clientFreeCamoBundles(bundles, (size_t)i);
            jsonFree(body);
            return CLIENT_ERR_PROTOCOL;
        }
    }
    jsonFree(body);
    *outBundles = bundles;
    *outCount = (size_t)count;
    return CLIENT_OK;
}

ClientResult clientGetCamoBundle(Client *client, const char *bundleId, ClientCamoBundle *out) {
    if (!bundleId || !out) return CLIENT_ERR_INVALID_PARAM;
    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/bundles/%s", bundleId);
    return requestBundle(client, "GET", path, NULL, out);
}

ClientResult clientCreateCamoBundle(Client *client, const char *name, ClientCamoBundle *out) {
    if (!name) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "name", name);
    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;

    ClientResult r = requestBundle(client, "POST", CLIENT_API_BASE "/camo-manager/bundles",
                                   reqBody, out);
    free(reqBody);
    return r;
}

ClientResult clientRenameCamoBundle(Client *client, const char *bundleId, const char *name,
                                    ClientCamoBundle *out) {
    if (!bundleId || !name) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "name", name);
    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;

    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/bundles/%s", bundleId);
    ClientResult r = requestBundle(client, "PATCH", path, reqBody, out);
    free(reqBody);
    return r;
}

ClientResult clientDeleteCamoBundle(Client *client, const char *bundleId) {
    if (!bundleId) return CLIENT_ERR_INVALID_PARAM;
    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/bundles/%s", bundleId);
    return clientRequest(client, "DELETE", path, NULL, NULL);
}

ClientResult clientAssignCamo(Client *client, const char *bundleId, const char *weaponId,
                              const char *camoId) {
    if (!bundleId || !weaponId || !camoId) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "camo-id", camoId);
    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;

    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/bundles/%s/weapons/%s",
             bundleId, weaponId);
    ClientResult r = clientRequest(client, "PUT", path, reqBody, NULL);
    free(reqBody);
    return r;
}

ClientResult clientUnassignCamo(Client *client, const char *bundleId, const char *weaponId) {
    if (!bundleId || !weaponId) return CLIENT_ERR_INVALID_PARAM;
    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/bundles/%s/weapons/%s",
             bundleId, weaponId);
    return clientRequest(client, "DELETE", path, NULL, NULL);
}

ClientResult clientInstallCamoBundle(Client *client, const char *bundleId) {
    if (!bundleId) return CLIENT_ERR_INVALID_PARAM;
    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/bundles/%s/install", bundleId);
    return clientRequest(client, "POST", path, NULL, NULL);
}

ClientResult clientUninstallCamoBundle(Client *client, const char *bundleId) {
    if (!bundleId) return CLIENT_ERR_INVALID_PARAM;
    char path[CLIENT_CAMO_PATH_SIZE];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/camo-manager/bundles/%s/uninstall", bundleId);
    return clientRequest(client, "POST", path, NULL, NULL);
}
