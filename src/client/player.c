#include "client/player.h"
#include "client/client_internal.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Scalar attributes
// ---------------------------------------------------------------------------

ClientResult clientGetPlayer(Client *client, PlayerAttributes *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/player", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    snprintf(out->name, sizeof(out->name), "%s", jsonObjectGetString(body, "name", ""));
    out->health = jsonObjectGetInt(body, "health", 0);
    out->points = jsonObjectGetInt(body, "points", 0);
    out->kills = jsonObjectGetInt(body, "kills", 0);
    out->headshots = jsonObjectGetInt(body, "headshots", 0);
    out->movementSpeed = (float)jsonObjectGetNumber(body, "movement-speed", 0.0);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientSetPlayerName(Client *client, const char *name) {
    if (!name) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "name", name);
    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;
    ClientResult r = clientRequest(client, "PATCH", CLIENT_API_BASE "/player", reqBody, NULL);
    free(reqBody);
    return r;
}

static ClientResult setPlayerInt(Client *client, const char *key, int value) {
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, key, value);
    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;
    ClientResult r = clientRequest(client, "PATCH", CLIENT_API_BASE "/player", reqBody, NULL);
    free(reqBody);
    return r;
}

ClientResult clientSetPlayerHealth(Client *client, int health) {
    return setPlayerInt(client, "health", health);
}

ClientResult clientSetPlayerPoints(Client *client, int points) {
    return setPlayerInt(client, "points", points);
}

ClientResult clientSetPlayerKills(Client *client, int kills) {
    return setPlayerInt(client, "kills", kills);
}

ClientResult clientSetPlayerHeadshots(Client *client, int headshots) {
    return setPlayerInt(client, "headshots", headshots);
}

ClientResult clientSetPlayerMovementSpeed(Client *client, int movementSpeed) {
    return setPlayerInt(client, "movement-speed", movementSpeed);
}

ClientResult clientGetPosition(Client *client, TeleportCoords *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/player/position", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    out->x = (float)jsonObjectGetNumber(body, "x", 0.0);
    out->y = (float)jsonObjectGetNumber(body, "y", 0.0);
    out->z = (float)jsonObjectGetNumber(body, "z", 0.0);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientTeleport(Client *client, float x, float y, float z) {
    char reqBody[128];
    snprintf(reqBody, sizeof(reqBody), "{\"x\":%g,\"y\":%g,\"z\":%g}", x, y, z);
    return clientRequest(client, "POST", CLIENT_API_BASE "/player/teleport", reqBody, NULL);
}

ClientResult clientGiveAmmo(Client *client) {
    return clientRequest(client, "POST", CLIENT_API_BASE "/player/ammo", NULL, NULL);
}

ClientResult clientTakeWeapons(Client *client) {
    return clientRequest(client, "DELETE", CLIENT_API_BASE "/player/weapons", NULL, NULL);
}

static const char *WEAPON_NAMES[] = {
    "cymbal-monkey", "black-hole", "nesting-dolls", "quantum-bomb", "m1911",
    "mustang-and-sally", "python", "cobra", "cz75", "calamity", "m14", "mnesia",
    "m16", "skullcrusher", "g11", "g115-generator", "famas", "g16-gl35", "ak74u",
    "ak74fu2", "mp5k", "mp115-kollider", "mp40", "the-afterburner", "mpl", "mpl-lf",
    "pm63", "tokyo-and-rose", "spectre", "phantom", "cz75-dual-wield",
    "calamity-and-jane", "stakeout", "raid", "olympia", "hades", "spas-12", "spaz-24",
    "hs10", "typhoid-and-mary", "aug", "aug-50m3", "galil", "lamentation", "commando",
    "predator", "fn-fal", "epc-wn", "dragunov", "d115-disassembler", "l96a1",
    "l115-isolator", "rpk", "r115-resonator", "hk21", "h115-oscilator", "m72-law",
    "m72-anarchy", "china-lake", "china-beach", "ray-gun", "porters-x2-ray-gun",
    "thundergun", "zeus-cannon", "crossbow", "awful-lawton", "ballistic-knife",
    "krauss-refibrillator", "ballistic-knife-bowie", "krauss-refibrillator-bowie",
};

static const int WEAPON_NAMES_SIZE = (int)(sizeof(WEAPON_NAMES) / sizeof(WEAPON_NAMES[0]));

bool clientWeaponFromName(const char *name, Weapon *out) {
    if (!name || !out) return false;
    for (int i = 0; i < WEAPON_NAMES_SIZE; i++) {
        if (strcmp(WEAPON_NAMES[i], name) == 0) {
            *out = (Weapon)i;
            return true;
        }
    }
    return false;
}

static const char *weaponApiName(Weapon weapon) {
    if ((int)weapon < 0 || (int)weapon >= WEAPON_NAMES_SIZE) return NULL;
    return WEAPON_NAMES[(int)weapon];
}

ClientResult clientGiveWeapons(Client *client, const Weapon *weapons, int count) {
    if (!weapons || count <= 0) return CLIENT_ERR_INVALID_PARAM;

    JsonValue *obj = jsonNewObject();
    JsonValue *arr = jsonNewArray();
    for (int i = 0; i < count; i++) {
        const char *name = weaponApiName(weapons[i]);
        if (!name) { jsonFree(obj); jsonFree(arr); return CLIENT_ERR_NOT_FOUND; }
        jsonArrayAppend(arr, jsonNewString(name));
    }
    jsonObjectSet(obj, "weapons", arr);

    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;
    ClientResult r = clientRequest(client, "POST", CLIENT_API_BASE "/player/weapons", reqBody, NULL);
    free(reqBody);
    return r;
}

ClientResult clientGetPerkCount(Client *client, int *count) {
    if (!count) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/player/perks", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    *count = jsonObjectGetInt(body, "count", 0);
    jsonFree(body);
    return CLIENT_OK;
}

static const char *PERK_NAMES[] = {
    "quick-revive", "juggernaut", "speed-cola", "double-tap", "stamina-up", "mule-kick",
};

static const int PERK_NAMES_SIZE = (int)(sizeof(PERK_NAMES) / sizeof(PERK_NAMES[0]));

bool clientPerkFromName(const char *name, Perk *out) {
    if (!name || !out) return false;
    for (int i = 0; i < PERK_NAMES_SIZE; i++) {
        if (strcmp(PERK_NAMES[i], name) == 0) {
            *out = (Perk)i;
            return true;
        }
    }
    return false;
}

static const char *perkApiName(Perk perk) {
    if ((int)perk < 0 || (int)perk >= PERK_NAMES_SIZE) return NULL;
    return PERK_NAMES[(int)perk];
}

static ClientResult modifyPerks(Client *client, const char *action, const Perk *perks, int count) {
    if (!perks || count <= 0) return CLIENT_ERR_INVALID_PARAM;

    JsonValue *obj = jsonNewObject();
    jsonObjectSetString(obj, "action", action);
    JsonValue *arr = jsonNewArray();
    for (int i = 0; i < count; i++) {
        const char *name = perkApiName(perks[i]);
        if (!name) { jsonFree(obj); jsonFree(arr); return CLIENT_ERR_NOT_FOUND; }
        jsonArrayAppend(arr, jsonNewString(name));
    }
    jsonObjectSet(obj, "perks", arr);

    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;
    ClientResult r = clientRequest(client, "POST", CLIENT_API_BASE "/player/perks", reqBody, NULL);
    free(reqBody);
    return r;
}

ClientResult clientAddPerks(Client *client, const Perk *perks, int count) {
    return modifyPerks(client, "add", perks, count);
}

ClientResult clientRemovePerks(Client *client, const Perk *perks, int count) {
    return modifyPerks(client, "remove", perks, count);
}
