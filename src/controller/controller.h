#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include "../memory/memory.h"
#include "../cheat/cheat.h"

typedef struct Controller Controller;

Controller* controllerCreate();

ProcessHandle* controllerGetProcessHandle(Controller *controller);

bool controllerGetCheat(Controller *controller, CheatName cheat);
bool controllerSetCheat(Controller *controller, CheatName cheat, bool enabled);

bool controllerSetSimpleCheat(Controller *controller, SimpleCheatName cheat, void *value);

bool controllerIsCheckboxChecked(Controller *controller, CheatName cheat);

TeleportCoords *controllerGetPlayerCurrentCoords(Controller *controller);

Weapon controllerGetPlayerCurrentWeapon(Controller *controller);
bool controllerSetPlayerWeapon(Controller *controller, Weapon weapon, int slot);

void controllerDestroy(Controller *controller);

#endif // CONTROLLER_H_