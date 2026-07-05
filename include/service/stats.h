#ifndef SERVICE_STATS_H_
#define SERVICE_STATS_H_

#include "service.h"

// special-* fields use a sentinel of -1 to mean "null" (the special-round type
// does not apply to the current map); the router serializes those as JSON null.

typedef struct {
    int entitiesCurrent;
    int entitiesMax;
    int claymores;
    int revives;
    int specialDogs;     // -1 when not applicable to the map
    int specialMonkeys;  // -1 when not applicable to the map
    int specialThief;    // -1 when not applicable to the map
    double sph;
    // Human-readable prediction of the map's upcoming special-round numbers
    // (e.g. "5, 6, 7"), mirroring gameNextPotentialSpecialRounds(). Empty when
    // there is no prediction (no active game / type not applicable).
    char nextSpecialRounds[64];
} ServiceStats;

typedef struct {
    int dogs;          // -1 when not applicable to the map
    int monkeys;       // -1 when not applicable to the map
    int thief;         // -1 when not applicable to the map
    char next[64];
} ServiceSpecialRounds;

ServiceResult serviceStatsGet(Service *service, int scopeRound, ServiceStats *out);

ServiceResult serviceStatsGetSph(Service *service, int scopeRound, double *sphOut);
ServiceResult serviceStatsGetClaymores(Service *service, int *claymoresOut);
ServiceResult serviceStatsGetEntities(Service *service, int *currentOut, int *maxOut);
ServiceResult serviceStatsGetRevives(Service *service, int *revivesOut);

#endif // SERVICE_STATS_H_
