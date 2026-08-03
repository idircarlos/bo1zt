#include "twitch/eventsub.h"
#include "twitch/twitch_internal.h"
#include "win/websocket.h"
#include "utils/json.h"
#include "logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define EVENTSUB_HOST "eventsub.wss.twitch.tv"
#define EVENTSUB_PORT 443
#define EVENTSUB_PATH "/ws?keepalive_timeout_seconds=30"

#define EVENTSUB_KEEPALIVE_MARGIN_MS 40000
#define EVENTSUB_FRAME_SIZE (16 * 1024)

struct TwitchEventSub {
    TwitchClient *client;
    WebSocket *socket;
    char channel[64];
    char broadcasterId[32];
    char sessionId[64];
};

static void setError(TwitchEventSub *session, const char *message) {
    twitchSetError(session->client, message);
}

static void copyChannelName(const char *in, char *out, size_t size) {
    while (*in && (unsigned char)*in <= ' ') in++;
    if (*in == '#') in++;

    size_t length = 0;
    while ((unsigned char)in[length] > ' ' && length + 1 < size) {
        out[length] = (char)tolower((unsigned char)in[length]);
        length++;
    }
    out[length] = '\0';
}

static bool splitUrl(const char *url, char *host, size_t hostSize, char *path, size_t pathSize) {
    if (!url) return false;

    const char *scheme = strstr(url, "://");
    const char *authority = scheme ? scheme + 3 : url;
    const char *pathStart = strchr(authority, '/');

    size_t hostLength = pathStart ? (size_t)(pathStart - authority) : strlen(authority);
    if (hostLength == 0 || hostLength >= hostSize) return false;

    memcpy(host, authority, hostLength);
    host[hostLength] = '\0';
    snprintf(path, pathSize, "%s", pathStart ? pathStart : "/");
    return true;
}

static TwitchResult receiveFrame(TwitchEventSub *session, WebSocket *socket, char *frame, int size) {
    switch (webSocketReceiveText(socket, frame, size, NULL)) {
        case WEBSOCKET_MESSAGE:
            LOG_TRACE("EventSub < %s", frame);
            return TWITCH_OK;
        case WEBSOCKET_TIMEOUT:
            setError(session, "Twitch stopped sending keepalives");
            return TWITCH_ERR_UNREACHABLE;
        case WEBSOCKET_CLOSED:
            setError(session, "connection closed by Twitch");
            return TWITCH_ERR_UNREACHABLE;
        default:
            setError(session, "receive failed");
            return TWITCH_ERR_UNREACHABLE;
    }
}

static TwitchResult awaitWelcome(TwitchEventSub *session, WebSocket *socket) {
    char frame[EVENTSUB_FRAME_SIZE];
    for (;;) {
        TwitchResult r = receiveFrame(session, socket, frame, (int)sizeof(frame));
        if (r != TWITCH_OK) return r;

        JsonValue *root = jsonParse(frame);
        if (!root) {
            setError(session, "malformed EventSub frame");
            return TWITCH_ERR_PROTOCOL;
        }

        const char *type = jsonObjectGetString(jsonObjectGet(root, "metadata"), "message_type", "");
        if (strcmp(type, "session_welcome") != 0) {
            jsonFree(root);
            continue;
        }

        JsonValue *info = jsonObjectGet(jsonObjectGet(root, "payload"), "session");
        snprintf(session->sessionId, sizeof(session->sessionId), "%s",
                 jsonObjectGetString(info, "id", ""));
        jsonFree(root);

        if (!session->sessionId[0]) {
            setError(session, "the welcome carried no session id");
            return TWITCH_ERR_PROTOCOL;
        }
        return TWITCH_OK;
    }
}

// Takes ownership of condition.
static TwitchResult subscribe(TwitchEventSub *session, const char *type, JsonValue *condition) {
    JsonValue *transport = jsonNewObject();
    jsonObjectSetString(transport, "method", "websocket");
    jsonObjectSetString(transport, "session_id", session->sessionId);

    JsonValue *payload = jsonNewObject();
    jsonObjectSetString(payload, "type", type);
    jsonObjectSetString(payload, "version", "1");
    jsonObjectSet(payload, "condition", condition);
    jsonObjectSet(payload, "transport", transport);

    char *body = jsonSerialize(payload);
    jsonFree(payload);
    if (!body) return TWITCH_ERR_PROTOCOL;

    JsonValue *response = NULL;
    TwitchResult r = twitchHelix(session->client, "POST", HELIX_BASE "/eventsub/subscriptions",
                                 body, &response);
    free(body);
    jsonFree(response);

    if (r != TWITCH_OK) {
        char message[320];
        snprintf(message, sizeof(message), "%s: %s", type, twitchLastError(session->client));
        setError(session, message);
    }
    return r;
}

static TwitchResult subscribeAll(TwitchEventSub *session) {
    JsonValue *chat = jsonNewObject();
    jsonObjectSetString(chat, "broadcaster_user_id", session->broadcasterId);
    jsonObjectSetString(chat, "user_id", session->client->userId);
    TwitchResult r = subscribe(session, "channel.chat.message", chat);
    if (r != TWITCH_OK) return r;

    JsonValue *raid = jsonNewObject();
    jsonObjectSetString(raid, "to_broadcaster_user_id", session->broadcasterId);
    return subscribe(session, "channel.raid", raid);
}

static TwitchResult reconnect(TwitchEventSub *session, const char *url) {
    char host[128];
    char path[512];
    if (!splitUrl(url, host, sizeof(host), path, sizeof(path))) {
        setError(session, "malformed reconnect URL");
        return TWITCH_ERR_PROTOCOL;
    }

    WebSocket *socket = webSocketConnect(host, EVENTSUB_PORT, path, EVENTSUB_KEEPALIVE_MARGIN_MS);
    if (!socket) {
        setError(session, "could not follow the EventSub reconnect");
        return TWITCH_ERR_UNREACHABLE;
    }

    TwitchResult r = awaitWelcome(session, socket);
    if (r != TWITCH_OK) {
        webSocketClose(socket);
        return r;
    }

    webSocketClose(session->socket);
    session->socket = socket;
    return TWITCH_OK;
}

static void readChatter(const JsonValue *json, const char *loginKey, const char *nameKey,
                        TwitchChatter *out) {
    snprintf(out->login, sizeof(out->login), "%s", jsonObjectGetString(json, loginKey, ""));
    snprintf(out->displayName, sizeof(out->displayName), "%s",
             jsonObjectGetString(json, nameKey, out->login));
}

static bool readEvent(const char *subscriptionType, const JsonValue *json, TwitchEvent *out) {
    memset(out, 0, sizeof(*out));

    if (strcmp(subscriptionType, "channel.chat.message") == 0) {
        out->type = TWITCH_EVENT_MESSAGE;
        readChatter(json, "chatter_user_login", "chatter_user_name", &out->chatter);
        snprintf(out->chatter.color, sizeof(out->chatter.color), "%s",
                 jsonObjectGetString(json, "color", ""));
        snprintf(out->text, sizeof(out->text), "%s",
                 jsonObjectGetString(jsonObjectGet(json, "message"), "text", ""));
        return true;
    }

    if (strcmp(subscriptionType, "channel.raid") == 0) {
        out->type = TWITCH_EVENT_RAID;
        readChatter(json, "from_broadcaster_user_login", "from_broadcaster_user_name",
                    &out->chatter);
        out->viewerCount = jsonObjectGetInt(json, "viewers", 0);
        return true;
    }

    return false;
}

static TwitchResult dispatch(TwitchEventSub *session, const char *frame,
                             TwitchEventHandler handler, void *userData) {
    JsonValue *root = jsonParse(frame);
    if (!root) {
        setError(session, "malformed EventSub frame");
        return TWITCH_ERR_PROTOCOL;
    }

    JsonValue *metadata = jsonObjectGet(root, "metadata");
    JsonValue *payload = jsonObjectGet(root, "payload");
    const char *messageType = jsonObjectGetString(metadata, "message_type", "");

    TwitchResult result = TWITCH_OK;
    if (strcmp(messageType, "notification") == 0) {
        TwitchEvent event;
        if (readEvent(jsonObjectGetString(metadata, "subscription_type", ""),
                      jsonObjectGet(payload, "event"), &event)) {
            handler(&event, userData);
        }
    } else if (strcmp(messageType, "session_reconnect") == 0) {
        JsonValue *info = jsonObjectGet(payload, "session");
        result = reconnect(session, jsonObjectGetString(info, "reconnect_url", NULL));
    } else if (strcmp(messageType, "revocation") == 0) {
        JsonValue *subscription = jsonObjectGet(payload, "subscription");
        setError(session, jsonObjectGetString(subscription, "status", "subscription revoked"));
        result = TWITCH_ERR_AUTH;
    }

    jsonFree(root);
    return result;
}

TwitchEventSub *twitchEventSubConnect(TwitchClient *client, const char *channel) {
    if (!client || !channel) return NULL;
    if (!client->userId[0]) {
        twitchSetError(client, "the token has not been validated yet");
        return NULL;
    }

    TwitchEventSub *session = (TwitchEventSub *)calloc(1, sizeof(TwitchEventSub));
    if (!session) return NULL;
    session->client = client;

    copyChannelName(channel, session->channel, sizeof(session->channel));
    if (!session->channel[0]) {
        setError(session, "empty channel name");
        free(session);
        return NULL;
    }

    TwitchUser broadcaster;
    if (twitchGetUser(client, session->channel, &broadcaster) != TWITCH_OK) {
        free(session);
        return NULL;
    }
    snprintf(session->broadcasterId, sizeof(session->broadcasterId), "%s", broadcaster.id);

    session->socket = webSocketConnect(EVENTSUB_HOST, EVENTSUB_PORT, EVENTSUB_PATH,
                                       EVENTSUB_KEEPALIVE_MARGIN_MS);
    if (!session->socket) {
        setError(session, "could not open the EventSub WebSocket");
        free(session);
        return NULL;
    }

    if (awaitWelcome(session, session->socket) != TWITCH_OK ||
        subscribeAll(session) != TWITCH_OK) {
        LOG_ERROR("EventSub: %s", twitchLastError(client));
        twitchEventSubDisconnect(session);
        return NULL;
    }
    return session;
}

void twitchEventSubShutdown(TwitchEventSub *session) {
    if (!session) return;
    webSocketShutdown(session->socket);
}

void twitchEventSubDisconnect(TwitchEventSub *session) {
    if (!session) return;
    webSocketClose(session->socket);
    free(session);
}

TwitchResult twitchEventSubPoll(TwitchEventSub *session, TwitchEventHandler handler,
                                void *userData) {
    if (!session || !handler) return TWITCH_ERR_INVALID_PARAM;

    char frame[EVENTSUB_FRAME_SIZE];
    TwitchResult r = receiveFrame(session, session->socket, frame, (int)sizeof(frame));
    if (r != TWITCH_OK) return r;

    return dispatch(session, frame, handler, userData);
}

TwitchResult twitchEventSubSendMessage(TwitchEventSub *session, const char *text) {
    if (!session || !text) return TWITCH_ERR_INVALID_PARAM;
    return twitchSendChatMessage(session->client, session->broadcasterId, text);
}
