#ifndef TWITCH_EVENTSUB_H_
#define TWITCH_EVENTSUB_H_

#include "twitch.h"

typedef struct TwitchEventSub TwitchEventSub;

typedef enum {
    TWITCH_EVENT_MESSAGE,
    TWITCH_EVENT_RAID,
} TwitchEventType;

typedef struct {
    char login[64];
    char displayName[64];
    char color[8]; // "#RRGGBB", empty when the chatter never picked one
} TwitchChatter;

typedef struct {
    TwitchEventType type;
    TwitchChatter chatter;
    char text[512];
    int viewerCount;
} TwitchEvent;

typedef void (*TwitchEventHandler)(const TwitchEvent *event, void *context);
TwitchEventSub *twitchEventSubConnect(TwitchClient *client, const char *channel);
void twitchEventSubShutdown(TwitchEventSub *session);
void twitchEventSubDisconnect(TwitchEventSub *session);
TwitchResult twitchEventSubPoll(TwitchEventSub *session, TwitchEventHandler handler, void *context);
TwitchResult twitchEventSubSendMessage(TwitchEventSub *session, const char *text);

#endif // TWITCH_EVENTSUB_H_
