#ifndef TWITCH_INTERNAL_H_
#define TWITCH_INTERNAL_H_

#include "twitch.h"
#include "utils/json.h"

#define TWITCH_API_HOST "api.twitch.tv"
#define TWITCH_ID_HOST  "id.twitch.tv"
#define TWITCH_PORT     443

#define HELIX_BASE "/helix"

typedef struct {
    TwitchTokenHandler handler;
    void *context;
} TwitchListener;

struct TwitchClient {
    char clientId[128];
    char token[512];
    char refreshToken[512];
    char login[64];
    char userId[32];
    char error[256];
    TwitchListener listener;
};

void twitchSetError(TwitchClient *client, const char *message);
TwitchResult twitchRequest(TwitchClient *client, const char *host, const char *method, const char *path, const char *headers, const char *body, JsonValue **out);
TwitchResult twitchHelix(TwitchClient *client, const char *method, const char *path, const char *body, JsonValue **out);

#endif // TWITCH_INTERNAL_H_
