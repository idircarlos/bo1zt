#ifndef API_H_
#define API_H_

#include <stdint.h>
#include <stdbool.h>
#include "../memory/memory.h"
#include "../controller/controller.h"

typedef struct Api Api;

Api *apiCreate(Controller *controller);

bool apiGetGodMode(Api *api);
bool apiSetGodMode(Api *api, bool enabled);

bool apiGetInvisible(Api *api);
bool apiSetInvisible(Api *api, bool enabled);

bool apiGetNoClip(Api *api);
bool apiSetNoClip(Api *api, bool enabled);

bool apiGetNoRecoil(Api *api);
bool apiSetNoRecoil(Api *api, bool enabled);

bool apiGetBoxNeverMoves(Api *api);
bool apiSetBoxNeverMoves(Api *api, bool enabled);

bool apiGetThirdPerson(Api *api);
bool apiSetThirdPerson(Api *api, bool enabled);

bool apiGetInfiniteAmmo(Api *api);
bool apiSetInfiniteAmmo(Api *api, bool enabled);

bool apiGetInstantKill(Api *api);
bool apiSetInstantKill(Api *api, bool enabled);

#endif // API_H_