#include "client/commands.h"
#include "client/client_internal.h"

#include <stdio.h>

ClientResult clientGetCommands(Client *client, CommandInfo *out, int max, int *countOut) {
    if (!out || max <= 0) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/commands", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    int n = jsonArrayCount(body);
    if (n > max) n = max;
    for (int i = 0; i < n; i++) {
        JsonValue *entry = jsonArrayAt(body, i);
        snprintf(out[i].name, sizeof(out[i].name), "%s", jsonObjectGetString(entry, "name", ""));
        snprintf(out[i].usage, sizeof(out[i].usage), "%s", jsonObjectGetString(entry, "usage", ""));
        snprintf(out[i].description, sizeof(out[i].description), "%s",
                 jsonObjectGetString(entry, "description", ""));
    }
    jsonFree(body);
    if (countOut) *countOut = n;
    return CLIENT_OK;
}
