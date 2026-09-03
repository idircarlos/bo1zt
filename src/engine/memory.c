#include "engine/memory.h"
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

struct MemoryBackend {
    Controller *controller;
    Map *hooks;
};

bool _memoryBackendGetGodMode(Process *process);
bool _memoryBackendSetGodMode(Process *process, Controller *controller, bool enabled);

bool _memoryBackendGetInvisible(Process *process);
bool _memoryBackendSetInvisible(Process *process, Controller *controller, bool enabled);

bool _memoryBackendGetNoClip(Process *process);
bool _memoryBackendSetNoClip(Process *process, bool enabled);

bool _memoryBackendGetNoRecoil(Process *process);
bool _memoryBackendSetNoRecoil(Process *process, bool enabled);

bool _memoryBackendGetSmallCrosshair(Process *process);
bool _memoryBackendSetSmallCrosshair(Process *process, bool enabled);

bool _memoryBackendGetFastGameplay(Process *process);
bool _memoryBackendSetFastGameplay(Process *process, bool enabled);

bool _memoryBackendGetNoShellshock(Process *process);
bool _memoryBackendSetNoShellshock(Process *process, bool enabled);

bool _memoryBackendGetIncreaseKnifeRange(Process *process);
bool _memoryBackendSetIncreaseKnifeRange(Process *process, bool enabled);

bool _memoryBackendGetBoxNeverMoves(Process *process);
bool _memoryBackendSetBoxNeverMoves(Process *process, bool enabled);

bool _memoryBackendGetThirdPerson(Process *process);
bool _memoryBackendSetThirdPerson(Process *process, bool enabled);

bool _memoryBackendGetInfiniteAmmo(Process *process);
bool _memoryBackendSetInfiniteAmmo(Process *process, bool enabled);

bool _memoryBackendGetInstantKill(Process *process, Map *hooks);
bool _memoryBackendSetInstantKill(Process *process, Map *hooks, bool enabled);

bool _memoryBackendGetMakeBorderless(Process *process);
bool _memoryBackendSetMakeBorderless(Process *process, bool enabled);

bool _memoryBackendGetUnlimitFps(Process *process);
bool _memoryBackendSetUnlimitFps(Process *process, Controller *controller, bool enabled);

bool _memoryBackendGetDisableHud(Process *process);
bool _memoryBackendSetDisableHud(Process *process, bool enabled);

bool _memoryBackendGetDisableFog(Process *process);
bool _memoryBackendSetDisableFog(Process *process, bool enabled);

bool _memoryBackendGetFullbright(Process *process);
bool _memoryBackendSetFullbright(Process *process, bool enabled);

bool _memoryBackendGetColorized(Process *process);
bool _memoryBackendSetColorized(Process *process, bool enabled);

bool _memoryBackendGetFixMovementSpeed(Process *process);
bool _memoryBackendSetFixMovementSpeed(Process *process, bool enabled);

bool _memoryBackendGetPatchChat(Process *process);
bool _memoryBackendSetPatchChat(Process *process, bool enabled);

bool _memoryBackendGetShowFps(Process *process);
bool _memoryBackendSetShowFps(Process *process, bool enabled);

bool _memoryBackendChangeName(Process *process, char *name);
bool _memoryBackendSetSpeed(Process *process, uint32_t value);
bool _memoryBackendTeleport(Process *process, TeleportCoords value);
bool _memoryBackendChangeHostname(Process *process, char *hostname);
bool _memoryBackendFov(Process *process, float value);
bool _memoryBackendFovScale(Process *process, float value);
bool _memoryBackendFpsCap(Process *process, uint32_t value);

bool _memoryBackendCustomizerColor(Process *process, SimpleCheatName cheatName, RGBAColor color);
bool _memoryBackendCustomizerFloat(Process *process, SimpleCheatName cheatName, float value);

bool _memoryBackendSetSimpleCheatIntValue(Process *process, SimpleCheatName simpleCheatName, uint32_t value);

// Hooks IDs (Hash for Hook Map)
static const char* HOOK_INSTANT_KILL_ID = "HOOK_INSTANT_KILL";

MemoryBackend *memoryBackendCreate(Controller *controller) {
    MemoryBackend *memoryBackend = (MemoryBackend*)malloc(sizeof(MemoryBackend));
    if (!memoryBackend) return NULL;
    memoryBackend->controller = controller;
    memoryBackend->hooks = mapCreate();
    if (!memoryBackend->hooks) {
        LOG_ERROR("Couldn't create Hook Map");
    }
    return memoryBackend;
}

void memoryBackendDestroy(MemoryBackend *memoryBackend) {
    if (memoryBackend) {
        if (memoryBackend->hooks) {
            mapDestroy(memoryBackend->hooks);
        }
        free(memoryBackend);
    }
}

bool memoryBackendIsCheatEnabled(MemoryBackend *memoryBackend, CheatName cheatName) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
    }

    switch (cheatName) {
        case CHEAT_NAME_GOD_MODE:
            return _memoryBackendGetGodMode(process);
        case CHEAT_NAME_INVISIBLE:
            return _memoryBackendGetInvisible(process);
        case CHEAT_NAME_NO_CLIP:
            return _memoryBackendGetNoClip(process);
        case CHEAT_NAME_NO_RECOIL:
            return _memoryBackendGetNoRecoil(process);
        case CHEAT_NAME_SMALL_CROSSHAIR:
            return _memoryBackendGetSmallCrosshair(process);
        case CHEAT_NAME_FAST_GAMEPLAY:
            return _memoryBackendGetFastGameplay(process);
        case CHEAT_NAME_NO_SHELLSHOCK:
            return _memoryBackendGetNoShellshock(process);
        case CHEAT_NAME_INCREASE_KNIFE_RANGE:
            return _memoryBackendGetIncreaseKnifeRange(process);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return _memoryBackendGetBoxNeverMoves(process);
        case CHEAT_NAME_THIRD_PERSON:
            return _memoryBackendGetThirdPerson(process);
        case CHEAT_NAME_INFINITE_AMMO:
            return _memoryBackendGetInfiniteAmmo(process);
        case CHEAT_NAME_INSTANT_KILL:
            return _memoryBackendGetInstantKill(process, memoryBackend->hooks);
        case CHEAT_NAME_MAKE_BORDERLESS:
            return _memoryBackendGetMakeBorderless(process);
        case CHEAT_NAME_UNLIMIT_FPS:
            return _memoryBackendGetUnlimitFps(process);
        case CHEAT_NAME_DISABLE_HUD:
            return _memoryBackendGetDisableHud(process);
        case CHEAT_NAME_DISABLE_FOG:
            return _memoryBackendGetDisableFog(process);
        case CHEAT_NAME_FULLBRIGHT:
            return _memoryBackendGetFullbright(process);
        case CHEAT_NAME_COLORIZED:
            return _memoryBackendGetColorized(process);
        case CHEAT_NAME_FIX_MOVEMENT_SPEED:
            return _memoryBackendGetFixMovementSpeed(process);
        case CHEAT_NAME_SHOW_FPS:
            return _memoryBackendGetShowFps(process);
        case CHEAT_NAME_PATCH_CHAT:
            return _memoryBackendGetPatchChat(process);
        
        default:
            LOG_WARN("Unknown CheatName value %d", cheatName);
            return false;
    }
}

bool memoryBackendSetCheatEnabled(MemoryBackend *memoryBackend, CheatName cheatName, bool enabled) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
    }

    switch (cheatName) {
        case CHEAT_NAME_GOD_MODE:
            return _memoryBackendSetGodMode(process, memoryBackend->controller, enabled);
        case CHEAT_NAME_INVISIBLE:
            return _memoryBackendSetInvisible(process, memoryBackend->controller, enabled);
        case CHEAT_NAME_NO_CLIP:
            return _memoryBackendSetNoClip(process, enabled);
        case CHEAT_NAME_NO_RECOIL:
            return _memoryBackendSetNoRecoil(process, enabled);
        case CHEAT_NAME_SMALL_CROSSHAIR:
            return _memoryBackendSetSmallCrosshair(process, enabled);
        case CHEAT_NAME_FAST_GAMEPLAY:
            return _memoryBackendSetFastGameplay(process, enabled);
        case CHEAT_NAME_NO_SHELLSHOCK:
            return _memoryBackendSetNoShellshock(process, enabled);
        case CHEAT_NAME_INCREASE_KNIFE_RANGE:
            return _memoryBackendSetIncreaseKnifeRange(process, enabled);
        case CHEAT_NAME_BOX_NEVER_MOVES:
            return _memoryBackendSetBoxNeverMoves(process, enabled);
        case CHEAT_NAME_THIRD_PERSON:
            return _memoryBackendSetThirdPerson(process, enabled);
        case CHEAT_NAME_INFINITE_AMMO:
            return _memoryBackendSetInfiniteAmmo(process, enabled);
        case CHEAT_NAME_INSTANT_KILL:
            return _memoryBackendSetInstantKill(process, memoryBackend->hooks, enabled);
        case CHEAT_NAME_MAKE_BORDERLESS:
            return _memoryBackendSetMakeBorderless(process, enabled);
        case CHEAT_NAME_UNLIMIT_FPS:
            return _memoryBackendSetUnlimitFps(process, memoryBackend->controller, enabled);
        case CHEAT_NAME_DISABLE_HUD:
            return _memoryBackendSetDisableHud(process, enabled);
        case CHEAT_NAME_DISABLE_FOG:
            return _memoryBackendSetDisableFog(process, enabled);
        case CHEAT_NAME_FULLBRIGHT:
            return _memoryBackendSetFullbright(process, enabled);
        case CHEAT_NAME_COLORIZED:
            return _memoryBackendSetColorized(process, enabled);
        case CHEAT_NAME_FIX_MOVEMENT_SPEED:
            return _memoryBackendSetFixMovementSpeed(process, enabled);
        case CHEAT_NAME_SHOW_FPS:
            return _memoryBackendSetShowFps(process, enabled);
        case CHEAT_NAME_PATCH_CHAT:
            return _memoryBackendSetPatchChat(process, enabled);
        default:
            LOG_WARN("Unknown CheatName value %d", cheatName);
            return false;
    }
}

bool memoryBackendSetSimpleCheat(MemoryBackend *memoryBackend, SimpleCheatName simpleCheatName, void *value) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
    }

    LOG_DEBUG("Setting simple cheat %d from value at %p", simpleCheatName, value);

    switch(simpleCheatName) {
        case SIMPLE_CHEAT_NAME_CHANGE_NAME:
            return _memoryBackendChangeName(process, (char*)value);
        case SIMPLE_CHEAT_NAME_SET_SPEED:
            return _memoryBackendSetSpeed(process, (uint32_t)(*(int*)value));
        case SIMPLE_CHEAT_NAME_TELEPORT:
            return _memoryBackendTeleport(process, (TeleportCoords)(*(TeleportCoords*)value));
        case SIMPLE_CHEAT_NAME_CHANGE_HOSTNAME:
            return _memoryBackendChangeHostname(process, (char*)value);
        case SIMPLE_CHEAT_NAME_FOV:
            return _memoryBackendFov(process, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_FOV_SCALE:
            return _memoryBackendFovScale(process, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_FPS_CAP:
            return _memoryBackendFpsCap(process, (uint32_t)(*(int*)value));
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
            return _memoryBackendCustomizerColor(process, simpleCheatName, (RGBAColor)(*(RGBAColor*)value));
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_SCOREBOARD:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_POINTS:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MIN:
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MAX:
            return _memoryBackendCustomizerFloat(process, simpleCheatName, ((float)(*(int*)value)/100.0f));
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_FREQUENCY:
            return _memoryBackendCustomizerFloat(process, simpleCheatName, (float)(*(int*)value));
        case SIMPLE_CHEAT_NAME_SET_HEALTH:
        case SIMPLE_CHEAT_NAME_SET_POINTS:
        case SIMPLE_CHEAT_NAME_SET_KILLS:
        case SIMPLE_CHEAT_NAME_SET_HEADSHOTS:
            return _memoryBackendSetSimpleCheatIntValue(process, simpleCheatName, (uint32_t)(*(int*)value));
        default:
            LOG_WARN("Unknown SimpleCheatName value %d", simpleCheatName);
            return false;
    }
}

TeleportCoords *memoryBackendGetPlayerCurrentCoords(MemoryBackend *memoryBackend) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return NULL;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
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

bool memoryBackendSetRound(MemoryBackend *memoryBackend, int currentRound, int nextRound) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
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
        LOG_ERROR("Could not find round-change memory pattern");
        return false;
    }
    bool success = processWrite(process, addressFound, &nextRound, sizeof(nextRound));
    if (!success) {
        LOG_ERROR("Failed to write Next Round value");
        return false;
    }
    LOG_DEBUG("Round change prepared. Finish round %d to advance to round %d", currentRound, nextRound + 1);
    return true;
}

bool memoryBackendIsGameReady(MemoryBackend *memoryBackend) {
    return memoryBackendGetLevelElapsedTime(memoryBackend) > 0;
}

Level memoryBackendGetLevelName(MemoryBackend *memoryBackend) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return LEVEL_INVALID;
    }

    Process *process = controllerGetProcess(memoryBackend->controller);
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

double memoryBackendGetLevelElapsedTime(MemoryBackend *memoryBackend) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_DEBUG("MemoryBackend or Controller is null");
        return 0;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
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
        LOG_ERROR("Could not read game elapsed time. Error %lu", GetLastError());
        return 0;
    }
    return elapsed;
}

float memoryBackendGetMovementSpeed(MemoryBackend *memoryBackend) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("Cannot poll movement speed without a memory backend and controller");
        return false;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        LOG_ERROR("Cannot poll movement speed without an attached process");
        return false;
    }
    float speed;
    bool success = processRead(process, GAME_CHEAT.movementSpeed, &speed, sizeof(speed));
    if (!success) {
        LOG_ERROR("Could not read movement speed");
        return false;
    }
    return speed;
}

int memoryBackendGetSimpleCheatIntValue(MemoryBackend *memoryBackend, SimpleCheatName simpleCheatName) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return 0;
    }
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return 0;
    }
    SimpleCheat cheat = cheatGetSimpleCheat(simpleCheatName);
    uint32_t value = 0;
    if (!processRead(process, cheat.offset, &value, sizeof(value))) {
        LOG_ERROR("Failed to read simple cheat %d value", simpleCheatName);
        return 0;
    }
    return (int)value;
}

bool memoryBackendGetName(MemoryBackend *memoryBackend, char *out, size_t size) {
    if (!memoryBackend || !memoryBackend->controller || !out || size == 0) {
        LOG_ERROR("Invalid arguments to memoryBackendGetName");
        return false;
    }
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return false;
    }
    SimpleCheat cheat = SIMPLE_CHEAT_CHANGE_NAME;
    if (!processRead(process, cheat.offset, out, size)) {
        LOG_ERROR("Failed to read player name");
        return false;
    }
    out[size - 1] = '\0'; // guard against an unterminated read
    return true;
}

bool memoryBackendIsZombiesGameOngoing(MemoryBackend *memoryBackend) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_DEBUG("MemoryBackend or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        LOG_DEBUG("Process is null");
        return false;
    }
    uint32_t active;
    bool success = processRead(process, GAME_CHEAT.isZombiesGameOngoingOffset, &active, sizeof(uint32_t));
    if (!success) {
        LOG_ERROR("Could not read active Zombies game state");
        return false;
    }
    return active == 1;
}

bool memoryBackendIsZombiesGamePaused(MemoryBackend *memoryBackend) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("Cannot poll pause state without a memory backend and controller");
        return false;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        LOG_ERROR("Cannot poll pause state without an attached process");
        return false;
    }
    uint32_t active;
    bool success = processRead(process, GAME_CHEAT.isZombiesGamePausedOffset, &active, sizeof(uint32_t));
    if (!success) {
        LOG_ERROR("Could not read game pause state");
        return false;
    }
    return active == 1;
}

int memoryBackendGetGameResets(MemoryBackend *memoryBackend) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("Cannot poll game resets without a memory backend and controller");
        return 0;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        LOG_ERROR("Cannot poll game resets without an attached process");
        return 0;
    }
    uint32_t resets;
    bool success = processRead(process, GAME_CHEAT.nResetsOffset, &resets, sizeof(uint32_t));
    if (!success) {
        LOG_ERROR("Could not read game resets");
        return 0;
    }
    resets = resets == 0 ? resets : resets - 1;
    return (int)resets; 
}

bool memoryBackendSVSendServerCommand(MemoryBackend *memoryBackend, int commandType, int clientNumber, const char *commands) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
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
        LOG_ERROR("SV_SendServerCommand timed out after 100 ms for command [%s]", commands);
        success = false;
    }
    threadClose(thread);
    free(bytes);
    processFreePage(process, addr);
    return success;
}

bool memoryBackendCBuffAddText(MemoryBackend *memoryBackend, const char *commands) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
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
        LOG_ERROR("Cbuf_AddText timed out after 100 ms");
        success = false;
    }
    threadClose(thread);
    processFreePage(process, addr);
    free(bytes);
    return success;
}

uintptr_t memoryBackendGetDVarPointer(MemoryBackend *memoryBackend, const char *dVar) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return 0;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
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
        LOG_ERROR("GetDVarPointer timed out after 100 ms for DVar '%s'", dVar);
        success = false;
    }
    int exitCode = success ? threadGetExitCode(thread) : 0;
    threadClose(thread);
    processFreePage(process, addr);
    return (uintptr_t)exitCode;
}

// Private implementations

bool _memoryBackendGetGodMode(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_GOD_MODE.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read God Mode value");
        return false;
    }
    return value == CHEAT_GOD_MODE.on.u32;
}

bool _memoryBackendSetGodMode(Process *process, Controller *controller, bool enabled) {
    bool isInvisibleModeChecked = controllerIsCheatCheckboxChecked(controller, CHEAT_NAME_INVISIBLE);
    uint32_t value = enabled ? CHEAT_GOD_MODE.on.u32 : (isInvisibleModeChecked ? CHEAT_INVISIBLE.on.u32 : CHEAT_INVISIBLE.off.u32);
    return processWrite(process, CHEAT_GOD_MODE.offset, &value, sizeof(value));
}

bool _memoryBackendGetInvisible(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_INVISIBLE.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Invisible value");
        return false;
    }
    return value == CHEAT_INVISIBLE.on.u32;
}

bool _memoryBackendSetInvisible(Process *process, Controller *controller, bool enabled) {
    bool isGodModeChecked = controllerIsCheatCheckboxChecked(controller, CHEAT_NAME_GOD_MODE);
    uint32_t value = enabled ? CHEAT_INVISIBLE.on.u32 : (isGodModeChecked ? CHEAT_GOD_MODE.on.u32 : CHEAT_GOD_MODE.off.u32);
    return processWrite(process, CHEAT_INVISIBLE.offset, &value, sizeof(value));
}

bool _memoryBackendGetNoClip(Process *process) {
    uint8_t value = 0;
    bool success = processRead(process, CHEAT_NO_CLIP.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read No Clip value");
        return false;
    }
    return value == CHEAT_NO_CLIP.on.byte;
}

bool _memoryBackendSetNoClip(Process *process, bool enabled) {
    uint8_t value = enabled ? CHEAT_NO_CLIP.on.byte : CHEAT_NO_CLIP.off.byte;
    return processWrite(process, CHEAT_NO_CLIP.offset, &value, sizeof(value));
}

bool _memoryBackendGetNoRecoil(Process *process) {
    uint8_t value = 0;
    bool success = processRead(process, CHEAT_NO_RECOIL.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read No Recoil value");
        return false;
    }
    return value == CHEAT_NO_RECOIL.on.byte;
}

bool _memoryBackendSetNoRecoil(Process *process, bool enabled) {
    uint8_t value = enabled ? CHEAT_NO_RECOIL.on.byte : CHEAT_NO_RECOIL.off.byte;
    return processWrite(process, CHEAT_NO_RECOIL.offset, &value, sizeof(value));
}

bool _memoryBackendGetFastGameplay(Process *process) {
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

bool _memoryBackendSetFastGameplay(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_FAST_GAMEPLAY.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Fast Gameplay address");
        return false;
    }
    float value = enabled ? CHEAT_FAST_GAMEPLAY.on.f32 : CHEAT_FAST_GAMEPLAY.off.f32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _memoryBackendGetNoShellshock(Process *process) {
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

bool _memoryBackendSetNoShellshock(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_NO_SHELLSHOCK.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read No Shellshock address");
        return false;
    }
    uint8_t value = enabled ? CHEAT_NO_SHELLSHOCK.on.u32 : CHEAT_NO_SHELLSHOCK.off.u32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _memoryBackendGetIncreaseKnifeRange(Process *process) {
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

bool _memoryBackendSetIncreaseKnifeRange(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_INCREASE_KNIFE_RANGE.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Increase Knife Range address");
        return false;
    }
    float value = enabled ? CHEAT_INCREASE_KNIFE_RANGE.on.f32 : CHEAT_INCREASE_KNIFE_RANGE.off.f32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _memoryBackendGetBoxNeverMoves(Process *process) {
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

bool _memoryBackendSetBoxNeverMoves(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_BOX_NEVER_MOVES.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Box Never Moves address");
        return false;
    }
    uint8_t value = enabled ? CHEAT_BOX_NEVER_MOVES.on.u32 : CHEAT_BOX_NEVER_MOVES.off.u32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _memoryBackendGetThirdPerson(Process *process) {
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

bool _memoryBackendSetThirdPerson(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_THIRD_PERSON.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Third Person address");
        return false;
    }
    uint8_t value = enabled ? CHEAT_THIRD_PERSON.on.byte : CHEAT_THIRD_PERSON.off.byte;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _memoryBackendGetInfiniteAmmo(Process *process) {
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

bool _memoryBackendSetInfiniteAmmo(Process *process, bool enabled) {
    CheatAsm *instructionSet = &CHEAT_ASM_INFINITE_AMMO;
    uint8_t *instructions = enabled ? instructionSet->on.instructions : instructionSet->off.instructions;
    size_t size = enabled ? instructionSet->on.size : instructionSet->off.size;
    return processWrite(process, instructionSet->offset, instructions, size);
}

bool _memoryBackendGetSmallCrosshair(Process *process) {
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

bool _memoryBackendSetSmallCrosshair(Process *process, bool enabled) {
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
    LOG_DEBUG("Small Crosshair pointer: 0x%08X", (unsigned)address1);
    if (!success) {
        LOG_ERROR("Failed to read Small Crosshair address");
        return false;
    }
    float value = enabled ? CHEAT_SMALL_CROSSHAIR.on.f32 : CHEAT_SMALL_CROSSHAIR.off.f32;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _memoryBackendGetInstantKill(Process *process, Map *hooks) {
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

bool _memoryBackendSetInstantKill(Process *process, Map *hooks, bool enabled) {
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

bool _memoryBackendGetMakeBorderless(Process *process) {
    return processIsBorderless(process);
}

bool _memoryBackendSetMakeBorderless(Process *process, bool enabled) {
    return processMakeBorderless(process, enabled);
}

bool _memoryBackendGetUnlimitFps(Process *process) {
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

bool _memoryBackendSetUnlimitFps(Process *process, Controller *controller, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_UNLIMIT_FPS.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Unlimit Fps address");
        return false;
    }
    uint32_t value = enabled ? CHEAT_UNLIMIT_FPS.on.u32 : (uint32_t)controllerUiGraphicsGetFpsCap(controller);
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _memoryBackendGetDisableHud(Process *process) {
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

bool _memoryBackendSetDisableHud(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_DISABLE_HUD.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Disable Hud address");
        return false;
    }
    LOG_DEBUG("Disable HUD pointer: base=0x%08X target=0x%08X",
              (unsigned)CHEAT_DISABLE_HUD.offset, (unsigned)address1);
    uint8_t value = enabled ? CHEAT_DISABLE_HUD.on.byte : CHEAT_DISABLE_HUD.off.byte;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _memoryBackendGetDisableFog(Process *process) {
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

bool _memoryBackendSetDisableFog(Process *process, bool enabled) {
    uint32_t address1 = 0;
    bool success = processRead(process, CHEAT_DISABLE_FOG.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Disable Fog address");
        return false;
    }
    uint8_t value = enabled ? CHEAT_DISABLE_FOG.on.byte : CHEAT_DISABLE_FOG.off.byte;
    return processWrite(process, address1 + 0x18, &value, sizeof(value));
}

bool _memoryBackendGetFullbright(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_FULLBRIGHT.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Fullbright value");
        return false;
    }
    return value == CHEAT_FULLBRIGHT.on.u32;
}

bool _memoryBackendSetFullbright(Process *process, bool enabled) {
    uint32_t value = enabled ? CHEAT_FULLBRIGHT.on.u32 : CHEAT_FULLBRIGHT.off.u32;
    return processWrite(process, CHEAT_FULLBRIGHT.offset, &value, sizeof(value));
}

bool _memoryBackendGetColorized(Process *process) {
    uint32_t value = 0;
    bool success = processRead(process, CHEAT_COLORIZED.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Colorized value");
        return false;
    }
    return value == CHEAT_COLORIZED.on.u32;
}

bool _memoryBackendSetColorized(Process *process, bool enabled) {
    uint32_t value = enabled ? CHEAT_COLORIZED.on.u32 : CHEAT_COLORIZED.off.u32;
    return processWrite(process, CHEAT_COLORIZED.offset, &value, sizeof(value));
}

bool _memoryBackendGetFixMovementSpeed(Process *process) {
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

bool _memoryBackendSetFixMovementSpeed(Process *process, bool enabled) {
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

bool _memoryBackendGetPatchChat(Process *process) {
    uint8_t value = 0;
    bool success = processRead(process, CHEAT_PATCH_CHAT.offset, &value, sizeof(value));
    if (!success) {
        LOG_ERROR("Failed to read Patch Chat value");
        return false;
    }
    return value == CHEAT_PATCH_CHAT.on.byte;
}

bool _memoryBackendSetPatchChat(Process *process, bool enabled) {
    uint8_t value = enabled ? CHEAT_PATCH_CHAT.on.byte : CHEAT_PATCH_CHAT.off.byte;
    uint32_t oldProtect;
    processVirtualProtect(process, CHEAT_PATCH_CHAT.offset, sizeof(value), PAGE_EXECUTE_READWRITE, &oldProtect);
    bool success = processWrite(process, CHEAT_PATCH_CHAT.offset, &value, sizeof(value));
    processVirtualProtect(process, CHEAT_PATCH_CHAT.offset, sizeof(value), oldProtect, &oldProtect);
    return success;
}

bool _memoryBackendGetShowFps(Process *process) {
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

bool _memoryBackendSetShowFps(Process *process, bool enabled) {
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

bool _memoryBackendChangeName(Process *process, char *name) {
    SimpleCheat cheat = SIMPLE_CHEAT_CHANGE_NAME;
    return processWrite(process, cheat.offset, name, strlen(name) + 1);
}

bool _memoryBackendSetSpeed(Process *process, uint32_t value) {
    SimpleCheat cheat = SIMPLE_CHEAT_SET_SPEED;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Speed address");
        return false;
    }
    LOG_DEBUG("Writing %u to 0x%08X", (unsigned)value, (unsigned)cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(uint32_t));
}

bool _memoryBackendTeleport(Process *process, TeleportCoords value) {
    return processWrite(process, TELEPORT_CHEAT.xOffset, &(value.x), sizeof(value.x)) &&
           processWrite(process, TELEPORT_CHEAT.yOffset, &(value.y), sizeof(value.y)) &&
           processWrite(process, TELEPORT_CHEAT.zOffset, &(value.z), sizeof(value.z));
}

bool _memoryBackendChangeHostname(Process *process, char *hostname) {
    SimpleCheat cheat = SIMPLE_CHEAT_CHANGE_HOSTNAME;
    return processWrite(process, cheat.offset, hostname, strlen(hostname) + 1);
}

bool _memoryBackendFov(Process *process, float value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FOV;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Fov address");
        return false;
    }
    LOG_DEBUG("Writing %.2f to 0x%08X", value, (unsigned)cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(float));
}

bool _memoryBackendFovScale(Process *process, float value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FOV_SCALE;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Fov Scale address");
        return false;
    }
    value = value/100;
    LOG_DEBUG("Writing %.2f to 0x%08X", value, (unsigned)cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(float));
}

bool _memoryBackendFpsCap(Process *process, uint32_t value) {
    SimpleCheat cheat = SIMPLE_CHEAT_FPS_CAP;
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.offset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Fps Cap address");
        return false;
    }
    LOG_DEBUG("Writing %u to 0x%08X", (unsigned)value, (unsigned)cheat.offset);
    return processWrite(process, address1 + 0x18, &value, sizeof(uint32_t));
}

// Colors
static uint32_t _mergeColorComponents(RGBAColor color) {
    uint32_t mergedColor = 0;
    mergedColor |= (uint32_t)color.r;
    mergedColor |= (uint32_t)color.g << 8;
    mergedColor |= (uint32_t)color.b << 16;
    return mergedColor;
}

bool _memoryBackendCustomizerColor(Process *process, SimpleCheatName cheatName, RGBAColor color) {
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

bool _memoryBackendCustomizerFloat(Process *process, SimpleCheatName cheatName, float value) {
    CustomizerCheat cheat = cheatGetCustomizerCheat(cheatName);
    uint32_t address1 = 0;
    bool success = processRead(process, cheat.baseOffset, &address1, sizeof(address1));
    if (!success) {
        LOG_ERROR("Failed to read Customizer %d address", cheatName);
        return false;
    }
    return processWrite(process, address1 + cheat.offset, &value, sizeof(value));
}

bool _memoryBackendSetSimpleCheatIntValue(Process *process, SimpleCheatName simpleCheatName, uint32_t value) {
    SimpleCheat cheat = cheatGetSimpleCheat(simpleCheatName);
    LOG_DEBUG("Writing %u to 0x%08X", (unsigned)value, (unsigned)cheat.offset);
    return processWrite(process, cheat.offset, &value, sizeof(uint32_t));
}

int memoryBackendGetClaymoreCount(MemoryBackend *memoryBackend) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return 0;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
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

int memoryBackendGetCurrentSnapshotEntities(MemoryBackend *memoryBackend) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return 0;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return 0;
    }

    int32_t current = 0;
    processRead(process, GAME_CHEAT.currentSnapshotEntitiesOffset, &current, sizeof(current));
    return current;
}

int memoryBackendGetMaxSnapshotEntities(MemoryBackend *memoryBackend) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return 0;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        LOG_ERROR("Process is null");
        return 0;
    }

    int32_t maxRaw = 0;
    processRead(process, GAME_CHEAT.maxSnapshotEntitiesOffset, &maxRaw, sizeof(maxRaw));
    // 2147483646 - maxRaw
    return 2147483646 - maxRaw;
}

bool memoryBackendIsChatOpen(MemoryBackend *memoryBackend) {
    if (!memoryBackend || !memoryBackend->controller) {
        LOG_ERROR("MemoryBackend or Controller is null");
        return false;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
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

bool memoryBackendWriteToChatInput(MemoryBackend *memoryBackend, const char *text) {
    if (!memoryBackend || !memoryBackend->controller) {
        return false;
    }
    
    Process *process = controllerGetProcess(memoryBackend->controller);
    if (!process) {
        return false;
    }

    if (!memoryBackendIsChatOpen(memoryBackend)) {
        LOG_WARN("Chat input write skipped because chat is closed");
        return false;
    }

    char buffer[256] = {0};
    if (text) {
        strncpy(buffer, text, sizeof(buffer) - 1);
    }
    return processWrite(process, GAME_CHEAT.chatInputBufferOffset, buffer, sizeof(buffer));
}
