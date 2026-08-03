#ifndef TWITCH_AUTH_H_
#define TWITCH_AUTH_H_

#include "twitch.h"

#define TWITCH_SCOPES "user:read:chat user:write:chat"

typedef struct {
    char deviceCode[128];
    char userCode[16];
    char verificationUri[256];
    int interval;
    int expiresIn;
} TwitchAuthFlow;

TwitchResult twitchAuthStart(TwitchClient *client, TwitchAuthFlow *out);
TwitchResult twitchAuthPoll(TwitchClient *client, const TwitchAuthFlow *flow);
TwitchResult twitchAuthRefresh(TwitchClient *client);
bool twitchAuthSave(const TwitchClient *client);
TwitchClient *twitchAuthLoad(void);

#endif // TWITCH_AUTH_H_
