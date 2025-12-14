#ifndef API_GSC_H_
#define API_GSC_H_

#include <stdbool.h>
#include "logic/gsc.h"
#include "utils/list.h"

typedef struct ApiGsc ApiGsc;

ApiGsc *apiGscCreate(GSC *gsc);
void apiGscDestroy(ApiGsc *apiGsc);

bool apiGscAddPerks(ApiGsc *apiGsc, List *perks);
bool apiGscRemovePerks(ApiGsc *apiGsc, List *perks);

#endif // API_GSC_H_
