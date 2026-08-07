#ifndef CLIENT_TWITCH_H_
#define CLIENT_TWITCH_H_

#include "client.h"

typedef enum {
    CLIENT_TWITCH_DISCONNECTED,
    CLIENT_TWITCH_AWAITING_AUTHORIZATION,
    CLIENT_TWITCH_CONNECTING,
    CLIENT_TWITCH_CONNECTED,
} ClientTwitchState;

typedef struct {
    ClientTwitchState state;
    bool authorized;
    char clientId[128];
    char login[64];
    char displayName[64];
    char userCode[16];
    char verificationUri[256];
    char error[256];
} ClientTwitchConnection;

typedef struct {
    bool showChat;
    bool sendChat;
    bool announceRaids;
} ClientTwitchOptions;

ClientResult clientGetTwitchConnection(Client *client, ClientTwitchConnection *out);
ClientResult clientTwitchConnect(Client *client, const char *clientId);
ClientResult clientTwitchDisconnect(Client *client);

ClientResult clientGetTwitchOptions(Client *client, ClientTwitchOptions *out);
ClientResult clientTwitchSetOption(Client *client, const char *option, bool enabled);

#endif // CLIENT_TWITCH_H_
