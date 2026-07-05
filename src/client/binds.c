#include "client/binds.h"
#include "client/client_internal.h"

#include <stdlib.h>
#include <stdio.h>

ClientResult clientGetBinds(Client *client, BindsConfig *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/binds", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    JsonValue *arr = jsonObjectGet(body, "binds");
    int count = jsonArrayCount(arr);
    if (count > MAX_BINDS) count = MAX_BINDS;
    out->bindCount = count;
    for (int i = 0; i < count; i++) {
        JsonValue *bind = jsonArrayAt(arr, i);
        snprintf(out->binds[i].keyName, sizeof(out->binds[i].keyName), "%s",
                 jsonObjectGetString(bind, "key", ""));
        snprintf(out->binds[i].command, sizeof(out->binds[i].command), "%s",
                 jsonObjectGetString(bind, "command", ""));
    }
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientSetBinds(Client *client, const BindsConfig *config) {
    if (!config) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *obj = jsonNewObject();
    JsonValue *arr = jsonNewArray();
    for (int i = 0; i < config->bindCount && i < MAX_BINDS; i++) {
        JsonValue *bind = jsonNewObject();
        jsonObjectSetString(bind, "key", config->binds[i].keyName);
        jsonObjectSetString(bind, "command", config->binds[i].command);
        jsonArrayAppend(arr, bind);
    }
    jsonObjectSet(obj, "binds", arr);

    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;
    ClientResult r = clientRequest(client, "PUT", CLIENT_API_BASE "/binds", reqBody, NULL);
    free(reqBody);
    return r;
}

ClientResult clientResetBinds(Client *client) {
    return clientRequest(client, "POST", CLIENT_API_BASE "/binds/reset", NULL, NULL);
}
