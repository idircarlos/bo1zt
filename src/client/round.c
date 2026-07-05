#include "client/round.h"
#include "client/client_internal.h"

#include <stdio.h>

ClientResult clientGetRound(Client *client, int *round) {
    if (!round) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/round", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    *round = jsonObjectGetInt(body, "number", 0);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientSetRound(Client *client, int round) {
    char reqBody[32];
    snprintf(reqBody, sizeof(reqBody), "{\"round\":%d}", round);
    return clientRequest(client, "PUT", CLIENT_API_BASE "/round", reqBody, NULL);
}

ClientResult clientGetSpecialRound(Client *client, SpecialRound *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/round/special", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    out->type[0] = '\0';
    out->count = -1;
    out->next[0] = '\0';

    // The count lives under whichever type key the server emitted (mutually
    // exclusive); find it and record which one it was.
    static const char *TYPES[] = { "dogs", "monkeys", "thief" };
    for (int i = 0; i < 3; i++) {
        if (jsonObjectGet(body, TYPES[i])) {
            snprintf(out->type, sizeof(out->type), "%s", TYPES[i]);
            out->count = jsonObjectGetInt(body, TYPES[i], -1);
            break;
        }
    }
    snprintf(out->next, sizeof(out->next), "%s", jsonObjectGetString(body, "next", ""));
    jsonFree(body);
    return CLIENT_OK;
}
