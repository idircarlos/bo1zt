#ifndef CLIENT_STATE_H_
#define CLIENT_STATE_H_

#include "client.h"

typedef struct {
    bool isGameAttached;
    bool isZombiesGameOngoing;
    bool isZombiesGamePaused;
    int gameResets;
    char level[32];       // short kebab-case level name ("kino", ...)
    int elapsed;
    float movementSpeed;
    int round;
    int entitiesCurrent;
    int entitiesMax;
} GameState;

ClientResult clientGetState(Client *client, GameState *out);

#endif // CLIENT_STATE_H_
