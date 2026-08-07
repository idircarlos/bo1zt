#include "twitch/twitch_internal.h"
#include "twitch/auth.h"
#include "win/http.h"
#include "utils/json.h"
#include "logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void twitchSetError(TwitchClient *client, const char *message) {
    if (client) snprintf(client->error, sizeof(client->error), "%s", message ? message : "");
}

// Pasted credentials arrive padded with blanks or a CR that would corrupt the HTTP headers.
static void copyFirstWord(const char *in, char *out, size_t size) {
    while (*in && (unsigned char)*in <= ' ') in++;

    size_t length = 0;
    while ((unsigned char)in[length] > ' ' && length + 1 < size) length++;

    memcpy(out, in, length);
    out[length] = '\0';
}

static void stripOauthPrefix(char *token) {
    static const char PREFIX[] = "oauth:";
    size_t prefixLength = sizeof(PREFIX) - 1;
    if (strncmp(token, PREFIX, prefixLength) != 0) return;
    memmove(token, token + prefixLength, strlen(token + prefixLength) + 1);
}

TwitchClient *twitchCreate(const char *clientId, const char *token) {
    if (!clientId || !clientId[0]) return NULL;

    TwitchClient *client = (TwitchClient *)calloc(1, sizeof(TwitchClient));
    if (!client) return NULL;

    copyFirstWord(clientId, client->clientId, sizeof(client->clientId));
    if (token) {
        copyFirstWord(token, client->token, sizeof(client->token));
        stripOauthPrefix(client->token);
    }
    return client;
}

void twitchDestroy(TwitchClient *client) {
    if (client) free(client);
}

void twitchSetTokenHandler(TwitchClient *client, TwitchTokenHandler handler, void *context) {
    if (!client) return;
    client->listener.handler = handler;
    client->listener.context = context;
}

const char *twitchClientId(const TwitchClient *client) {
    return client ? client->clientId : "";
}

const char *twitchLogin(const TwitchClient *client) {
    return client ? client->login : "";
}

const char *twitchLastError(const TwitchClient *client) {
    return client ? client->error : "";
}

static TwitchResult resultFromStatus(int status) {
    if (status < 0) return TWITCH_ERR_UNREACHABLE;
    if (status >= 200 && status < 300) return TWITCH_OK;
    if (status == 401) return TWITCH_ERR_AUTH;
    return TWITCH_ERR_HTTP;
}

static void reportFailure(TwitchClient *client, TwitchResult result, const char *responseBody) {
    if (result == TWITCH_ERR_UNREACHABLE) {
        twitchSetError(client, "could not reach Twitch (transport/TLS failure)");
        return;
    }
    JsonValue *error = responseBody ? jsonParse(responseBody) : NULL;
    twitchSetError(client, jsonObjectGetString(error, "message", "request failed"));
    jsonFree(error);
}

TwitchResult twitchRequest(TwitchClient *client, const char *host, const char *method,
                           const char *path, const char *headers, const char *body,
                           JsonValue **out) {
    if (out) *out = NULL;
    if (!client) return TWITCH_ERR_INVALID_PARAM;

    HttpClientResponse resp = httpsClientRequest(host, TWITCH_PORT, method, path, headers, body);
    TwitchResult result = resultFromStatus(resp.status);

    if (result != TWITCH_OK) {
        reportFailure(client, result, resp.body);
    } else if (out && resp.body && resp.body[0] != '\0') {
        *out = jsonParse(resp.body);
        if (!*out) result = TWITCH_ERR_PROTOCOL;
    }

    httpClientResponseFree(&resp);
    return result;
}

static TwitchResult helixRequest(TwitchClient *client, const char *method, const char *path,
                                 const char *body, JsonValue **out) {
    char headers[768];
    snprintf(headers, sizeof(headers),
             "Authorization: Bearer %s\r\n"
             "Client-Id: %s\r\n"
             "%s",
             client->token, client->clientId,
             body ? "Content-Type: application/json\r\n" : "");
    return twitchRequest(client, TWITCH_API_HOST, method, path, headers, body, out);
}

static bool refreshedAfterRejection(TwitchClient *client, TwitchResult result, const char *reason) {
    if (result != TWITCH_ERR_AUTH || !client->refreshToken[0]) return false;
    LOG_INFO("Twitch: %s, refreshing the access token", reason);
    return twitchAuthRefresh(client) == TWITCH_OK;
}

TwitchResult twitchHelix(TwitchClient *client, const char *method, const char *path,
                         const char *body, JsonValue **out) {
    if (!client) return TWITCH_ERR_INVALID_PARAM;

    TwitchResult result = helixRequest(client, method, path, body, out);
    if (!refreshedAfterRejection(client, result, "Helix rejected the access token")) return result;

    return helixRequest(client, method, path, body, out);
}

static TwitchResult validateRequest(TwitchClient *client) {
    // Unlike Helix, this endpoint expects "OAuth <token>" instead of "Bearer <token>".
    char headers[640];
    snprintf(headers, sizeof(headers), "Authorization: OAuth %s\r\n", client->token);

    JsonValue *body = NULL;
    TwitchResult r = twitchRequest(client, TWITCH_ID_HOST, "GET", "/oauth2/validate",
                                   headers, NULL, &body);
    if (r != TWITCH_OK) return r;
    if (!body) return TWITCH_ERR_PROTOCOL;

    if (strcmp(jsonObjectGetString(body, "client_id", ""), client->clientId) != 0) {
        twitchSetError(client, "the token was issued for a different Client-ID");
        jsonFree(body);
        return TWITCH_ERR_AUTH;
    }

    snprintf(client->login, sizeof(client->login), "%s", jsonObjectGetString(body, "login", ""));
    snprintf(client->userId, sizeof(client->userId), "%s", jsonObjectGetString(body, "user_id", ""));
    jsonFree(body);

    if (!client->login[0] || !client->userId[0]) {
        twitchSetError(client, "the token is not a user access token");
        return TWITCH_ERR_AUTH;
    }
    return TWITCH_OK;
}

TwitchResult twitchValidateToken(TwitchClient *client) {
    if (!client) return TWITCH_ERR_INVALID_PARAM;

    TwitchResult result = validateRequest(client);
    if (!refreshedAfterRejection(client, result, "the stored access token is dead")) return result;

    return validateRequest(client);
}

TwitchResult twitchGetUser(TwitchClient *client, const char *login, TwitchUser *out) {
    if (!client || !login || !out) return TWITCH_ERR_INVALID_PARAM;
    memset(out, 0, sizeof(*out));

    char path[128];
    snprintf(path, sizeof(path), HELIX_BASE "/users?login=%s", login);

    JsonValue *body = NULL;
    TwitchResult r = twitchHelix(client, "GET", path, NULL, &body);
    if (r != TWITCH_OK) return r;
    if (!body) return TWITCH_ERR_PROTOCOL;

    JsonValue *data = jsonObjectGet(body, "data");
    if (jsonArrayCount(data) < 1) {
        twitchSetError(client, "user not found");
        jsonFree(body);
        return TWITCH_ERR_HTTP;
    }

    JsonValue *user = jsonArrayAt(data, 0);
    snprintf(out->id, sizeof(out->id), "%s", jsonObjectGetString(user, "id", ""));
    snprintf(out->login, sizeof(out->login), "%s", jsonObjectGetString(user, "login", ""));
    snprintf(out->displayName, sizeof(out->displayName), "%s", jsonObjectGetString(user, "display_name", ""));
    snprintf(out->broadcasterType, sizeof(out->broadcasterType), "%s", jsonObjectGetString(user, "broadcaster_type", ""));
    snprintf(out->description, sizeof(out->description), "%s", jsonObjectGetString(user, "description", ""));

    jsonFree(body);
    return TWITCH_OK;
}

TwitchResult twitchGetStream(TwitchClient *client, const char *login, TwitchStream *out) {
    if (!client || !login || !out) return TWITCH_ERR_INVALID_PARAM;
    memset(out, 0, sizeof(*out));

    char path[128];
    snprintf(path, sizeof(path), HELIX_BASE "/streams?user_login=%s", login);

    JsonValue *body = NULL;
    TwitchResult r = twitchHelix(client, "GET", path, NULL, &body);
    if (r != TWITCH_OK) return r;
    if (!body) return TWITCH_ERR_PROTOCOL;

    // An empty "data" array means the channel is offline.
    JsonValue *data = jsonObjectGet(body, "data");
    if (jsonArrayCount(data) < 1) {
        out->live = false;
        jsonFree(body);
        return TWITCH_OK;
    }

    JsonValue *stream = jsonArrayAt(data, 0);
    out->live = true;
    snprintf(out->id, sizeof(out->id), "%s", jsonObjectGetString(stream, "id", ""));
    snprintf(out->userLogin, sizeof(out->userLogin), "%s", jsonObjectGetString(stream, "user_login", ""));
    snprintf(out->gameName, sizeof(out->gameName), "%s", jsonObjectGetString(stream, "game_name", ""));
    snprintf(out->title, sizeof(out->title), "%s", jsonObjectGetString(stream, "title", ""));
    out->viewerCount = jsonObjectGetInt(stream, "viewer_count", 0);

    jsonFree(body);
    return TWITCH_OK;
}

TwitchResult twitchSendChatMessage(TwitchClient *client, const char *broadcasterId,
                                   const char *message) {
    if (!client || !broadcasterId || !message) return TWITCH_ERR_INVALID_PARAM;
    if (!client->userId[0]) {
        twitchSetError(client, "the token has not been validated yet");
        return TWITCH_ERR_INVALID_PARAM;
    }

    JsonValue *payload = jsonNewObject();
    jsonObjectSetString(payload, "broadcaster_id", broadcasterId);
    jsonObjectSetString(payload, "sender_id", client->userId);
    jsonObjectSetString(payload, "message", message);
    char *body = jsonSerialize(payload);
    jsonFree(payload);
    if (!body) return TWITCH_ERR_PROTOCOL;

    JsonValue *respBody = NULL;
    TwitchResult r = twitchHelix(client, "POST", HELIX_BASE "/chat/messages", body, &respBody);
    free(body);
    jsonFree(respBody);
    return r;
}
