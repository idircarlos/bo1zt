#include "api.h"
#include "../cheat/cheat.h"
#include "../controller/controller.h"
#include "../logger/logger.h"
#include <string.h>
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
    bool success = memoryRead(ph, CHEAT_GOD_MODE.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read God Mode value\n");
        return false;
    }
    return value == CHEAT_GOD_MODE.on.u32;
}

bool apiSetGodMode(Api *api, bool enabled) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    bool isInvisibleModeChecked = controllerIsCheckboxChecked(api->controller, CHEAT_NAME_INVISIBLE);

    uint32_t value = enabled ? CHEAT_GOD_MODE.on.u32 : (isInvisibleModeChecked ? CHEAT_INVISIBLE.on.u32 : CHEAT_INVISIBLE.off.u32); // Restore back Invisible if it was enabled in the GUI, otherwise disable God Mode.
    return memoryWrite(ph, CHEAT_GOD_MODE.offset, &value, sizeof(value));
}

bool apiGetNoClip(Api *api) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    uint8_t value = 0;
    bool success = memoryRead(ph, CHEAT_NO_CLIP.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read No Clip value\n");
        return false;
    }
    return value == CHEAT_NO_CLIP.on.byte;
}

bool apiSetNoClip(Api *api, bool enabled) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    uint8_t value = enabled ? CHEAT_NO_CLIP.on.byte : CHEAT_NO_CLIP.off.byte;
    return memoryWrite(ph, CHEAT_NO_CLIP.offset, &value, sizeof(value));
}

bool apiGetInvisible(Api *api) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    uint32_t value = 0;
    bool success = memoryRead(ph, CHEAT_INVISIBLE.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Invisible value\n");
        return false;
    }
    return value == CHEAT_INVISIBLE.on.u32;
}

bool apiSetInvisible(Api *api, bool enabled) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    bool isGodModeChecked = controllerIsCheckboxChecked(api->controller, CHEAT_NAME_GOD_MODE);
    
    uint32_t value = enabled ? CHEAT_INVISIBLE.on.u32 : (isGodModeChecked ? CHEAT_GOD_MODE.on.u32 : CHEAT_GOD_MODE.off.u32); // Restore back God Mode if it was enabled in the GUI.
    return memoryWrite(ph, CHEAT_INVISIBLE.offset, &value, sizeof(value));
}

bool apiGetNoRecoil(Api *api) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    uint8_t value = 0;
    bool success = memoryRead(ph, CHEAT_NO_RECOIL.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read No Recoil value\n");
        return false;
    }
    return value == CHEAT_NO_RECOIL.on.byte;
}

bool apiSetNoRecoil(Api *api, bool enabled) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    uint8_t value = enabled ? CHEAT_NO_RECOIL.on.byte : CHEAT_NO_RECOIL.off.byte;
    return memoryWrite(ph, CHEAT_NO_RECOIL.offset, &value, sizeof(value));
}

bool apiGetInfiniteAmmo(Api *api) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    uint8_t buffer[MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE];
    bool success = memoryRead(ph, CHEAT_ASM_INSTRUCTION_INFINITE_AMMO.offset, &buffer, sizeof(buffer)); // Read all MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE bytes even if we need less. This is for a future comparation with Cheat On vs Cheat Off, which may differ in Bytes OpCode length.
    if (!success) {
        printf("Failed to read Infinite Ammo value\n");
        return false;
    }
    bool infiniteAmmoEnabled = memcmp(CHEAT_ASM_INSTRUCTION_INFINITE_AMMO.on.instructions, buffer, sizeof(CHEAT_ASM_INSTRUCTION_INFINITE_AMMO.on.size)) == 0; // Compare only with the first OpCode Bytes when Cheat is On.
    if (infiniteAmmoEnabled) return true;

    bool infiniteAmmoDisabled = memcmp(CHEAT_ASM_INSTRUCTION_INFINITE_AMMO.off.instructions, buffer, sizeof(CHEAT_ASM_INSTRUCTION_INFINITE_AMMO.off.size)) == 0; // Compare only with the first OpCode Bytes when Cheat is Off.
    if (!infiniteAmmoDisabled) {
        LOG_WARN("Infinite Ammo bytes do not match known patterns. Possible memory corruption or external modification.\n");
        return false;
    }
    return false;
}

bool apiSetInfiniteAmmo(Api *api, bool enabled) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    CheatAsm *instructionSet = &CHEAT_ASM_INSTRUCTION_INFINITE_AMMO;
    uint8_t *instructions = enabled ? instructionSet->on.instructions : instructionSet->off.instructions;
    size_t size = enabled ? instructionSet->on.size : instructionSet->off.size;
    return memoryWrite(ph, instructionSet->offset, instructions, size);
}

bool apiGetInstantKill(Api *api) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    uint8_t buffer[MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE];
    bool success = memoryRead(ph, CHEAT_ASM_INSTRUCTION_INSTANT_KILL.offset, &buffer, sizeof(buffer)); // Read all MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE bytes even if we need less. This is for a future comparation with Cheat On vs Cheat Off, which may differ in Bytes OpCode length.
    if (!success) {
        printf("Failed to read Instant Kill value\n");
        return false;
    }
    bool instantKillEnabled = memcmp(CHEAT_ASM_INSTRUCTION_INSTANT_KILL.on.instructions, buffer, sizeof(CHEAT_ASM_INSTRUCTION_INSTANT_KILL.on.size)) == 0; // Compare only with the first OpCode Bytes when Cheat is On.
    if (instantKillEnabled) return true;

    bool instantKillDisabled = memcmp(CHEAT_ASM_INSTRUCTION_INSTANT_KILL.off.instructions, buffer, sizeof(CHEAT_ASM_INSTRUCTION_INSTANT_KILL.off.size)) == 0; // Compare only with the first OpCode Bytes when Cheat is Off.
    if (!instantKillDisabled) {
        LOG_WARN("Instant Kill bytes do not match known patterns. Possible memory corruption or external modification.\n");
        return false;
    }
    return false;
}

bool apiSetInstantKill(Api *api, bool enabled) {
    if (!api || !api->controller) return false;
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) return false;

    CheatAsm *instructionSet = &CHEAT_ASM_INSTRUCTION_INSTANT_KILL;
    uint8_t *instructions = enabled ? instructionSet->on.instructions : instructionSet->off.instructions;
    size_t size = enabled ? instructionSet->on.size : instructionSet->off.size;
    return memoryWrite(ph, instructionSet->offset, instructions, size);
}