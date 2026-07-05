#include "service/player.h"
#include "service/service_internal.h"
#include "logic/cheat.h"
#include "logic/game/perk.h"
#include "logic/game/weapon.h"
#include "utils/list.h"

#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Scalar attributes (name, health, points, kills, headshots, speed).
// ---------------------------------------------------------------------------

ServiceResult servicePlayerGetAttributes(Service *service, ServicePlayerAttributes *out) {
    if (!service || !out) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;

    Controller *controller = service->controller;

    if (!controllerGetPlayerName(controller, out->name, sizeof(out->name))) out->name[0] = '\0';
    out->health = controllerGetPlayerHealth(controller);
    out->points = controllerGetPlayerPoints(controller);
    out->kills = controllerGetPlayerKills(controller);
    out->headshots = controllerGetPlayerHeadshots(controller);
    out->movementSpeed = controllerGetMovementSpeed(controller);
    return SERVICE_OK;
}

ServiceResult servicePlayerUpdate(Service *service, const ServicePlayerPatch *patch) {
    if (!service || !patch) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;

    Controller *controller = service->controller;

    if (patch->hasName) {
        if (!controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_CHANGE_NAME, (void *)patch->name))
            return SERVICE_ENGINE_FAILED;
    }
    if (patch->hasHealth) {
        int value = patch->health;
        if (!controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_SET_HEALTH, &value))
            return SERVICE_ENGINE_FAILED;
    }
    if (patch->hasPoints) {
        int value = patch->points;
        if (!controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_SET_POINTS, &value))
            return SERVICE_ENGINE_FAILED;
    }
    if (patch->hasKills) {
        int value = patch->kills;
        if (!controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_SET_KILLS, &value))
            return SERVICE_ENGINE_FAILED;
    }
    if (patch->hasHeadshots) {
        int value = patch->headshots;
        if (!controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_SET_HEADSHOTS, &value))
            return SERVICE_ENGINE_FAILED;
    }
    if (patch->hasMovementSpeed) {
        int value = patch->movementSpeed;
        if (!controllerSetSimpleCheat(controller, SIMPLE_CHEAT_NAME_SET_SPEED, &value))
            return SERVICE_ENGINE_FAILED;
    }
    return SERVICE_OK;
}

// ---------------------------------------------------------------------------
// Position / ammo / weapons
// ---------------------------------------------------------------------------

ServiceResult servicePlayerGetPosition(Service *service, ServicePlayerPosition *positionOut) {
    if (!service || !positionOut) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    TeleportCoords *coords = controllerGetPlayerCurrentCoords(service->controller);
    if (!coords) return SERVICE_ENGINE_FAILED;
    positionOut->x = coords->x;
    positionOut->y = coords->y;
    positionOut->z = coords->z;
    free(coords);
    return SERVICE_OK;
}

ServiceResult servicePlayerTeleport(Service *service, float x, float y, float z) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    TeleportCoords coords = { x, y, z };
    if (!controllerSetSimpleCheat(service->controller, SIMPLE_CHEAT_NAME_TELEPORT, &coords)) {
        return SERVICE_ENGINE_FAILED;
    }
    return SERVICE_OK;
}

ServiceResult servicePlayerGiveAmmo(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    if (!controllerGiveAmmo(service->controller)) return SERVICE_ENGINE_FAILED;
    return SERVICE_OK;
}

ServiceResult servicePlayerTakeWeapons(Service *service) {
    if (!service) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    if (!controllerTakeWeapons(service->controller)) return SERVICE_ENGINE_FAILED;
    return SERVICE_OK;
}

// ---------------------------------------------------------------------------
// Perks
// ---------------------------------------------------------------------------

typedef struct {
    const char *name;
    Perk perk;
} PerkEntry;

static const PerkEntry PERK_TABLE[] = {
    { "quick-revive", PERK_QUICK_REVIVE },
    { "juggernaut",   PERK_JUGGERNAUT },
    { "speed-cola",   PERK_SPEED_COLA },
    { "double-tap",   PERK_DOUBLE_TAP },
    { "stamina-up",   PERK_STAMINA_UP },
    { "mule-kick",    PERK_MULE_KICK },
};

static const int PERK_TABLE_SIZE = (int)(sizeof(PERK_TABLE) / sizeof(PERK_TABLE[0]));

static Perk findPerk(const char *name) {
    if (!name) return PERK_INVALID;
    for (int i = 0; i < PERK_TABLE_SIZE; i++) {
        if (strcmp(PERK_TABLE[i].name, name) == 0) return PERK_TABLE[i].perk;
    }
    return PERK_INVALID;
}

bool servicePerkExists(const char *name) {
    return findPerk(name) != PERK_INVALID;
}

ServiceResult servicePlayerGetPerkCount(Service *service, int *countOut) {
    if (!service || !countOut) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;
    *countOut = controllerGetPerkCount(service->controller);
    return SERVICE_OK;
}

ServiceResult servicePlayerModifyPerks(Service *service, ServicePerkAction action,
                                       const char **names, int count) {
    if (!service || !names || count <= 0) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;

    // Validate every name before applying so the batch is all-or-nothing.
    for (int i = 0; i < count; i++) {
        if (findPerk(names[i]) == PERK_INVALID) return SERVICE_NOT_FOUND;
    }

    List *perks = listCreate();
    if (!perks) return SERVICE_ENGINE_FAILED;
    for (int i = 0; i < count; i++) {
        listAddInt(perks, findPerk(names[i]));
    }

    bool ok = (action == SERVICE_PERK_ADD)
        ? controllerAddPerks(service->controller, perks)
        : controllerRemovePerks(service->controller, perks);
    listDestroy(perks);
    return ok ? SERVICE_OK : SERVICE_ENGINE_FAILED;
}

// ---------------------------------------------------------------------------
// Weapons
// ---------------------------------------------------------------------------

// Kebab-case weapon names, indexed to match the Weapon enum order in weapon.h.
static const char *WEAPON_NAMES[] = {
    "cymbal-monkey", "black-hole", "nesting-dolls", "quantum-bomb", "m1911",
    "mustang-and-sally", "python", "cobra", "cz75", "calamity", "m14", "mnesia",
    "m16", "skullcrusher", "g11", "g115-generator", "famas", "g16-gl35", "ak74u",
    "ak74fu2", "mp5k", "mp115-kollider", "mp40", "the-afterburner", "mpl", "mpl-lf",
    "pm63", "tokyo-and-rose", "spectre", "phantom", "cz75-dual-wield",
    "calamity-and-jane", "stakeout", "raid", "olympia", "hades", "spas-12", "spaz-24",
    "hs10", "typhoid-and-mary", "aug", "aug-50m3", "galil", "lamentation", "commando",
    "predator", "fn-fal", "epc-wn", "dragunov", "d115-disassembler", "l96a1",
    "l115-isolator", "rpk", "r115-resonator", "hk21", "h115-oscilator", "m72-law",
    "m72-anarchy", "china-lake", "china-beach", "ray-gun", "porters-x2-ray-gun",
    "thundergun", "zeus-cannon", "crossbow", "awful-lawton", "ballistic-knife",
    "krauss-refibrillator", "ballistic-knife-bowie", "krauss-refibrillator-bowie",
};

static const int WEAPON_NAMES_SIZE = (int)(sizeof(WEAPON_NAMES) / sizeof(WEAPON_NAMES[0]));

static Weapon findWeapon(const char *name) {
    if (!name) return WEAPON_INVALID;
    for (int i = 0; i < WEAPON_NAMES_SIZE; i++) {
        if (strcmp(WEAPON_NAMES[i], name) == 0) return (Weapon)i;
    }
    return WEAPON_INVALID;
}

bool serviceWeaponExists(const char *name) {
    return findWeapon(name) != WEAPON_INVALID;
}

ServiceResult servicePlayerGiveWeapons(Service *service, const char **names, int count) {
    if (!service || !names || count <= 0) return SERVICE_INVALID_PARAM;
    if (!controllerIsZombiesGameOngoing(service->controller)) return SERVICE_GAME_NOT_ATTACHED;

    // Validate every name before applying so the batch is all-or-nothing.
    for (int i = 0; i < count; i++) {
        if (findWeapon(names[i]) == WEAPON_INVALID) return SERVICE_NOT_FOUND;
    }

    List *weapons = listCreate();
    if (!weapons) return SERVICE_ENGINE_FAILED;
    for (int i = 0; i < count; i++) {
        listAddInt(weapons, findWeapon(names[i]));
    }

    bool ok = controllerGiveWeapons(service->controller, weapons);
    listDestroy(weapons);
    return ok ? SERVICE_OK : SERVICE_ENGINE_FAILED;
}
