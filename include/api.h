#ifndef API_H_
#define API_H_

#include <stdint.h>
#include <stdbool.h>
#include "controller.h"
#include "logic/game/level.h"
#include "utils/list.h"

#define API_CHAT_MESSAGE_LENGTH 64

typedef struct Api Api;

Api *apiCreate(Controller *controller);
void apiDestroy(Api *api);

// Raw backend operations
bool apiIsCheatEnabled(Api *api, CheatName cheatName);
bool apiSetCheatEnabled(Api *api, CheatName cheatName, bool enabled);
bool apiSetSimpleCheat(Api *api, SimpleCheatName simpleCheatName, void *value);
TeleportCoords *apiGetPlayerCurrentCoords(Api *api);
WeaponName apiGetPlayerCurrentWeapon(Api *api);
WeaponName apiGetPlayerWeapon(Api *api, int slot);
bool apiSetPlayerWeapon(Api *api, WeaponName weapon, int slot);
bool apiGivePlayerAmmo(Api *api);
bool apiSetRound(Api *api, int currentRound, int nextRound);
bool apiIsGameReady(Api *api);
Level apiGetLevelName(Api *api);
double apiGetLevelElapsedTime(Api *api);
float apiGetMovementSpeed(Api *api);
bool apiIsZombiesGameOngoing(Api *api);
bool apiIsZombiesGamePaused(Api *api);
int apiGetGameResets(Api *api);
bool apiSVSendServerCommand(Api *api, int commandType, int clientNumber, const char *commands);
bool apiCBuffAddText(Api *api, const char *commands);
uintptr_t apiGetDVarPointer(Api *api, const char *dVar);
int apiGetClaymoreCount(Api *api);
int apiGetCurrentSnapshotEntities(Api *api);
int apiGetMaxSnapshotEntities(Api *api);
bool apiIsChatOpen(Api *api);
bool apiWriteToChatInput(Api *api, const char *text);

// GSC backend operations
bool apiAddPerks(Api *api, List *perks);
bool apiRemovePerks(Api *api, List *perks);
bool apiGetStaticBox(Api *api);
bool apiSetStaticBox(Api *api, bool enabled);
bool apiPlayEasterEggSong(Api *api);

#endif // API_H_
