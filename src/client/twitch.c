#include "client/twitch.h"
#include "client/client_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const struct {
    const char *name;
    ClientTwitchState state;
} STATE_TABLE[] = {
    { "disconnected",           CLIENT_TWITCH_DISCONNECTED },
    { "awaiting-authorization", CLIENT_TWITCH_AWAITING_AUTHORIZATION },
    { "connecting",             CLIENT_TWITCH_CONNECTING },
    { "connected",              CLIENT_TWITCH_CONNECTED },
};

static ClientTwitchState stateFromName(const char *name) {
    for (size_t i = 0; i < sizeof(STATE_TABLE) / sizeof(STATE_TABLE[0]); i++) {
        if (strcmp(name, STATE_TABLE[i].name) == 0) return STATE_TABLE[i].state;
    }
    return CLIENT_TWITCH_DISCONNECTED;
}

ClientResult clientGetTwitchConnection(Client *client, ClientTwitchConnection *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/twitch", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    memset(out, 0, sizeof(*out));
    out->state = stateFromName(jsonObjectGetString(body, "state", "disconnected"));
    out->authorized = jsonObjectGetBool(body, "authorized", false);
    snprintf(out->clientId, sizeof(out->clientId), "%s",
             jsonObjectGetString(body, "client-id", ""));
    snprintf(out->login, sizeof(out->login), "%s", jsonObjectGetString(body, "login", ""));
    snprintf(out->displayName, sizeof(out->displayName), "%s",
             jsonObjectGetString(body, "display-name", ""));
    snprintf(out->userCode, sizeof(out->userCode), "%s",
             jsonObjectGetString(body, "user-code", ""));
    snprintf(out->verificationUri, sizeof(out->verificationUri), "%s",
             jsonObjectGetString(body, "verification-uri", ""));
    snprintf(out->error, sizeof(out->error), "%s", jsonObjectGetString(body, "error", ""));
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientTwitchConnect(Client *client, const char *clientId) {
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "client-id", clientId ? clientId : "");
    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;

    ClientResult r = clientRequest(client, "POST", CLIENT_API_BASE "/twitch/connect", reqBody, NULL);
    free(reqBody);
    return r;
}

ClientResult clientTwitchDisconnect(Client *client) {
    return clientRequest(client, "POST", CLIENT_API_BASE "/twitch/disconnect", NULL, NULL);
}

ClientResult clientGetTwitchOptions(Client *client, ClientTwitchOptions *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/twitch/options", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    out->showChat = jsonObjectGetBool(body, "show-chat", false);
    out->sendChat = jsonObjectGetBool(body, "send-chat", false);
    out->announceRaids = jsonObjectGetBool(body, "announce-raids", false);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientTwitchSetOption(Client *client, const char *option, bool enabled) {
    if (!option) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *obj = jsonNewObject();
    jsonObjectSetBool(obj, option, enabled);
    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;

    ClientResult r = clientRequest(client, "PATCH", CLIENT_API_BASE "/twitch/options", reqBody, NULL);
    free(reqBody);
    return r;
}
