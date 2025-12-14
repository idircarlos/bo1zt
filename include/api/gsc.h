#ifndef API_GSC_H_
#define API_GSC_H_

#include <stdbool.h>
#include "controller.h"
#include "utils/list.h"

typedef struct GscApi GscApi;

GscApi *gscApiCreate(Controller *controller);
void gscApiDestroy(GscApi *gscApi);

bool gscApiAddPerks(GscApi *gscApi, List *perks);
bool gscApiRemovePerks(GscApi *gscApi, List *perks);

#endif // API_GSC_H_
