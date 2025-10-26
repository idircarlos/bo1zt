#include "api.h"
#include "../cheat/cheat.h"
#include "../controller/controller.h"
#include "../logger/logger.h"
#include "../map/map.h"
#include "../hook/hook.h"
#include "../process/process.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Api {
    Controller *controller;
    Map *hooks;
};

bool _apiGetGodMode(Process *process);
bool _apiSetGodMode(Process *process, Controller *controller, bool enabled);

bool _apiGetInvisible(Process *process);
bool _apiSetInvisible(Process *process, Controller *controller, bool enabled);

bool _apiGetNoClip(Process *process);
bool _apiSetNoClip(Process *process, bool enabled);

bool _apiGetNoRecoil(Process *process);
bool _apiSetNoRecoil(Process *process, bool enabled);

bool _apiGetSmallCrosshair(Process *process);
bool _apiSetSmallCrosshair(Process *process, bool enabled);

bool _apiGetFastGameplay(Process *process);
bool _apiSetFastGameplay(Process *process, bool enabled);

bool _apiGetNoShellshock(Process *process);
bool _apiSetNoShellshock(Process *process, bool enabled);

bool _apiGetIncreaseKnifeRange(Process *process);
bool _apiSetIncreaseKnifeRange(Process *process, bool enabled);

bool _apiGetBoxNeverMoves(Process *process);
bool _apiSetBoxNeverMoves(Process *process, bool enabled);

bool _apiGetThirdPerson(Process *process);
bool _apiSetThirdPerson(Process *process, bool enabled);

bool _apiGetInfiniteAmmo(Process *process);
bool _apiSetInfiniteAmmo(Process *process, bool enabled);

bool _apiGetInstantKill(Map *hooks);
bool _apiSetInstantKill(Process *process, Map *hooks, bool enabled);

bool _apiGetMakeBorderless(Process *process);
bool _apiSetMakeBorderless(Process *process, bool enabled);

bool _apiGetUnlimitFps(Process *process);
bool _apiSetUnlimitFps(Process *process, Controller *controller, bool enabled);

bool _apiGetDisableHud(Process *process);
bool _apiSetDisableHud(Process *process, bool enabled);

bool _apiGetDisableFog(Process *process);
bool _apiSetDisableFog(Process *process, bool enabled);

bool _apiGetFullbright(Process *process);
bool _apiSetFullbright(Process *process, bool enabled);

bool _apiGetColorized(Process *process);
bool _apiSetColorized(Process *process, bool enabled);

bool _apiChangeName(Process *process, char *name);
bool _apiSetSpeed(Process *process, uint32_t value);
bool _apiGiveWeaponAmmo(Process *process, Weapon weapon);
bool _apiTeleport(Process *process, TeleportCoords value);
bool _apiFov(Process *process, float value);
bool _apiFovScale(Process *process, float value);
bool _apiFpsCap(Process *process, uint32_t value);
bool _apiSetSimpleCheatIntValue(Process *process, SimpleCheatName simpleCheatName, uint32_t value);



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
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return false;
    }

    switch (cheatName) {
        case CHEAT_NAME_GOD_MODE:
            return _apiGetGodMode(process);
        case CHEAT_NAME_INVISIBLE:
            return _apiGetInvisible(process);
        case CHEAT_NAME_NO_CLIP:
            return _apiGetNoClip(process);
        case CHEAT_NAME_NO_RECOIL:
            return _apiGetNoRecoil(process);
        case CHEAT_NAME_SMALL_CROSSHAIR:
            return _apiGetSmallCrosshair(process);
        case CHEAT_NAME_FAST_GAMEPLAY:
            return _apiGetFastGameplay(process);
        case CHEAT_NAME_NO_SHELLSHOCK:
            return _apiGetNoShellshock(process);
        case CHEAT_NAME_INCREASE_KNIFE_RANGE:
            return _apiGetIncreaseKnifeRange(process);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return _apiGetBoxNeverMoves(process);
        case CHEAT_NAME_THIRD_PERSON:
            return _apiGetThirdPerson(process);
        case CHEAT_NAME_INFINITE_AMMO:
            return _apiGetInfiniteAmmo(process);
        case CHEAT_NAME_INSTANT_KILL:
            return _apiGetInstantKill(api->hooks);
        case CHEAT_NAME_MAKE_BORDERLESS:
            return _apiGetMakeBorderless(process);
        case CHEAT_NAME_UNLIMIT_FPS:
            return _apiGetUnlimitFps(process); // TODO: SAVE CURRENT FPS CAP BEFORE ACTIVATING STATE
        case CHEAT_NAME_DISABLE_HUD:
            return _apiGetDisableHud(process);
        case CHEAT_NAME_DISABLE_FOG:
            return _apiGetDisableFog(process);
        case CHEAT_NAME_FULLBRIGHT:
            return _apiGetFullbright(process);
        case CHEAT_NAME_COLORIZED:
            return _apiGetColorized(process);
        
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
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return false;
    }

    switch (cheatName) {
        case CHEAT_NAME_GOD_MODE:
            return _apiSetGodMode(process, api->controller, enabled);
        case CHEAT_NAME_INVISIBLE:
            return _apiSetInvisible(process, api->controller, enabled);
        case CHEAT_NAME_NO_CLIP:
            return _apiSetNoClip(process, enabled);
        case CHEAT_NAME_NO_RECOIL:
            return _apiSetNoRecoil(process, enabled);
        case CHEAT_NAME_SMALL_CROSSHAIR:
            return _apiSetSmallCrosshair(process, enabled);
        case CHEAT_NAME_FAST_GAMEPLAY:
            return _apiSetFastGameplay(process, enabled);
        case CHEAT_NAME_NO_SHELLSHOCK:
            return _apiSetNoShellshock(process, enabled);
        case CHEAT_NAME_INCREASE_KNIFE_RANGE:
            return _apiSetIncreaseKnifeRange(process, enabled);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return _apiSetBoxNeverMoves(process, enabled);
        case CHEAT_NAME_THIRD_PERSON:
            return _apiSetThirdPerson(process, enabled);
        case CHEAT_NAME_INFINITE_AMMO:
            return _apiSetInfiniteAmmo(process, enabled);
        case CHEAT_NAME_INSTANT_KILL:
            return _apiSetInstantKill(process, api->hooks, enabled);
        case CHEAT_NAME_MAKE_BORDERLESS:
            return _apiSetMakeBorderless(process, enabled);
        case CHEAT_NAME_UNLIMIT_FPS:
            return _apiSetUnlimitFps(process, api->controller, enabled); // TODO: SAVE CURRENT FPS CAP BEFORE ACTIVATING STATE
        case CHEAT_NAME_DISABLE_HUD:
            return _apiSetDisableHud(process, enabled);
        case CHEAT_NAME_DISABLE_FOG:
            return _apiSetDisableFog(process, enabled);
        case CHEAT_NAME_FULLBRIGHT:
            return _apiSetFullbright(process, enabled);
        case CHEAT_NAME_COLORIZED:
            return _apiSetColorized(process, enabled);
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
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return false;
    }

    LOG_INFO("Setting Simple Cheat %d with value %x\n", simpleCheatName, value);

    switch(simpleCheatName) {
        case SIMPLE_CHEAT_NAME_CHANGE_NAME:
            return _apiChangeName(process, (char*)value);
        case SIMPLE_CHEAT_NAME_SET_SPEED:
            return _apiSetSpeed(process, (uint32_t)(*(int*)value));
        case SIMPLE_CHEAT_NAME_TELEPORT:
            return _apiTeleport(process, (TeleportCoords)(*(TeleportCoords*)value));
        case SIMPLE_CHEAT_NAME_FOV:
            return _apiFov(process, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_FOV_SCALE:
            return _apiFovScale(process, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_FPS_CAP:
            return _apiFpsCap(process, (uint32_t)(*(int*)value));
        case SIMPLE_CHEAT_NAME_SET_HEALTH:
        case SIMPLE_CHEAT_NAME_SET_POINTS:
        case SIMPLE_CHEAT_NAME_SET_KILLS:
        case SIMPLE_CHEAT_NAME_SET_HEADSHOTS:
            return _apiSetSimpleCheatIntValue(process, simpleCheatName, (uint32_t)(*(int*)value));
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
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return NULL;
    }

    TeleportCoords *coords = (TeleportCoords*)malloc(sizeof(TeleportCoords));
    processRead(process, TELEPORT_CHEAT.xOffset, &coords->x, sizeof(coords->x));
    processRead(process, TELEPORT_CHEAT.yOffset, &coords->y, sizeof(coords->y));
    processRead(process, TELEPORT_CHEAT.zOffset, &coords->z, sizeof(coords->z));

    return coords;
}

WeaponName apiGetPlayerCurrentWeapon(Api *api) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return WEAPON_UNKNOWNWEAPON;
    }
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return WEAPON_UNKNOWNWEAPON;
    }

    uint8_t weapon;
    bool success = processRead(process, WEAPON_CHEAT.currentWeaponOffset, &weapon, sizeof(uint8_t));
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
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return WEAPON_UNKNOWNWEAPON;
    }

    if (slot < 1 || slot > 3) {
        printf("Slot %d is invalid. Posible slots are 1, 2 or 3.\n", slot);
        return WEAPON_UNKNOWNWEAPON;
    }
    uint32_t slotOffset = slot == 1 ? WEAPON_CHEAT.weapon1.weaponOffset : (slot == 2 ? WEAPON_CHEAT.weapon2.weaponOffset : WEAPON_CHEAT.weapon3.weaponOffset);
    uint8_t weapon;
    bool success = processRead(process, slotOffset, &weapon, sizeof(weapon));
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
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return false;
    }

    if (slot < 1 || slot > 3) {
        printf("Slot %d is invalid. Posible slots are 1, 2 or 3.\n", slot);
        return false;
    }
    uint32_t slotOffset = slot == 1 ? WEAPON_CHEAT.weapon1.weaponOffset : (slot == 2 ? WEAPON_CHEAT.weapon2.weaponOffset : WEAPON_CHEAT.weapon3.weaponOffset);
    uint8_t weaponValue = (uint8_t)weapon;
    return processWrite(process, slotOffset, &weapon, sizeof(weaponValue));
}

bool apiGivePlayerAmmo(Api *api) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return false;
    }

    return _apiGiveWeaponAmmo(process, WEAPON_CHEAT.weapon1) &&
           _apiGiveWeaponAmmo(process, WEAPON_CHEAT.weapon2) &&
           _apiGiveWeaponAmmo(process, WEAPON_CHEAT.weapon3);
}

bool apiSetRound(Api *api, int currentRound, int nextRound) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return false;
    }

    uint8_t pattern[ROUND_CHANGE_PATTERN_SIZE];
    memcpy(pattern, ROUND_CHEAT.pattern, ROUND_CHEAT.patternSize);  // Copy the Cheat pattern to avoid modifying the array.
    memcpy(pattern, &currentRound, 4*sizeof(uint8_t));              // Replacing first 4 bytes with current round.
    uintptr_t addressFound;
    bool found = processFindPattern(process, ROUND_CHEAT.regionOffset, ROUND_CHEAT.regionSize, pattern, sizeof(pattern), &addressFound);
    if (!found) {
        LOG_ERROR("Couldn't find Round Change memory pattern!\n");
        return false;
    }
    bool success = processWrite(process, addressFound, &nextRound, sizeof(nextRound));
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
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return false;
    }
    uint32_t active;
    bool success = processRead(process, GAME_CHEAT.isZombiesGameActiveOffset, &active, sizeof(uint32_t));
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
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return 0;
    }
    uint32_t resets;
    bool success = processRead(process, GAME_CHEAT.nResetsOffset, &resets, sizeof(uint32_t));
    if (!success) {
        printf("Failed to read Game Resets value\n");
        return 0;
    }
    // We substract one since this value are the total number of games. Usually, resets refers to the number of retries, where the first try isn't a retry at all.
    // If there are no games played so far, we keep the 0.
    resets = resets == 0 ? resets : resets - 1;
    return (int)resets; 
}

bool _apiGetGodMode(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_GOD_MODE.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read God Mode value\n");
        return false;
    }
    return value == CHEAT_GOD_MODE.on.u32;
}

bool _apiSetGodMode(Process *process, Controller *controller, bool enabled) {
    bool isInvisibleModeChecked = controllerIsCheatCheckboxChecked(controller, CHEAT_NAME_INVISIBLE);
    uint32_t value = enabled ? CHEAT_GOD_MODE.on.u32 : (isInvisibleModeChecked ? CHEAT_INVISIBLE.on.u32 : CHEAT_INVISIBLE.off.u32); // Restore back Invisible if it was enabled in the GUI, otherwise disable God Mode.
    return processWrite(process, CHEAT_GOD_MODE.offset, &value, sizeof(value));
}

bool _apiGetInvisible(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_INVISIBLE.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Invisible value\n");
        return false;
    }
    return value == CHEAT_INVISIBLE.on.u32;
}

bool _apiSetInvisible(Process *process, Controller *controller, bool enabled) {
    bool isGodModeChecked = controllerIsCheatCheckboxChecked(controller, CHEAT_NAME_GOD_MODE);
    uint32_t value = enabled ? CHEAT_INVISIBLE.on.u32 : (isGodModeChecked ? CHEAT_GOD_MODE.on.u32 : CHEAT_GOD_MODE.off.u32); // Restore back God Mode if it was enabled in the GUI.
    return processWrite(process, CHEAT_INVISIBLE.offset, &value, sizeof(value));
}

bool _apiGetNoClip(Process *process) {
    uint8_t value = 0;
    bool success = processRead(process, CHEAT_NO_CLIP.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read No Clip value\n");
        return false;
    }
    return value == CHEAT_NO_CLIP.on.byte;
}

bool _apiSetNoClip(Process *process, bool enabled) {
    uint8_t value = enabled ? CHEAT_NO_CLIP.on.byte : CHEAT_NO_CLIP.off.byte;
    return processWrite(process, CHEAT_NO_CLIP.offset, &value, sizeof(value));
}

bool _apiGetNoRecoil(Process *process) {
    uint8_t value = 0;
    bool success = processRead(process, CHEAT_NO_RECOIL.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read No Recoil value\n");
        return false;
    }
    return value == CHEAT_NO_RECOIL.on.byte;
}

bool _apiSetNoRecoil(Process *process, bool enabled) {
    uint8_t value = enabled ? CHEAT_NO_RECOIL.on.byte : CHEAT_NO_RECOIL.off.byte;
    return processWrite(process, CHEAT_NO_RECOIL.offset, &value, sizeof(value));
}

bool _apiGetFastGameplay(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_FAST_GAMEPLAY.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Fast Gameplay address\n");
        return false;
    }
    float value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Fast Gameplay value\n");
        return false;
    }
    return value == CHEAT_FAST_GAMEPLAY.on.f32;
}

bool _apiSetFastGameplay(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_FAST_GAMEPLAY.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Fast Gameplay address\n");
        return false;
    }
    float value = enabled ? CHEAT_FAST_GAMEPLAY.on.f32 : CHEAT_FAST_GAMEPLAY.off.f32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _apiGetNoShellshock(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_NO_SHELLSHOCK.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read No Shellshock address\n");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read No Shellshock value\n");
        return false;
    }
    return value == CHEAT_NO_SHELLSHOCK.on.u32;
}

bool _apiSetNoShellshock(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_NO_SHELLSHOCK.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read No Shellshock address\n");
        return false;
    }
    uint8_t value = enabled ? CHEAT_NO_SHELLSHOCK.on.u32 : CHEAT_NO_SHELLSHOCK.off.u32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _apiGetIncreaseKnifeRange(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_INCREASE_KNIFE_RANGE.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Increase Knife Range address\n");
        return false;
    }
    float value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Increase Knife Range value\n");
        return false;
    }
    return value == CHEAT_INCREASE_KNIFE_RANGE.on.f32;
}

bool _apiSetIncreaseKnifeRange(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_INCREASE_KNIFE_RANGE.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Increase Knife Range address\n");
        return false;
    }
    float value = enabled ? CHEAT_INCREASE_KNIFE_RANGE.on.f32 : CHEAT_INCREASE_KNIFE_RANGE.off.f32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _apiGetBoxNeverMoves(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_BOX_NEVER_MOVES.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Box Never Moves address\n");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Box Never Moves value\n");
        return false;
    }
    return value == CHEAT_BOX_NEVER_MOVES.on.u32;
}

bool _apiSetBoxNeverMoves(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_BOX_NEVER_MOVES.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Box Never Moves address\n");
        return false;
    }
    uint8_t value = enabled ? CHEAT_BOX_NEVER_MOVES.on.u32 : CHEAT_BOX_NEVER_MOVES.off.u32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _apiGetThirdPerson(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_THIRD_PERSON.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Third Person address\n");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Third Person value\n");
        return false;
    }
    return value == CHEAT_THIRD_PERSON.on.byte;
}

bool _apiSetThirdPerson(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_THIRD_PERSON.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Third Person address\n");
        return false;
    }
    uint8_t value = enabled ? CHEAT_THIRD_PERSON.on.byte : CHEAT_THIRD_PERSON.off.byte;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _apiGetInfiniteAmmo(Process *process) {
    uint8_t buffer[MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE];
    bool success = processRead(process, CHEAT_ASM_INFINITE_AMMO.offset, &buffer, sizeof(buffer)); // Read all MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE bytes even if we need less. This is for a future comparation with Cheat On vs Cheat Off, which may differ in Bytes OpCode length.
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

bool _apiSetInfiniteAmmo(Process *process, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_INFINITE_AMMO;
    uint8_t *instructions = enabled ? instructionSet->on.instructions : instructionSet->off.instructions;
    size_t size = enabled ? instructionSet->on.size : instructionSet->off.size;
    return processWrite(process, instructionSet->offset, instructions, size);
}

bool _apiGetSmallCrosshair(Process *process) {
    uint8_t buffer[MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE];
    bool success = processRead(process, CHEAT_ASM_SMALL_CROSSHAIR.offset, &buffer, sizeof(buffer)); // Read all MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE bytes even if we need less. This is for a future comparation with Cheat On vs Cheat Off, which may differ in Bytes OpCode length.
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

bool _apiSetSmallCrosshair(Process *process, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_SMALL_CROSSHAIR;
    uint8_t *instructions = enabled ? instructionSet->on.instructions : instructionSet->off.instructions;
    size_t size = enabled ? instructionSet->on.size : instructionSet->off.size;
    bool success = processWrite(process, instructionSet->offset, instructions, size);
    if (!success) {
        printf("Failed to write Asm code for Small Crosshair.\n");
        return false;
    }
    uint32_t address1 = 0;
    success = processRead(process, CHEAT_SMALL_CROSSHAIR.offset, &address1, sizeof(address1));
    LOG_INFO("address1 %x\n", address1);
    if (!success) {
        printf("Failed to read Small Crosshair address\n");
        return false;
    }
    float value = enabled ? CHEAT_SMALL_CROSSHAIR.on.f32 : CHEAT_SMALL_CROSSHAIR.off.f32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _apiGetInstantKill(Map *hooks) {
    Hook *hook = (Hook*)mapGet(hooks, HOOK_INSTANT_KILL_ID);
    return hookIsActivated(hook);
}

bool _apiSetInstantKill(Process *process, Map *hooks, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_INSTANT_KILL;
    Hook *hook;
    if (!mapContains(hooks, HOOK_INSTANT_KILL_ID)) {
        hook = hookCreate(process, instructionSet->offset, instructionSet->off.size, instructionSet->on.instructions, instructionSet->on.size);
        mapPut(hooks, HOOK_INSTANT_KILL_ID, hook);
    } else {
        hook = (Hook*)mapGet(hooks, HOOK_INSTANT_KILL_ID);
    }

    return enabled ? hookActivate(hook) : hookDeactivate(hook);
}

bool _apiGetMakeBorderless(Process *process) {
    return processIsBorderless(process);
}

bool _apiSetMakeBorderless(Process *process, bool enabled) {
    return processMakeBorderless(process, enabled);
}


bool _apiGetUnlimitFps(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_UNLIMIT_FPS.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Unlimit Fps address\n");
        return false;
    }
    uint32_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Unlimit Fps value\n");
        return false;
    }
    return value == CHEAT_UNLIMIT_FPS.on.u32;
}

bool _apiSetUnlimitFps(Process *process, Controller *controller, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_UNLIMIT_FPS.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Unlimit Fps address\n");
        return false;
    }
    uint32_t value = enabled ? CHEAT_UNLIMIT_FPS.on.u32 : (uint32_t)controllerUiGraphicsGetFpsCap(controller);
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}


bool _apiGetDisableHud(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_DISABLE_HUD.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Disable Hud address\n");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Disable Hud value\n");
        return false;
    }
    return value == CHEAT_DISABLE_HUD.on.byte;
}

bool _apiSetDisableHud(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_DISABLE_HUD.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Disable Hud address\n");
        return false;
    }
    LOG_INFO("Reading Disable HUD pointer %x and inside is %x\n", CHEAT_DISABLE_HUD.offset, address1);
    uint8_t value = enabled ? CHEAT_DISABLE_HUD.on.byte : CHEAT_DISABLE_HUD.off.byte;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}


bool _apiGetDisableFog(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_DISABLE_FOG.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Disable Fog address\n");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Disable Fog value\n");
        return false;
    }
    return value == CHEAT_DISABLE_FOG.on.byte;
}

bool _apiSetDisableFog(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_DISABLE_FOG.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Disable Fog address\n");
        return false;
    }
    uint8_t value = enabled ? CHEAT_DISABLE_FOG.on.byte : CHEAT_DISABLE_FOG.off.byte;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}


bool _apiGetFullbright(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_FULLBRIGHT.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Fullbright value\n");
        return false;
    }
    return value == CHEAT_FULLBRIGHT.on.u32;
}

bool _apiSetFullbright(Process *process, bool enabled) {
    uint32_t value = enabled ? CHEAT_FULLBRIGHT.on.u32 : CHEAT_FULLBRIGHT.off.u32;
    return processWrite(process, CHEAT_FULLBRIGHT.offset, &value, sizeof(value));
}


bool _apiGetColorized(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_COLORIZED.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Colorized value\n");
        return false;
    }
    return value == CHEAT_COLORIZED.on.u32;
}

bool _apiSetColorized(Process *process, bool enabled) {
    uint32_t value = enabled ? CHEAT_COLORIZED.on.u32 : CHEAT_COLORIZED.off.u32;
    return processWrite(process, CHEAT_COLORIZED.offset, &value, sizeof(value));
}


// Simple cheats

bool _apiChangeName(Process *process, char *name) {
    SimpleCheat cheat = SIMPLE_CHEAT_CHANGE_NAME;
    return processWrite(process, cheat.offset, name, strlen(name) + 1);
}

bool _apiSetSpeed(Process *process, uint32_t value) {
    SimpleCheat cheat = SIMPLE_CHEAT_SET_SPEED;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Speed address\n");
        return false;
    }
    LOG_INFO("Writting %d in %x\n", value, cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(uint32_t));
}

bool _apiGiveWeaponAmmo(Process *process, Weapon weapon) {
    uint32_t bullets = 1000;
    return processWrite(process, weapon.clipOffset, &bullets, sizeof(bullets)) &&
           processWrite(process, weapon.ammoOffset, &bullets, sizeof(bullets));
}

bool _apiTeleport(Process *process, TeleportCoords value) {
    return processWrite(process, TELEPORT_CHEAT.xOffset, &(value.x), sizeof(value.x)) &&
           processWrite(process, TELEPORT_CHEAT.yOffset, &(value.y), sizeof(value.y)) &&
           processWrite(process, TELEPORT_CHEAT.zOffset, &(value.z), sizeof(value.z));
}

bool _apiFov(Process *process, float value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FOV;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Fov address\n");
        return false;
    }
    LOG_INFO("Writting %f in %x\n", value, cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(float));;
}

bool _apiFovScale(Process *process, float value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FOV_SCALE;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Fov Scale address\n");
        return false;
    }
    value = value/100; // FOV Scale goes from [0.20, 2.00]. Value is an float between [20.00, 200.00] so we need to divide by 100. This could be also done in UI module though. 
    LOG_INFO("Writting %f in %x\n", value, cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(float));
}

bool _apiFpsCap(Process *process, uint32_t value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FPS_CAP;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Fps Cap address\n");
        return false;
    }
    LOG_INFO("Writting %d in %x\n", value, cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(uint32_t));
}

bool _apiSetSimpleCheatIntValue(Process *process, SimpleCheatName simpleCheatName, uint32_t value) {
    SimpleCheat cheat = cheatGetSimpleCheat(simpleCheatName);
    LOG_INFO("Writting %d in %x\n", value, cheat.offset);
    return processWrite(process, cheat.offset, &value, sizeof(uint32_t));
}
