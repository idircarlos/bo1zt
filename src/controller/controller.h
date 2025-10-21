#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include "../memory/memory.h"
#include "../cheat/cheat.h"

typedef struct Controller Controller;

Controller* controllerCreate();
ProcessHandle* controllerGetProcessHandle(Controller *controller);
bool controllerIsGameAttached(Controller *controller);
bool controllerAttachGame(Controller *controller);
bool controllerDetachGame(Controller *controller);
bool controllerIsGameRunning(Controller *controller);
void controllerWaitUntilGameCloses(Controller *controller); // This method should be called from a different thread to not block the main thread (UI)
bool controllerGetCheat(Controller *controller, CheatName cheat);
bool controllerSetCheat(Controller *controller, CheatName cheat, bool enabled);
bool controllerSetSimpleCheat(Controller *controller, SimpleCheatName cheat, void *value);
bool controllerIsCheatCheckboxChecked(Controller *controller, CheatName cheat);
TeleportCoords *controllerGetPlayerCurrentCoords(Controller *controller);
WeaponName controllerGetPlayerCurrentWeapon(Controller *controller);
WeaponName controllerGetPlayerWeapon(Controller *controller, int slot);
bool controllerSetPlayerWeapon(Controller *controller, WeaponName weapon, int slot);
bool controllerGivePlayerAmmo(Controller *controller);
bool controllerSetRound(Controller *controller, int currentRound, int nextRound);
bool controllerIsZombiesGameActive(Controller *controller);
int controllerGetGameResets(Controller *controller);
void controllerDestroy(Controller *controller);

#endif // CONTROLLER_H_