#include "api/raw.h"
#include "logic/cheat.h"
#include "controller.h"
#include "logger.h"
#include "logic/game/level.h"
#include "utils/map.h"
#include "win/hook.h"
#include "win/process.h"
#include "win/thread.h"
#include <errhandlingapi.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

struct RawApi {
    Controller *controller;
    Map *hooks;
};

bool _rawApiGetGodMode(Process *process);
bool _rawApiSetGodMode(Process *process, Controller *controller, bool enabled);

bool _rawApiGetInvisible(Process *process);
bool _rawApiSetInvisible(Process *process, Controller *controller, bool enabled);

bool _rawApiGetNoClip(Process *process);
bool _rawApiSetNoClip(Process *process, bool enabled);

bool _rawApiGetNoRecoil(Process *process);
bool _rawApiSetNoRecoil(Process *process, bool enabled);

bool _rawApiGetSmallCrosshair(Process *process);
bool _rawApiSetSmallCrosshair(Process *process, bool enabled);

bool _rawApiGetFastGameplay(Process *process);
bool _rawApiSetFastGameplay(Process *process, bool enabled);

bool _rawApiGetNoShellshock(Process *process);
bool _rawApiSetNoShellshock(Process *process, bool enabled);

bool _rawApiGetIncreaseKnifeRange(Process *process);
bool _rawApiSetIncreaseKnifeRange(Process *process, bool enabled);

bool _rawApiGetBoxNeverMoves(Process *process);
bool _rawApiSetBoxNeverMoves(Process *process, bool enabled);

bool _rawApiGetThirdPerson(Process *process);
bool _rawApiSetThirdPerson(Process *process, bool enabled);

bool _rawApiGetInfiniteAmmo(Process *process);
bool _rawApiSetInfiniteAmmo(Process *process, bool enabled);

bool _rawApiGetInstantKill(Process *process, Map *hooks);
bool _rawApiSetInstantKill(Process *process, Map *hooks, bool enabled);

bool _rawApiGetMakeBorderless(Process *process);
bool _rawApiSetMakeBorderless(Process *process, bool enabled);

bool _rawApiGetUnlimitFps(Process *process);
bool _rawApiSetUnlimitFps(Process *process, Controller *controller, bool enabled);

bool _rawApiGetDisableHud(Process *process);
bool _rawApiSetDisableHud(Process *process, bool enabled);

bool _rawApiGetDisableFog(Process *process);
bool _rawApiSetDisableFog(Process *process, bool enabled);

bool _rawApiGetFullbright(Process *process);
bool _rawApiSetFullbright(Process *process, bool enabled);

bool _rawApiGetColorized(Process *process);
bool _rawApiSetColorized(Process *process, bool enabled);

bool _rawApiGetFixMovementSpeed(Process *process);
bool _rawApiSetFixMovementSpeed(Process *process, bool enabled);

bool _rawApiGetPatchChat(Process *process);
bool _rawApiSetPatchChat(Process *process, bool enabled);

bool _rawApiGetShowFps(Process *process);
bool _rawApiSetShowFps(Process *process, bool enabled);

bool _rawApiChangeName(Process *process, char *name);
bool _rawApiSetSpeed(Process *process, uint32_t value);
bool _rawApiTeleport(Process *process, TeleportCoords value);
bool _rawApiChangeHostname(Process *process, char *hostname);
bool _rawApiFov(Process *process, float value);
bool _rawApiFovScale(Process *process, float value);
bool _rawApiFpsCap(Process *process, uint32_t value);

bool _rawApiCustomizerColor(Process *process, SimpleCheatName cheatName, Color color);
bool _rawApiCustomizerFloat(Process *process, SimpleCheatName cheatName, float value);

bool _rawApiSetSimpleCheatIntValue(Process *process, SimpleCheatName simpleCheatName, uint32_t value);

// Hooks IDs (Hash for Hook Map)
static const char* HOOK_INSTANT_KILL_ID = "HOOK_INSTANT_KILL";

RawApi *rawApiCreate(Controller *controller) {
    RawApi *rawApi = (RawApi*)malloc(sizeof(RawApi));
    if (!rawApi) return NULL;
    rawApi->controller = controller;
    rawApi->hooks = mapCreate();
    if (!rawApi->hooks) {
        LOG_ERROR("Couldn't create Hook Map");
    }
    return rawApi;
}

void rawApiDestroy(RawApi *rawApi) {
    if (rawApi) {
        if (rawApi->hooks) {
            mapDestroy(rawApi->hooks);
        }
        free(rawApi);
    }
}

bool rawApiIsCheatEnabled(RawApi *rawApi, CheatName cheatName) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
    }

    switch (cheatName) {
        case CHEAT_NAME_GOD_MODE:
            return _rawApiGetGodMode(process);
        case CHEAT_NAME_INVISIBLE:
            return _rawApiGetInvisible(process);
        case CHEAT_NAME_NO_CLIP:
            return _rawApiGetNoClip(process);
        case CHEAT_NAME_NO_RECOIL:
            return _rawApiGetNoRecoil(process);
        case CHEAT_NAME_SMALL_CROSSHAIR:
            return _rawApiGetSmallCrosshair(process);
        case CHEAT_NAME_FAST_GAMEPLAY:
            return _rawApiGetFastGameplay(process);
        case CHEAT_NAME_NO_SHELLSHOCK:
            return _rawApiGetNoShellshock(process);
        case CHEAT_NAME_INCREASE_KNIFE_RANGE:
            return _rawApiGetIncreaseKnifeRange(process);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return _rawApiGetBoxNeverMoves(process);
        case CHEAT_NAME_THIRD_PERSON:
            return _rawApiGetThirdPerson(process);
        case CHEAT_NAME_INFINITE_AMMO:
            return _rawApiGetInfiniteAmmo(process);
        case CHEAT_NAME_INSTANT_KILL:
            return _rawApiGetInstantKill(process, rawApi->hooks);
        case CHEAT_NAME_MAKE_BORDERLESS:
            return _rawApiGetMakeBorderless(process);
        case CHEAT_NAME_UNLIMIT_FPS:
            return _rawApiGetUnlimitFps(process);
        case CHEAT_NAME_DISABLE_HUD:
            return _rawApiGetDisableHud(process);
        case CHEAT_NAME_DISABLE_FOG:
            return _rawApiGetDisableFog(process);
        case CHEAT_NAME_FULLBRIGHT:
            return _rawApiGetFullbright(process);
        case CHEAT_NAME_COLORIZED:
            return _rawApiGetColorized(process);
        case CHEAT_NAME_FIX_MOVEMENT_SPEED:
            return _rawApiGetFixMovementSpeed(process);
        case CHEAT_NAME_SHOW_FPS:
            return _rawApiGetShowFps(process);
        case CHEAT_NAME_PATCH_CHAT:
            return _rawApiGetPatchChat(process);
        
        default:
            LOG_WARN("Unknown cheatName %d", cheatName);
            return false;
    }
}

bool rawApiSetCheatEnabled(RawApi *rawApi, CheatName cheatName, bool enabled) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
    }

    switch (cheatName) {
        case CHEAT_NAME_GOD_MODE:
            return _rawApiSetGodMode(process, rawApi->controller, enabled);
        case CHEAT_NAME_INVISIBLE:
            return _rawApiSetInvisible(process, rawApi->controller, enabled);
        case CHEAT_NAME_NO_CLIP:
            return _rawApiSetNoClip(process, enabled);
        case CHEAT_NAME_NO_RECOIL:
            return _rawApiSetNoRecoil(process, enabled);
        case CHEAT_NAME_SMALL_CROSSHAIR:
            return _rawApiSetSmallCrosshair(process, enabled);
        case CHEAT_NAME_FAST_GAMEPLAY:
            return _rawApiSetFastGameplay(process, enabled);
        case CHEAT_NAME_NO_SHELLSHOCK:
            return _rawApiSetNoShellshock(process, enabled);
        case CHEAT_NAME_INCREASE_KNIFE_RANGE:
            return _rawApiSetIncreaseKnifeRange(process, enabled);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return _rawApiSetBoxNeverMoves(process, enabled);
        case CHEAT_NAME_THIRD_PERSON:
            return _rawApiSetThirdPerson(process, enabled);
        case CHEAT_NAME_INFINITE_AMMO:
            return _rawApiSetInfiniteAmmo(process, enabled);
        case CHEAT_NAME_INSTANT_KILL:
            return _rawApiSetInstantKill(process, rawApi->hooks, enabled);
        case CHEAT_NAME_MAKE_BORDERLESS:
            return _rawApiSetMakeBorderless(process, enabled);
        case CHEAT_NAME_UNLIMIT_FPS:
            return _rawApiSetUnlimitFps(process, rawApi->controller, enabled);
        case CHEAT_NAME_DISABLE_HUD:
            return _rawApiSetDisableHud(process, enabled);
        case CHEAT_NAME_DISABLE_FOG:
            return _rawApiSetDisableFog(process, enabled);
        case CHEAT_NAME_FULLBRIGHT:
            return _rawApiSetFullbright(process, enabled);
        case CHEAT_NAME_COLORIZED:
            return _rawApiSetColorized(process, enabled);
        case CHEAT_NAME_FIX_MOVEMENT_SPEED:
            return _rawApiSetFixMovementSpeed(process, enabled);
        case CHEAT_NAME_SHOW_FPS:
            return _rawApiSetShowFps(process, enabled);
        case CHEAT_NAME_PATCH_CHAT:
            return _rawApiSetPatchChat(process, enabled);
        default:
            LOG_WARN("Unknown cheatName %d", cheatName);
            return false;
    }
}

bool rawApiSetSimpleCheat(RawApi *rawApi, SimpleCheatName simpleCheatName, void *value) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
    }

    LOG_DEBUG("Setting Simple Cheat %d with value %x", simpleCheatName, value);

    switch(simpleCheatName) {
        case SIMPLE_CHEAT_NAME_CHANGE_NAME:
            return _rawApiChangeName(process, (char*)value);
        case SIMPLE_CHEAT_NAME_SET_SPEED:
            return _rawApiSetSpeed(process, (uint32_t)(*(int*)value));
        case SIMPLE_CHEAT_NAME_TELEPORT:
            return _rawApiTeleport(process, (TeleportCoords)(*(TeleportCoords*)value));
        case SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME:
            return _rawApiChangeHostname(process, (char*)value);
        case SIMPLE_CHEAT_NAME_FOV:
            return _rawApiFov(process, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_FOV_SCALE:
            return _rawApiFovScale(process, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_FPS_CAP:
            return _rawApiFpsCap(process, (uint32_t)(*(int*)value));
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
            return _rawApiCustomizerColor(process, simpleCheatName, (Color)(*(Color*)value));
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_SCOREBOARD:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_POINTS:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MIN:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MAX:
            return _rawApiCustomizerFloat(process, simpleCheatName, ((float)(*(int*)value)/100.0f));
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_FREQUENCY:
            return _rawApiCustomizerFloat(process, simpleCheatName, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_SET_HEALTH:
        case SIMPLE_CHEAT_NAME_SET_POINTS:
        case SIMPLE_CHEAT_NAME_SET_KILLS:
        case SIMPLE_CHEAT_NAME_SET_HEADSHOTS:
            return _rawApiSetSimpleCheatIntValue(process, simpleCheatName, (uint32_t)(*(int*)value));
        default:
            LOG_WARN("Unknown simpleCheatName %d", simpleCheatName);
            return false;
    }
}

TeleportCoords *rawApiGetPlayerCurrentCoords(RawApi *rawApi) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return NULL;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return NULL;
    }

    TeleportCoords *coords = (TeleportCoords*)malloc(sizeof(TeleportCoords));
    processRead(process, TELEPORT_CHEAT.xOffset, &coords->x, sizeof(coords->x));
    processRead(process, TELEPORT_CHEAT.yOffset, &coords->y, sizeof(coords->y));
    processRead(process, TELEPORT_CHEAT.zOffset, &coords->z, sizeof(coords->z));

    return coords;
}

bool rawApiSetRound(RawApi *rawApi, int currentRound, int nextRound) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
    }
    nextRound = nextRound - 1; // This is needed since this value overrides the current level round. We substract one to jump to the desired round after completing it.
    uint8_t pattern[ROUND_CHANGE_PATTERN_SIZE];
    memcpy(pattern, ROUND_CHEAT.pattern, ROUND_CHEAT.patternSize);
    memcpy(pattern, &currentRound, 4*sizeof(uint8_t));
    uintptr_t addressFound;
    bool found = processFindPattern(process, ROUND_CHEAT.regionOffset, ROUND_CHEAT.regionSize, pattern, sizeof(pattern), &addressFound);
    if (!found) {
        LOG_ERROR("Couldn't find Round Change memory pattern!");
        return false;
    }
    bool success = processWrite(process, addressFound, &nextRound, sizeof(nextRound));
    if (!success) {
        LOG_ERROR("Failed to write Next Round value");
        return false;
    }
    LOG_DEBUG("Next round successfully changed. Finish the current round %d and next round will be %d", currentRound, nextRound);
    return true;
}

bool rawApiIsGameReady(RawApi *rawApi) {
    return rawApiGetLevelElapsedTime(rawApi) > 0;
}

Level rawApiGetLevelName(RawApi *rawApi) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return LEVEL_INVALID;
    }

    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return LEVEL_INVALID;
    }

    uint32_t address1;
    bool success = processRead(process, GAME_CHEAT.levelName, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Level Name address1");
        return LEVEL_INVALID;
    }

    uint32_t address2;
    success = processRead(process, address1 + 0x18, &address2, sizeof(address2));
    if (!success) {
        LOG_ERROR("Failed to read Level Name address2");
        return LEVEL_INVALID;
    }

    char levelId[64];
    success = processReadString(process, address2, levelId);
    if (!success) {
        LOG_ERROR("Failed to read Level Id value");
        return LEVEL_INVALID;
    }
    return levelGetFromId(levelId);
}

double rawApiGetLevelElapsedTime(RawApi *rawApi) {
    if (!rawApi || !rawApi->controller) {
        LOG_DEBUG("RawApi or Controller is null");
        return 0;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_DEBUG("Process is null");
        return 0;
    }
    
    // Check if process handle is still valid (avoids race condition during game restart)
    if (!processIsValid(process)) {
        return 0;
    }
    
    uint32_t elapsed;
    bool success = processRead(process, GAME_CHEAT.levelElapsed, &elapsed, sizeof(elapsed));
    if (!success) {
        LOG_ERROR("Failed to read Game Level Elapsed Time Ready value %d", GetLastError());
        return 0;
    }
    return elapsed;
}

float rawApiGetMovementSpeed(RawApi *rawApi) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
    }
    float speed;
    bool success = processRead(process, GAME_CHEAT.movementSpeed, &speed, sizeof(speed));
    if (!success) {
        LOG_ERROR("Failed to read Movement Speed value");
        return false;
    }
    return speed;
}

bool rawApiIsZombiesGameOngoing(RawApi *rawApi) {
    if (!rawApi || !rawApi->controller) {
        LOG_DEBUG("RawApi or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_DEBUG("Process is null");
        return false;
    }
    uint32_t active;
    bool success = processRead(process, GAME_CHEAT.isZombiesGameOngoingOffset, &active, sizeof(uint32_t));
    if (!success) {
        LOG_ERROR("Failed to read Is Zombies Game Active value");
        return false;
    }
    return active == 1;
}

bool rawApiIsZombiesGamePaused(RawApi *rawApi) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
    }
    uint32_t active;
    bool success = processRead(process, GAME_CHEAT.isZombiesGamePausedOffset, &active, sizeof(uint32_t));
    if (!success) {
        LOG_ERROR("Failed to read Is Game Paused value");
        return false;
    }
    return active == 1;
}

int rawApiGetGameResets(RawApi *rawApi) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return 0;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return 0;
    }
    uint32_t resets;
    bool success = processRead(process, GAME_CHEAT.nResetsOffset, &resets, sizeof(uint32_t));
    if (!success) {
        LOG_ERROR("Failed to read Game Resets value");
        return 0;
    }
    resets = resets == 0 ? resets : resets - 1;
    return (int)resets; 
}

bool rawApiSVSendServerCommand(RawApi *rawApi, int commandType, int clientNumber, const char *commands) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
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
        LOG_ERROR("Thread wait timed out! Could not execute remote SV_SendServerCommand.");
        success = false;
    }
    threadClose(thread);
    free(bytes);
    processFreePage(process, addr);
    return success;
}

bool rawApiCBuffAddText(RawApi *rawApi, const char *commands) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
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
        LOG_ERROR("Thread wait timed out! Could not execute remote Cbuf_AddText.");
        success = false;
    }
    threadClose(thread);
    processFreePage(process, addr);
    free(bytes);
    return success;
}

uintptr_t rawApiGetDVarPointer(RawApi *rawApi, const char *dVar) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return 0;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
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
        LOG_ERROR("Thread wait timed out! Could not execute remote GetDVarPointer.");
        success = false;
    }
    int exitCode = success ? threadGetExitCode(thread) : 0;
    threadClose(thread);
    processFreePage(process, addr);
    return (uintptr_t)exitCode;
}

// Private implementations

bool _rawApiGetGodMode(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_GOD_MODE.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read God Mode value");
        return false;
    }
    return value == CHEAT_GOD_MODE.on.u32;
}

bool _rawApiSetGodMode(Process *process, Controller *controller, bool enabled) {
    bool isInvisibleModeChecked = controllerIsCheatCheckboxChecked(controller, CHEAT_NAME_INVISIBLE);
    uint32_t value = enabled ? CHEAT_GOD_MODE.on.u32 : (isInvisibleModeChecked ? CHEAT_INVISIBLE.on.u32 : CHEAT_INVISIBLE.off.u32);
    return processWrite(process, CHEAT_GOD_MODE.offset, &value, sizeof(value));
}

bool _rawApiGetInvisible(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_INVISIBLE.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Invisible value");
        return false;
    }
    return value == CHEAT_INVISIBLE.on.u32;
}

bool _rawApiSetInvisible(Process *process, Controller *controller, bool enabled) {
    bool isGodModeChecked = controllerIsCheatCheckboxChecked(controller, CHEAT_NAME_GOD_MODE);
    uint32_t value = enabled ? CHEAT_INVISIBLE.on.u32 : (isGodModeChecked ? CHEAT_GOD_MODE.on.u32 : CHEAT_GOD_MODE.off.u32);
    return processWrite(process, CHEAT_INVISIBLE.offset, &value, sizeof(value));
}

bool _rawApiGetNoClip(Process *process) {
    uint8_t value = 0;
    bool success = processRead(process, CHEAT_NO_CLIP.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read No Clip value");
        return false;
    }
    return value == CHEAT_NO_CLIP.on.byte;
}

bool _rawApiSetNoClip(Process *process, bool enabled) {
    uint8_t value = enabled ? CHEAT_NO_CLIP.on.byte : CHEAT_NO_CLIP.off.byte;
    return processWrite(process, CHEAT_NO_CLIP.offset, &value, sizeof(value));
}

bool _rawApiGetNoRecoil(Process *process) {
    uint8_t value = 0;
    bool success = processRead(process, CHEAT_NO_RECOIL.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read No Recoil value");
        return false;
    }
    return value == CHEAT_NO_RECOIL.on.byte;
}

bool _rawApiSetNoRecoil(Process *process, bool enabled) {
    uint8_t value = enabled ? CHEAT_NO_RECOIL.on.byte : CHEAT_NO_RECOIL.off.byte;
    return processWrite(process, CHEAT_NO_RECOIL.offset, &value, sizeof(value));
}

bool _rawApiGetFastGameplay(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_FAST_GAMEPLAY.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Fast Gameplay address");
        return false;
    }
    float value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Fast Gameplay value");
        return false;
    }
    return value == CHEAT_FAST_GAMEPLAY.on.f32;
}

bool _rawApiSetFastGameplay(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_FAST_GAMEPLAY.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Fast Gameplay address");
        return false;
    }
    float value = enabled ? CHEAT_FAST_GAMEPLAY.on.f32 : CHEAT_FAST_GAMEPLAY.off.f32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _rawApiGetNoShellshock(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_NO_SHELLSHOCK.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read No Shellshock address");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read No Shellshock value");
        return false;
    }
    return value == CHEAT_NO_SHELLSHOCK.on.u32;
}

bool _rawApiSetNoShellshock(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_NO_SHELLSHOCK.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read No Shellshock address");
        return false;
    }
    uint8_t value = enabled ? CHEAT_NO_SHELLSHOCK.on.u32 : CHEAT_NO_SHELLSHOCK.off.u32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _rawApiGetIncreaseKnifeRange(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_INCREASE_KNIFE_RANGE.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Increase Knife Range address");
        return false;
    }
    float value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Increase Knife Range value");
        return false;
    }
    return value == CHEAT_INCREASE_KNIFE_RANGE.on.f32;
}

bool _rawApiSetIncreaseKnifeRange(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_INCREASE_KNIFE_RANGE.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Increase Knife Range address");
        return false;
    }
    float value = enabled ? CHEAT_INCREASE_KNIFE_RANGE.on.f32 : CHEAT_INCREASE_KNIFE_RANGE.off.f32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _rawApiGetBoxNeverMoves(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_BOX_NEVER_MOVES.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Box Never Moves address");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Box Never Moves value");
        return false;
    }
    return value == CHEAT_BOX_NEVER_MOVES.on.u32;
}

bool _rawApiSetBoxNeverMoves(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_BOX_NEVER_MOVES.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Box Never Moves address");
        return false;
    }
    uint8_t value = enabled ? CHEAT_BOX_NEVER_MOVES.on.u32 : CHEAT_BOX_NEVER_MOVES.off.u32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _rawApiGetThirdPerson(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_THIRD_PERSON.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Third Person address");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Third Person value");
        return false;
    }
    return value == CHEAT_THIRD_PERSON.on.byte;
}

bool _rawApiSetThirdPerson(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_THIRD_PERSON.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Third Person address");
        return false;
    }
    uint8_t value = enabled ? CHEAT_THIRD_PERSON.on.byte : CHEAT_THIRD_PERSON.off.byte;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _rawApiGetInfiniteAmmo(Process *process) {
    uint8_t buffer[MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE];
    bool success = processRead(process, CHEAT_ASM_INFINITE_AMMO.offset, &buffer, sizeof(buffer));
    if (!success) {
        LOG_ERROR("Failed to read Infinite Ammo value");
        return false;
    }
    bool infiniteAmmoEnabled = memcmp(CHEAT_ASM_INFINITE_AMMO.on.instructions, buffer, CHEAT_ASM_INFINITE_AMMO.on.size) == 0;
    if (infiniteAmmoEnabled) return true;

    bool infiniteAmmoDisabled = memcmp(CHEAT_ASM_INFINITE_AMMO.off.instructions, buffer, CHEAT_ASM_INFINITE_AMMO.off.size) == 0;
    if (!infiniteAmmoDisabled) {
        LOG_WARN("Infinite Ammo bytes do not match known patterns. Possible memory corruption or external modification.");
        return false;
    }
    return false;
}

bool _rawApiSetInfiniteAmmo(Process *process, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_INFINITE_AMMO;
    uint8_t *instructions = enabled ? instructionSet->on.instructions : instructionSet->off.instructions;
    size_t size = enabled ? instructionSet->on.size : instructionSet->off.size;
    return processWrite(process, instructionSet->offset, instructions, size);
}

bool _rawApiGetSmallCrosshair(Process *process) {
    uint8_t buffer[MAX_CHEAT_ASM_INSTRUCTION_SET_SIZE];
    bool success = processRead(process, CHEAT_ASM_SMALL_CROSSHAIR.offset, &buffer, sizeof(buffer));
    if (!success) {
        LOG_ERROR("Failed to read Small Crosshair value");
        return false;
    }
    bool smallCrosshairEnabled = memcmp(CHEAT_ASM_SMALL_CROSSHAIR.on.instructions, buffer, CHEAT_ASM_SMALL_CROSSHAIR.on.size) == 0;
    if (smallCrosshairEnabled) return true;

    bool smallCrosshairDisabled = memcmp(CHEAT_ASM_SMALL_CROSSHAIR.off.instructions, buffer, CHEAT_ASM_SMALL_CROSSHAIR.off.size) == 0;
    if (!smallCrosshairDisabled) {
        LOG_WARN("Small Crosshair bytes do not match known patterns. Possible memory corruption or external modification.");
        return false;
    }
    return false;
}

bool _rawApiSetSmallCrosshair(Process *process, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_SMALL_CROSSHAIR;
    uint8_t *instructions = enabled ? instructionSet->on.instructions : instructionSet->off.instructions;
    size_t size = enabled ? instructionSet->on.size : instructionSet->off.size;
    bool success = processWrite(process, instructionSet->offset, instructions, size);
    if (!success) {
        LOG_ERROR("Failed to write Asm code for Small Crosshair.");
        return false;
    }
    uint32_t address1 = 0;
    success = processRead(process, CHEAT_SMALL_CROSSHAIR.offset, &address1, sizeof(address1));
    LOG_DEBUG("address1 %x", address1);
    if (!success) {
        LOG_ERROR("Failed to read Small Crosshair address");
        return false;
    }
    float value = enabled ? CHEAT_SMALL_CROSSHAIR.on.f32 : CHEAT_SMALL_CROSSHAIR.off.f32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _rawApiGetInstantKill(Process *process, Map *hooks) {
    CheatAsm *instructionSet = &CHEAT_ASM_INSTANT_KILL;
    Hook *hook;
    if (!mapContains(hooks, HOOK_INSTANT_KILL_ID)) {
        hook = hookCreate(process, instructionSet->offset, instructionSet->off.size, instructionSet->on.instructions, instructionSet->on.size, instructionSet->off.instructions);
        mapPut(hooks, HOOK_INSTANT_KILL_ID, hook);
    } else {
        hook = (Hook*)mapGet(hooks, HOOK_INSTANT_KILL_ID);
    }
    
    return hookIsActivated(hook);
}

bool _rawApiSetInstantKill(Process *process, Map *hooks, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_INSTANT_KILL;
    Hook *hook;
    if (!mapContains(hooks, HOOK_INSTANT_KILL_ID)) {
        hook = hookCreate(process, instructionSet->offset, instructionSet->off.size, instructionSet->on.instructions, instructionSet->on.size, instructionSet->off.instructions);
        mapPut(hooks, HOOK_INSTANT_KILL_ID, hook);
    } else {
        hook = (Hook*)mapGet(hooks, HOOK_INSTANT_KILL_ID);
    }
    return enabled ? hookActivate(hook) : hookDeactivate(hook);
}

bool _rawApiGetMakeBorderless(Process *process) {
    return processIsBorderless(process);
}

bool _rawApiSetMakeBorderless(Process *process, bool enabled) {
    return processMakeBorderless(process, enabled);
}

bool _rawApiGetUnlimitFps(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_UNLIMIT_FPS.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Unlimit Fps address");
        return false;
    }
    uint32_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Unlimit Fps value");
        return false;
    }
    return value == CHEAT_UNLIMIT_FPS.on.u32;
}

bool _rawApiSetUnlimitFps(Process *process, Controller *controller, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_UNLIMIT_FPS.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Unlimit Fps address");
        return false;
    }
    uint32_t value = enabled ? CHEAT_UNLIMIT_FPS.on.u32 : (uint32_t)controllerUiGraphicsGetFpsCap(controller);
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _rawApiGetDisableHud(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_DISABLE_HUD.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Disable Hud address");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Disable Hud value");
        return false;
    }
    return value == CHEAT_DISABLE_HUD.on.byte;
}

bool _rawApiSetDisableHud(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_DISABLE_HUD.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Disable Hud address");
        return false;
    }
    LOG_DEBUG("Reading Disable HUD pointer %x and inside is %x", CHEAT_DISABLE_HUD.offset, address1);
    uint8_t value = enabled ? CHEAT_DISABLE_HUD.on.byte : CHEAT_DISABLE_HUD.off.byte;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _rawApiGetDisableFog(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_DISABLE_FOG.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Disable Fog address");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Disable Fog value");
        return false;
    }
    return value == CHEAT_DISABLE_FOG.on.byte;
}

bool _rawApiSetDisableFog(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_DISABLE_FOG.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Disable Fog address");
        return false;
    }
    uint8_t value = enabled ? CHEAT_DISABLE_FOG.on.byte : CHEAT_DISABLE_FOG.off.byte;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _rawApiGetFullbright(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_FULLBRIGHT.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Fullbright value");
        return false;
    }
    return value == CHEAT_FULLBRIGHT.on.u32;
}

bool _rawApiSetFullbright(Process *process, bool enabled) {
    uint32_t value = enabled ? CHEAT_FULLBRIGHT.on.u32 : CHEAT_FULLBRIGHT.off.u32;
    return processWrite(process, CHEAT_FULLBRIGHT.offset, &value, sizeof(value));
}

bool _rawApiGetColorized(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_COLORIZED.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Colorized value");
        return false;
    }
    return value == CHEAT_COLORIZED.on.u32;
}

bool _rawApiSetColorized(Process *process, bool enabled) {
    uint32_t value = enabled ? CHEAT_COLORIZED.on.u32 : CHEAT_COLORIZED.off.u32;
    return processWrite(process, CHEAT_COLORIZED.offset, &value, sizeof(value));
}

bool _rawApiGetFixMovementSpeed(Process *process) {
    uint32_t backwardsAddress1 = 0;
    bool success = processRead(process, CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS.offset, &backwardsAddress1, sizeof(backwardsAddress1));
    if (!success) {
        LOG_ERROR("Failed to read Fix Movement Speed Backward address");
        return false;
    }
    float backwardsValue = 0;
    success = processRead(process, backwardsAddress1 + 0x18, &backwardsValue, sizeof(backwardsValue));
    if (!success) {
        LOG_ERROR("Failed to read Fix Movement Speed Backward value");
        return false;
    }
    uint32_t straifAddress1 = 0;
    success = processRead(process, CHEAT_FIX_MOVEMENT_SPEED_STRAIF.offset, &straifAddress1, sizeof(straifAddress1));
    if (!success) {
        LOG_ERROR("Failed to read Fix Movement Speed Straif address");
        return false;
    }
    float straifValue = 0;
    success = processRead(process, straifAddress1 + 0x18, &straifValue, sizeof(straifValue));
    if (!success) {
        LOG_ERROR("Failed to read Fix Movement Speed Straif value");
        return false;
    }
    return backwardsValue == CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS.on.f32 && straifValue == CHEAT_FIX_MOVEMENT_SPEED_STRAIF.on.f32;
}

bool _rawApiSetFixMovementSpeed(Process *process, bool enabled) {
    uint32_t backwardsAddress1 = 0;
    bool success = processRead(process, CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS.offset, &backwardsAddress1, sizeof(backwardsAddress1));
    if (!success) {
        LOG_ERROR("Failed to read Fix Movement Speed Backwards address");
        return false;
    }
    uint32_t straifAddress1 = 0;
    success = processRead(process, CHEAT_FIX_MOVEMENT_SPEED_STRAIF.offset, &straifAddress1, sizeof(straifAddress1));
    if (!success) {
        LOG_ERROR("Failed to read Fix Movement Speed Straif address");
        return false;
    }
    float backwardsValue = enabled ? CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS.on.f32 : CHEAT_FIX_MOVEMENT_SPEED_BACKWARDS.off.f32;
    float straifValue = enabled ? CHEAT_FIX_MOVEMENT_SPEED_STRAIF.on.f32 : CHEAT_FIX_MOVEMENT_SPEED_STRAIF.off.f32;
    return processWrite(process, backwardsAddress1 + 0x18, &backwardsValue, sizeof(backwardsValue)) && processWrite(process, straifAddress1 + 0x18, &straifValue, sizeof(straifValue));
}

bool _rawApiGetPatchChat(Process *process) {
    uint8_t value = 0;
    bool success = processRead(process, CHEAT_PATCH_CHAT.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Patch Chat value");
        return false;
    }
    return value == CHEAT_PATCH_CHAT.on.byte;
}

bool _rawApiSetPatchChat(Process *process, bool enabled) {
    uint8_t value = enabled ? CHEAT_PATCH_CHAT.on.byte : CHEAT_PATCH_CHAT.off.byte;
    uint32_t oldProtect;
    processVirtualProtect(process, CHEAT_PATCH_CHAT.offset, sizeof(value), PAGE_EXECUTE_READWRITE, &oldProtect);
    bool success = processWrite(process, CHEAT_PATCH_CHAT.offset, &value, sizeof(value));
    processVirtualProtect(process, CHEAT_PATCH_CHAT.offset, sizeof(value), oldProtect, &oldProtect);
    return success;
}

bool _rawApiGetShowFps(Process *process) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_SHOW_FPS.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Show FPS address");
        return false;
    }
    uint8_t value = 0;
    success = processRead(process, address1 + 0x18, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Show FPS value");
        return false;
    }
    return value == CHEAT_SHOW_FPS.on.byte;
}

bool _rawApiSetShowFps(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_SHOW_FPS.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Show FPS address");
        return false;
    }
    uint8_t value = enabled ? CHEAT_SHOW_FPS.on.byte : CHEAT_SHOW_FPS.off.byte;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

// Simple cheats

bool _rawApiChangeName(Process *process, char *name) {
    SimpleCheat cheat = SIMPLE_CHEAT_CHANGE_NAME;
    return processWrite(process, cheat.offset, name, strlen(name) + 1);
}

bool _rawApiSetSpeed(Process *process, uint32_t value) {
    SimpleCheat cheat = SIMPLE_CHEAT_SET_SPEED;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Speed address");
        return false;
    }
    LOG_DEBUG("Writing %d in %x", value, cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(uint32_t));
}

bool _rawApiTeleport(Process *process, TeleportCoords value) {
    return processWrite(process, TELEPORT_CHEAT.xOffset, &(value.x), sizeof(value.x)) &&
           processWrite(process, TELEPORT_CHEAT.yOffset, &(value.y), sizeof(value.y)) &&
           processWrite(process, TELEPORT_CHEAT.zOffset, &(value.z), sizeof(value.z));
}

bool _rawApiChangeHostname(Process *process, char *hostname) {
    SimpleCheat cheat = SIMPLE_CHEAT_CHANGE_HOSTNAME;
    return processWrite(process, cheat.offset, hostname, strlen(hostname) + 1);
}

bool _rawApiFov(Process *process, float value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FOV;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Fov address");
        return false;
    }
    LOG_DEBUG("Writing %f in %x", value, cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(float));
}

bool _rawApiFovScale(Process *process, float value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FOV_SCALE;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Fov Scale address");
        return false;
    }
    value = value/100;
    LOG_DEBUG("Writing %f in %x", value, cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(float));
}

bool _rawApiFpsCap(Process *process, uint32_t value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FPS_CAP;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Fps Cap address");
        return false;
    }
    LOG_DEBUG("Writing %d in %x", value, cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(uint32_t));
}

// Colors
static uint32_t _mergeColorComponents(Color color) {
    uint32_t mergedColor = 0;
    mergedColor |= (uint32_t)color.r;
    mergedColor |= (uint32_t)color.g << 8;
    mergedColor |= (uint32_t)color.b << 16;
    return mergedColor;
}

bool _rawApiCustomizerColor(Process *process, SimpleCheatName cheatName, Color color) {
    CustomizerCheat cheat = cheatGetCustomizerCheat(cheatName);
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.baseOffset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Customizer %d address", cheatName);
        return false;
    }
    uint32_t mergedColor = _mergeColorComponents(color);
    return processWrite(process, address1 + cheat.offset, &mergedColor, 3);
}

bool _rawApiCustomizerFloat(Process *process, SimpleCheatName cheatName, float value) {
    CustomizerCheat cheat = cheatGetCustomizerCheat(cheatName);
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.baseOffset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Customizer %d address", cheatName);
        return false;
    }
    return processWrite(process, address1 + cheat.offset, &value, sizeof(value));
}

bool _rawApiSetSimpleCheatIntValue(Process *process, SimpleCheatName simpleCheatName, uint32_t value) {
    SimpleCheat cheat = cheatGetSimpleCheat(simpleCheatName);
    LOG_DEBUG("Writing %d in %x", value, cheat.offset);
    return processWrite(process, cheat.offset, &value, sizeof(uint32_t));
}

int rawApiGetClaymoreCount(RawApi *rawApi) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return 0;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return 0;
    }

    uint32_t entityCount = 0;
    bool success = processRead(process, GAME_CHEAT.entityCountOffset, &entityCount, sizeof(entityCount));
    if (!success) {
        LOG_ERROR("Failed to read entity count");
        return 0;
    }

    uint32_t entityAddr = GAME_CHEAT.entityBaseOffset;
    int32_t claymoreCount = 0;
    for (uint32_t i = 0; i < entityCount; i++) {
        uint8_t isActive = 0;
        processRead(process, entityAddr - 71, &isActive, sizeof(isActive));
        
        if (isActive != 0) {
            uint16_t entityType = 0;
            processRead(process, entityAddr - 102, &entityType, sizeof(entityType));
            
            // 0x4 = Missile (claymores/betties)
            if (entityType == 0x4) {
                claymoreCount++;
            }
        }
        
        entityAddr += 844;
    }

    return claymoreCount;
}

int rawApiGetCurrentSnapshotEntities(RawApi *rawApi) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return 0;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return 0;
    }

    int32_t current = 0;
    processRead(process, GAME_CHEAT.currentSnapshotEntitiesOffset, &current, sizeof(current));
    return current;
}

int rawApiGetMaxSnapshotEntities(RawApi *rawApi) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return 0;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return 0;
    }

    int32_t maxRaw = 0;
    processRead(process, GAME_CHEAT.maxSnapshotEntitiesOffset, &maxRaw, sizeof(maxRaw));
    // 2147483646 - maxRaw
    return 2147483646 - maxRaw;
}

bool rawApiIsChatOpen(RawApi *rawApi) {
    if (!rawApi || !rawApi->controller) {
        LOG_ERROR("RawApi or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
    }

    uint32_t chatStatus = 0;
    bool success = processRead(process, GAME_CHEAT.chatStatusOffset, &chatStatus, sizeof(chatStatus));
    if (!success) {
        return false;
    }
    return chatStatus == 0x20; // 0x20 -> Chat is opened
}

bool rawApiWriteToChatInput(RawApi *rawApi, const char *text) {
    if (!rawApi || !rawApi->controller) {
        return false;
    }
    
    Process *process = controllerGetProcess(rawApi->controller);
    if (!process) {
        return false;
    }

    if (!rawApiIsChatOpen(rawApi)) {
        LOG_WARN("Writing to Chat Input when chat is not open shouldn't happen!");
        return false;
    }

    char buffer[256] = {0};
    if (text) {
        strncpy(buffer, text, sizeof(buffer) - 1);
    }
    return processWrite(process, GAME_CHEAT.chatInputBufferOffset, buffer, sizeof(buffer));
}
