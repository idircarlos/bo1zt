#ifndef API_H_
#define API_H_

#include <stdint.h>
#include <stdbool.h>
#include "../memory/memory.h"
#include "../controller/controller.h"

typedef struct Api Api;

Api *apiCreate(Controller *controller);

bool apiIsCheatEnabled(Api *api, CheatName cheat);
bool apiSetCheatEnabled(Api *api, CheatName cheat, bool enabled);

#endif // API_H_