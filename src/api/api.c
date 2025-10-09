#include "api.h"
#include "../offset/offset.h"
#include "../controller/controller.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct  Api {
    Controller *controller;
};

Api *apiCreate(Controller *controller) {
    Api *api = (Api*)malloc(sizeof(Api));
    if (!api) return NULL;
    api->controller = controller;
    return api;
}

bool apiGetGodMode(Api *api) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    uint32_t value = 0;
    bool success = memoryRead(ph, CHEAT_OFFSET_GOD_MODE, &value, sizeof(value));
    if (!success) {
        printf("Failed to read God Mode value\n");
        return false;
    }
    return value == CHEAT_VALUE_GOD_MODE_ON;
}

bool apiSetGodMode(Api *api, bool enabled) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    bool isInvisibleModeChecked = controllerIsCheckboxChecked(api->controller, CHEAT_INVISIBLE);

    uint32_t value = enabled ? CHEAT_VALUE_GOD_MODE_ON : (isInvisibleModeChecked ? CHEAT_VALUE_INVISIBLE_ON : CHEAT_VALUE_GOD_MODE_OFF); // Restore back Invisible if it was enabled in the GUI, otherwise disable God Mode.
    return memoryWrite(ph, CHEAT_OFFSET_GOD_MODE, &value, sizeof(value));
}

bool apiGetNoClip(Api *api) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    uint32_t value = 0;
    bool success = memoryRead(ph, CHEAT_OFFSET_NO_CLIP, &value, sizeof(value));
    if (!success) {
        printf("Failed to read No Clip value\n");
        return false;
    }
    return value == CHEAT_VALUE_NO_CLIP_ON;
}

bool apiSetNoClip(Api *api, bool enabled) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    uint32_t value = enabled ? CHEAT_VALUE_NO_CLIP_ON : CHEAT_VALUE_NO_CLIP_OFF;
    return memoryWrite(ph, CHEAT_OFFSET_NO_CLIP, &value, sizeof(value));
}

bool apiGetInvisible(Api *api) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    uint32_t value = 0;
    bool success = memoryRead(ph, CHEAT_OFFSET_INVISIBLE, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Invisible value\n");
        return false;
    }
    return value == CHEAT_VALUE_INVISIBLE_ON;
}

bool apiSetInvisible(Api *api, bool enabled) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    bool isGodModeChecked = controllerIsCheckboxChecked(api->controller, CHEAT_GOD_MODE);
    
    uint32_t value = enabled ? CHEAT_VALUE_INVISIBLE_ON : (isGodModeChecked ? CHEAT_VALUE_GOD_MODE_ON : CHEAT_VALUE_GOD_MODE_OFF); // Restore back God Mode if it was enabled in the GUI.
    return memoryWrite(ph, CHEAT_OFFSET_INVISIBLE, &value, sizeof(value));
}
