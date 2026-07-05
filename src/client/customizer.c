#include "client/customizer.h"
#include "client/client_internal.h"

#include <stdlib.h>

static void parseCustomizer(const JsonValue *obj, CustomizerConfig *c) {
    clientParseColor(jsonObjectGet(obj, "score-background"), &c->scoreBackground);
    clientParseColor(jsonObjectGet(obj, "score-player-1"), &c->scorePlayer1);
    clientParseColor(jsonObjectGet(obj, "score-player-2"), &c->scorePlayer2);
    clientParseColor(jsonObjectGet(obj, "score-player-3"), &c->scorePlayer3);
    clientParseColor(jsonObjectGet(obj, "score-player-4"), &c->scorePlayer4);
    clientParseColor(jsonObjectGet(obj, "reload-warn-primary"), &c->reloadWarnPrimary);
    clientParseColor(jsonObjectGet(obj, "reload-warn-secondary"), &c->reloadWarnSecondary);
    clientParseColor(jsonObjectGet(obj, "low-ammo-warn-primary"), &c->lowAmmoWarnPrimary);
    clientParseColor(jsonObjectGet(obj, "low-ammo-warn-secondary"), &c->lowAmmoWarnSecondary);
    clientParseColor(jsonObjectGet(obj, "no-ammo-warn-primary"), &c->noAmmoWarnPrimary);
    clientParseColor(jsonObjectGet(obj, "no-ammo-warn-secondary"), &c->noAmmoWarnSecondary);
    c->scoreboardTransparency = jsonObjectGetInt(obj, "scoreboard-transparency", 0);
    c->pointsTransparency = jsonObjectGetInt(obj, "points-transparency", 0);
    c->warningTransitionsFrequency = jsonObjectGetInt(obj, "warning-frequency", 0);
    c->warningTransitionsMin = jsonObjectGetInt(obj, "warning-min", 0);
    c->warningTransitionsMax = jsonObjectGetInt(obj, "warning-max", 0);
}

ClientResult clientGetCustomizer(Client *client, CustomizerConfig *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/customizer", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;
    parseCustomizer(body, out);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientSetCustomizer(Client *client, const CustomizerConfig *config) {
    if (!config) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *obj = jsonNewObject();
    jsonObjectSet(obj, "score-background", clientColorJson(config->scoreBackground));
    jsonObjectSet(obj, "score-player-1", clientColorJson(config->scorePlayer1));
    jsonObjectSet(obj, "score-player-2", clientColorJson(config->scorePlayer2));
    jsonObjectSet(obj, "score-player-3", clientColorJson(config->scorePlayer3));
    jsonObjectSet(obj, "score-player-4", clientColorJson(config->scorePlayer4));
    jsonObjectSet(obj, "reload-warn-primary", clientColorJson(config->reloadWarnPrimary));
    jsonObjectSet(obj, "reload-warn-secondary", clientColorJson(config->reloadWarnSecondary));
    jsonObjectSet(obj, "low-ammo-warn-primary", clientColorJson(config->lowAmmoWarnPrimary));
    jsonObjectSet(obj, "low-ammo-warn-secondary", clientColorJson(config->lowAmmoWarnSecondary));
    jsonObjectSet(obj, "no-ammo-warn-primary", clientColorJson(config->noAmmoWarnPrimary));
    jsonObjectSet(obj, "no-ammo-warn-secondary", clientColorJson(config->noAmmoWarnSecondary));
    jsonObjectSetInt(obj, "scoreboard-transparency", config->scoreboardTransparency);
    jsonObjectSetInt(obj, "points-transparency", config->pointsTransparency);
    jsonObjectSetInt(obj, "warning-frequency", config->warningTransitionsFrequency);
    jsonObjectSetInt(obj, "warning-min", config->warningTransitionsMin);
    jsonObjectSetInt(obj, "warning-max", config->warningTransitionsMax);

    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;
    ClientResult r = clientRequest(client, "PATCH", CLIENT_API_BASE "/customizer", reqBody, NULL);
    free(reqBody);
    return r;
}

ClientResult clientResetCustomizer(Client *client) {
    return clientRequest(client, "POST", CLIENT_API_BASE "/customizer/reset", NULL, NULL);
}
