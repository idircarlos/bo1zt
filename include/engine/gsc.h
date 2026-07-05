#ifndef ENGINE_GSC_H_
#define ENGINE_GSC_H_

#include <stdbool.h>
#include "controller.h"
#include "utils/list.h"

typedef struct GscBackend GscBackend;

GscBackend *gscBackendCreate(Controller *controller);
void gscBackendDestroy(GscBackend *gscBackend);

bool gscBackendAddPerks(GscBackend *gscBackend, List *perks);
bool gscBackendRemovePerks(GscBackend *gscBackend, List *perks);

bool gscBackendGetStaticBox(GscBackend *gscBackend);
bool gscBackendSetStaticBox(GscBackend *gscBackend, bool enabled);

bool gscBackendPlayEasterEggSong(GscBackend *gscBackend);

int gscBackendGetRound(GscBackend *gscBackend);

bool gscBackendGiveWeapons(GscBackend *gscBackend, List *weapons);
bool gscBackendTakeWeapons(GscBackend *gscBackend);

#endif // ENGINE_GSC_H_
