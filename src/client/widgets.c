#include "client/widgets.h"
#include "client/client_internal.h"

#include <stdlib.h>
#include <stdio.h>

static const char *WIDGET_NAMES[] = {
    "timer", "round-timer", "velocity", "cycle", "zombies", "entities",
};

static const int WIDGET_NAMES_SIZE = (int)(sizeof(WIDGET_NAMES) / sizeof(WIDGET_NAMES[0]));

int clientWidgetCount(void) {
    return WIDGET_NAMES_SIZE;
}

const char *clientWidgetNameAt(int index) {
    if (index < 0 || index >= WIDGET_NAMES_SIZE) return NULL;
    return WIDGET_NAMES[index];
}

ClientResult clientGetWidget(Client *client, const char *name, WidgetConfig *out) {
    if (!name || !out) return CLIENT_ERR_INVALID_PARAM;
    char path[64];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/widgets/%s", name);
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", path, NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    out->enabled = jsonObjectGetBool(body, "enabled", false);
    snprintf(out->font, sizeof(out->font), "%s", jsonObjectGetString(body, "font", ""));
    out->fontSize = jsonObjectGetInt(body, "font-size", 0);
    clientParseColor(jsonObjectGet(body, "text-color"), &out->textColor);
    out->hideOutsideGame = jsonObjectGetBool(body, "hide-outside-game", false);
    clientParseRect(jsonObjectGet(body, "rect"), &out->rect);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientSetWidget(Client *client, const char *name, const WidgetConfig *config) {
    if (!name || !config) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *obj = jsonNewObject();
    jsonObjectSetBool(obj, "enabled", config->enabled);
    jsonObjectSetString(obj, "font", config->font);
    jsonObjectSetInt(obj, "font-size", config->fontSize);
    jsonObjectSet(obj, "text-color", clientColorJson(config->textColor));
    jsonObjectSetBool(obj, "hide-outside-game", config->hideOutsideGame);
    jsonObjectSet(obj, "rect", clientRectJson(config->rect));

    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;

    char path[64];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/widgets/%s", name);
    ClientResult r = clientRequest(client, "PATCH", path, reqBody, NULL);
    free(reqBody);
    return r;
}

ClientResult clientResetWidget(Client *client, const char *name) {
    if (!name) return CLIENT_ERR_INVALID_PARAM;
    char path[64];
    snprintf(path, sizeof(path), CLIENT_API_BASE "/widgets/%s/reset", name);
    return clientRequest(client, "POST", path, NULL, NULL);
}
