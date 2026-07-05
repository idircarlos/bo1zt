#include "client/state.h"
#include "client/client_internal.h"

#include <stdio.h>

ClientResult clientGetState(Client *client, GameState *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/state", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    out->isGameAttached = jsonObjectGetBool(body, "is-game-attached", false);
    out->isZombiesGameOngoing = jsonObjectGetBool(body, "is-zombies-game-ongoing", false);
    out->isZombiesGamePaused = jsonObjectGetBool(body, "is-zombies-game-paused", false);
    out->gameResets = jsonObjectGetInt(body, "game-resets", 0);
    snprintf(out->level, sizeof(out->level), "%s", jsonObjectGetString(body, "level", ""));
    out->elapsed = jsonObjectGetInt(body, "elapsed", 0);
    out->movementSpeed = (float)jsonObjectGetNumber(body, "movement-speed", 0.0);
    out->round = jsonObjectGetInt(body, "round", 0);

    JsonValue *entities = jsonObjectGet(body, "entities");
    out->entitiesCurrent = jsonObjectGetInt(entities, "current", 0);
    out->entitiesMax = jsonObjectGetInt(entities, "max", 0);
    jsonFree(body);
    return CLIENT_OK;
}
