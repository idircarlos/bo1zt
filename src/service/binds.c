#include "service/binds.h"
#include "service/service_internal.h"
#include "controller.h"
#include "logic/bind/manager.h"

ServiceResult serviceBindsGet(Service *service, BindsConfig *configOut) {
    if (!service || !configOut) return SERVICE_INVALID_PARAM;
    *configOut = controllerGetBindsConfig(service->controller);
    return SERVICE_OK;
}

ServiceResult serviceBindsSet(Service *service, BindsConfig *config) {
    if (!service || !config) return SERVICE_INVALID_PARAM;
    if (config->bindCount < 0 || config->bindCount > MAX_BINDS) return SERVICE_INVALID_PARAM;
    controllerUpdateBindsConfig(service->controller, config);
    // Reload the live bind runtime so new binds fire immediately.
    bindManagerReload(controllerGetBindManager(service->controller));
    return SERVICE_OK;
}

ServiceResult serviceBindsReset(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    controllerResetBindsConfig(service->controller);
    bindManagerReload(controllerGetBindManager(service->controller));
    return SERVICE_OK;
}
