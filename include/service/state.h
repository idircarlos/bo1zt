#ifndef SERVICE_STATE_H_
#define SERVICE_STATE_H_

#include "service.h"

typedef struct {
    bool isGameAttached;
    bool isZombiesGameOngoing;
    bool isZombiesGamePaused;
    int gameResets;
    const char *level;   // short kebab-case level name ("kino", "ascension", ...)
    int elapsed;
    float movementSpeed;
    int round;
    int entitiesCurrent;
    int entitiesMax;
} ServiceStateSnapshot;

ServiceStateSnapshot serviceStateSnapshot(Service *service);

#endif // SERVICE_STATE_H_
