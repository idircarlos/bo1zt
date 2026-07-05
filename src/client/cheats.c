#include "client/cheats.h"
#include "client/client_internal.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Only the cheats exposed over the API are listed; a CheatName not in this table
// (e.g. CHEAT_NAME_PATCH_CHAT) is rejected locally as NOT_FOUND without a round-trip.
typedef struct {
    const char *name;
    CheatName cheat;
} CheatEntry;

static const CheatEntry CHEAT_TABLE[] = {
    { "god",                  CHEAT_NAME_GOD_MODE },
    { "noclip",               CHEAT_NAME_NO_CLIP },
    { "invisible",            CHEAT_NAME_INVISIBLE },
    { "infinite-ammo",        CHEAT_NAME_INFINITE_AMMO },
    { "instant-kill",         CHEAT_NAME_INSTANT_KILL },
    { "no-recoil",            CHEAT_NAME_NO_RECOIL },
    { "small-crosshair",      CHEAT_NAME_SMALL_CROSSHAIR },
    { "fast-gameplay",        CHEAT_NAME_FAST_GAMEPLAY },
    { "no-shellshock",        CHEAT_NAME_NO_SHELLSHOCK },
    { "increase-knife-range", CHEAT_NAME_INCREASE_KNIFE_RANGE },
    { "static-box",           CHEAT_NAME_BOX_NEVER_MOVES },
    { "third-person",         CHEAT_NAME_THIRD_PERSON },
    { "borderless",           CHEAT_NAME_MAKE_BORDERLESS },
    { "unlimit-fps",          CHEAT_NAME_UNLIMIT_FPS },
    { "disable-hud",          CHEAT_NAME_DISABLE_HUD },
    { "disable-fog",          CHEAT_NAME_DISABLE_FOG },
    { "fullbright",           CHEAT_NAME_FULLBRIGHT },
    { "colorized",            CHEAT_NAME_COLORIZED },
    { "show-fps",             CHEAT_NAME_SHOW_FPS },
    { "fix-movement-speed",   CHEAT_NAME_FIX_MOVEMENT_SPEED },
};

static const int CHEAT_TABLE_SIZE = (int)(sizeof(CHEAT_TABLE) / sizeof(CHEAT_TABLE[0]));

static const char *cheatApiName(CheatName cheat) {
    for (int i = 0; i < CHEAT_TABLE_SIZE; i++) {
        if (CHEAT_TABLE[i].cheat == cheat) return CHEAT_TABLE[i].name;
    }
    return NULL;
}

int clientCheatCount(void) {
    return CHEAT_TABLE_SIZE;
}

const char *clientCheatNameAt(int index) {
    if (index < 0 || index >= CHEAT_TABLE_SIZE) return NULL;
    return CHEAT_TABLE[index].name;
}

static int cheatIndexOfName(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CHEAT_TABLE_SIZE; i++) {
        if (strcmp(CHEAT_TABLE[i].name, name) == 0) return i;
    }
    return -1;
}

bool clientCheatFromName(const char *name, CheatName *out) {
    if (!name || !out) return false;
    for (int i = 0; i < CHEAT_TABLE_SIZE; i++) {
        if (strcmp(CHEAT_TABLE[i].name, name) == 0) {
            *out = CHEAT_TABLE[i].cheat;
            return true;
        }
    }
    return false;
}

ClientResult clientGetCheat(Client *client, CheatName cheat, bool *enabled) {
    if (!enabled) return CLIENT_ERR_INVALID_PARAM;
    const char *name = cheatApiName(cheat);
    if (!name) return CLIENT_ERR_NOT_FOUND;

    char path[64];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/cheats/%s", name);
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", path, NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    *enabled = jsonObjectGetBool(body, "enabled", false);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientSetCheat(Client *client, CheatName cheat, bool enabled) {
    const char *name = cheatApiName(cheat);
    if (!name) return CLIENT_ERR_NOT_FOUND;

    char path[64];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/cheats/%s", name);
    const char *reqBody = enabled ? "{\"enabled\":true}" : "{\"enabled\":false}";
    return clientRequest(client, "PUT", path, reqBody, NULL);
}

ClientResult clientGetCheats(Client *client, bool *enabledOut, int max, int *countOut) {
    if (!enabledOut || max <= 0) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/cheats", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    int total = CHEAT_TABLE_SIZE < max ? CHEAT_TABLE_SIZE : max;
    for (int i = 0; i < total; i++) enabledOut[i] = false;

    int n = jsonArrayCount(body);
    for (int i = 0; i < n; i++) {
        JsonValue *entry = jsonArrayAt(body, i);
        int idx = cheatIndexOfName(jsonObjectGetString(entry, "name", ""));
        if (idx >= 0 && idx < total) enabledOut[idx] = jsonObjectGetBool(entry, "enabled", false);
    }
    jsonFree(body);
    if (countOut) *countOut = total;
    return CLIENT_OK;
}

ClientResult clientSetCheats(Client *client, const CheatName *names, const bool *values, int count) {
    if (!names || !values || count <= 0) return CLIENT_ERR_INVALID_PARAM;

    JsonValue *obj = jsonNewObject();
    for (int i = 0; i < count; i++) {
        const char *name = cheatApiName(names[i]);
        if (!name) { jsonFree(obj); return CLIENT_ERR_NOT_FOUND; }
        jsonObjectSetBool(obj, name, values[i]);
    }

    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;
    ClientResult r = clientRequest(client, "PATCH", CLIENT_API_BASE "/cheats", reqBody, NULL);
    free(reqBody);
    return r;
}
