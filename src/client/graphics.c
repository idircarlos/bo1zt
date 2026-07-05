#include "client/graphics.h"
#include "client/client_internal.h"

#include <stdlib.h>

static void parseGraphics(const JsonValue *obj, GraphicsConfig *g) {
    g->fov        = jsonObjectGetInt(obj, "fov", 0);
    g->fovScale   = jsonObjectGetInt(obj, "fov-scale", 0);
    g->fpsCap     = jsonObjectGetInt(obj, "fps-cap", 0);
    g->borderless = jsonObjectGetBool(obj, "borderless", false);
    g->unlimitFps = jsonObjectGetBool(obj, "unlimit-fps", false);
    g->disableHud = jsonObjectGetBool(obj, "disable-hud", false);
    g->disableFog = jsonObjectGetBool(obj, "disable-fog", false);
    g->fullbright = jsonObjectGetBool(obj, "fullbright", false);
    g->colorized  = jsonObjectGetBool(obj, "colorized", false);
}

ClientResult clientGetGraphics(Client *client, GraphicsConfig *out) {
    if (!out) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/graphics", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;
    parseGraphics(body, out);
    jsonFree(body);
    return CLIENT_OK;
}

ClientResult clientSetGraphics(Client *client, const GraphicsConfig *config) {
    if (!config) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "fov", config->fov);
    jsonObjectSetInt(obj, "fov-scale", config->fovScale);
    jsonObjectSetInt(obj, "fps-cap", config->fpsCap);
    jsonObjectSetBool(obj, "borderless", config->borderless);
    jsonObjectSetBool(obj, "unlimit-fps", config->unlimitFps);
    jsonObjectSetBool(obj, "disable-hud", config->disableHud);
    jsonObjectSetBool(obj, "disable-fog", config->disableFog);
    jsonObjectSetBool(obj, "fullbright", config->fullbright);
    jsonObjectSetBool(obj, "colorized", config->colorized);

    char *reqBody = jsonSerialize(obj);
    jsonFree(obj);
    if (!reqBody) return CLIENT_ERR_PROTOCOL;
    ClientResult r = clientRequest(client, "PATCH", CLIENT_API_BASE "/graphics", reqBody, NULL);
    free(reqBody);
    return r;
}

ClientResult clientResetGraphics(Client *client) {
    return clientRequest(client, "POST", CLIENT_API_BASE "/graphics/reset", NULL, NULL);
}
