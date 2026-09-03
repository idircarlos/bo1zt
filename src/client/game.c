#include "client/game.h"
#include "client/client_internal.h"

#include <stdio.h>
#include <stdlib.h>

ClientResult clientGetGameStatus(Client *client, GameStatus *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/game", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    out->attached = jsonObjectGetBool(body, "attached", false);
    out->running = jsonObjectGetBool(body, "running", false);
    out->ready = jsonObjectGetBool(body, "ready", false);
    out->windowFocused = jsonObjectGetBool(body, "window-focused", false);
    out->dllInjected = jsonObjectGetBool(body, "dll-injected", false);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientLaunchGame(Client *client) {
    return clientRequest(client, "POST", CLIENT_API_BASE "/game/launch", NULL, NULL);
}

ClientResult clientCloseGame(Client *client) {
    return clientRequest(client, "POST", CLIENT_API_BASE "/game/close", NULL, NULL);
}

ClientResult clientRestartGame(Client *client) {
    return clientRequest(client, "POST", CLIENT_API_BASE "/game/restart", NULL, NULL);
}

ClientResult clientGetGameConfig(Client *client, GameConfigInfo *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/game/config", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;
    snprintf(out->location, sizeof(out->location), "%s", jsonObjectGetString(body, "location", ""));
    snprintf(out->hostname, sizeof(out->hostname), "%s", jsonObjectGetString(body, "hostname", ""));
    snprintf(out->character, sizeof(out->character), "%s", jsonObjectGetString(body, "character", ""));
    jsonFree(body);
    return CLIENT_OK;
}

static ClientResult patchGameConfig(Client *client, const char *key, const char *value) {
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, key, value);
    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;
    ClientResult r = clientRequest(client, "PATCH", CLIENT_API_BASE "/game/config", reqBody, NULL);
    free(reqBody);
    return r;
}

ClientResult clientSetGameLocation(Client *client, const char *location) {
    if (!location) return CLIENT_ERR_INVALID_PARAM;
    return patchGameConfig(client, "location", location);
}

ClientResult clientSetGameHostname(Client *client, const char *hostname) {
    if (!hostname) return CLIENT_ERR_INVALID_PARAM;
    return patchGameConfig(client, "hostname", hostname);
}

ClientResult clientSetGameCharacter(Client *client, const char *character) {
    if (!character) return CLIENT_ERR_INVALID_PARAM;
    return patchGameConfig(client, "character", character);
}
