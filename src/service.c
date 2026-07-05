#include "service/service_internal.h"
#include "service/cheats.h"
#include "service/game.h"
#include "service/state.h"
#include "service/round.h"
#include "service/player.h"
#include "service/graphics.h"
#include "service/customizer.h"
#include "service/widgets.h"
#include "service/binds.h"
#include "service/actions.h"
#include "service/commands.h"
#include "service/stats.h"
#include "service/trade.h"
#include "service/server.h"

#include "win/http.h"
#include "win/thread.h"
#include "utils/json.h"
#include "utils/common.h"
#include "logic/config.h"
#include "metadata.h"
#include "logger.h"

#include <iniparser.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define API_PREFIX "/api/v1"
#define API_PREFIX_LEN 7

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

Service *serviceCreate(Controller *controller) {
    if (!controller) return NULL;
    Service *service = (Service *)malloc(sizeof(Service));
    if (!service) return NULL;
    service->controller = controller;
    return service;
}

void serviceDestroy(Service *service) {
    if (service) free(service);
}

const char *serviceGetVersion(Service *service) {
    (void)service;
    return BO1ZT_VERSION;
}

int serviceResolvePort(void) {
    const char *env = getenv("BO1ZT_PORT");
    if (env && *env) {
        int p = atoi(env);
        if (p > 0 && p < 65536) return p;
    }
    int port = SERVICE_DEFAULT_PORT;
    dictionary *dict = iniparser_load("bo1zt.ini");
    if (dict) {
        port = iniparser_getint(dict, "Api:Port", SERVICE_DEFAULT_PORT);
        iniparser_freedict(dict);
    }
    return port;
}

// ---------------------------------------------------------------------------
// Response helpers
// ---------------------------------------------------------------------------

static void respondJson(HttpResponse *response, int status, JsonValue *json) {
    char *body = jsonSerialize(json);
    jsonFree(json);
    if (!body) {
        httpResponseJson(response, 500,
            "{\"error\":{\"code\":\"ENGINE_FAILED\",\"message\":\"Serialization failed\"}}");
        return;
    }
    httpResponseJson(response, status, body);
    free(body);
}

static void respondError(HttpResponse *response, int status, const char *code, const char *message) {
    JsonValue *err = jsonNewObject();
    JsonValue *inner = jsonNewObject();
    jsonObjectSetString(inner, "code", code);
    jsonObjectSetString(inner, "message", message);
    jsonObjectSet(err, "error", inner);
    respondJson(response, status, err);
}

static void respondServiceError(HttpResponse *response, ServiceResult result) {
    switch (result) {
        case SERVICE_NOT_FOUND:
            respondError(response, 404, "NOT_FOUND", "Unknown resource"); break;
        case SERVICE_INVALID_PARAM:
            respondError(response, 400, "INVALID_PARAM", "Invalid parameters"); break;
        case SERVICE_GAME_NOT_ATTACHED:
            respondError(response, 409, "GAME_NOT_ATTACHED", "Game is not attached"); break;
        case SERVICE_ENGINE_FAILED:
            respondError(response, 500, "ENGINE_FAILED", "Engine call failed"); break;
        default:
            respondError(response, 500, "ENGINE_FAILED", "Unexpected error"); break;
    }
}

static void respondNotFound(HttpResponse *response) {
    respondError(response, 404, "NOT_FOUND", "Unknown resource");
}

static JsonValue *colorJson(Color color) {
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "r", color.r);
    jsonObjectSetInt(obj, "g", color.g);
    jsonObjectSetInt(obj, "b", color.b);
    jsonObjectSetInt(obj, "a", color.a);
    return obj;
}

static JsonValue *rectJson(Rect rect) {
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "x", rect.x);
    jsonObjectSetInt(obj, "y", rect.y);
    jsonObjectSetInt(obj, "w", rect.w);
    jsonObjectSetInt(obj, "h", rect.h);
    return obj;
}

static bool parseColor(JsonValue *obj, Color *out) {
    if (jsonTypeOf(obj) != JSON_OBJECT) return false;
    JsonValue *r = jsonObjectGet(obj, "r"), *g = jsonObjectGet(obj, "g");
    JsonValue *b = jsonObjectGet(obj, "b"), *a = jsonObjectGet(obj, "a");
    if (jsonTypeOf(r) != JSON_NUMBER || jsonTypeOf(g) != JSON_NUMBER ||
        jsonTypeOf(b) != JSON_NUMBER || jsonTypeOf(a) != JSON_NUMBER) return false;
    *out = colorCreate((uint8_t)jsonGetInt(r, 0), (uint8_t)jsonGetInt(g, 0),
                       (uint8_t)jsonGetInt(b, 0), (uint8_t)jsonGetInt(a, 0));
    return true;
}

static bool parseRect(JsonValue *obj, Rect *out) {
    if (jsonTypeOf(obj) != JSON_OBJECT) return false;
    JsonValue *x = jsonObjectGet(obj, "x"), *y = jsonObjectGet(obj, "y");
    JsonValue *w = jsonObjectGet(obj, "w"), *h = jsonObjectGet(obj, "h");
    if (jsonTypeOf(x) != JSON_NUMBER || jsonTypeOf(y) != JSON_NUMBER ||
        jsonTypeOf(w) != JSON_NUMBER || jsonTypeOf(h) != JSON_NUMBER) return false;
    *out = rectCreate((uint32_t)jsonGetInt(x, 0), (uint32_t)jsonGetInt(y, 0),
                      (uint32_t)jsonGetInt(w, 0), (uint32_t)jsonGetInt(h, 0));
    return true;
}

static int getIntField(JsonValue *obj, const char *key, int *out) {
    JsonValue *v = jsonObjectGet(obj, key);
    if (!v) return 0;
    if (jsonTypeOf(v) != JSON_NUMBER) return -1;
    *out = jsonGetInt(v, 0);
    return 1;
}

static int getBoolField(JsonValue *obj, const char *key, bool *out) {
    JsonValue *v = jsonObjectGet(obj, key);
    if (!v) return 0;
    if (jsonTypeOf(v) != JSON_BOOL) return -1;
    *out = jsonGetBool(v, false);
    return 1;
}

static int getColorField(JsonValue *obj, const char *key, Color *out) {
    JsonValue *v = jsonObjectGet(obj, key);
    if (!v) return 0;
    return parseColor(v, out) ? 1 : -1;
}

static int getRectField(JsonValue *obj, const char *key, Rect *out) {
    JsonValue *v = jsonObjectGet(obj, key);
    if (!v) return 0;
    return parseRect(v, out) ? 1 : -1;
}

static bool hasOnlyKnownKeys(JsonValue *obj, const char **keys, int keyCount) {
    int count = jsonObjectCount(obj);
    for (int i = 0; i < count; i++) {
        const char *key = jsonObjectKeyAt(obj, i);
        bool known = false;
        for (int k = 0; k < keyCount; k++) {
            if (strcmp(key, keys[k]) == 0) { known = true; break; }
        }
        if (!known) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Handlers: discovery / health
// ---------------------------------------------------------------------------

static void handleHealth(Service *service, HttpResponse *response) {
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "version", serviceGetVersion(service));
    respondJson(response, 200, obj);
}

static void handleCommandsList(Service *service, HttpResponse *response) {
    ServiceCommandInfo commands[64];
    int count = 0;
    ServiceResult r = serviceCommandsList(service, commands, 64, &count);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *arr = jsonNewArray();
    for (int i = 0; i < count; i++) {
        JsonValue *obj = jsonNewObject();
        jsonObjectSetString(obj, "name", commands[i].name);
        jsonObjectSetString(obj, "usage", commands[i].usage);
        jsonObjectSetString(obj, "description", commands[i].description);
        jsonArrayAppend(arr, obj);
    }
    respondJson(response, 200, arr);
}

// ---------------------------------------------------------------------------
// Handlers: cheats
// ---------------------------------------------------------------------------

static JsonValue *cheatObject(const char *name, bool enabled) {
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "name", name);
    jsonObjectSetBool(obj, "enabled", enabled);
    return obj;
}

static void handleCheatList(Service *service, HttpResponse *response) {
    JsonValue *arr = jsonNewArray();
    int count = serviceCheatCount();
    for (int i = 0; i < count; i++) {
        const char *name = serviceCheatNameAt(i);
        bool enabled = false;
        serviceCheatGet(service, name, &enabled);
        jsonArrayAppend(arr, cheatObject(name, enabled));
    }
    respondJson(response, 200, arr);
}

static void handleCheatGet(Service *service, HttpResponse *response, const char *name) {
    bool enabled = false;
    ServiceResult r = serviceCheatGet(service, name, &enabled);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    respondJson(response, 200, cheatObject(name, enabled));
}

static void handleCheatPut(Service *service, HttpResponse *response, const char *name, const char *body) {
    JsonValue *parsed = jsonParse(body);
    JsonValue *enabledField = jsonObjectGet(parsed, "enabled");
    if (!enabledField || jsonTypeOf(enabledField) != JSON_BOOL) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Missing boolean 'enabled' field");
        return;
    }
    bool enabled = jsonGetBool(enabledField, false);
    jsonFree(parsed);

    bool result = false;
    ServiceResult r = serviceCheatSet(service, name, enabled, &result);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    respondJson(response, 200, cheatObject(name, result));
}

// PATCH /cheats — set multiple cheats at once. Body is an object mapping cheat
// names to booleans. Unknown names or non-boolean values are rejected before
// any change is applied so the batch stays atomic.
static void handleCheatPatch(Service *service, HttpResponse *response, const char *body) {
    JsonValue *parsed = jsonParse(body);
    if (jsonTypeOf(parsed) != JSON_OBJECT) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Expected a JSON object of cheat states");
        return;
    }

    int count = jsonObjectCount(parsed);
    // Validate every entry first (name known, value boolean).
    for (int i = 0; i < count; i++) {
        const char *key = jsonObjectKeyAt(parsed, i);
        JsonValue *value = jsonObjectValueAt(parsed, i);
        if (!serviceCheatExists(key)) {
            jsonFree(parsed);
            respondError(response, 404, "NOT_FOUND", "Unknown cheat name");
            return;
        }
        if (jsonTypeOf(value) != JSON_BOOL) {
            jsonFree(parsed);
            respondError(response, 400, "INVALID_PARAM", "Cheat values must be booleans");
            return;
        }
    }

    // Apply each cheat and echo back ONLY the cheats that were set, using the
    // result serviceCheatSet already hands us. We deliberately do NOT re-read
    // the full cheat list: reading every cheat's live state would fire a GSC
    // query for static-box on every batch set, even when the caller only
    // touched unrelated cheats (e.g. `cheats noclip=on`).
    JsonValue *arr = jsonNewArray();
    for (int i = 0; i < count; i++) {
        const char *key = jsonObjectKeyAt(parsed, i);
        bool enabled = jsonGetBool(jsonObjectValueAt(parsed, i), false);
        bool result = false;
        ServiceResult r = serviceCheatSet(service, key, enabled, &result);
        if (r != SERVICE_OK) { jsonFree(arr); jsonFree(parsed); respondServiceError(response, r); return; }
        jsonArrayAppend(arr, cheatObject(key, result));
    }
    jsonFree(parsed);
    respondJson(response, 200, arr);
}

// ---------------------------------------------------------------------------
// Handlers: game
// ---------------------------------------------------------------------------

static void handleGameStatus(Service *service, HttpResponse *response) {
    ServiceGameStatus status = serviceGameStatus(service);
    JsonValue *obj = jsonNewObject();
    jsonObjectSetBool(obj, "attached", status.attached);
    jsonObjectSetBool(obj, "running", status.running);
    jsonObjectSetBool(obj, "ready", status.ready);
    jsonObjectSetBool(obj, "window-focused", status.windowFocused);
    respondJson(response, 200, obj);
}

static void handleGameConfigGet(Service *service, HttpResponse *response) {
    ServiceGameConfig cfg;
    ServiceResult r = serviceGameGetConfig(service, &cfg);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "location", cfg.location);
    jsonObjectSetString(obj, "hostname", cfg.hostname);
    jsonObjectSetString(obj, "character", cfg.character);
    respondJson(response, 200, obj);
}

static void handleGameConfigPatch(Service *service, HttpResponse *response, const char *body) {
    static const char *KEYS[] = { "location", "hostname", "character" };
    JsonValue *parsed = jsonParse(body);
    if (jsonTypeOf(parsed) != JSON_OBJECT) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Expected a JSON object");
        return;
    }
    if (!hasOnlyKnownKeys(parsed, KEYS, (int)(sizeof(KEYS) / sizeof(KEYS[0])))) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Unknown field");
        return;
    }

    ServiceGameConfigPatch patch = {};
    JsonValue *loc = jsonObjectGet(parsed, "location");
    if (loc) {
        if (jsonTypeOf(loc) != JSON_STRING) {
            jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected string for location"); return;
        }
        snprintf(patch.location, sizeof(patch.location), "%s", jsonGetString(loc, ""));
        patch.hasLocation = true;
    }
    JsonValue *host = jsonObjectGet(parsed, "hostname");
    if (host) {
        if (jsonTypeOf(host) != JSON_STRING) {
            jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected string for hostname"); return;
        }
        snprintf(patch.hostname, sizeof(patch.hostname), "%s", jsonGetString(host, ""));
        patch.hasHostname = true;
    }
    JsonValue *character = jsonObjectGet(parsed, "character");
    if (character) {
        if (jsonTypeOf(character) != JSON_STRING) {
            jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected string for character"); return;
        }
        snprintf(patch.character, sizeof(patch.character), "%s", jsonGetString(character, ""));
        patch.hasCharacter = true;
    }
    jsonFree(parsed);

    ServiceResult r = serviceGameUpdateConfig(service, &patch);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    handleGameConfigGet(service, response);
}

// ---------------------------------------------------------------------------
// Handlers: state
// ---------------------------------------------------------------------------

static void handleState(Service *service, HttpResponse *response) {
    ServiceStateSnapshot s = serviceStateSnapshot(service);
    JsonValue *obj = jsonNewObject();
    jsonObjectSetBool(obj, "is-game-attached", s.isGameAttached);
    jsonObjectSetBool(obj, "is-zombies-game-ongoing", s.isZombiesGameOngoing);
    jsonObjectSetBool(obj, "is-zombies-game-paused", s.isZombiesGamePaused);
    jsonObjectSetInt(obj, "game-resets", s.gameResets);
    jsonObjectSetString(obj, "level", s.level);
    jsonObjectSetInt(obj, "elapsed", s.elapsed);
    jsonObjectSetDouble(obj, "movement-speed", s.movementSpeed);
    jsonObjectSetInt(obj, "round", s.round);
    JsonValue *entities = jsonNewObject();
    jsonObjectSetInt(entities, "current", s.entitiesCurrent);
    jsonObjectSetInt(entities, "max", s.entitiesMax);
    jsonObjectSet(obj, "entities", entities);
    respondJson(response, 200, obj);
}

// ---------------------------------------------------------------------------
// Handlers: round
// ---------------------------------------------------------------------------

static void handleRoundGet(Service *service, HttpResponse *response) {
    ServiceRoundInfo info;
    ServiceResult r = serviceRoundGet(service, &info);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "number", info.number);
    jsonObjectSetBool(obj, "is-special", info.isSpecial);
    jsonObjectSetInt(obj, "zombies-left", info.zombiesLeft);
    respondJson(response, 200, obj);
}

static void handleRoundSpecial(Service *service, HttpResponse *response) {
    ServiceSpecialRound special;
    ServiceResult r = serviceRoundGetSpecial(service, &special);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    // A map has at most one special type, so emit only that key (mutually
    // exclusive) alongside the prediction.
    if (special.type[0] != '\0') jsonObjectSetInt(obj, special.type, special.count);
    jsonObjectSetString(obj, "next", special.next);
    respondJson(response, 200, obj);
}

static void handleRoundPut(Service *service, HttpResponse *response, const char *body) {
    JsonValue *parsed = jsonParse(body);
    JsonValue *roundField = jsonObjectGet(parsed, "round");
    if (!roundField || jsonTypeOf(roundField) != JSON_NUMBER) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Missing numeric 'round' field");
        return;
    }
    int round = jsonGetInt(roundField, 0);
    jsonFree(parsed);

    ServiceResult r = serviceRoundSet(service, round);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "round", round);
    respondJson(response, 200, obj);
}

// ---------------------------------------------------------------------------
// Handlers: player
// ---------------------------------------------------------------------------

static void handlePlayerPosition(Service *service, HttpResponse *response) {
    ServicePlayerPosition pos;
    ServiceResult r = servicePlayerGetPosition(service, &pos);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetDouble(obj, "x", pos.x);
    jsonObjectSetDouble(obj, "y", pos.y);
    jsonObjectSetDouble(obj, "z", pos.z);
    respondJson(response, 200, obj);
}

static void handlePlayerTeleport(Service *service, HttpResponse *response, const char *body) {
    JsonValue *parsed = jsonParse(body);
    JsonValue *xf = jsonObjectGet(parsed, "x");
    JsonValue *yf = jsonObjectGet(parsed, "y");
    JsonValue *zf = jsonObjectGet(parsed, "z");
    if (jsonTypeOf(xf) != JSON_NUMBER || jsonTypeOf(yf) != JSON_NUMBER || jsonTypeOf(zf) != JSON_NUMBER) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Expected numeric x, y, z");
        return;
    }
    float x = (float)jsonGetNumber(xf, 0);
    float y = (float)jsonGetNumber(yf, 0);
    float z = (float)jsonGetNumber(zf, 0);
    jsonFree(parsed);

    ServiceResult r = servicePlayerTeleport(service, x, y, z);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    httpResponseStatus(response, 204);
}

static void handlePlayerAmmo(Service *service, HttpResponse *response) {
    ServiceResult r = servicePlayerGiveAmmo(service);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    httpResponseStatus(response, 204);
}

static void handlePlayerWeaponsDelete(Service *service, HttpResponse *response) {
    ServiceResult r = servicePlayerTakeWeapons(service);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    httpResponseStatus(response, 204);
}

static void handlePlayerPerksGet(Service *service, HttpResponse *response) {
    int count = 0;
    ServiceResult r = servicePlayerGetPerkCount(service, &count);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "count", count);
    respondJson(response, 200, obj);
}

static void handlePlayerPerksPost(Service *service, HttpResponse *response, const char *body) {
    JsonValue *parsed = jsonParse(body);
    const char *action = jsonObjectGetString(parsed, "action", NULL);
    JsonValue *perksArr = jsonObjectGet(parsed, "perks");
    bool validAction = action && (strcmp(action, "add") == 0 || strcmp(action, "remove") == 0);
    if (!validAction || jsonTypeOf(perksArr) != JSON_ARRAY) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Expected 'action' (add|remove) and 'perks' array");
        return;
    }
    int n = jsonArrayCount(perksArr);
    if (n <= 0 || n > 16) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Invalid number of perks");
        return;
    }
    const char *names[16];
    for (int i = 0; i < n; i++) {
        JsonValue *item = jsonArrayAt(perksArr, i);
        if (jsonTypeOf(item) != JSON_STRING) {
            jsonFree(parsed);
            respondError(response, 400, "INVALID_PARAM", "Perk names must be strings");
            return;
        }
        names[i] = jsonGetString(item, "");
    }

    ServicePerkAction act = (strcmp(action, "add") == 0) ? SERVICE_PERK_ADD : SERVICE_PERK_REMOVE;
    ServiceResult r = servicePlayerModifyPerks(service, act, names, n);
    if (r != SERVICE_OK) { jsonFree(parsed); respondServiceError(response, r); return; }

    JsonValue *obj = jsonNewObject();
    JsonValue *arr = jsonNewArray();
    for (int i = 0; i < n; i++) jsonArrayAppend(arr, jsonNewString(names[i]));
    jsonObjectSet(obj, "perks", arr);
    jsonFree(parsed);
    respondJson(response, 200, obj);
}

static void handlePlayerWeaponsPost(Service *service, HttpResponse *response, const char *body) {
    JsonValue *parsed = jsonParse(body);
    JsonValue *weaponsArr = jsonObjectGet(parsed, "weapons");
    if (jsonTypeOf(weaponsArr) != JSON_ARRAY) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Expected 'weapons' array");
        return;
    }
    int n = jsonArrayCount(weaponsArr);
    if (n <= 0 || n > 32) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Invalid number of weapons");
        return;
    }
    const char *names[32];
    for (int i = 0; i < n; i++) {
        JsonValue *item = jsonArrayAt(weaponsArr, i);
        if (jsonTypeOf(item) != JSON_STRING) {
            jsonFree(parsed);
            respondError(response, 400, "INVALID_PARAM", "Weapon names must be strings");
            return;
        }
        names[i] = jsonGetString(item, "");
    }

    ServiceResult r = servicePlayerGiveWeapons(service, names, n);
    if (r != SERVICE_OK) { jsonFree(parsed); respondServiceError(response, r); return; }

    JsonValue *obj = jsonNewObject();
    JsonValue *arr = jsonNewArray();
    for (int i = 0; i < n; i++) jsonArrayAppend(arr, jsonNewString(names[i]));
    jsonObjectSet(obj, "weapons", arr);
    jsonFree(parsed);
    respondJson(response, 200, obj);
}

static JsonValue *playerAttributesJson(const ServicePlayerAttributes *a) {
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "name", a->name);
    jsonObjectSetInt(obj, "health", a->health);
    jsonObjectSetInt(obj, "points", a->points);
    jsonObjectSetInt(obj, "kills", a->kills);
    jsonObjectSetInt(obj, "headshots", a->headshots);
    jsonObjectSetDouble(obj, "movement-speed", a->movementSpeed);
    return obj;
}

static void handlePlayerGet(Service *service, HttpResponse *response) {
    ServicePlayerAttributes attrs;
    ServiceResult r = servicePlayerGetAttributes(service, &attrs);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    respondJson(response, 200, playerAttributesJson(&attrs));
}

static void handlePlayerPatch(Service *service, HttpResponse *response, const char *body) {
    static const char *KEYS[] = {
        "name", "health", "points", "kills", "headshots", "movement-speed",
    };
    JsonValue *parsed = jsonParse(body);
    if (jsonTypeOf(parsed) != JSON_OBJECT) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Expected a JSON object");
        return;
    }
    if (!hasOnlyKnownKeys(parsed, KEYS, (int)(sizeof(KEYS) / sizeof(KEYS[0])))) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Unknown field");
        return;
    }

    ServicePlayerPatch patch = {};
    int r;
    if ((r = getIntField(parsed, "health", &patch.health)) < 0) {
        jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected number for health"); return;
    }
    if (r) patch.hasHealth = true;
    if ((r = getIntField(parsed, "points", &patch.points)) < 0) {
        jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected number for points"); return;
    }
    if (r) patch.hasPoints = true;
    if ((r = getIntField(parsed, "kills", &patch.kills)) < 0) {
        jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected number for kills"); return;
    }
    if (r) patch.hasKills = true;
    if ((r = getIntField(parsed, "headshots", &patch.headshots)) < 0) {
        jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected number for headshots"); return;
    }
    if (r) patch.hasHeadshots = true;
    if ((r = getIntField(parsed, "movement-speed", &patch.movementSpeed)) < 0) {
        jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected number for movement-speed"); return;
    }
    if (r) patch.hasMovementSpeed = true;

    JsonValue *nameField = jsonObjectGet(parsed, "name");
    if (nameField) {
        if (jsonTypeOf(nameField) != JSON_STRING) {
            jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected string for name"); return;
        }
        snprintf(patch.name, sizeof(patch.name), "%s", jsonGetString(nameField, ""));
        patch.hasName = true;
    }
    jsonFree(parsed);

    ServiceResult sr = servicePlayerUpdate(service, &patch);
    if (sr != SERVICE_OK) { respondServiceError(response, sr); return; }
    handlePlayerGet(service, response);
}

// ---------------------------------------------------------------------------
// Handlers: stats
// ---------------------------------------------------------------------------

// Set an int field, or JSON null when the value is the -1 "not applicable" sentinel.
static void setIntOrNull(JsonValue *obj, const char *key, int value) {
    if (value < 0) jsonObjectSetNull(obj, key);
    else jsonObjectSetInt(obj, key, value);
}

static void handleStats(Service *service, HttpResponse *response, const char *query) {
    // Parse optional ?round=n to scope sph.
    int scopeRound = 0;
    if (query && *query) {
        const char *p = strstr(query, "round=");
        if (p) scopeRound = atoi(p + 6);
    }

    ServiceStats stats;
    ServiceResult r = serviceStatsGet(service, scopeRound, &stats);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }

    JsonValue *obj = jsonNewObject();
    JsonValue *entities = jsonNewObject();
    jsonObjectSetInt(entities, "current", stats.entitiesCurrent);
    jsonObjectSetInt(entities, "max", stats.entitiesMax);
    jsonObjectSet(obj, "entities", entities);
    jsonObjectSetInt(obj, "claymores", stats.claymores);
    jsonObjectSetInt(obj, "revives", stats.revives);
    JsonValue *special = jsonNewObject();
    setIntOrNull(special, "dogs", stats.specialDogs);
    setIntOrNull(special, "monkeys", stats.specialMonkeys);
    setIntOrNull(special, "thief", stats.specialThief);
    jsonObjectSet(obj, "special-rounds", special);
    jsonObjectSetString(obj, "next-special-rounds", stats.nextSpecialRounds);
    jsonObjectSetDouble(obj, "sph", stats.sph);
    respondJson(response, 200, obj);
}

static void handleStatsSph(Service *service, HttpResponse *response, const char *query) {
    int scopeRound = 0;
    if (query && *query) {
        const char *p = strstr(query, "round=");
        if (p) scopeRound = atoi(p + 6);
    }
    double sph = 0.0;
    ServiceResult r = serviceStatsGetSph(service, scopeRound, &sph);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetDouble(obj, "sph", sph);
    respondJson(response, 200, obj);
}

static void handleStatsClaymores(Service *service, HttpResponse *response) {
    int claymores = 0;
    ServiceResult r = serviceStatsGetClaymores(service, &claymores);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "claymores", claymores);
    respondJson(response, 200, obj);
}

static void handleStatsEntities(Service *service, HttpResponse *response) {
    int current = 0, max = 0;
    ServiceResult r = serviceStatsGetEntities(service, &current, &max);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "current", current);
    jsonObjectSetInt(obj, "max", max);
    respondJson(response, 200, obj);
}

static void handleStatsRevives(Service *service, HttpResponse *response) {
    int revives = 0;
    ServiceResult r = serviceStatsGetRevives(service, &revives);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "revives", revives);
    respondJson(response, 200, obj);
}

// ---------------------------------------------------------------------------
// Handlers: trade
// ---------------------------------------------------------------------------

static void handleTradeStatus(Service *service, HttpResponse *response) {
    ServiceTradeStatus status;
    ServiceResult r = serviceTradeStatus(service, &status);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetBool(obj, "running", status.running);
    jsonObjectSetInt(obj, "elapsed-ms", status.elapsedMs);
    jsonObjectSetInt(obj, "hits", status.hits);
    respondJson(response, 200, obj);
}

static void handleTradeTotal(Service *service, HttpResponse *response) {
    ServiceTradeTotal total;
    ServiceResult r = serviceTradeTotal(service, &total);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "trades", total.trades);
    jsonObjectSetInt(obj, "total-ms", total.totalMs);
    jsonObjectSetInt(obj, "total-hits", total.totalHits);
    respondJson(response, 200, obj);
}

static void handleTradeStart(Service *service, HttpResponse *response) {
    ServiceResult r = serviceTradeStart(service);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetBool(obj, "running", true);
    respondJson(response, 200, obj);
}

static void handleTradeEnd(Service *service, HttpResponse *response) {
    ServiceTradeStatus status;
    ServiceResult r = serviceTradeEnd(service, &status);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "elapsed-ms", status.elapsedMs);
    jsonObjectSetInt(obj, "hits", status.hits);
    respondJson(response, 200, obj);
}

static void handleTradeCancel(Service *service, HttpResponse *response) {
    ServiceResult r = serviceTradeCancel(service);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    httpResponseStatus(response, 204);
}

// ---------------------------------------------------------------------------
// Handlers: server passthrough
// ---------------------------------------------------------------------------

static void handleServerCommand(Service *service, HttpResponse *response, const char *body) {
    JsonValue *parsed = jsonParse(body);
    const char *command = jsonObjectGetString(parsed, "command", NULL);
    if (!command || !*command) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Missing 'command' string");
        return;
    }
    ServiceResult r = serviceServerCommand(service, command);
    jsonFree(parsed);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    httpResponseStatus(response, 204);
}

static void handleServerDvarGet(Service *service, HttpResponse *response, const char *name) {
    char *value = NULL;
    ServiceResult r = serviceServerGetDvar(service, name, &value);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "name", name);
    jsonObjectSetString(obj, "value", value);
    free(value);
    respondJson(response, 200, obj);
}

static void handleServerDvarPut(Service *service, HttpResponse *response, const char *name, const char *body) {
    JsonValue *parsed = jsonParse(body);
    JsonValue *valueField = jsonObjectGet(parsed, "value");
    // Accept string or number; coerce number to its string form.
    char valueBuf[128];
    const char *value = NULL;
    if (jsonTypeOf(valueField) == JSON_STRING) {
        value = jsonGetString(valueField, "");
    } else if (jsonTypeOf(valueField) == JSON_NUMBER) {
        double n = jsonGetNumber(valueField, 0);
        if (n == (double)(long)n) snprintf(valueBuf, sizeof(valueBuf), "%ld", (long)n);
        else snprintf(valueBuf, sizeof(valueBuf), "%g", n);
        value = valueBuf;
    }
    if (!value) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Missing 'value' (string or number)");
        return;
    }
    char valueCopy[128];
    snprintf(valueCopy, sizeof(valueCopy), "%s", value);
    jsonFree(parsed);

    ServiceResult r = serviceServerSetDvar(service, name, valueCopy);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "name", name);
    jsonObjectSetString(obj, "value", valueCopy);
    respondJson(response, 200, obj);
}

// ---------------------------------------------------------------------------
// Handlers: graphics
// ---------------------------------------------------------------------------

static void handleGraphicsGet(Service *service, HttpResponse *response) {
    GraphicsConfig g;
    ServiceResult r = serviceGraphicsGet(service, &g);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "fov", g.fov);
    jsonObjectSetInt(obj, "fov-scale", g.fovScale);
    jsonObjectSetInt(obj, "fps-cap", g.fpsCap);
    jsonObjectSetBool(obj, "borderless", g.borderless);
    jsonObjectSetBool(obj, "unlimit-fps", g.unlimitFps);
    jsonObjectSetBool(obj, "disable-hud", g.disableHud);
    jsonObjectSetBool(obj, "disable-fog", g.disableFog);
    jsonObjectSetBool(obj, "fullbright", g.fullbright);
    jsonObjectSetBool(obj, "colorized", g.colorized);
    respondJson(response, 200, obj);
}

static void handleGraphicsReset(Service *service, HttpResponse *response) {
    ServiceResult r = serviceGraphicsReset(service);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    httpResponseStatus(response, 204);
}

static void handleGraphicsPatch(Service *service, HttpResponse *response, const char *body) {
    static const char *KEYS[] = {
        "fov", "fov-scale", "fps-cap", "borderless", "unlimit-fps",
        "disable-hud", "disable-fog", "fullbright", "colorized",
    };
    JsonValue *parsed = jsonParse(body);
    if (jsonTypeOf(parsed) != JSON_OBJECT) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Expected a JSON object");
        return;
    }
    if (!hasOnlyKnownKeys(parsed, KEYS, (int)(sizeof(KEYS) / sizeof(KEYS[0])))) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Unknown field");
        return;
    }

    GraphicsPatch patch = {};
#define INT_FIELD(key, hasf, valf) do { \
        int _r = getIntField(parsed, key, &patch.valf); \
        if (_r < 0) { jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected number for " key); return; } \
        if (_r) patch.hasf = true; } while (0)
#define BOOL_FIELD(key, hasf, valf) do { \
        int _r = getBoolField(parsed, key, &patch.valf); \
        if (_r < 0) { jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected boolean for " key); return; } \
        if (_r) patch.hasf = true; } while (0)

    INT_FIELD("fov", hasFov, fov);
    INT_FIELD("fov-scale", hasFovScale, fovScale);
    INT_FIELD("fps-cap", hasFpsCap, fpsCap);
    BOOL_FIELD("borderless", hasBorderless, borderless);
    BOOL_FIELD("unlimit-fps", hasUnlimitFps, unlimitFps);
    BOOL_FIELD("disable-hud", hasDisableHud, disableHud);
    BOOL_FIELD("disable-fog", hasDisableFog, disableFog);
    BOOL_FIELD("fullbright", hasFullbright, fullbright);
    BOOL_FIELD("colorized", hasColorized, colorized);
#undef INT_FIELD
#undef BOOL_FIELD
    jsonFree(parsed);

    ServiceResult r = serviceGraphicsUpdate(service, &patch);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    handleGraphicsGet(service, response);
}

// ---------------------------------------------------------------------------
// Handlers: customizer
// ---------------------------------------------------------------------------

static void handleCustomizerGet(Service *service, HttpResponse *response) {
    CustomizerConfig c;
    ServiceResult r = serviceCustomizerGet(service, &c);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    JsonValue *obj = jsonNewObject();
    jsonObjectSet(obj, "score-background", colorJson(c.scoreBackground));
    jsonObjectSet(obj, "score-player-1", colorJson(c.scorePlayer1));
    jsonObjectSet(obj, "score-player-2", colorJson(c.scorePlayer2));
    jsonObjectSet(obj, "score-player-3", colorJson(c.scorePlayer3));
    jsonObjectSet(obj, "score-player-4", colorJson(c.scorePlayer4));
    jsonObjectSet(obj, "reload-warn-primary", colorJson(c.reloadWarnPrimary));
    jsonObjectSet(obj, "reload-warn-secondary", colorJson(c.reloadWarnSecondary));
    jsonObjectSet(obj, "low-ammo-warn-primary", colorJson(c.lowAmmoWarnPrimary));
    jsonObjectSet(obj, "low-ammo-warn-secondary", colorJson(c.lowAmmoWarnSecondary));
    jsonObjectSet(obj, "no-ammo-warn-primary", colorJson(c.noAmmoWarnPrimary));
    jsonObjectSet(obj, "no-ammo-warn-secondary", colorJson(c.noAmmoWarnSecondary));
    jsonObjectSetInt(obj, "scoreboard-transparency", c.scoreboardTransparency);
    jsonObjectSetInt(obj, "points-transparency", c.pointsTransparency);
    jsonObjectSetInt(obj, "warning-frequency", c.warningTransitionsFrequency);
    jsonObjectSetInt(obj, "warning-min", c.warningTransitionsMin);
    jsonObjectSetInt(obj, "warning-max", c.warningTransitionsMax);
    respondJson(response, 200, obj);
}

static void handleCustomizerReset(Service *service, HttpResponse *response) {
    ServiceResult r = serviceCustomizerReset(service);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    httpResponseStatus(response, 204);
}

static void handleCustomizerPatch(Service *service, HttpResponse *response, const char *body) {
    static const char *KEYS[] = {
        "score-background", "score-player-1", "score-player-2", "score-player-3",
        "score-player-4", "reload-warn-primary", "reload-warn-secondary",
        "low-ammo-warn-primary", "low-ammo-warn-secondary", "no-ammo-warn-primary",
        "no-ammo-warn-secondary", "scoreboard-transparency", "points-transparency",
        "warning-frequency", "warning-min", "warning-max",
    };
    JsonValue *parsed = jsonParse(body);
    if (jsonTypeOf(parsed) != JSON_OBJECT) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Expected a JSON object");
        return;
    }
    if (!hasOnlyKnownKeys(parsed, KEYS, (int)(sizeof(KEYS) / sizeof(KEYS[0])))) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Unknown field");
        return;
    }

    CustomizerPatch patch = {};
#define COLOR_FIELD(key, hasf, valf) do { \
        int _r = getColorField(parsed, key, &patch.valf); \
        if (_r < 0) { jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected color for " key); return; } \
        if (_r) patch.hasf = true; } while (0)
#define INT_FIELD(key, hasf, valf) do { \
        int _r = getIntField(parsed, key, &patch.valf); \
        if (_r < 0) { jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected number for " key); return; } \
        if (_r) patch.hasf = true; } while (0)

    COLOR_FIELD("score-background", hasScoreBackground, scoreBackground);
    COLOR_FIELD("score-player-1", hasScorePlayer1, scorePlayer1);
    COLOR_FIELD("score-player-2", hasScorePlayer2, scorePlayer2);
    COLOR_FIELD("score-player-3", hasScorePlayer3, scorePlayer3);
    COLOR_FIELD("score-player-4", hasScorePlayer4, scorePlayer4);
    COLOR_FIELD("reload-warn-primary", hasReloadWarnPrimary, reloadWarnPrimary);
    COLOR_FIELD("reload-warn-secondary", hasReloadWarnSecondary, reloadWarnSecondary);
    COLOR_FIELD("low-ammo-warn-primary", hasLowAmmoWarnPrimary, lowAmmoWarnPrimary);
    COLOR_FIELD("low-ammo-warn-secondary", hasLowAmmoWarnSecondary, lowAmmoWarnSecondary);
    COLOR_FIELD("no-ammo-warn-primary", hasNoAmmoWarnPrimary, noAmmoWarnPrimary);
    COLOR_FIELD("no-ammo-warn-secondary", hasNoAmmoWarnSecondary, noAmmoWarnSecondary);
    INT_FIELD("scoreboard-transparency", hasScoreboardTransparency, scoreboardTransparency);
    INT_FIELD("points-transparency", hasPointsTransparency, pointsTransparency);
    INT_FIELD("warning-frequency", hasWarningFrequency, warningFrequency);
    INT_FIELD("warning-min", hasWarningMin, warningMin);
    INT_FIELD("warning-max", hasWarningMax, warningMax);
#undef COLOR_FIELD
#undef INT_FIELD
    jsonFree(parsed);

    ServiceResult r = serviceCustomizerUpdate(service, &patch);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    handleCustomizerGet(service, response);
}

// ---------------------------------------------------------------------------
// Handlers: widgets
// ---------------------------------------------------------------------------

static JsonValue *widgetJson(int index, WidgetConfig w) {
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "name", serviceWidgetNameAt(index));
    jsonObjectSetString(obj, "label", serviceWidgetLabelAt(index));
    jsonObjectSetBool(obj, "enabled", w.enabled);
    jsonObjectSetString(obj, "font", w.font);
    jsonObjectSetInt(obj, "font-size", w.fontSize);
    jsonObjectSet(obj, "text-color", colorJson(w.textColor));
    jsonObjectSetBool(obj, "hide-outside-game", w.hideOutsideGame);
    jsonObjectSet(obj, "rect", rectJson(w.rect));
    return obj;
}

static void handleWidgetList(Service *service, HttpResponse *response) {
    JsonValue *arr = jsonNewArray();
    int count = serviceWidgetCount();
    for (int i = 0; i < count; i++) {
        WidgetConfig w;
        if (serviceWidgetGet(service, i, &w) == SERVICE_OK) {
            jsonArrayAppend(arr, widgetJson(i, w));
        }
    }
    respondJson(response, 200, arr);
}

static void handleWidgetGet(Service *service, HttpResponse *response, const char *name) {
    int index = serviceWidgetIndexOf(name);
    if (index < 0) { respondNotFound(response); return; }
    WidgetConfig w;
    ServiceResult r = serviceWidgetGet(service, index, &w);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    respondJson(response, 200, widgetJson(index, w));
}

static void handleWidgetReset(Service *service, HttpResponse *response, const char *name) {
    int index = serviceWidgetIndexOf(name);
    if (index < 0) { respondNotFound(response); return; }
    ServiceResult r = serviceWidgetReset(service, index);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    httpResponseStatus(response, 204);
}

static void handleWidgetPatch(Service *service, HttpResponse *response, const char *name, const char *body) {
    static const char *KEYS[] = {
        "enabled", "font", "font-size", "text-color", "hide-outside-game", "rect",
    };
    int index = serviceWidgetIndexOf(name);
    if (index < 0) { respondNotFound(response); return; }

    JsonValue *parsed = jsonParse(body);
    if (jsonTypeOf(parsed) != JSON_OBJECT) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Expected a JSON object");
        return;
    }
    if (!hasOnlyKnownKeys(parsed, KEYS, (int)(sizeof(KEYS) / sizeof(KEYS[0])))) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Unknown field");
        return;
    }

    WidgetPatch patch = {};
    int r;
    if ((r = getBoolField(parsed, "enabled", &patch.enabled)) < 0) {
        jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected boolean for enabled"); return;
    }
    if (r) patch.hasEnabled = true;

    if ((r = getIntField(parsed, "font-size", &patch.fontSize)) < 0) {
        jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected number for font-size"); return;
    }
    if (r) patch.hasFontSize = true;

    if ((r = getColorField(parsed, "text-color", &patch.textColor)) < 0) {
        jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected color for text-color"); return;
    }
    if (r) patch.hasTextColor = true;

    if ((r = getBoolField(parsed, "hide-outside-game", &patch.hideOutsideGame)) < 0) {
        jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected boolean for hide-outside-game"); return;
    }
    if (r) patch.hasHideOutsideGame = true;

    if ((r = getRectField(parsed, "rect", &patch.rect)) < 0) {
        jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected rect for rect"); return;
    }
    if (r) patch.hasRect = true;

    JsonValue *fontField = jsonObjectGet(parsed, "font");
    char fontBuf[256];
    if (fontField) {
        if (jsonTypeOf(fontField) != JSON_STRING) {
            jsonFree(parsed); respondError(response, 400, "INVALID_PARAM", "Expected string for font"); return;
        }
        snprintf(fontBuf, sizeof(fontBuf), "%s", jsonGetString(fontField, ""));
        patch.font = fontBuf;
        patch.hasFont = true;
    }

    ServiceResult sr = serviceWidgetUpdate(service, index, &patch);
    jsonFree(parsed);
    if (sr != SERVICE_OK) { respondServiceError(response, sr); return; }
    handleWidgetGet(service, response, name);
}

// ---------------------------------------------------------------------------
// Handlers: binds
// ---------------------------------------------------------------------------

static JsonValue *bindsJson(BindsConfig *b) {
    JsonValue *obj = jsonNewObject();
    JsonValue *arr = jsonNewArray();
    for (int i = 0; i < b->bindCount; i++) {
        JsonValue *bind = jsonNewObject();
        jsonObjectSetString(bind, "key", b->binds[i].keyName);
        jsonObjectSetString(bind, "command", b->binds[i].command);
        jsonArrayAppend(arr, bind);
    }
    jsonObjectSet(obj, "binds", arr);
    return obj;
}

static void handleBindsGet(Service *service, HttpResponse *response) {
    BindsConfig b;
    ServiceResult r = serviceBindsGet(service, &b);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    respondJson(response, 200, bindsJson(&b));
}

static void handleBindsPut(Service *service, HttpResponse *response, const char *body) {
    JsonValue *parsed = jsonParse(body);
    JsonValue *binds = jsonObjectGet(parsed, "binds");
    if (jsonTypeOf(binds) != JSON_ARRAY) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Expected 'binds' array");
        return;
    }
    int count = jsonArrayCount(binds);
    if (count > MAX_BINDS) {
        jsonFree(parsed);
        respondError(response, 400, "INVALID_PARAM", "Too many binds");
        return;
    }

    BindsConfig config;
    config.bindCount = 0;
    for (int i = 0; i < count; i++) {
        JsonValue *bind = jsonArrayAt(binds, i);
        const char *key = jsonObjectGetString(bind, "key", NULL);
        const char *command = jsonObjectGetString(bind, "command", NULL);
        if (!key || !command) {
            jsonFree(parsed);
            respondError(response, 400, "INVALID_PARAM", "Each bind needs 'key' and 'command'");
            return;
        }
        snprintf(config.binds[config.bindCount].keyName, MAX_KEY_NAME_LENGTH, "%s", key);
        snprintf(config.binds[config.bindCount].command, MAX_COMMAND_LENGTH, "%s", command);
        config.bindCount++;
    }
    jsonFree(parsed);

    ServiceResult r = serviceBindsSet(service, &config);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    respondJson(response, 200, bindsJson(&config));
}

static void handleBindsReset(Service *service, HttpResponse *response) {
    ServiceResult r = serviceBindsReset(service);
    if (r != SERVICE_OK) { respondServiceError(response, r); return; }
    httpResponseStatus(response, 204);
}

// ---------------------------------------------------------------------------
// Router
// ---------------------------------------------------------------------------

static bool isExact(const char *sub, const char *seg) {
    return strcmp(sub, seg) == 0;
}

static void route(Service *service, const HttpRequest *request, HttpResponse *response) {
    const char *method = request->method;
    const char *path = request->path;
    const char *body = request->body;

    if (strncmp(path, API_PREFIX, API_PREFIX_LEN) != 0) {
        respondNotFound(response);
        return;
    }
    const char *sub = path + API_PREFIX_LEN;

    // --- discovery / health ---
    if (isExact(sub, "/health")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleHealth(service, response);
        return;
    }
    if (isExact(sub, "/commands")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleCommandsList(service, response);
        return;
    }

    // --- game ---
    if (isExact(sub, "/game")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleGameStatus(service, response);
        return;
    }
    if (isExact(sub, "/game/config")) {
        if (strcmp(method, "GET") == 0) handleGameConfigGet(service, response);
        else if (strcmp(method, "PATCH") == 0) handleGameConfigPatch(service, response, body);
        else respondNotFound(response);
        return;
    }
    if (isExact(sub, "/game/launch")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        ServiceResult r = serviceGameLaunch(service);
        if (r != SERVICE_OK) { respondServiceError(response, r); return; }
        httpResponseStatus(response, 204);
        return;
    }
    if (isExact(sub, "/game/close")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        ServiceResult r = serviceGameClose(service);
        if (r != SERVICE_OK) { respondServiceError(response, r); return; }
        httpResponseStatus(response, 204);
        return;
    }
    if (isExact(sub, "/game/restart")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        ServiceResult r = serviceGameRestart(service);
        if (r != SERVICE_OK) { respondServiceError(response, r); return; }
        httpResponseStatus(response, 204);
        return;
    }

    // --- state ---
    if (isExact(sub, "/state")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleState(service, response);
        return;
    }

    // --- cheats ---
    if (isExact(sub, "/cheats")) {
        if (strcmp(method, "GET") == 0) handleCheatList(service, response);
        else if (strcmp(method, "PATCH") == 0) handleCheatPatch(service, response, body);
        else respondNotFound(response);
        return;
    }
    if (strncmp(sub, "/cheats/", 8) == 0) {
        const char *name = sub + 8;
        if (*name == '\0') { respondNotFound(response); return; }
        if (strcmp(method, "GET") == 0) handleCheatGet(service, response, name);
        else if (strcmp(method, "PUT") == 0) handleCheatPut(service, response, name, body);
        else respondNotFound(response);
        return;
    }

    // --- round ---
    if (isExact(sub, "/round/special")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleRoundSpecial(service, response);
        return;
    }
    if (isExact(sub, "/round")) {
        if (strcmp(method, "GET") == 0) handleRoundGet(service, response);
        else if (strcmp(method, "PUT") == 0) handleRoundPut(service, response, body);
        else respondNotFound(response);
        return;
    }

    // --- player ---
    if (isExact(sub, "/player")) {
        if (strcmp(method, "GET") == 0) handlePlayerGet(service, response);
        else if (strcmp(method, "PATCH") == 0) handlePlayerPatch(service, response, body);
        else respondNotFound(response);
        return;
    }
    if (isExact(sub, "/player/position")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handlePlayerPosition(service, response);
        return;
    }
    if (isExact(sub, "/player/teleport")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        handlePlayerTeleport(service, response, body);
        return;
    }
    if (isExact(sub, "/player/ammo")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        handlePlayerAmmo(service, response);
        return;
    }
    if (isExact(sub, "/player/weapons")) {
        if (strcmp(method, "POST") == 0) handlePlayerWeaponsPost(service, response, body);
        else if (strcmp(method, "DELETE") == 0) handlePlayerWeaponsDelete(service, response);
        else respondNotFound(response);
        return;
    }
    if (isExact(sub, "/player/perks")) {
        if (strcmp(method, "GET") == 0) handlePlayerPerksGet(service, response);
        else if (strcmp(method, "POST") == 0) handlePlayerPerksPost(service, response, body);
        else respondNotFound(response);
        return;
    }

    // --- graphics ---
    if (isExact(sub, "/graphics")) {
        if (strcmp(method, "GET") == 0) handleGraphicsGet(service, response);
        else if (strcmp(method, "PATCH") == 0) handleGraphicsPatch(service, response, body);
        else respondNotFound(response);
        return;
    }
    if (isExact(sub, "/graphics/reset")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        handleGraphicsReset(service, response);
        return;
    }

    // --- customizer ---
    if (isExact(sub, "/customizer")) {
        if (strcmp(method, "GET") == 0) handleCustomizerGet(service, response);
        else if (strcmp(method, "PATCH") == 0) handleCustomizerPatch(service, response, body);
        else respondNotFound(response);
        return;
    }
    if (isExact(sub, "/customizer/reset")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        handleCustomizerReset(service, response);
        return;
    }

    // --- widgets ---
    if (isExact(sub, "/widgets")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleWidgetList(service, response);
        return;
    }
    if (strncmp(sub, "/widgets/", 9) == 0) {
        const char *rest = sub + 9;
        if (*rest == '\0') { respondNotFound(response); return; }
        // /widgets/{name}/reset
        const char *slash = strchr(rest, '/');
        if (slash) {
            if (strcmp(slash, "/reset") == 0 && strcmp(method, "POST") == 0) {
                char name[64];
                size_t n = (size_t)(slash - rest);
                if (n >= sizeof(name)) { respondNotFound(response); return; }
                memcpy(name, rest, n);
                name[n] = '\0';
                handleWidgetReset(service, response, name);
            } else {
                respondNotFound(response);
            }
            return;
        }
        if (strcmp(method, "GET") == 0) handleWidgetGet(service, response, rest);
        else if (strcmp(method, "PATCH") == 0) handleWidgetPatch(service, response, rest, body);
        else respondNotFound(response);
        return;
    }

    // --- binds ---
    if (isExact(sub, "/binds")) {
        if (strcmp(method, "GET") == 0) handleBindsGet(service, response);
        else if (strcmp(method, "PUT") == 0) handleBindsPut(service, response, body);
        else respondNotFound(response);
        return;
    }
    if (isExact(sub, "/binds/reset")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        handleBindsReset(service, response);
        return;
    }

    // --- misc ---
    if (isExact(sub, "/misc/music")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        ServiceResult r = serviceActionsMusic(service);
        if (r != SERVICE_OK) { respondServiceError(response, r); return; }
        httpResponseStatus(response, 204);
        return;
    }

    // --- stats ---
    if (isExact(sub, "/stats")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleStats(service, response, request->query);
        return;
    }
    if (isExact(sub, "/stats/sph")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleStatsSph(service, response, request->query);
        return;
    }
    if (isExact(sub, "/stats/claymores")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleStatsClaymores(service, response);
        return;
    }
    if (isExact(sub, "/stats/entities")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleStatsEntities(service, response);
        return;
    }
    if (isExact(sub, "/stats/revives")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleStatsRevives(service, response);
        return;
    }

    // --- trade ---
    if (isExact(sub, "/trade")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleTradeStatus(service, response);
        return;
    }
    if (isExact(sub, "/trade/total")) {
        if (strcmp(method, "GET") != 0) { respondNotFound(response); return; }
        handleTradeTotal(service, response);
        return;
    }
    if (isExact(sub, "/trade/start")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        handleTradeStart(service, response);
        return;
    }
    if (isExact(sub, "/trade/end")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        handleTradeEnd(service, response);
        return;
    }
    if (isExact(sub, "/trade/cancel")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        handleTradeCancel(service, response);
        return;
    }

    // --- server passthrough ---
    if (isExact(sub, "/server/command")) {
        if (strcmp(method, "POST") != 0) { respondNotFound(response); return; }
        handleServerCommand(service, response, body);
        return;
    }
    if (strncmp(sub, "/server/dvar/", 13) == 0) {
        const char *name = sub + 13;
        if (*name == '\0') { respondNotFound(response); return; }
        if (strcmp(method, "GET") == 0) handleServerDvarGet(service, response, name);
        else if (strcmp(method, "PUT") == 0) handleServerDvarPut(service, response, name, body);
        else respondNotFound(response);
        return;
    }

    respondNotFound(response);
}

static void serviceHandleRequest(const HttpRequest *request, HttpResponse *response, void *userData) {
    route((Service *)userData, request, response);
}

// ---------------------------------------------------------------------------
// Serving
// ---------------------------------------------------------------------------

typedef struct {
    Service *service;
    int port;
} ServeCtx;

static int serveThread(void *data) {
    ServeCtx *ctx = (ServeCtx *)data;
    Service *service = ctx->service;
    int port = ctx->port;
    free(ctx);
    return httpServe(port, serviceHandleRequest, service);
}

void serviceServe(Service *service, int port) {
    if (!service) return;
    ServeCtx *ctx = (ServeCtx *)malloc(sizeof(ServeCtx));
    if (!ctx) return;
    ctx->service = service;
    ctx->port = port;
    threadCreate(serveThread, ctx);
}
