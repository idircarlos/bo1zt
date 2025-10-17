#ifndef API_H_
#define API_H_

#include <stdint.h>
#include <stdbool.h>
#include "../memory/memory.h"
#include "../controller/controller.h"

typedef struct Api Api;

Api *apiCreate(Controller *controller);

bool apiIsCheatEnabled(Api *api, CheatName cheatName);
bool apiSetCheatEnabled(Api *api, CheatName cheatName, bool enabled);

bool apiSetSimpleCheat(Api *api, SimpleCheatName simpleCheatName, void *value);

#endif // API_H_