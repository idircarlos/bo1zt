#include "client/client_internal.h"
#include "win/http.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

Client *clientCreate(int port) {
    Client *client = (Client *)calloc(1, sizeof(Client));
    if (!client) return NULL;
    client->port = port;
    return client;
}

void clientDestroy(Client *client) {
    if (client) free(client);
}

const char *clientLastErrorCode(const Client *client) {
    return client ? client->errorCode : "";
}

const char *clientLastErrorMessage(const Client *client) {
    return client ? client->errorMessage : "";
}

void clientClearError(Client *client) {
    if (!client) return;
    client->errorCode[0] = '\0';
    client->errorMessage[0] = '\0';
}

// ---------------------------------------------------------------------------
// Request helper
// ---------------------------------------------------------------------------

static ClientResult resultFromStatus(int status) {
    if (status < 0) return CLIENT_ERR_UNREACHABLE;
    if (status >= 200 && status < 300) return CLIENT_OK;
    switch (status) {
        case 400: return CLIENT_ERR_INVALID_PARAM;
        case 404: return CLIENT_ERR_NOT_FOUND;
        case 409: return CLIENT_ERR_CONFLICT;
        default:  return CLIENT_ERR_ENGINE; // 500 and any other non-2xx
    }
}

static void captureError(Client *client, const char *body) {
    if (!body || !*body) return;
    JsonValue *parsed = jsonParse(body);
    if (!parsed) return;
    JsonValue *error = jsonObjectGet(parsed, "error");
    if (error) {
        const char *code = jsonObjectGetString(error, "code", "");
        const char *message = jsonObjectGetString(error, "message", "");
        snprintf(client->errorCode, sizeof(client->errorCode), "%s", code);
        snprintf(client->errorMessage, sizeof(client->errorMessage), "%s", message);
    }
    jsonFree(parsed);
}

ClientResult clientRequest(Client *client, const char *method, const char *path,
                           const char *body, JsonValue **out) {
    if (out) *out = NULL;
    if (!client) return CLIENT_ERR_INVALID_PARAM;
    clientClearError(client);

    HttpClientResponse resp = httpClientRequest("127.0.0.1", client->port, method, path,
                                                "Content-Type: application/json\r\n", body);
    ClientResult result = resultFromStatus(resp.status);

    if (result == CLIENT_OK) {
        if (out && resp.body && resp.body[0] != '\0') {
            JsonValue *parsed = jsonParse(resp.body);
            if (!parsed) result = CLIENT_ERR_PROTOCOL;
            else *out = parsed;
        }
    } else if (result != CLIENT_ERR_UNREACHABLE) {
        captureError(client, resp.body);
    }

    httpClientResponseFree(&resp);
    return result;
}

// ---------------------------------------------------------------------------
// Health
// ---------------------------------------------------------------------------

ClientResult clientGetVersion(Client *client, char *out, size_t size) {
    if (!out || size == 0) return CLIENT_ERR_INVALID_PARAM;
    JsonValue *body = NULL;
    ClientResult r = clientRequest(client, "GET", CLIENT_API_BASE "/health", NULL, &body);
    if (r != CLIENT_OK) return r;
    if (!body) return CLIENT_ERR_PROTOCOL;

    const char *version = jsonObjectGetString(body, "version", NULL);
    if (!version) { jsonFree(body); return CLIENT_ERR_PROTOCOL; }
    snprintf(out, size, "%s", version);
    jsonFree(body);
    return CLIENT_OK;
}

// ---------------------------------------------------------------------------
// Shared color / rect (de)serialization (matches src/service.c)
// ---------------------------------------------------------------------------

JsonValue *clientColorJson(RGBAColor color) {
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "r", color.r);
    jsonObjectSetInt(obj, "g", color.g);
    jsonObjectSetInt(obj, "b", color.b);
    jsonObjectSetInt(obj, "a", color.a);
    return obj;
}

JsonValue *clientRectJson(Rect rect) {
    JsonValue *obj = jsonNewObject();
    jsonObjectSetInt(obj, "x", (long long)rect.x);
    jsonObjectSetInt(obj, "y", (long long)rect.y);
    jsonObjectSetInt(obj, "w", (long long)rect.w);
    jsonObjectSetInt(obj, "h", (long long)rect.h);
    return obj;
}

bool clientParseColor(const JsonValue *obj, RGBAColor *out) {
    if (!out || jsonTypeOf(obj) != JSON_OBJECT) return false;
    out->r = (uint8_t)jsonObjectGetInt(obj, "r", 0);
    out->g = (uint8_t)jsonObjectGetInt(obj, "g", 0);
    out->b = (uint8_t)jsonObjectGetInt(obj, "b", 0);
    out->a = (uint8_t)jsonObjectGetInt(obj, "a", 0);
    return true;
}

bool clientParseRect(const JsonValue *obj, Rect *out) {
    if (!out || jsonTypeOf(obj) != JSON_OBJECT) return false;
    out->x = (uint32_t)jsonObjectGetInt(obj, "x", 0);
    out->y = (uint32_t)jsonObjectGetInt(obj, "y", 0);
    out->w = (uint32_t)jsonObjectGetInt(obj, "w", 0);
    out->h = (uint32_t)jsonObjectGetInt(obj, "h", 0);
    return true;
}
