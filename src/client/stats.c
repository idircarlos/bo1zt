#include "client/stats.h"
#include "client/client_internal.h"

#include <stdio.h>

ClientResult clientGetStats(Client *client, int round, Stats *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    char path[64];
    if (round > 0)
        snprintf(path, sizeof(path), CLIENT_API_BASE "/stats?round=%d", round);
    else
        snprintf(path, sizeof(path), CLIENT_API_BASE "/stats");

    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", path, NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    JsonValue *entities = jsonObjectGet(body, "entities");
    out->entitiesCurrent = jsonObjectGetInt(entities, "current", 0);
    out->entitiesMax = jsonObjectGetInt(entities, "max", 0);
    out->claymores = jsonObjectGetInt(body, "claymores", 0);
    out->revives = jsonObjectGetInt(body, "revives", 0);

    JsonValue *special = jsonObjectGet(body, "special-rounds");
    out->specialDogs = jsonObjectGetInt(special, "dogs", -1);
    out->specialMonkeys = jsonObjectGetInt(special, "monkeys", -1);
    out->specialThief = jsonObjectGetInt(special, "thief", -1);
    out->sph = jsonObjectGetNumber(body, "sph", 0.0);
    snprintf(out->nextSpecialRounds, sizeof(out->nextSpecialRounds), "%s",
             jsonObjectGetString(body, "next-special-rounds", ""));
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientGetSph(Client *client, int round, double *sphOut) {
    if (!sphOut) return CLIENT_ERR_INVALID_PARAM;
    char path[64];
    if (round > 0)
        snprintf(path, sizeof(path), CLIENT_API_BASE "/stats/sph?round=%d", round);
    else
        snprintf(path, sizeof(path), CLIENT_API_BASE "/stats/sph");

    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", path, NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;
    *sphOut = jsonObjectGetNumber(body, "sph", 0.0);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientGetClaymores(Client *client, int *claymoresOut) {
    if (!claymoresOut) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/stats/claymores", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;
    *claymoresOut = jsonObjectGetInt(body, "claymores", 0);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientGetEntities(Client *client, int *currentOut, int *maxOut) {
    if (!currentOut || !maxOut) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/stats/entities", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;
    *currentOut = jsonObjectGetInt(body, "current", 0);
    *maxOut = jsonObjectGetInt(body, "max", 0);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientGetRevives(Client *client, int *revivesOut) {
    if (!revivesOut) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/stats/revives", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;
    *revivesOut = jsonObjectGetInt(body, "revives", 0);
    jsonFree(body);
    return CLIENT_OK;
}
