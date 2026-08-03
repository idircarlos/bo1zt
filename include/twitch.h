#ifndef TWITCH_H_
#define TWITCH_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct TwitchClient TwitchClient;

typedef enum {
    TWITCH_OK = 0,
    TWITCH_PENDING,
    TWITCH_ERR_INVALID_PARAM,
    TWITCH_ERR_UNREACHABLE,
    TWITCH_ERR_AUTH,
    TWITCH_ERR_HTTP,
    TWITCH_ERR_PROTOCOL,
} TwitchResult;

typedef struct {
    char id[32];
    char login[64];
    char displayName[64];
    char broadcasterType[16];
    char description[512];
} TwitchUser;

typedef struct {
    bool live;
    char id[32];
    char userLogin[64];
    char gameName[128];
    char title[256];
    int  viewerCount;
} TwitchStream;

typedef void (*TwitchTokenHandler)(TwitchClient *client, void *userData);
TwitchClient *twitchCreate(const char *clientId, const char *token);
void twitchDestroy(TwitchClient *client);
void twitchSetTokenHandler(TwitchClient *client, TwitchTokenHandler handler, void *userData);
const char *twitchClientId(const TwitchClient *client);
const char *twitchLogin(const TwitchClient *client);
const char *twitchLastError(const TwitchClient *client);
TwitchResult twitchValidateToken(TwitchClient *client);
TwitchResult twitchGetUser(TwitchClient *client, const char *login, TwitchUser *out);
TwitchResult twitchGetStream(TwitchClient *client, const char *login, TwitchStream *out);
TwitchResult twitchSendChatMessage(TwitchClient *client, const char *broadcasterId, const char *message);

#endif // TWITCH_H_
