#ifndef CLIENT_PLAYER_H_
#define CLIENT_PLAYER_H_

#include "client.h"
#include "logic/cheat.h"       // TeleportCoords
#include "logic/game/perk.h"   // Perk
#include "logic/game/weapon.h" // Weapon

// Player operations (mirror of service/player.h).

typedef struct {
    char name[64];
    int health;
    int points;
    int kills;
    int headshots;
    float movementSpeed;
} PlayerAttributes;

ClientResult clientGetPlayer(Client *client, PlayerAttributes *out);
// Per-field setters (PATCH /player). Each sends only its own field, so setting
// one attribute never re-applies the others.
ClientResult clientSetPlayerName(Client *client, const char *name);
ClientResult clientSetPlayerHealth(Client *client, int health);
ClientResult clientSetPlayerPoints(Client *client, int points);
ClientResult clientSetPlayerKills(Client *client, int kills);
ClientResult clientSetPlayerHeadshots(Client *client, int headshots);
ClientResult clientSetPlayerMovementSpeed(Client *client, int movementSpeed);

ClientResult clientGetPosition(Client *client, TeleportCoords *out);
ClientResult clientTeleport(Client *client, float x, float y, float z);

ClientResult clientGiveAmmo(Client *client);
ClientResult clientTakeWeapons(Client *client);
// Resolve an API weapon name (kebab-case) to its Weapon. False if unknown.
bool clientWeaponFromName(const char *name, Weapon *out);
ClientResult clientGiveWeapons(Client *client, const Weapon *weapons, int count);

ClientResult clientGetPerkCount(Client *client, int *count);
// Resolve an API perk name (kebab-case) to its Perk. False if unknown.
bool clientPerkFromName(const char *name, Perk *out);
ClientResult clientAddPerks(Client *client, const Perk *perks, int count);
ClientResult clientRemovePerks(Client *client, const Perk *perks, int count);

#endif // CLIENT_PLAYER_H_
