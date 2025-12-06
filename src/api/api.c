#include "api/api.h"
#include "logic/cheat.h"
#include "controller/controller.h"
#include "logger/logger.h"
#include "utils/map.h"
#include "win/hook.h"
#include "win/process.h"
#include "win/thread.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

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

bool _apiGetFixMovementSpeed(Process *process);
bool _apiSetFixMovementSpeed(Process *process, bool enabled);

bool _apiGetPatchChat(Process *process);
bool _apiSetPatchChat(Process *process, bool enabled);

bool _apiGetShowFps(Process *process);
bool _apiSetShowFps(Process *process, bool enabled);

bool _apiChangeName(Process *process, char *name);
bool _apiSetSpeed(Process *process, uint32_t value);
bool _apiGiveWeaponAmmo(Process *process, Weapon weapon);
bool _apiTeleport(Process *process, TeleportCoords value);
bool _apiChangeHostname(Process *process, char *hostname);
bool _apiFov(Process *process, float value);
bool _apiFovScale(Process *process, float value);
bool _apiFpsCap(Process *process, uint32_t value);

bool _apiCustomizerColor(Process *process, SimpleCheatName cheatName, Color color);
bool _apiCustomizerFloat(Process *process, SimpleCheatName cheatName, float value);

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
            return _apiGetUnlimitFps(process);
        case CHEAT_NAME_DISABLE_HUD:
            return _apiGetDisableHud(process);
        case CHEAT_NAME_DISABLE_FOG:
            return _apiGetDisableFog(process);
        case CHEAT_NAME_FULLBRIGHT:
            return _apiGetFullbright(process);
        case CHEAT_NAME_COLORIZED:
            return _apiGetColorized(process);
        case CHEAT_NAME_FIX_MOVEMENT_SPEED:
            return _apiGetFixMovementSpeed(process);
        case CHEAT_NAME_SHOW_FPS:
            return _apiGetShowFps(process);
        case CHEAT_NAME_PATCH_CHAT:
            return _apiGetPatchChat(process);
        
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
            return _apiSetUnlimitFps(process, api->controller, enabled);
        case CHEAT_NAME_DISABLE_HUD:
            return _apiSetDisableHud(process, enabled);
        case CHEAT_NAME_DISABLE_FOG:
            return _apiSetDisableFog(process, enabled);
        case CHEAT_NAME_FULLBRIGHT:
            return _apiSetFullbright(process, enabled);
        case CHEAT_NAME_COLORIZED:
            return _apiSetColorized(process, enabled);
        case CHEAT_NAME_FIX_MOVEMENT_SPEED:
            return _apiSetFixMovementSpeed(process, enabled);
        case CHEAT_NAME_SHOW_FPS:
            return _apiSetShowFps(process, enabled);
        case CHEAT_NAME_PATCH_CHAT:
            return _apiSetPatchChat(process, enabled);
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

    LOG_DEBUG("Setting Simple Cheat %d with value %x\n", simpleCheatName, value);

    switch(simpleCheatName) {
        case SIMPLE_CHEAT_NAME_CHANGE_NAME:
            return _apiChangeName(process, (char*)value);
        case SIMPLE_CHEAT_NAME_SET_SPEED:
            return _apiSetSpeed(process, (uint32_t)(*(int*)value));
        case SIMPLE_CHEAT_NAME_TELEPORT:
            return _apiTeleport(process, (TeleportCoords)(*(TeleportCoords*)value));
        case SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME:
            return _apiChangeHostname(process, (char*)value);
        case SIMPLE_CHEAT_NAME_FOV:
            return _apiFov(process, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_FOV_SCALE:
            return _apiFovScale(process, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_FPS_CAP:
            return _apiFpsCap(process, (uint32_t)(*(int*)value));
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_BACKGROUND:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P1:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P2:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P3:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P4:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_PRIMARY:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_SECONDARY:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_PRIMARY:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_SECONDARY:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_PRIMARY:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_SECONDARY:
            return _apiCustomizerColor(process, simpleCheatName, (Color)(*(Color*)value));
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_SCOREBOARD:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_POINTS:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MIN:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MAX:
            return _apiCustomizerFloat(process, simpleCheatName, ((float)(*(int*)value)/100.0f));   // These are percetaged
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_FREQUENCY:
            return _apiCustomizerFloat(process, simpleCheatName, (float)(*(int*)value));
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
    LOG_DEBUG("Next round successfully changed. Finish the current round %d and next round will be %d\n", currentRound, nextRound);
    return true;
}

bool apiIsGameReady(Api *api) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return false;
    }
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return false;
    }
    uint32_t ready;
    bool success = processRead(process, GAME_CHEAT.isGameReady, &ready, sizeof(ready));
    if (!success) {
        printf("Failed to read Is Game Ready value\n");
        return false;
    }
    return ready > 0; // This value starts getting populated when the initial loading screen ends. Semms like a timer tho.
}

bool apiIsZombiesGameOngoing(Api *api) {
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
    bool success = processRead(process, GAME_CHEAT.isZombiesGameOngoingOffset, &active, sizeof(uint32_t));
    if (!success) {
        printf("Failed to read Is Zombies Game Active value\n");
        return false;
    }
    return active == 1;
}

bool apiIsZombiesGamePaused(Api *api) {
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
    bool success = processRead(process, GAME_CHEAT.isZombiesGamePausedOffset, &active, sizeof(uint32_t));
    if (!success) {
        printf("Failed to read Is Game Paused value\n");
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

bool apiSVSendServerCommand(Api *api, int commandType, int clientNumber, const char *commands) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return NULL;
    }
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return NULL;
    }

    ServerCheat sendCommand = SERVER_CHEAT_SEND_COMMAND;
    CheatAsmInstructionSet asmSet = sendCommand.instructions;
    size_t offset = sendCommand.offset;
    uint8_t *bytes = (uint8_t*)malloc(asmSet.size * sizeof(uint8_t));
    memcpy(bytes, asmSet.instructions, asmSet.size);

    uintptr_t commandsAddr;
    size_t pageSize = sizeof(commandsAddr) + sizeof(commandType) + sizeof(clientNumber) + asmSet.size + (strlen(commands) + 1);
    
    uintptr_t addr;
    processAllocatePage(process, pageSize, &addr);
    uintptr_t relativeAddr = offset - (addr + 4 + 4 + 4 + 24);
    memcpy(bytes + 20, &relativeAddr, 4);    
    
    size_t commandOffset = 4 + 4 + 4 + asmSet.size + 1;
    commandsAddr = addr + commandOffset;
    processWrite(process, addr, &commandsAddr, sizeof(commandsAddr));
    processWrite(process, addr + 4, &commandType, sizeof(commandType));
    processWrite(process, addr + 8, &clientNumber, sizeof(clientNumber));
    processWrite(process, addr + 12, bytes, asmSet.size);
    processWrite(process, addr + commandOffset, commands, strlen(commands)+1);

    Thread *thread = threadCreateRemote(process, addr + 12, addr);
    bool success = true;
    if (!threadWait(thread, 100)) {
        LOG_ERROR("Thread wait timed out! Could not execute remote SV_SendServerCommand.\n");
        success = false;
    }
    threadClose(thread);
    free(bytes);
    processFreePage(process, addr);
    return success;
}

bool apiCBuffAddText(Api *api, const char *commands) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return NULL;
    }
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return NULL;
    }

    ServerCheat cbuffAddText = SERVER_CHEAT_CBUF_ADDTEXT;
    CheatAsmInstructionSet asmSet = cbuffAddText.instructions;
    size_t offset = cbuffAddText.offset;
    uint8_t *bytes = (uint8_t*)malloc(asmSet.size * sizeof(uint8_t));
    memcpy(bytes, asmSet.instructions, asmSet.size);

    size_t pageSize = asmSet.size + (strlen(commands) + 1);
    
    uintptr_t addr;
    processAllocatePage(process, pageSize, &addr);
    uintptr_t relativeAddr = offset - (addr + 12);
    memcpy(bytes + 8, &relativeAddr, 4);    
    
    processWrite(process, addr, bytes, asmSet.size);
    processWrite(process, addr + asmSet.size + 1, commands, strlen(commands)+1);
    Thread *thread = threadCreateRemote(process, addr, addr + asmSet.size + 1);
    bool success = true;
    if (!threadWait(thread, 100)) {
        LOG_ERROR("Thread wait timed out! Could not execute remote Cbuf_AddText.\n");
        success = false;
    }
    threadClose(thread);
    processFreePage(process, addr);
    free(bytes);
    return success;
}

uintptr_t apiGetDVarPointer(Api *api, const char *dVar) {
    if (!api || !api->controller) {
        LOG_ERROR("Api or Controller is null\n");
        return 0;
    }
    
    Process *process = controllerGetProcess(api->controller);
    if (!process) {
        LOG_ERROR("Process is null\n");
        return 0;
    }

    ServerCheat getDVarPtr = SERVER_CHEAT_GET_DVAR_PTR;
    size_t offset = getDVarPtr.offset;

    size_t pageSize = strlen(dVar) + 1;
    uintptr_t addr;
    processAllocatePage(process, pageSize, &addr);
    processWrite(process, addr, dVar, strlen(dVar)+1);
    Thread *thread = threadCreateRemote(process, offset, addr);
    bool success = true;
    if (!threadWait(thread, 100)) {
        LOG_ERROR("Thread wait timed out! Could not execute remote GetDVarPointer.\n");
        success = false;
    }
    int exitCode = success ? threadGetExitCode(thread) : 0;
    threadClose(thread);
    processFreePage(process, addr);
    return (uintptr_t)exitCode;
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
    LOG_DEBUG("address1 %x\n", address1);
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
    LOG_DEBUG("Reading Disable HUD pointer %x and inside is %x\n", CHEAT_DISABLE_HUD.offset, address1);
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

bool _apiGetFixMovementSpeed(Process *process) {
    uint32_t backwardsAddress1 = 0;
    bool success = processRead(process, CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS.offset, &backwardsAddress1, sizeof(backwardsAddress1));
    if (!success) {
        printf("Failed to read Fix Movement Speed Backward address\n");
        return false;
    }
    float backwardsValue = 0;
    success = processRead(process, backwardsAddress1 + 0x18, &backwardsValue, sizeof(backwardsValue));
    if (!success) {
        printf("Failed to read Fix Movement Speed Backward value\n");
        return false;
    }
    uint32_t straifAddress1 = 0;
    success = processRead(process, CHEAT_FIX_MOVEMENT_SPEED_STRAIF.offset, &straifAddress1, sizeof(straifAddress1));
    if (!success) {
        printf("Failed to read Fix Movement Speed Straif address\n");
        return false;
    }
    float straifValue = 0;
    success = processRead(process, straifAddress1 + 0x18, &straifValue, sizeof(straifValue));
    if (!success) {
        printf("Failed to read Fix Movement Speed Straif value\n");
        return false;
    }
    return backwardsValue == CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS.on.f32 && straifValue == CHEAT_FIX_MOVEMENT_SPEED_STRAIF.on.f32;
}

bool _apiSetFixMovementSpeed(Process *process, bool enabled) {
    uint32_t backwardsAddress1 = 0;
    bool success = processRead(process, CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS.offset, &backwardsAddress1, sizeof(backwardsAddress1));
    if (!success) {
        printf("Failed to read Fix Movement Speed Backwards address\n");
        return false;
    }
    uint32_t straifAddress1 = 0;
    success = processRead(process, CHEAT_FIX_MOVEMENT_SPEED_STRAIF.offset, &straifAddress1, sizeof(straifAddress1));
    if (!success) {
        printf("Failed to read Fix Movement Speed Straif address\n");
        return false;
    }
    float backwardsValue = enabled ? CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS.on.f32 : CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS.off.f32;
    float straifValue = enabled ? CHEAT_FIX_MOVEMENT_SPEED_STRAIF.on.f32 : CHEAT_FIX_MOVEMENT_SPEED_STRAIF.off.f32;
    return processWrite(process, backwardsAddress1 + 0x18, &backwardsValue, sizeof(backwardsValue)) && processWrite(process, straifAddress1 + 0x18, &straifValue, sizeof(straifValue));
}

bool _apiGetPatchChat(Process *process) {
    uint8_t value = 0;
    bool success = processRead(process, CHEAT_PATCH_CHAT.offset, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Patch Chat value\n");
        return false;
    }
    return value == CHEAT_PATCH_CHAT.on.byte;
}

bool _apiSetPatchChat(Process *process, bool enabled) {
    uint8_t value = enabled ? CHEAT_PATCH_CHAT.on.byte : CHEAT_PATCH_CHAT.off.byte;
    uint32_t oldProtect;
    processVirtualProtect(process, CHEAT_PATCH_CHAT.offset, sizeof(value), PAGE_EXECUTE_READWRITE, &oldProtect);
    bool success = processWrite(process, CHEAT_PATCH_CHAT.offset, &value, sizeof(value));
    processVirtualProtect(process, CHEAT_PATCH_CHAT.offset, sizeof(value), oldProtect, &oldProtect);
    return success;
}


bool _apiGetShowFps(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_SHOW_FPS.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Show FPS address\n");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        printf("Failed to read Show FPS value\n");
        return false;
    }
    return value == CHEAT_SHOW_FPS.on.byte;
}

bool _apiSetShowFps(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_SHOW_FPS.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Show FPS address\n");
        return false;
    }
    uint8_t value = enabled ? CHEAT_SHOW_FPS.on.byte : CHEAT_SHOW_FPS.off.byte;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
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
    LOG_DEBUG("Writting %d in %x\n", value, cheat.offset);
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

bool _apiChangeHostname(Process *process, char *hostname) {
    SimpleCheat cheat = SIMPLE_CHEAT_CHANGE_HOSTNAME;
    return processWrite(process, cheat.offset, hostname, strlen(hostname) + 1);
}

bool _apiFov(Process *process, float value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FOV;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Fov address\n");
        return false;
    }
    LOG_DEBUG("Writting %f in %x\n", value, cheat.offset);
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
    LOG_DEBUG("Writting %f in %x\n", value, cheat.offset);
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
    LOG_DEBUG("Writting %d in %x\n", value, cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(uint32_t));
}

// Colors
uint32_t _mergeColorComponents(Color color) {
    // Pack as: [A][B][G][R]
    // Being R less significative byte.
    uint32_t mergedColor = 0;

    mergedColor |= (uint32_t)color.r;         // Byte 0 (LSB)
    mergedColor |= (uint32_t)color.g << 8;    // Byte 1
    mergedColor |= (uint32_t)color.b << 16;   // Byte 2
                                              // Byte 3 (alfa) we dont care about this value 
    return mergedColor;
}

bool _apiCustomizerColor(Process *process, SimpleCheatName cheatName, Color color) {
    CustomizerCheat cheat = cheatGetCustomizerCheat(cheatName);
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.baseOffset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Customizer %d address\n", cheatName);
        return false;
    }
    uint32_t mergedColor = _mergeColorComponents(color);
    return processWrite(process, address1 + cheat.offset, &mergedColor, 3); // Only writing RGB, not overwriting Alpha.
}

bool _apiCustomizerFloat(Process *process, SimpleCheatName cheatName, float value) {
    CustomizerCheat cheat = cheatGetCustomizerCheat(cheatName);
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.baseOffset, &address1, sizeof(address1));
    if (!success) {
        printf("Failed to read Customizer %d address\n", cheatName);
        return false;
    }
    return processWrite(process, address1 + cheat.offset, &value, sizeof(value));
}



bool _apiSetSimpleCheatIntValue(Process *process, SimpleCheatName simpleCheatName, uint32_t value) {
    SimpleCheat cheat = cheatGetSimpleCheat(simpleCheatName);
    LOG_DEBUG("Writting %d in %x\n", value, cheat.offset);
    return processWrite(process, cheat.offset, &value, sizeof(uint32_t));
}
