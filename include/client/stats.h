#ifndef CLIENT_STATS_H_
#define CLIENT_STATS_H_

#include "client.h"

// special-* fields are -1 when the special-round type does not apply to the map (JSON null).

typedef struct {
    int entitiesCurrent;
    int entitiesMax;
    int claymores;
    int revives;
    int specialDogs;    // -1 if not applicable
    int specialMonkeys; // -1 if not applicable
    int specialThief;   // -1 if not applicable
    double sph;
    char nextSpecialRounds[64]; // predicted next special-round numbers, "" if none
} Stats;

ClientResult clientGetStats(Client *client, int round, Stats *out);

ClientResult clientGetSph(Client *client, int round, double *sphOut);
ClientResult clientGetClaymores(Client *client, int *claymoresOut);
ClientResult clientGetEntities(Client *client, int *currentOut, int *maxOut);
ClientResult clientGetRevives(Client *client, int *revivesOut);

#endif // CLIENT_STATS_H_
