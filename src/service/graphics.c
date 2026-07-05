#include "service/graphics.h"
#include "service/service_internal.h"
#include "logic/cheat.h"
#include "logic/cheat/manager/actions.h"

static bool applyToggle(Controller *controller, CheatName cheat, bool enabled) {
    CheatResult r = cheatManagerSetToggle(controllerGetCheatManager(controller), cheat, enabled);
    return r != CHEAT_RESULT_API_FAILED;
}

static bool applyValue(Controller *controller, SimpleCheatName cheat, int value) {
    CheatResult r = cheatManagerSetValue(controllerGetCheatManager(controller), cheat, &value);
    return r != CHEAT_RESULT_API_FAILED;
}

ServiceResult serviceGraphicsGet(Service *service, GraphicsConfig *configOut) {
    if (!service || !configOut) return SERVICE_INVALID_PARAM;
    *configOut = controllerGetGraphicsConfig(service->controller);
    return SERVICE_OK;
}

ServiceResult serviceGraphicsReset(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    controllerResetConfig(service->controller, CONFIG_GRAPHICS);
    return SERVICE_OK;
}

ServiceResult serviceGraphicsUpdate(Service *service, const GraphicsPatch *patch) {
    if (!service || !patch) return SERVICE_INVALID_PARAM;
    Controller *c = service->controller;
    bool ok = true;

    if (patch->hasFov)        ok = applyValue(c, SIMPLE_CHEAT_NAME_FOV, patch->fov) && ok;
    if (patch->hasFovScale)   ok = applyValue(c, SIMPLE_CHEAT_NAME_FOV_SCALE, patch->fovScale) && ok;
    if (patch->hasFpsCap)     ok = applyValue(c, SIMPLE_CHEAT_NAME_FPS_CAP, patch->fpsCap) && ok;
    if (patch->hasBorderless) ok = applyToggle(c, CHEAT_NAME_MAKE_BORDERLESS, patch->borderless) && ok;
    if (patch->hasUnlimitFps) ok = applyToggle(c, CHEAT_NAME_UNLIMIT_FPS, patch->unlimitFps) && ok;
    if (patch->hasDisableHud) ok = applyToggle(c, CHEAT_NAME_DISABLE_HUD, patch->disableHud) && ok;
    if (patch->hasDisableFog) ok = applyToggle(c, CHEAT_NAME_DISABLE_FOG, patch->disableFog) && ok;
    if (patch->hasFullbright) ok = applyToggle(c, CHEAT_NAME_FULLBRIGHT, patch->fullbright) && ok;
    if (patch->hasColorized)  ok = applyToggle(c, CHEAT_NAME_COLORIZED, patch->colorized) && ok;

    return ok ? SERVICE_OK : SERVICE_ENGINE_FAILED;
}
