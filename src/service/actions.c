#include "service/actions.h"
#include "service/service_internal.h"

ServiceResult serviceActionsMusic(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (!controllerIsGameAttached(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    if (!controllerPlayEasterEggSong(service->controller)) return SERVICE_ENGINE_FAILED;
    return SERVICE_OK;
}
