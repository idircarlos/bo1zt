#include "service/twitch.h"
#include "service/service_internal.h"
#include "controller.h"

static const char *STATE_NAMES[] = {
    "disconnected",
    "awaiting-authorization",
    "connecting",
    "connected",
};

const char *serviceTwitchStateName(TwitchConnectionState state) {
    if (state < 0 || state >= (int)(sizeof(STATE_NAMES) / sizeof(STATE_NAMES[0]))) {
        return STATE_NAMES[TWITCH_CONNECTION_DISCONNECTED];
    }
    return STATE_NAMES[state];
}

ServiceResult serviceTwitchGetConnection(Service *service, TwitchConnection *out) {
    if (!service || !out) return SERVICE_INVALID_PARAM;
    TwitchManager *manager = controllerGetTwitchManager(service->controller);
    if (!manager) return SERVICE_ENGINE_FAILED;
    twitchManagerGetConnection(manager, out);
    return SERVICE_OK;
}

ServiceResult serviceTwitchConnect(Service *service, const char *clientId) {
    if (!service) return SERVICE_INVALID_PARAM;
    TwitchManager *manager = controllerGetTwitchManager(service->controller);
    if (!manager) return SERVICE_ENGINE_FAILED;
    switch (twitchManagerConnect(manager, clientId)) {
        case TWITCH_MANAGER_OK:   return SERVICE_OK;
        case TWITCH_MANAGER_BUSY: return SERVICE_IN_USE;
        default:                  return SERVICE_INVALID_PARAM;
    }
}

ServiceResult serviceTwitchDisconnect(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    TwitchManager *manager = controllerGetTwitchManager(service->controller);
    if (!manager) return SERVICE_ENGINE_FAILED;
    twitchManagerDisconnect(manager);
    return SERVICE_OK;
}
