#include "logic/twitch/manager.h"
#include "logic/twitch/color.h"
#include "logic/server.h"
#include "controller.h"
#include "controller/controller_internal.h"
#include "twitch.h"
#include "twitch/auth.h"
#include "twitch/eventsub.h"
#include "win/thread.h"
#include "logger.h"

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SESSION_JOIN_TIMEOUT_MS 5000
#define SEND_CHAT_MESSAGE_SIZE 512
#define GAME_CHAT_LINE_SIZE 200
#define GAME_CHAT_NAME_SIZE 80

#define GAME_CHAT_USER_FORMAT SERVER_CHAT_COLOR_FORMAT "%s" SERVER_CHAT_COLOR_FORMAT ": " // "^%c%s^%c: " 

struct TwitchManager {
    CRITICAL_SECTION lock;
    Controller *controller;
    TwitchClient *client;
    TwitchEventSub *session;
    Thread *sessionThread;
    TwitchConnection connection;
    bool needsAuthorization;
    volatile bool working;
    volatile bool stopping;
};

typedef struct {
    TwitchManager *manager;
    char text[SEND_CHAT_MESSAGE_SIZE];
} SendChatMessageArgs;

static void setState(TwitchManager *manager, TwitchConnectionState state) {
    EnterCriticalSection(&manager->lock);
    manager->connection.state = state;
    LeaveCriticalSection(&manager->lock);
}

static void failWith(TwitchManager *manager, const char *message) {
    LOG_ERROR("Twitch: %s", message ? message : "connection failed");
    EnterCriticalSection(&manager->lock);
    manager->connection.state = TWITCH_CONNECTION_DISCONNECTED;
    snprintf(manager->connection.error, sizeof(manager->connection.error), "%s",
             message ? message : "connection failed");
    LeaveCriticalSection(&manager->lock);
}

static void onTokensChanged(TwitchClient *client, void *context) {
    TwitchManager *manager = (TwitchManager *)context;
    if (!twitchAuthSave(client)) {
        LOG_ERROR("Twitch: the authorization could not be stored");
        return;
    }
    EnterCriticalSection(&manager->lock);
    manager->connection.authorized = true;
    snprintf(manager->connection.clientId, sizeof(manager->connection.clientId), "%s",
             twitchClientId(client));
    LeaveCriticalSection(&manager->lock);
}

// Chatters control this text and it ends up inside a quoted server command.
static void sanitize(char *text) {
    for (char *c = text; *c; c++) {
        if (*c == '"') *c = '\'';
        if (*c == '\n' || *c == '\r' || *c == ';') *c = ' ';
    }
}

// The name is expected to be sanitized already, its color code must survive.
static void sendChatLines(Server *server, const char *name, const char *text) {
    char line[GAME_CHAT_LINE_SIZE];
    while (*text) {
        size_t prefix = strlen(name);
        size_t room = sizeof(line) - prefix - 1;
        size_t taken = strlen(text);
        if (taken > room) taken = room;
        snprintf(line, sizeof(line), "%s%.*s", name, (int)taken, text);
        sanitize(line + prefix);
        serverChatMessage(server, line);
        text += taken;
        name = "";
    }
}

static void showChatMessage(TwitchManager *manager, const TwitchEvent *event) {
    char chatter[GAME_CHAT_NAME_SIZE];
    snprintf(chatter, sizeof(chatter), "%s", event->chatter.displayName);
    sanitize(chatter);

    char name[GAME_CHAT_NAME_SIZE];
    snprintf(name, sizeof(name), GAME_CHAT_USER_FORMAT,
             twitchColorToChatColor(event->chatter.color), chatter, CHAT_COLOR_WHITE);
    sendChatLines(_controllerGetServer(manager->controller), name, event->text);
}

static void announceRaid(TwitchManager *manager, const TwitchEvent *event) {
    char line[GAME_CHAT_LINE_SIZE];
    snprintf(line, sizeof(line), SERVER_BO1ZT_MSG_PREFIX "%s is raiding with %d viewers!",
             event->chatter.displayName, event->viewerCount);
    sanitize(line);
    serverCenterMessage(_controllerGetServer(manager->controller), line);
}

// Whatever we forward to Twitch comes back through our own chat subscription.
static bool isOwnChatter(TwitchManager *manager, const char *login) {
    EnterCriticalSection(&manager->lock);
    bool own = manager->connection.login[0] && strcmp(login, manager->connection.login) == 0;
    LeaveCriticalSection(&manager->lock);
    return own;
}

static void onEvent(const TwitchEvent *event, void *context) {
    TwitchManager *manager = (TwitchManager *)context;
    if (event->type == TWITCH_EVENT_RAID) {
        LOG_INFO("Twitch: %s raids with %d viewers", event->chatter.displayName,
                 event->viewerCount);
    } else {
        LOG_DEBUG("Twitch: %s: %s", event->chatter.displayName, event->text);
    }

    if (!controllerIsGameAttached(manager->controller) ||
        !controllerIsGameReady(manager->controller)) return;

    TwitchConfig config = controllerGetTwitchConfig(manager->controller);
    if (event->type == TWITCH_EVENT_RAID) {
        if (config.announceRaids) announceRaid(manager, event);
        return;
    }
    if (config.showChat && !(config.sendChat && isOwnChatter(manager, event->chatter.login))) {
        showChatMessage(manager, event);
    }
}

// Returns false when a disconnect came in while sleeping.
static bool sleepInterruptibly(TwitchManager *manager, int seconds) {
    for (int i = 0; i < seconds; i++) {
        if (manager->stopping) return false;
        threadSleep(1000);
    }
    return !manager->stopping;
}

static void publishUserCode(TwitchManager *manager, const TwitchAuthFlow *flow) {
    EnterCriticalSection(&manager->lock);
    manager->connection.state = TWITCH_CONNECTION_AWAITING_AUTHORIZATION;
    snprintf(manager->connection.userCode, sizeof(manager->connection.userCode), "%s",
             flow->userCode);
    snprintf(manager->connection.verificationUri, sizeof(manager->connection.verificationUri),
             "%s", flow->verificationUri);
    LeaveCriticalSection(&manager->lock);
}

static bool pollUntilAuthorized(TwitchManager *manager, const TwitchAuthFlow *flow) {
    for (int waited = 0; waited < flow->expiresIn; waited += flow->interval) {
        if (!sleepInterruptibly(manager, flow->interval)) return false;

        TwitchResult r = twitchAuthPoll(manager->client, flow);
        if (r == TWITCH_OK) return true;
        if (r != TWITCH_PENDING) {
            failWith(manager, twitchLastError(manager->client));
            return false;
        }
    }
    failWith(manager, "the code expired before you authorized bo1zt");
    return false;
}

static bool authorize(TwitchManager *manager) {
    TwitchAuthFlow flow;
    if (twitchAuthStart(manager->client, &flow) != TWITCH_OK) {
        failWith(manager, twitchLastError(manager->client));
        return false;
    }

    publishUserCode(manager, &flow);
    ShellExecuteA(NULL, "open", flow.verificationUri, NULL, NULL, SW_SHOWNORMAL);
    return pollUntilAuthorized(manager, &flow);
}

static bool validateToken(TwitchManager *manager) {
    setState(manager, TWITCH_CONNECTION_CONNECTING);
    if (twitchValidateToken(manager->client) == TWITCH_OK) return true;
    failWith(manager, twitchLastError(manager->client));
    return false;
}

static bool openSession(TwitchManager *manager) {
    if (manager->needsAuthorization && !authorize(manager)) return false;
    if (manager->stopping) return false;

    if (!validateToken(manager)) {
        if (manager->needsAuthorization) return false;
        LOG_INFO("Twitch: the stored authorization no longer works, asking for a new one");
        if (!authorize(manager) || !validateToken(manager)) return false;
    }

    TwitchUser owner;
    if (twitchGetUser(manager->client, twitchLogin(manager->client), &owner) != TWITCH_OK) {
        failWith(manager, twitchLastError(manager->client));
        return false;
    }

    TwitchEventSub *session = twitchEventSubConnect(manager->client, owner.login);
    if (!session) {
        failWith(manager, twitchLastError(manager->client));
        return false;
    }

    EnterCriticalSection(&manager->lock);
    manager->session = session;
    manager->connection.state = TWITCH_CONNECTION_CONNECTED;
    manager->connection.error[0] = '\0';
    manager->connection.userCode[0] = '\0';
    snprintf(manager->connection.login, sizeof(manager->connection.login), "%s", owner.login);
    snprintf(manager->connection.displayName, sizeof(manager->connection.displayName), "%s",
             owner.displayName[0] ? owner.displayName : owner.login);
    snprintf(manager->connection.profileImageUrl, sizeof(manager->connection.profileImageUrl),
             "%s", owner.profileImageUrl);
    LeaveCriticalSection(&manager->lock);
    LOG_INFO("Twitch: listening to #%s", owner.login);
    return true;
}

static void closeSession(TwitchManager *manager) {
    EnterCriticalSection(&manager->lock);
    TwitchEventSub *session = manager->session;
    manager->session = NULL;
    manager->connection.state = TWITCH_CONNECTION_DISCONNECTED;
    manager->connection.login[0] = '\0';
    manager->connection.displayName[0] = '\0';
    manager->connection.profileImageUrl[0] = '\0';
    manager->connection.userCode[0] = '\0';
    manager->connection.verificationUri[0] = '\0';
    LeaveCriticalSection(&manager->lock);

    twitchEventSubDisconnect(session);
}

static int sessionThreadProc(void *data) {
    TwitchManager *manager = (TwitchManager *)data;
    if (openSession(manager)) {
        while (!manager->stopping &&
               twitchEventSubPoll(manager->session, onEvent, manager) == TWITCH_OK) {}
        if (!manager->stopping) failWith(manager, twitchLastError(manager->client));
    }
    closeSession(manager);
    manager->working = false;
    return 0;
}

TwitchManager *twitchManagerCreate(Controller *controller) {
    TwitchManager *manager = (TwitchManager *)calloc(1, sizeof(TwitchManager));
    if (!manager) {
        LOG_ERROR("Couldn't allocate TwitchManager");
        return NULL;
    }
    InitializeCriticalSection(&manager->lock);
    manager->controller = controller;

    manager->client = twitchAuthLoad();
    if (manager->client) {
        twitchSetTokenHandler(manager->client, onTokensChanged, manager);
        manager->connection.authorized = true;
        snprintf(manager->connection.clientId, sizeof(manager->connection.clientId), "%s",
                 twitchClientId(manager->client));
    }
    return manager;
}

static void joinSessionThread(TwitchManager *manager) {
    if (!manager->sessionThread) return;
    threadWait(manager->sessionThread, SESSION_JOIN_TIMEOUT_MS);
    threadClose(manager->sessionThread);
    manager->sessionThread = NULL;
}

void twitchManagerDestroy(TwitchManager *manager) {
    if (!manager) return;
    twitchManagerDisconnect(manager);
    joinSessionThread(manager);
    twitchDestroy(manager->client);
    DeleteCriticalSection(&manager->lock);
    free(manager);
}

TwitchManagerResult twitchManagerConnect(TwitchManager *manager, const char *clientId) {
    if (!manager) return TWITCH_MANAGER_INVALID;

    EnterCriticalSection(&manager->lock);
    if (manager->working) {
        LeaveCriticalSection(&manager->lock);
        return TWITCH_MANAGER_BUSY;
    }

    char target[sizeof(manager->connection.clientId)];
    snprintf(target, sizeof(target), "%s",
             clientId && clientId[0] ? clientId : manager->connection.clientId);
    if (!target[0]) {
        LeaveCriticalSection(&manager->lock);
        return TWITCH_MANAGER_INVALID;
    }

    bool stored = manager->connection.authorized &&
                  strcmp(target, manager->connection.clientId) == 0;
    if (!stored) {
        TwitchClient *client = twitchCreate(target, NULL);
        if (!client) {
            LOG_ERROR("Twitch: '%s' is not a usable Client-ID", target);
            LeaveCriticalSection(&manager->lock);
            return TWITCH_MANAGER_INVALID;
        }
        twitchDestroy(manager->client);
        manager->client = client;
        twitchSetTokenHandler(manager->client, onTokensChanged, manager);
        manager->connection.authorized = false;
    }

    manager->needsAuthorization = !stored;
    manager->stopping = false;
    manager->working = true;
    manager->connection.state = TWITCH_CONNECTION_CONNECTING;
    manager->connection.error[0] = '\0';
    manager->connection.userCode[0] = '\0';
    manager->connection.login[0] = '\0';
    manager->connection.displayName[0] = '\0';
    manager->connection.profileImageUrl[0] = '\0';
    snprintf(manager->connection.clientId, sizeof(manager->connection.clientId), "%s", target);
    LeaveCriticalSection(&manager->lock);

    joinSessionThread(manager);
    manager->sessionThread = threadCreate(sessionThreadProc, manager);
    if (!manager->sessionThread) {
        LOG_ERROR("Twitch: couldn't start the session thread");
        manager->working = false;
        setState(manager, TWITCH_CONNECTION_DISCONNECTED);
        return TWITCH_MANAGER_INVALID;
    }
    return TWITCH_MANAGER_OK;
}

void twitchManagerDisconnect(TwitchManager *manager) {
    if (!manager) return;
    EnterCriticalSection(&manager->lock);
    manager->stopping = true;
    manager->connection.state = TWITCH_CONNECTION_DISCONNECTED;
    manager->connection.error[0] = '\0';
    if (manager->session) twitchEventSubShutdown(manager->session);
    LeaveCriticalSection(&manager->lock);
}

void twitchManagerGetConnection(TwitchManager *manager, TwitchConnection *out) {
    if (!manager || !out) return;
    EnterCriticalSection(&manager->lock);
    *out = manager->connection;
    LeaveCriticalSection(&manager->lock);
}

static int sendChatMessageThreadProc(void *context) {
    SendChatMessageArgs *args = (SendChatMessageArgs *)context;
    TwitchManager *manager = args->manager;

    EnterCriticalSection(&manager->lock);
    TwitchEventSub *session = manager->session;
    LeaveCriticalSection(&manager->lock);

    if (session && twitchEventSubSendMessage(session, args->text) != TWITCH_OK) {
        LOG_ERROR("Twitch: %s", twitchLastError(manager->client));
    }
    free(args);
    return 0;
}

// Helix makes the sender wait, so callers never pay for it on their own thread.
void twitchManagerSendChatMessage(TwitchManager *manager, const char *message) {
    if (!manager || !message || !message[0]) return;
    if (!controllerGetTwitchConfig(manager->controller).sendChat) return;

    SendChatMessageArgs *args = (SendChatMessageArgs *)malloc(sizeof(SendChatMessageArgs));
    if (!args) return;
    args->manager = manager;
    snprintf(args->text, sizeof(args->text), "%s", message);

    Thread *thread = threadCreate(sendChatMessageThreadProc, args);
    threadClose(thread);
}
