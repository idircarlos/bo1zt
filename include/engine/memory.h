#ifndef ENGINE_MEMORY_H_
#define ENGINE_MEMORY_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "controller.h"
#include "logic/game/level.h"

typedef struct MemoryBackend MemoryBackend;

MemoryBackend *memoryBackendCreate(Controller *controller);
void memoryBackendDestroy(MemoryBackend *memoryBackend);

bool memoryBackendIsCheatEnabled(MemoryBackend *memoryBackend, CheatName cheatName);
bool memoryBackendSetCheatEnabled(MemoryBackend *memoryBackend, CheatName cheatName, bool enabled);
bool memoryBackendSetSimpleCheat(MemoryBackend *memoryBackend, SimpleCheatName simpleCheatName, void *value);
TeleportCoords *memoryBackendGetPlayerCurrentCoords(MemoryBackend *memoryBackend);
bool memoryBackendSetRound(MemoryBackend *memoryBackend, int currentRound, int nextRound);
bool memoryBackendIsGameReady(MemoryBackend *memoryBackend);
Level memoryBackendGetLevelName(MemoryBackend *memoryBackend);
double memoryBackendGetLevelElapsedTime(MemoryBackend *memoryBackend);
float memoryBackendGetMovementSpeed(MemoryBackend *memoryBackend);
int memoryBackendGetSimpleCheatIntValue(MemoryBackend *memoryBackend, SimpleCheatName simpleCheatName);
bool memoryBackendGetName(MemoryBackend *memoryBackend, char *out, size_t size);
bool memoryBackendIsZombiesGameOngoing(MemoryBackend *memoryBackend);
bool memoryBackendIsZombiesGamePaused(MemoryBackend *memoryBackend);
int memoryBackendGetGameResets(MemoryBackend *memoryBackend);
bool memoryBackendSVSendServerCommand(MemoryBackend *memoryBackend, int commandType, int clientNumber, const char *commands);
bool memoryBackendCBuffAddText(MemoryBackend *memoryBackend, const char *commands);
uintptr_t memoryBackendGetDVarPointer(MemoryBackend *memoryBackend, const char *dVar);
int memoryBackendGetClaymoreCount(MemoryBackend *memoryBackend);
int memoryBackendGetCurrentSnapshotEntities(MemoryBackend *memoryBackend);
int memoryBackendGetMaxSnapshotEntities(MemoryBackend *memoryBackend);
bool memoryBackendIsChatOpen(MemoryBackend *memoryBackend);
bool memoryBackendWriteToChatInput(MemoryBackend *memoryBackend, const char *text);

#endif // ENGINE_MEMORY_H_
