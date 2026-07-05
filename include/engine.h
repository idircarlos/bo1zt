#ifndef ENGINE_H_
#define ENGINE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "controller.h"
#include "logic/game/level.h"
#include "utils/list.h"

#define ENGINE_CHAT_MESSAGE_LENGTH 64

typedef struct Engine Engine;

Engine *engineCreate(Controller *controller);
void engineDestroy(Engine *engine);

// Raw backend operations
bool engineIsCheatEnabled(Engine *engine, CheatName cheatName);
bool engineSetCheatEnabled(Engine *engine, CheatName cheatName, bool enabled);
bool engineSetSimpleCheat(Engine *engine, SimpleCheatName simpleCheatName, void *value);
TeleportCoords *engineGetPlayerCurrentCoords(Engine *engine);
bool engineSetRound(Engine *engine, int round);
bool engineIsGameReady(Engine *engine);
Level engineGetLevelName(Engine *engine);
double engineGetLevelElapsedTime(Engine *engine);
float engineGetMovementSpeed(Engine *engine);
int engineGetSimpleCheatIntValue(Engine *engine, SimpleCheatName simpleCheatName);
bool engineGetPlayerName(Engine *engine, char *out, size_t size);
bool engineIsZombiesGameOngoing(Engine *engine);
bool engineIsZombiesGamePaused(Engine *engine);
int engineGetGameResets(Engine *engine);
bool engineSVSendServerCommand(Engine *engine, int commandType, int clientNumber, const char *commands);
bool engineCBuffAddText(Engine *engine, const char *commands);
uintptr_t engineGetDVarPointer(Engine *engine, const char *dVar);
int engineGetClaymoreCount(Engine *engine);
int engineGetCurrentSnapshotEntities(Engine *engine);
int engineGetMaxSnapshotEntities(Engine *engine);
bool engineIsChatOpen(Engine *engine);
bool engineWriteToChatInput(Engine *engine, const char *text);

// GSC backend operations
bool engineAddPerks(Engine *engine, List *perks);
bool engineRemovePerks(Engine *engine, List *perks);
bool engineGetStaticBox(Engine *engine);
bool engineSetStaticBox(Engine *engine, bool enabled);
bool enginePlayEasterEggSong(Engine *engine);
int engineGetRound(Engine *engine);
bool engineGiveWeapons(Engine *engine, List *weapons);
bool engineTakeWeapons(Engine *engine);

// Others (such as using Server)
bool engineGiveAmmo(Engine *engine);

#endif // ENGINE_H_
