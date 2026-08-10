#ifndef TWITCH_MANAGER_H_
#define TWITCH_MANAGER_H_

#include <stdbool.h>

typedef struct Controller Controller;
typedef struct TwitchManager TwitchManager;

typedef enum {
    TWITCH_CONNECTION_DISCONNECTED,
    TWITCH_CONNECTION_AWAITING_AUTHORIZATION,
    TWITCH_CONNECTION_CONNECTING,
    TWITCH_CONNECTION_CONNECTED,
} TwitchConnectionState;

typedef struct {
    TwitchConnectionState state;
    bool authorized;
    char clientId[128];
    char login[64];
    char displayName[64];
    char profileImageUrl[256];
    char userCode[16];
    char verificationUri[256];
    char error[256];
} TwitchConnection;

typedef enum {
    TWITCH_MANAGER_OK,
    TWITCH_MANAGER_INVALID,
    TWITCH_MANAGER_BUSY,
} TwitchManagerResult;

TwitchManager *twitchManagerCreate(Controller *controller);
void twitchManagerDestroy(TwitchManager *manager);
TwitchManagerResult twitchManagerConnect(TwitchManager *manager, const char *clientId);
void twitchManagerDisconnect(TwitchManager *manager);
void twitchManagerGetConnection(TwitchManager *manager, TwitchConnection *out);
void twitchManagerSendChatMessage(TwitchManager *manager, const char *message);

#endif // TWITCH_MANAGER_H_
