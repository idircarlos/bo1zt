#include "api.h"
#include "../cheat/cheat.h"
#include "../controller/controller.h"
#include "../logger/logger.h"
#include "../map/map.h"
#include "../hook/hook.h"
#include "../memory/memory.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Api {
    Controller *controller;
    Map *hooks;
};

bool _apiGetGodMode(ProcessHandle *ph);
bool _apiSetGodMode(ProcessHandle *ph, Controller *controller, bool enabled);

bool _apiGetInvisible(ProcessHandle *ph);
bool _apiSetInvisible(ProcessHandle *ph, Controller *controller, bool enabled);

bool _apiGetNoClip(ProcessHandle *ph);
bool _apiSetNoClip(ProcessHandle *ph, bool enabled);

bool _apiGetNoRecoil(ProcessHandle *ph);
bool _apiSetNoRecoil(ProcessHandle *ph, bool enabled);

bool _apiGetSmallCrosshair(ProcessHandle *ph);
bool _apiSetSmallCrosshair(ProcessHandle *ph, bool enabled);

bool _apiGetFastGameplay(ProcessHandle *ph);
bool _apiSetFastGameplay(ProcessHandle *ph, bool enabled);

bool _apiGetNoShellshock(ProcessHandle *ph);
bool _apiSetNoShellshock(ProcessHandle *ph, bool enabled);

bool _apiGetIncreaseKnifeRange(ProcessHandle *ph);
bool _apiSetIncreaseKnifeRange(ProcessHandle *ph, bool enabled);

bool _apiGetBoxNeverMoves(ProcessHandle *ph);
bool _apiSetBoxNeverMoves(ProcessHandle *ph, bool enabled);

bool _apiGetThirdPerson(ProcessHandle *ph);
bool _apiSetThirdPerson(ProcessHandle *ph, bool enabled);

bool _apiGetInfiniteAmmo(ProcessHandle *ph);
bool _apiSetInfiniteAmmo(ProcessHandle *ph, bool enabled);

bool _apiGetInstantKill(Map *hooks);
bool _apiSetInstantKill(ProcessHandle *ph, Map *hooks, bool enabled);

bool _apiGetMakeBorderless(ProcessHandle *ph);
bool _apiSetMakeBorderless(ProcessHandle *ph, bool enabled);

bool _apiGetUnlimitFps(ProcessHandle *ph);
bool _apiSetUnlimitFps(ProcessHandle *ph, Controller *controller, bool enabled);

bool _apiGetDisableHud(ProcessHandle *ph);
bool _apiSetDisableHud(ProcessHandle *ph, bool enabled);

bool _apiGetDisableFog(ProcessHandle *ph);
bool _apiSetDisableFog(ProcessHandle *ph, bool enabled);

bool _apiGetFullbright(ProcessHandle *ph);
bool _apiSetFullbright(ProcessHandle *ph, bool enabled);

bool _apiGetColorized(ProcessHandle *ph);
bool _apiSetColorized(ProcessHandle *ph, bool enabled);

bool _apiChangeName(ProcessHandle *ph, char *name);
bool _apiSetSpeed(ProcessHandle *ph, uint32_t value);
bool _apiGiveWeaponAmmo(ProcessHandle *ph, Weapon weapon);
bool _apiTeleport(ProcessHandle *ph, TeleportCoords value);
bool _apiFov(ProcessHandle *ph, float value);
bool _apiFovScale(ProcessHandle *ph, float value);
bool _apiFpsCap(ProcessHandle *ph, uint32_t value);
bool _apiSetSimpleCheatIntValue(ProcessHandle *ph, SimpleCheatName simpleCheatName, uint32_t value);



// Hooks IDs (Hash for Hook Map)
static const char* HOOK_INSTANT_KILL_ID = "HOOK_INSTANT_KILL";

Api *apiCreate(Controller *controller) {
    Api *api = (Api*)malloc(sizeof(Api));
    if (!api) return NULL;
    api->controller = controller;
    api->hooks = mapCreate();
    if (!api->hooks) {
        LOG_ERROR("Couldn't create Hook Map\n");
    }
    return api;
}

bool apiIsCheatEnabled(Api *api, CheatName cheatName) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return false;
    }

    switch (cheatName) {
        case CHEAT_NAME_GOD_MODE:
            return _apiGetGodMode(ph);
        case CHEAT_NAME_INVISIBLE:
            return _apiGetInvisible(ph);
        case CHEAT_NAME_NO_CLIP:
            return _apiGetNoClip(ph);
        case CHEAT_NAME_NO_RECOIL:
            return _apiGetNoRecoil(ph);
        case CHEAT_NAME_SMALL_CROSSHAIR:
            return _apiGetSmallCrosshair(ph);
        case CHEAT_NAME_FAST_GAMEPLAY:
            return _apiGetFastGameplay(ph);
        case CHEAT_NAME_NO_SHELLSHOCK:
            return _apiGetNoShellshock(ph);
        case CHEAT_NAME_INCREASE_KNIFE_RANGE:
            return _apiGetIncreaseKnifeRange(ph);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return _apiGetBoxNeverMoves(ph);
        case CHEAT_NAME_THIRD_PERSON:
            return _apiGetThirdPerson(ph);
        case CHEAT_NAME_INFINITE_AMMO:
            return _apiGetInfiniteAmmo(ph);
        case CHEAT_NAME_INSTANT_KILL:
            return _apiGetInstantKill(api->hooks);
        case CHEAT_NAME_MAKE_BORDERLESS:
            return _apiGetMakeBorderless(ph);
        case CHEAT_NAME_UNLIMIT_FPS:
            return _apiGetUnlimitFps(ph); // TODO: SAVE CURRENT FPS CAP BEFORE ACTIVATING STATE
        case CHEAT_NAME_DISABLE_HUD:
            return _apiGetDisableHud(ph);
        case CHEAT_NAME_DISABLE_FOG:
            return _apiGetDisableFog(ph);
        case CHEAT_NAME_FULLBRIGHT:
            return _apiGetFullbright(ph);
        case CHEAT_NAME_COLORIZED:
            return _apiGetColorized(ph);
        
        default:
            LOG_WARN("Unkwown cheatName %d\n", cheatName);
            return false;
    }
}

bool apiSetCheatEnabled(Api *api, CheatName cheatName, bool enabled) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return false;
    }

    switch (cheatName) {
        case CHEAT_NAME_GOD_MODE:
            return _apiSetGodMode(ph, api->controller, enabled);
        case CHEAT_NAME_INVISIBLE:
            return _apiSetInvisible(ph, api->controller, enabled);
        case CHEAT_NAME_NO_CLIP:
            return _apiSetNoClip(ph, enabled);
        case CHEAT_NAME_NO_RECOIL:
            return _apiSetNoRecoil(ph, enabled);
        case CHEAT_NAME_SMALL_CROSSHAIR:
            return _apiSetSmallCrosshair(ph, enabled);
        case CHEAT_NAME_FAST_GAMEPLAY:
            return _apiSetFastGameplay(ph, enabled);
        case CHEAT_NAME_NO_SHELLSHOCK:
            return _apiSetNoShellshock(ph, enabled);
        case CHEAT_NAME_INCREASE_KNIFE_RANGE:
            return _apiSetIncreaseKnifeRange(ph, enabled);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return _apiSetBoxNeverMoves(ph, enabled);
        case CHEAT_NAME_THIRD_PERSON:
            return _apiSetThirdPerson(ph, enabled);
        case CHEAT_NAME_INFINITE_AMMO:
            return _apiSetInfiniteAmmo(ph, enabled);
        case CHEAT_NAME_INSTANT_KILL:
            return _apiSetInstantKill(ph, api->hooks, enabled);
        case CHEAT_NAME_MAKE_BORDERLESS:
            return _apiSetMakeBorderless(ph, enabled);
        case CHEAT_NAME_UNLIMIT_FPS:
            return _apiSetUnlimitFps(ph, api->controller, enabled); // TODO: SAVE CURRENT FPS CAP BEFORE ACTIVATING STATE
        case CHEAT_NAME_DISABLE_HUD:
            return _apiSetDisableHud(ph, enabled);
        case CHEAT_NAME_DISABLE_FOG:
            return _apiSetDisableFog(ph, enabled);
        case CHEAT_NAME_FULLBRIGHT:
            return _apiSetFullbright(ph, enabled);
        case CHEAT_NAME_COLORIZED:
            return _apiSetColorized(ph, enabled);
        default:
            LOG_WARN("Unknown cheatName %d\n", cheatName);
            return false;
    }
}

bool apiSetSimpleCheat(Api *api, SimpleCheatName simpleCheatName, void *value) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return false;
    }

    LOG_INFO("Setting Simple Cheat %d with value %x\n", simpleCheatName, value);

    switch(simpleCheatName) {
        case SIMPLE_CHEAT_NAME_CHANGE_NAME:
            return _apiChangeName(ph, (char*)value);
        case SIMPLE_CHEAT_NAME_SET_SPEED:
            return _apiSetSpeed(ph, (uint32_t)(*(int*)value));
        case SIMPLE_CHEAT_NAME_TELEPORT:
            return _apiTeleport(ph, (TeleportCoords)(*(TeleportCoords*)value));
        case SIMPLE_CHEAT_NAME_FOV:
            return _apiFov(ph, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_FOV_SCALE:
            return _apiFovScale(ph, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_FPS_CAP:
            return _apiFpsCap(ph, (uint32_t)(*(int*)value));
        case SIMPLE_CHEAT_NAME_SET_HEALTH:
        case SIMPLE_CHEAT_NAME_SET_POINTS:
        case SIMPLE_CHEAT_NAME_SET_KILLS:
        case SIMPLE_CHEAT_NAME_SET_HEADSHOTS:
            return _apiSetSimpleCheatIntValue(ph, simpleCheatName, (uint32_t)(*(int*)value));
        default:
            LOG_WARN("Unknown simpleCheatName %d\n", simpleCheatName);
            return false;
    }
}

TeleportCoords *apiGetPlayerCurrentCoords(Api *api) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return NULL;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return NULL;
    }

    TeleportCoords *coords = (TeleportCoords*)malloc(sizeof(TeleportCoords));
    memoryRead(ph, TELEPORT_CHEAT.xOffset, &coords->x, sizeof(coords->x));
    memoryRead(ph, TELEPORT_CHEAT.yOffset, &coords->y, sizeof(coords->y));
    memoryRead(ph, TELEPORT_CHEAT.zOffset, &coords->z, sizeof(coords->z));

    return coords;
}

WeaponName apiGetPlayerCurrentWeapon(Api *api) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return WEAPON_UNKNOWNWEAPON;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return WEAPON_UNKNOWNWEAPON;
    }

    uint8_t weapon;
    bool success = memoryRead(ph, WEAPON_CHEAT.currentWeaponOffset, &weapon, sizeof(uint8_t));
    if (!success) {
        printf("Failed to read Current WeaponName value\n");
        return WEAPON_UNKNOWNWEAPON;
    }
    return (WeaponName)weapon;
}

WeaponName apiGetPlayerWeapon(Api *api, int slot) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return WEAPON_UNKNOWNWEAPON;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return WEAPON_UNKNOWNWEAPON;
    }

    if (slot < 1 || slot > 3) {
        printf("Slot %d is invalid. Posible slots are 1, 2 or 3.\n", slot);
        return WEAPON_UNKNOWNWEAPON;
    }
    uint32_t slotOffset = slot == 1 ? WEAPON_CHEAT.weapon1.weaponOffset : (slot == 2 ? WEAPON_CHEAT.weapon2.weaponOffset : WEAPON_CHEAT.weapon3.weaponOffset);
    uint8_t weapon;
    bool success = memoryRead(ph, slotOffset, &weapon, sizeof(weapon));
    if (!success) {
        printf("Failed to read Player Weapon value on slot %d\n", slot);
        return WEAPON_UNKNOWNWEAPON;
    }
    return (WeaponName)weapon;
}

bool apiSetPlayerWeapon(Api *api, WeaponName weapon, int slot) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return false;
    }

    if (slot < 1 || slot > 3) {
        printf("Slot %d is invalid. Posible slots are 1, 2 or 3.\n", slot);
        return false;
    }
    uint32_t slotOffset = slot == 1 ? WEAPON_CHEAT.weapon1.weaponOffset : (slot == 2 ? WEAPON_CHEAT.weapon2.weaponOffset : WEAPON_CHEAT.weapon3.weaponOffset);
    uint8_t weaponValue = (uint8_t)weapon;
    return memoryWrite(ph, slotOffset, &weapon, sizeof(weaponValue));
}

bool apiGivePlayerAmmo(Api *api) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return false;
    }

    return _apiGiveWeaponAmmo(ph, WEAPON_CHEAT.weapon1) &&
           _apiGiveWeaponAmmo(ph, WEAPON_CHEAT.weapon2) &&
           _apiGiveWeaponAmmo(ph, WEAPON_CHEAT.weapon3);
}

bool apiSetRound(Api *api, int currentRound, int nextRound) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return false;
    }

    uint8_t pattern[ROUND_CHANGE_PATTERN_SIZE];
    memcpy(pattern, ROUND_CHEAT.pattern, ROUND_CHEAT.patternSize);  // Copy the Cheat pattern to avoid modifying the array.
    memcpy(pattern, &currentRound, 4*sizeof(uint8_t));              // Replacing first 4 bytes with current round.
    uintptr_t addressFound;
    bool found = memoryFindPattern(ph, ROUND_CHEAT.regionOffset, ROUND_CHEAT.regionSize, pattern, sizeof(pattern), &addressFound);
    if (!found) {
        LOG_ERROR("Couldn't find Round Change memory pattern!\n");
        return false;
    }
    bool success = memoryWrite(ph, addressFound, &nextRound, sizeof(nextRound));
    if (!success) {
        printf("Failed to write Next Round value\n");
        return false;
    }
    LOG_INFO("Next round successfully changed. Finish the current round %d and next round will be %d\n", currentRound, nextRound);
    return true;
}

bool apiIsZombiesGameRunning(Api *api) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return false;
    }
    uint32_t active;
    bool success = memoryRead(ph, GAME_CHEAT.isZombiesGameActiveOffset, &active, sizeof(uint32_t));
    if (!success) {
        printf("Failed to read Is Zombies Game Active value\n");
        return false;
    }
    return active == 1;
}

int apiGetGameResets(Api *api) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return 0;
    }
    
    ProcessHandle *ph = controllerGetProcessHandle(api->controller);
    if (!ph) {
        LOG_ERROR("ProcessHandle is null\n");
        return 0;
    }
    uint32_t resets;
    bool success = memoryRead(ph, GAME_CHEAT.nResetsOffset, &resets, sizeof(uint32_t));
    if (!success) {
        printf("Failed to read Game Resets value\n");
        return 0;
    }
    // We substract one since this value are the total number of games. Usually, resets refers to the number of retries, where the first try isn't a retry at all.
    // If there are no games played so far, we keep the 0.
    resets = resets == 0 ? resets : resets - 1;
    return (int)resets; 
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
    bool isInvisibleModeChecked = controllerIsCheatCheckboxChecked(controller, CHEAT_NAME_INVISIBLE);
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
    bool isGodModeChecked = controllerIsCheatCheckboxChecked(controller, CHEAT_NAME_GOD_MODE);
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

bool _apiGetFastGameplay(ProcessHandle *ph) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_FAST_GAMEPLAY.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Fast Gameplay address\n");
        return false;
    }
    float value = 0;
    success = memoryRead(ph, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Fast Gameplay value\n");
        return false;
    }
    return value == CHEAT_FAST_GAMEPLAY.on.f32;
}

bool _apiSetFastGameplay(ProcessHandle *ph, bool enabled) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_FAST_GAMEPLAY.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Fast Gameplay address\n");
        return false;
    }
    float value = enabled ? CHEAT_FAST_GAMEPLAY.on.f32 : CHEAT_FAST_GAMEPLAY.off.f32;
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(value));
}

bool _apiGetNoShellshock(ProcessHandle *ph) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_NO_SHELLSHOCK.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read No Shellshock address\n");
        return false;
    }
    uint8_t value = 0;
    success = memoryRead(ph, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read No Shellshock value\n");
        return false;
    }
    return value == CHEAT_NO_SHELLSHOCK.on.u32;
}

bool _apiSetNoShellshock(ProcessHandle *ph, bool enabled) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_NO_SHELLSHOCK.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read No Shellshock address\n");
        return false;
    }
    uint8_t value = enabled ? CHEAT_NO_SHELLSHOCK.on.u32 : CHEAT_NO_SHELLSHOCK.off.u32;
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(value));
}

bool _apiGetIncreaseKnifeRange(ProcessHandle *ph) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_INCREASE_KNIFE_RANGE.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Increase Knife Range address\n");
        return false;
    }
    float value = 0;
    success = memoryRead(ph, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Increase Knife Range value\n");
        return false;
    }
    return value == CHEAT_INCREASE_KNIFE_RANGE.on.f32;
}

bool _apiSetIncreaseKnifeRange(ProcessHandle *ph, bool enabled) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_INCREASE_KNIFE_RANGE.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Increase Knife Range address\n");
        return false;
    }
    float value = enabled ? CHEAT_INCREASE_KNIFE_RANGE.on.f32 : CHEAT_INCREASE_KNIFE_RANGE.off.f32;
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(value));
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
    bool success = memoryRead(ph, CHEAT_ASM_INFINITE_AMMO.offset, &buffer, sizeof(buffer)); // Read all MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE bytes even if we need less. This is for a future comparation with Cheat On vs Cheat Off, which may differ in Bytes OpCode length.
    if (!success) {
        printf("Failed to read Infinite Ammo value\n");
        return false;
    }
    bool infiniteAmmoEnabled = memcmp(CHEAT_ASM_INFINITE_AMMO.on.instructions, buffer, sizeof(CHEAT_ASM_INFINITE_AMMO.on.size)) == 0; // Compare only with the first OpCode Bytes when Cheat is On.
    if (infiniteAmmoEnabled) return true;

    bool infiniteAmmoDisabled = memcmp(CHEAT_ASM_INFINITE_AMMO.off.instructions, buffer, sizeof(CHEAT_ASM_INFINITE_AMMO.off.size)) == 0; // Compare only with the first OpCode Bytes when Cheat is Off.
    if (!infiniteAmmoDisabled) {
        LOG_WARN("Infinite Ammo bytes do not match known patterns. Possible memory corruption or external modification.\n");
        return false;
    }
    return false;
}

bool _apiSetInfiniteAmmo(ProcessHandle *ph, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_INFINITE_AMMO;
    uint8_t *instructions = enabled ? instructionSet->on.instructions : instructionSet->off.instructions;
    size_t size = enabled ? instructionSet->on.size : instructionSet->off.size;
    return memoryWrite(ph, instructionSet->offset, instructions, size);
}

bool _apiGetSmallCrosshair(ProcessHandle *ph) {
    uint8_t buffer[MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE];
    bool success = memoryRead(ph, CHEAT_ASM_SMALL_CROSSHAIR.offset, &buffer, sizeof(buffer)); // Read all MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE bytes even if we need less. This is for a future comparation with Cheat On vs Cheat Off, which may differ in Bytes OpCode length.
    if (!success) {
        printf("Failed to read Small Crosshair value\n");
        return false;
    }
    bool smallCrosshairEnabled = memcmp(CHEAT_ASM_SMALL_CROSSHAIR.on.instructions, buffer, sizeof(CHEAT_ASM_SMALL_CROSSHAIR.on.size)) == 0; // Compare only with the first OpCode Bytes when Cheat is On.
    if (smallCrosshairEnabled) return true;

    bool smallCrosshairDisabled = memcmp(CHEAT_ASM_SMALL_CROSSHAIR.off.instructions, buffer, sizeof(CHEAT_ASM_SMALL_CROSSHAIR.off.size)) == 0; // Compare only with the first OpCode Bytes when Cheat is Off.
    if (!smallCrosshairDisabled) {
        LOG_WARN("Small Crosshair bytes do not match known patterns. Possible memory corruption or external modification.\n");
        return false;
    }
    return false;
}

bool _apiSetSmallCrosshair(ProcessHandle *ph, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_SMALL_CROSSHAIR;
    uint8_t *instructions = enabled ? instructionSet->on.instructions : instructionSet->off.instructions;
    size_t size = enabled ? instructionSet->on.size : instructionSet->off.size;
    bool success = memoryWrite(ph, instructionSet->offset, instructions, size);
    if (!success) {
        printf("Failed to write Asm code for Small Crosshair.\n");
        return false;
    }
    uint32_t address1 = 0;
    success = memoryRead(ph, CHEAT_SMALL_CROSSHAIR.offset, &address1, sizeof(address1));
    LOG_INFO("address1 %x\n", address1);
    if (!success) {
        printf("Failed to read Small Crosshair address\n");
        return false;
    }
    float value = enabled ? CHEAT_SMALL_CROSSHAIR.on.f32 : CHEAT_SMALL_CROSSHAIR.off.f32;
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(value));
}

bool _apiGetInstantKill(Map *hooks) {
    Hook *hook = (Hook*)mapGet(hooks, HOOK_INSTANT_KILL_ID);
    return hookIsActivated(hook);
}

bool _apiSetInstantKill(ProcessHandle *ph, Map *hooks, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_INSTANT_KILL;
    Hook *hook;
    if (!mapContains(hooks, HOOK_INSTANT_KILL_ID)) {
        hook = hookCreate(ph, instructionSet->offset, instructionSet->off.size, instructionSet->on.instructions, instructionSet->on.size);
        mapPut(hooks, HOOK_INSTANT_KILL_ID, hook);
    } else {
        hook = (Hook*)mapGet(hooks, HOOK_INSTANT_KILL_ID);
    }

    return enabled ? hookActivate(hook) : hookDeactivate(hook);
}

bool _apiGetMakeBorderless(ProcessHandle *ph) {
    // TODO
    return false;
}

bool _apiSetMakeBorderless(ProcessHandle *ph, bool enabled) {
    // TODO
    return false;
}


bool _apiGetUnlimitFps(ProcessHandle *ph) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_UNLIMIT_FPS.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Unlimit Fps address\n");
        return false;
    }
    uint32_t value = 0;
    success = memoryRead(ph, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Unlimit Fps value\n");
        return false;
    }
    return value == CHEAT_UNLIMIT_FPS.on.u32;
}

bool _apiSetUnlimitFps(ProcessHandle *ph, Controller *controller, bool enabled) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_UNLIMIT_FPS.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Unlimit Fps address\n");
        return false;
    }
    uint32_t value = enabled ? CHEAT_UNLIMIT_FPS.on.u32 : (uint32_t)controllerUiGraphicsGetFpsCap(controller);
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(value));
}


bool _apiGetDisableHud(ProcessHandle *ph) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_DISABLE_HUD.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Disable Hud address\n");
        return false;
    }
    uint8_t value = 0;
    success = memoryRead(ph, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Disable Hud value\n");
        return false;
    }
    return value == CHEAT_DISABLE_HUD.on.byte;
}

bool _apiSetDisableHud(ProcessHandle *ph, bool enabled) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_DISABLE_HUD.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Disable Hud address\n");
        return false;
    }
    LOG_INFO("Reading Disable HUD pointer %x and inside is %x\n", CHEAT_DISABLE_HUD.offset, address1);
    uint8_t value = enabled ? CHEAT_DISABLE_HUD.on.byte : CHEAT_DISABLE_HUD.off.byte;
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(value));
}


bool _apiGetDisableFog(ProcessHandle *ph) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_DISABLE_FOG.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Disable Fog address\n");
        return false;
    }
    uint8_t value = 0;
    success = memoryRead(ph, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Disable Fog value\n");
        return false;
    }
    return value == CHEAT_DISABLE_FOG.on.byte;
}

bool _apiSetDisableFog(ProcessHandle *ph, bool enabled) {
    uint32_t address1 = 0;
    bool success = memoryRead(ph, CHEAT_DISABLE_FOG.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Disable Fog address\n");
        return false;
    }
    uint8_t value = enabled ? CHEAT_DISABLE_FOG.on.byte : CHEAT_DISABLE_FOG.off.byte;
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(value));
}


bool _apiGetFullbright(ProcessHandle *ph) {
    uint32_t value = 0;
    bool success = memoryRead(ph, CHEAT_FULLBRIGHT.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Fullbright value\n");
        return false;
    }
    return value == CHEAT_FULLBRIGHT.on.u32;
}

bool _apiSetFullbright(ProcessHandle *ph, bool enabled) {
    uint32_t value = enabled ? CHEAT_FULLBRIGHT.on.u32 : CHEAT_FULLBRIGHT.off.u32;
    return memoryWrite(ph, CHEAT_FULLBRIGHT.offset, &value, sizeof(value));
}


bool _apiGetColorized(ProcessHandle *ph) {
    uint32_t value = 0;
    bool success = memoryRead(ph, CHEAT_COLORIZED.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Colorized value\n");
        return false;
    }
    return value == CHEAT_COLORIZED.on.u32;
}

bool _apiSetColorized(ProcessHandle *ph, bool enabled) {
    uint32_t value = enabled ? CHEAT_COLORIZED.on.u32 : CHEAT_COLORIZED.off.u32;
    return memoryWrite(ph, CHEAT_COLORIZED.offset, &value, sizeof(value));
}


// Simple cheats

bool _apiChangeName(ProcessHandle *ph, char *name) {
    SimpleCheat cheat = SIMPLE_CHEAT_CHANGE_NAME;
    return memoryWrite(ph, cheat.offset, name, strlen(name) + 1);
}

bool _apiSetSpeed(ProcessHandle *ph, uint32_t value) {
    SimpleCheat cheat = SIMPLE_CHEAT_SET_SPEED;
    uint32_t address1 = 0;
    bool success = memoryRead(ph, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Speed address\n");
        return false;
    }
    LOG_INFO("Writting %d in %x\n", value, cheat.offset);
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(uint32_t));
}

bool _apiGiveWeaponAmmo(ProcessHandle *ph, Weapon weapon) {
    uint32_t bullets = 1000;
    return memoryWrite(ph, weapon.clipOffset, &bullets, sizeof(bullets)) &&
           memoryWrite(ph, weapon.ammoOffset, &bullets, sizeof(bullets));
}

bool _apiTeleport(ProcessHandle *ph, TeleportCoords value) {
    return memoryWrite(ph, TELEPORT_CHEAT.xOffset, &(value.x), sizeof(value.x)) &&
           memoryWrite(ph, TELEPORT_CHEAT.yOffset, &(value.y), sizeof(value.y)) &&
           memoryWrite(ph, TELEPORT_CHEAT.zOffset, &(value.z), sizeof(value.z));
}

bool _apiFov(ProcessHandle *ph, float value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FOV;
    uint32_t address1 = 0;
    bool success = memoryRead(ph, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Fov address\n");
        return false;
    }
    LOG_INFO("Writting %f in %x\n", value, cheat.offset);
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(float));;
}

bool _apiFovScale(ProcessHandle *ph, float value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FOV_SCALE;
    uint32_t address1 = 0;
    bool success = memoryRead(ph, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Fov Scale address\n");
        return false;
    }
    value = value/100; // FOV Scale goes from [0.20, 2.00]. Value is an float between [20.00, 200.00] so we need to divide by 100. This could be also done in UI module though. 
    LOG_INFO("Writting %f in %x\n", value, cheat.offset);
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(float));
}

bool _apiFpsCap(ProcessHandle *ph, uint32_t value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FPS_CAP;
    uint32_t address1 = 0;
    bool success = memoryRead(ph, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Fps Cap address\n");
        return false;
    }
    LOG_INFO("Writting %d in %x\n", value, cheat.offset);
    return memoryWrite(ph, address1 + 0x18, &value, sizeof(uint32_t));
}

bool _apiSetSimpleCheatIntValue(ProcessHandle *ph, SimpleCheatName simpleCheatName, uint32_t value) {
    SimpleCheat cheat = cheatGetSimpleCheat(simpleCheatName);
    LOG_INFO("Writting %d in %x\n", value, cheat.offset);
    return memoryWrite(ph, cheat.offset, &value, sizeof(uint32_t));
}
