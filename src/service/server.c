#include "service/server.h"
#include "service/service_internal.h"

ServiceResult serviceServerCommand(Service *service, const char *command) {
    if (!service || !command || !*command) return SERVICE_INVALID_PARAM;
    if (!controllerIsGameAttached(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    if (!controllerServerExecuteCommand(service->controller, command)) return SERVICE_ENGINE_FAILED;
    return SERVICE_OK;
}

ServiceResult serviceServerGetDvar(Service *service, const char *name, char **valueOut) {
    if (!service || !name || !*name || !valueOut) return SERVICE_INVALID_PARAM;
    if (!controllerIsGameAttached(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    char *value = controllerServerGetDvar(service->controller, name);
    if (!value) return SERVICE_ENGINE_FAILED;
    *valueOut = value;
    return SERVICE_OK;
}

ServiceResult serviceServerSetDvar(Service *service, const char *name, const char *value) {
    if (!service || !name || !*name || !value) return SERVICE_INVALID_PARAM;
    if (!controllerIsGameAttached(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    if (!controllerServerSetDvar(service->controller, name, value)) return SERVICE_ENGINE_FAILED;
    return SERVICE_OK;
}
