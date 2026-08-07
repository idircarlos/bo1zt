#ifndef SERVICE_TWITCH_H_
#define SERVICE_TWITCH_H_

#include "service.h"
#include "logic/config.h"
#include "logic/twitch/manager.h"

typedef struct {
    bool hasShowChat;      bool showChat;
    bool hasSendChat;      bool sendChat;
    bool hasAnnounceRaids; bool announceRaids;
} TwitchOptionsPatch;

ServiceResult serviceTwitchGetConnection(Service *service, TwitchConnection *out);
ServiceResult serviceTwitchConnect(Service *service, const char *clientId);
ServiceResult serviceTwitchDisconnect(Service *service);

ServiceResult serviceTwitchGetOptions(Service *service, TwitchConfig *out);
ServiceResult serviceTwitchUpdateOptions(Service *service, const TwitchOptionsPatch *patch);

const char *serviceTwitchStateName(TwitchConnectionState state);

#endif // SERVICE_TWITCH_H_
