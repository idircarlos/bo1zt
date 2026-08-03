#ifndef SERVICE_TWITCH_H_
#define SERVICE_TWITCH_H_

#include "service.h"
#include "logic/twitch/manager.h"

ServiceResult serviceTwitchGetConnection(Service *service, TwitchConnection *out);
ServiceResult serviceTwitchConnect(Service *service, const char *clientId);
ServiceResult serviceTwitchDisconnect(Service *service);

const char *serviceTwitchStateName(TwitchConnectionState state);

#endif // SERVICE_TWITCH_H_
