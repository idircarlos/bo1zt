#ifndef API_RAW_H_
#define API_RAW_H_

#include <stdint.h>
#include <stdbool.h>
#include "controller.h"
#include "logic/game/level.h"

typedef struct RawApi RawApi;

RawApi *rawApiCreate(Controller *controller);
void rawApiDestroy(RawApi *rawApi);

bool rawApiIsCheatEnabled(RawApi *rawApi, CheatName cheatName);
bool rawApiSetCheatEnabled(RawApi *rawApi, CheatName cheatName, bool enabled);
bool rawApiSetSimpleCheat(RawApi *rawApi, SimpleCheatName simpleCheatName, void *value);
TeleportCoords *rawApiGetPlayerCurrentCoords(RawApi *rawApi);
bool rawApiSetRound(RawApi *rawApi, int currentRound, int nextRound);
bool rawApiIsGameReady(RawApi *rawApi);
Level rawApiGetLevelName(RawApi *rawApi);
double rawApiGetLevelElapsedTime(RawApi *rawApi);
float rawApiGetMovementSpeed(RawApi *rawApi);
bool rawApiIsZombiesGameOngoing(RawApi *rawApi);
bool rawApiIsZombiesGamePaused(RawApi *rawApi);
int rawApiGetGameResets(RawApi *rawApi);
bool rawApiSVSendServerCommand(RawApi *rawApi, int commandType, int clientNumber, const char *commands);
bool rawApiCBuffAddText(RawApi *rawApi, const char *commands);
uintptr_t rawApiGetDVarPointer(RawApi *rawApi, const char *dVar);
int rawApiGetClaymoreCount(RawApi *rawApi);
int rawApiGetCurrentSnapshotEntities(RawApi *rawApi);
int rawApiGetMaxSnapshotEntities(RawApi *rawApi);
bool rawApiIsChatOpen(RawApi *rawApi);
bool rawApiWriteToChatInput(RawApi *rawApi, const char *text);
bool rawApiSetChatNameColorValue(RawApi *rawApi, ChatColor color);

#endif // API_RAW_H_
