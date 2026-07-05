#ifndef SERVICE_PLAYER_H_
#define SERVICE_PLAYER_H_

#include "service.h"

typedef struct { float x, y, z; } ServicePlayerPosition;

// name/health/points/kills/headshots are echoed from the last value set through
// the API (the engine cannot read them back); movement-speed is read live.
// Character is persisted config, exposed under /game/config.
typedef struct {
    char name[64];
    int health;
    int points;
    int kills;
    int headshots;
    float movementSpeed;
} ServicePlayerAttributes;

// Partial update for PATCH /player. Only fields with their has* flag set are applied.
typedef struct {
    bool hasName;          char name[64];
    bool hasHealth;        int health;
    bool hasPoints;        int points;
    bool hasKills;         int kills;
    bool hasHeadshots;     int headshots;
    bool hasMovementSpeed; int movementSpeed;
} ServicePlayerPatch;

ServiceResult servicePlayerGetAttributes(Service *service, ServicePlayerAttributes *out);
ServiceResult servicePlayerUpdate(Service *service, const ServicePlayerPatch *patch);

ServiceResult servicePlayerGetPosition(Service *service, ServicePlayerPosition *positionOut);
ServiceResult servicePlayerTeleport(Service *service, float x, float y, float z);
ServiceResult servicePlayerGiveAmmo(Service *service);
ServiceResult servicePlayerTakeWeapons(Service *service);

typedef enum {
    SERVICE_PERK_ADD,
    SERVICE_PERK_REMOVE,
} ServicePerkAction;

bool servicePerkExists(const char *name);
ServiceResult servicePlayerGetPerkCount(Service *service, int *countOut);
ServiceResult servicePlayerModifyPerks(Service *service, ServicePerkAction action,
                                       const char **names, int count);

bool serviceWeaponExists(const char *name);
ServiceResult servicePlayerGiveWeapons(Service *service, const char **names, int count);

#endif // SERVICE_PLAYER_H_
