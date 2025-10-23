#ifndef API_H_
#define API_H_

#include <stdint.h>
#include <stdbool.h>
#include "../process/process.h"
#include "../controller/controller.h"

typedef struct Api Api;

Api *apiCreate(Controller *controller);
bool apiIsCheatEnabled(Api *api, CheatName cheatName);
bool apiSetCheatEnabled(Api *api, CheatName cheatName, bool enabled);
bool apiSetSimpleCheat(Api *api, SimpleCheatName simpleCheatName, void *value);
TeleportCoords *apiGetPlayerCurrentCoords(Api *api);
WeaponName apiGetPlayerCurrentWeapon(Api *api);
WeaponName apiGetPlayerWeapon(Api *api, int slot);
bool apiSetPlayerWeapon(Api *api, WeaponName weapon, int slot);
bool apiGivePlayerAmmo(Api *api);
bool apiSetRound(Api *api, int currentRound, int nextRound);
bool apiIsZombiesGameRunning(Api *api);
int apiGetGameResets(Api *api);

#endif // API_H_