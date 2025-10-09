#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include "../memory/memory.h"
#include "../offset/offset.h"

typedef struct Controller Controller;

Controller* controllerCreate(const char *executableName);

ProcessHandle* controllerGetProcessHandle(Controller *controller);

bool controllerSetCheat(Controller *controller, Cheat cheat, bool enabled);

bool controllerIsCheckboxChecked(Controller *controller, Cheat cheat);
void controllerDestroy(Controller *controller);

#endif // CONTROLLER_H_