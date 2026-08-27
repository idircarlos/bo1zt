#include "client/gsc.h"
#include "client/client_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void parseFiles(const JsonValue *item, ClientGscMod *mod) {
    JsonValue *arr = jsonObjectGet(item, "files");
    if (jsonTypeOf(arr) != JSON_ARRAY) return;

    int count = jsonArrayCount(arr);
    if (count <= 0) return;

    ClientGscFile *files = (ClientGscFile *)calloc((size_t)count, sizeof(ClientGscFile));
    if (!files) return;

    for (int i = 0; i < count; i++) {
        snprintf(files[i].path, sizeof(files[i].path), "%s", jsonGetString(jsonArrayAt(arr, i), ""));
    }
    mod->files = files;
    mod->fileCount = (size_t)count;
}

ClientResult clientGetGscMods(Client *client, ClientGscMods *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    out->folder[0] = '\0';
    out->mods = NULL;
    out->modCount = 0;

    JsonValue *body = NULL;
    ClientResult result = clientRequest(client, "GET", CLIENT_API_BASE "/gsc-mods", NULL, &body);
    if (result != CLIENT_OK) return result;
    if (!body) return CLIENT_ERR_PROTOCOL;

    snprintf(out->folder, sizeof(out->folder), "%s", jsonObjectGetString(body, "folder", ""));

    JsonValue *arr = jsonObjectGet(body, "mods");
    if (jsonTypeOf(arr) != JSON_ARRAY) {
        jsonFree(body);
        return CLIENT_ERR_PROTOCOL;
    }

    int count = jsonArrayCount(arr);
    if (count > 0) {
        ClientGscMod *mods = (ClientGscMod *)calloc((size_t)count, sizeof(ClientGscMod));
        if (!mods) {
            jsonFree(body);
            return CLIENT_ERR_PROTOCOL;
        }
        for (int i = 0; i < count; i++) {
            const JsonValue *item = jsonArrayAt(arr, i);
            snprintf(mods[i].name, sizeof(mods[i].name), "%s",
                     jsonObjectGetString(item, "name", ""));
            parseFiles(item, &mods[i]);
        }
        out->mods = mods;
        out->modCount = (size_t)count;
    }

    jsonFree(body);
    return CLIENT_OK;
}

void clientFreeGscMods(ClientGscMods *mods) {
    if (!mods) return;
    for (size_t i = 0; i < mods->modCount; i++) free(mods->mods[i].files);
    free(mods->mods);
    mods->mods = NULL;
    mods->modCount = 0;
}

static ClientResult sendGscRequest(Client *client, const char *method, const char *key,
                                   const char *value) {
    if (!value || !value[0]) return CLIENT_ERR_INVALID_PARAM;

    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, key, value);
    char *body = jsonSerialize(obj);
    jsonFree(obj);
    if (!body) return CLIENT_ERR_PROTOCOL;

    ClientResult result = clientRequest(client, method, CLIENT_API_BASE "/gsc-mods", body, NULL);
    free(body);
    return result;
}

ClientResult clientCreateGscMod(Client *client, const char *name) {
    return sendGscRequest(client, "POST", "name", name);
}

ClientResult clientDeleteGscPath(Client *client, const char *path) {
    return sendGscRequest(client, "DELETE", "path", path);
}

ClientResult clientReadGscScript(Client *client, const char *path, char **out) {
    if (!path || !path[0] || !out) return CLIENT_ERR_INVALID_PARAM;
    *out = NULL;

    char url[CLIENT_GSC_PATH_SIZE + 64];
    int n = snprintf(url, sizeof(url), CLIENT_API_BASE "/gsc-mods/script?path=%s", path);
    if (n < 0 || (size_t)n >= sizeof(url)) return CLIENT_ERR_INVALID_PARAM;

    JsonValue *body = NULL;
    ClientResult result = clientRequest(client, "GET", url, NULL, &body);
    if (result != CLIENT_OK) return result;
    if (!body) return CLIENT_ERR_PROTOCOL;

    JsonValue *content = jsonObjectGet(body, "content");
    if (jsonTypeOf(content) != JSON_STRING) {
        jsonFree(body);
        return CLIENT_ERR_PROTOCOL;
    }

    *out = _strdup(jsonGetString(content, ""));
    jsonFree(body);
    return *out ? CLIENT_OK : CLIENT_ERR_PROTOCOL;
}

ClientResult clientWriteGscScript(Client *client, const char *path, const char *content) {
    if (!path || !path[0] || !content) return CLIENT_ERR_INVALID_PARAM;

    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "path", path);
    jsonObjectSetString(obj, "content", content);
    char *body = jsonSerialize(obj);
    jsonFree(obj);
    if (!body) return CLIENT_ERR_PROTOCOL;

    ClientResult result = clientRequest(client, "PUT", CLIENT_API_BASE "/gsc-mods/script",
                                        body, NULL);
    free(body);
    return result;
}

ClientResult clientCreateGscScript(Client *client, const char *path) {
    if (!path || !path[0]) return CLIENT_ERR_INVALID_PARAM;

    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "path", path);
    char *body = jsonSerialize(obj);
    jsonFree(obj);
    if (!body) return CLIENT_ERR_PROTOCOL;

    ClientResult result = clientRequest(client, "POST", CLIENT_API_BASE "/gsc-mods/script",
                                        body, NULL);
    free(body);
    return result;
}
