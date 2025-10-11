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

bool _apiGetGodMode(ProcessHandle *ph);
bool _apiSetGodMode(ProcessHandle *ph, Controller *controller, bool enabled);

bool _apiGetInvisible(ProcessHandle *ph);
bool _apiSetInvisible(ProcessHandle *ph, Controller *controller, bool enabled);

bool _apiGetNoClip(ProcessHandle *ph);
bool _apiSetNoClip(ProcessHandle *ph, bool enabled);

bool _apiGetNoRecoil(ProcessHandle *ph);
bool _apiSetNoRecoil(ProcessHandle *ph, bool enabled);

bool _apiGetBoxNeverMoves(ProcessHandle *ph);
bool _apiSetBoxNeverMoves(ProcessHandle *ph, bool enabled);

bool _apiGetThirdPerson(ProcessHandle *ph);
bool _apiSetThirdPerson(ProcessHandle *ph, bool enabled);

bool _apiGetInfiniteAmmo(ProcessHandle *ph);
bool _apiSetInfiniteAmmo(ProcessHandle *ph, bool enabled);

bool _apiGetInstantKill(ProcessHandle *phi);
bool _apiSetInstantKill(ProcessHandle *ph, bool enabled);

Api *apiCreate(Controller *controller) {
    Api *api = (Api*)malloc(sizeof(Api));
    if (!api) return NULL;
    api->controller = controller;
    return api;
}

bool apiIsCheatEnabled(Api *api, CheatName cheat) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return false;
    }

    switch (cheat) {
        case CHEAT_NAME_GOD_MODE:
            return _apiGetGodMode(ph);
        case CHEAT_NAME_INVISIBLE:
            return _apiGetInvisible(ph);
        case CHEAT_NAME_NO_CLIP:
            return _apiGetNoClip(ph);
        case CHEAT_NAME_NO_RECOIL:
            return _apiGetNoRecoil(ph);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return _apiGetBoxNeverMoves(ph);
        case CHEAT_NAME_THIRD_PERSON:
            return _apiGetThirdPerson(ph);
        case CHEAT_NAME_INFINITE_AMMO:
            return _apiGetInfiniteAmmo(ph);
        case CHEAT_NAME_INSTANT_KILL:
            return _apiGetInstantKill(ph);
        default:
            LOG_WARN("Unkwown cheat %d\n", cheat);
            return false;
    }
}

bool apiSetCheatEnabled(Api *api, CheatName cheat, bool enabled) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return false;
    }

    switch (cheat) {
        case CHEAT_NAME_GOD_MODE:
            return _apiSetGodMode(ph, api->controller, enabled);
        case CHEAT_NAME_INVISIBLE:
            return _apiSetInvisible(ph, api->controller, enabled);
        case CHEAT_NAME_NO_CLIP:
            return _apiSetNoClip(ph, enabled);
        case CHEAT_NAME_NO_RECOIL:
            return _apiSetNoRecoil(ph, enabled);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return _apiSetBoxNeverMoves(ph, enabled);
        case CHEAT_NAME_THIRD_PERSON:
            return _apiSetThirdPerson(ph, enabled);
        case CHEAT_NAME_INFINITE_AMMO:
            return _apiSetInfiniteAmmo(ph, enabled);
        case CHEAT_NAME_INSTANT_KILL:
            return _apiSetInstantKill(ph, enabled);
        default:
            LOG_WARN("Unkwown cheat %d\n", cheat);
            return false;
    }
}

bool _apiGetGodMode(ProcessHandle *ph) {
    uint32_t value = 0;
    bool success = memoryRead(ph, CHEAT_GOD_MODE.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read God Mode value\n");
        return false;
    }
    return value == CHEAT_GOD_MODE.on.u32;
}

bool _apiSetGodMode(ProcessHandle *ph, Controller *controller, bool enabled) {
    bool isInvisibleModeChecked = controllerIsCheckboxChecked(controller, CHEAT_NAME_INVISIBLE);
    uint32_t value = enabled ? CHEAT_GOD_MODE.on.u32 : (isInvisibleModeChecked ? CHEAT_INVISIBLE.on.u32 : CHEAT_INVISIBLE.off.u32); // Restore back Invisible if it was enabled in the GUI, otherwise disable God Mode.
    return memoryWrite(ph, CHEAT_GOD_MODE.offset, &value, sizeof(value));
}

bool _apiGetInvisible(ProcessHandle *ph) {
    uint32_t value = 0;
    bool success = memoryRead(ph, CHEAT_INVISIBLE.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Invisible value\n");
        return false;
    }
    return value == CHEAT_INVISIBLE.on.u32;
}

bool _apiSetInvisible(ProcessHandle *ph, Controller *controller, bool enabled) {
    bool isGodModeChecked = controllerIsCheckboxChecked(controller, CHEAT_NAME_GOD_MODE);
    uint32_t value = enabled ? CHEAT_INVISIBLE.on.u32 : (isGodModeChecked ? CHEAT_GOD_MODE.on.u32 : CHEAT_GOD_MODE.off.u32); // Restore back God Mode if it was enabled in the GUI.
    return memoryWrite(ph, CHEAT_INVISIBLE.offset, &value, sizeof(value));
}

bool _apiGetNoClip(ProcessHandle *ph) {
    uint8_t value = 0;
    bool success = memoryRead(ph, CHEAT_NO_CLIP.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read No Clip value\n");
        return false;
    }
    return value == CHEAT_NO_CLIP.on.byte;
}

bool _apiSetNoClip(ProcessHandle *ph, bool enabled) {
    uint8_t value = enabled ? CHEAT_NO_CLIP.on.byte : CHEAT_NO_CLIP.off.byte;
    return memoryWrite(ph, CHEAT_NO_CLIP.offset, &value, sizeof(value));
}

bool _apiGetNoRecoil(ProcessHandle *ph) {
    uint8_t value = 0;
    bool success = memoryRead(ph, CHEAT_NO_RECOIL.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read No Recoil value\n");
        return false;
    }
    return value == CHEAT_NO_RECOIL.on.byte;
}

bool _apiSetNoRecoil(ProcessHandle *ph, bool enabled) {
    uint8_t value = enabled ? CHEAT_NO_RECOIL.on.byte : CHEAT_NO_RECOIL.off.byte;
    return memoryWrite(ph, CHEAT_NO_RECOIL.offset, &value, sizeof(value));
}

bool _apiGetBoxNeverMoves(ProcessHandle *ph) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_BOX_NEVER_MOVES.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Box Never Moves address\n");
        return false;
    }
    uint8_t value = 0;
    success = memoryRead(ph, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Box Never Moves value\n");
        return false;
    }
    return value == CHEAT_BOX_NEVER_MOVES.on.u32;
}

bool _apiSetBoxNeverMoves(ProcessHandle *ph, bool enabled) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_BOX_NEVER_MOVES.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Box Never Moves address\n");
        return false;
    }
    uint8_t value = enabled ? CHEAT_BOX_NEVER_MOVES.on.u32 : CHEAT_BOX_NEVER_MOVES.off.u32;
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(value));
}

bool _apiGetThirdPerson(ProcessHandle *ph) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_THIRD_PERSON.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Third Person address\n");
        return false;
    }
    uint8_t value = 0;
    success = memoryRead(ph, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Third Person value\n");
        return false;
    }
    return value == CHEAT_THIRD_PERSON.on.byte;
}

bool _apiSetThirdPerson(ProcessHandle *ph, bool enabled) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_THIRD_PERSON.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Third Person address\n");
        return false;
    }
    uint8_t value = enabled ? CHEAT_THIRD_PERSON.on.byte : CHEAT_THIRD_PERSON.off.byte;
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(value));
}

bool _apiGetInfiniteAmmo(ProcessHandle *ph) {
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

bool _apiSetInfiniteAmmo(ProcessHandle *ph, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_INSTRUCTION_INFINITE_AMMO;
    uint8_t *instructions = enabled ? instructionSet->on.instructions : instructionSet->off.instructions;
    size_t size = enabled ? instructionSet->on.size : instructionSet->off.size;
    return memoryWrite(ph, instructionSet->offset, instructions, size);
}

bool _apiGetInstantKill(ProcessHandle *ph) {
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

bool _apiSetInstantKill(ProcessHandle *ph, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_INSTRUCTION_INSTANT_KILL;
    uint8_t *instructions = enabled ? instructionSet->on.instructions : instructionSet->off.instructions;
    size_t size = enabled ? instructionSet->on.size : instructionSet->off.size;
    return memoryWrite(ph, instructionSet->offset, instructions, size);
}
